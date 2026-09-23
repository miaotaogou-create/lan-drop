#include "discovery.h"

#include "files.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>
#include <QFile>
#include <QHostInfo>

static bool skipIface(const QNetworkInterface &iface)
{
    const QNetworkInterface::InterfaceFlags f = iface.flags();
    if (!(f & QNetworkInterface::IsUp))
        return true;
    if (f & QNetworkInterface::IsLoopBack)
        return true;
    if (isVirtualIfaceName(iface.humanReadableName()) || isVirtualIfaceName(iface.name()))
        return true;
    return false;
}

QString deviceId()
{
    QString host = QHostInfo::localHostName();
    if (host.trimmed().isEmpty())
        host = QStringLiteral("landrop");
    const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
    for (int i = 0; i < all.size(); ++i) {
        if (skipIface(all.at(i)))
            continue;
        const QString mac = all.at(i).hardwareAddress().toLower();
        if (!mac.isEmpty() && mac != QLatin1String("00:00:00:00:00:00"))
            return host + QLatin1Char('-') + mac;
    }
    return host + QLatin1String("-local");
}

QStringList localIpv4()
{
    QStringList ips;
    const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
    for (int i = 0; i < all.size(); ++i) {
        if (skipIface(all.at(i)))
            continue;
        const QList<QNetworkAddressEntry> ents = all.at(i).addressEntries();
        for (int j = 0; j < ents.size(); ++j) {
            if (ents.at(j).ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            const QString s = ents.at(j).ip().toString();
            if (!ips.contains(s))
                ips.append(s);
        }
    }
    return ips;
}

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#endif

QString localLinkLabel()
{
    const QString want = localIpv4().value(0);
#ifdef Q_OS_WIN
    ULONG sz = 32 * 1024;
    QByteArray buf(int(sz), 0);
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG ret = GetAdaptersAddresses(AF_INET, flags, 0,
                                     reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()), &sz);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buf.resize(int(sz));
        ret = GetAdaptersAddresses(AF_INET, flags, 0,
                                   reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data()), &sz);
    }
    if (ret == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES *a = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buf.data());
             a; a = a->Next) {
            if (a->OperStatus != IfOperStatusUp)
                continue;
            bool match = false;
            for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
                if (!u->Address.lpSockaddr || u->Address.lpSockaddr->sa_family != AF_INET)
                    continue;
                char text[64];
                DWORD n = sizeof(text);
                if (WSAAddressToStringA(u->Address.lpSockaddr, u->Address.iSockaddrLength,
                                        0, text, &n) != 0)
                    continue;
                if (QString::fromLatin1(text) == want)
                    match = true;
            }
            if (!match)
                continue;
            const qint64 mbps = qint64(a->TransmitLinkSpeed / 1000000ULL);
            const bool wifi = (a->IfType == 71);
            return formatLinkLabel(mbps, wifi);
        }
    }
#else
    const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
    for (int i = 0; i < all.size(); ++i) {
        if (skipIface(all.at(i)))
            continue;
        bool match = want.isEmpty();
        const QList<QNetworkAddressEntry> ents = all.at(i).addressEntries();
        for (int j = 0; j < ents.size(); ++j) {
            if (ents.at(j).ip().toString() == want)
                match = true;
        }
        if (!match)
            continue;
        QFile speedFile(QStringLiteral("/sys/class/net/%1/speed").arg(all.at(i).name()));
        qint64 mbps = -1;
        if (speedFile.open(QIODevice::ReadOnly))
            mbps = QString::fromLatin1(speedFile.readAll()).trimmed().toLongLong();
        const bool wifi = QFile::exists(QStringLiteral("/sys/class/net/%1/wireless").arg(all.at(i).name()));
        return formatLinkLabel(mbps, wifi);
    }
#endif
    return formatLinkLabel(-1, false);
}

QString localHostName()
{
    return QHostInfo::localHostName().trimmed();
}

bool Peer::online() const
{
    return lastSeen.isValid() && lastSeen.msecsTo(QDateTime::currentDateTime()) <= 12000;
}

QString Peer::label() const
{
    if (!alias.trimmed().isEmpty())
        return alias.trimmed();
    if (!name.trimmed().isEmpty())
        return name.trimmed();
    return ip;
}

QString Peer::key() const
{
    return ip + QLatin1Char(':') + QString::number(port);
}

Discovery::Discovery(QObject *parent)
    : QObject(parent)
{
    m_announce = new QTimer(this);
    m_announce->setInterval(3000);
    connect(m_announce, SIGNAL(timeout()), this, SLOT(announce()));
    m_prune = new QTimer(this);
    m_prune->setInterval(3000);
    connect(m_prune, SIGNAL(timeout()), this, SLOT(prune()));
}

void Discovery::setIdentity(const QString &id, const QString &name, int httpPort)
{
    m_id = id;
    m_name = name;
    m_httpPort = httpPort;
}

bool Discovery::start(int discoverPort)
{
    stop();
    m_sock = new QUdpSocket(this);
    if (!m_sock->bind(QHostAddress::AnyIPv4, discoverPort,
                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_sock->deleteLater();
        m_sock = 0;
        return false;
    }
    m_port = discoverPort;
    connect(m_sock, SIGNAL(readyRead()), this, SLOT(onDatagram()));
    m_announce->start();
    m_prune->start();
    announce();
    return true;
}

void Discovery::stop()
{
    m_announce->stop();
    m_prune->stop();
    if (m_sock) {
        m_sock->close();
        m_sock->deleteLater();
        m_sock = 0;
    }
}

QList<Peer> Discovery::peers() const
{
    return m_peers;
}

void Discovery::upsert(const Peer &in)
{
    for (int i = 0; i < m_peers.size(); ++i) {
        Peer &old = m_peers[i];
        const bool byId = !old.id.isEmpty() && !in.id.isEmpty() && old.id == in.id;
        const bool byAddr = !old.ip.isEmpty() && old.ip == in.ip && old.port == in.port && in.port != 0;
        if (!byId && !byAddr)
            continue;
        if (!in.id.isEmpty())
            old.id = in.id;
        if (!in.name.isEmpty())
            old.name = in.name;
        if (!in.ip.isEmpty())
            old.ip = in.ip;
        if (in.port != 0)
            old.port = in.port;
        if (!in.osName.isEmpty())
            old.osName = in.osName;
        if (!in.hostname.isEmpty())
            old.hostname = in.hostname;
        if (!in.alias.isEmpty())
            old.alias = in.alias;
        if (in.manual) {
            old.manual = true;
            old.tag = in.tag.trimmed(); // 手动再添加可覆盖/清空标签
        }
        if (in.lastSeen.isValid())
            old.lastSeen = in.lastSeen;
        emit changed();
        return;
    }
    Peer add = in;
    if (add.name.isEmpty())
        add.name = add.alias;
    m_peers.append(add);
    emit changed();
}

Peer Discovery::addManual(const QString &ip, int port, const QString &alias, const QString &osName,
                          const QString &tag)
{
    Peer p;
    p.ip = ip.trimmed();
    p.port = port > 0 ? port : 8848;
    p.alias = alias.trimmed();
    p.name = p.alias;
    p.osName = osName.trimmed();
    p.tag = tag.trimmed();
    p.manual = true;
    p.id = p.key();
    upsert(p);
    for (int i = 0; i < m_peers.size(); ++i) {
        if (m_peers.at(i).ip == p.ip && m_peers.at(i).port == p.port)
            return m_peers.at(i);
    }
    return p;
}

bool Discovery::removeManual(const QString &ip, int port)
{
    const QString wantIp = ip.trimmed();
    const int wantPort = port > 0 ? port : 8848;
    for (int i = 0; i < m_peers.size(); ++i) {
        const Peer &p = m_peers.at(i);
        if (!p.manual || p.ip != wantIp || p.port != wantPort)
            continue;
        m_peers.removeAt(i);
        emit changed();
        return true;
    }
    return false;
}

void Discovery::touch(const QString &ip, int port, const QString &id, const QString &name,
                      const QString &osName, const QString &hostname)
{
    Peer p;
    p.ip = ip.trimmed();
    p.port = port > 0 ? port : 8848;
    p.id = id;
    p.name = name;
    p.osName = osName;
    p.hostname = hostname.trimmed();
    p.lastSeen = QDateTime::currentDateTime();
    upsert(p);
}

bool Discovery::find(const QString &ip, int port, Peer *out) const
{
    for (int i = 0; i < m_peers.size(); ++i) {
        if (m_peers.at(i).ip == ip && m_peers.at(i).port == port) {
            if (out)
                *out = m_peers.at(i);
            return true;
        }
    }
    return false;
}

void Discovery::announce()
{
    if (!m_sock)
        return;
    QJsonObject o;
    o.insert(QStringLiteral("id"), m_id);
    o.insert(QStringLiteral("name"), m_name);
    o.insert(QStringLiteral("port"), m_httpPort);
#ifdef Q_OS_WIN
    o.insert(QStringLiteral("os"), QStringLiteral("windows"));
#else
    o.insert(QStringLiteral("os"), localOsTag());
#endif
    const QString host = localHostName();
    if (!host.isEmpty())
        o.insert(QStringLiteral("hostname"), host);
    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    QStringList targets;
    targets.append(QStringLiteral("255.255.255.255"));
    const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
    for (int i = 0; i < all.size(); ++i) {
        if (skipIface(all.at(i)))
            continue;
        const QList<QNetworkAddressEntry> ents = all.at(i).addressEntries();
        for (int j = 0; j < ents.size(); ++j) {
            if (ents.at(j).ip().protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            const QString b = broadcastAddress(ents.at(j).ip().toString(), ents.at(j).netmask().toString());
            if (!b.isEmpty() && !targets.contains(b))
                targets.append(b);
        }
    }
    bool any = false;
    for (int i = 0; i < targets.size(); ++i) {
        const qint64 n = m_sock->writeDatagram(body, QHostAddress(targets.at(i)), m_port);
        if (n == body.size())
            any = true;
    }
    if (!any && !m_loggedSendFail)
        m_loggedSendFail = true;
}

void Discovery::onDatagram()
{
    if (!m_sock)
        return;
    while (m_sock->hasPendingDatagrams()) {
        QByteArray buf;
        buf.resize(int(m_sock->pendingDatagramSize()));
        QHostAddress from;
        quint16 fromPort = 0;
        m_sock->readDatagram(buf.data(), buf.size(), &from, &fromPort);
        Q_UNUSED(fromPort);
        const QJsonObject o = QJsonDocument::fromJson(buf).object();
        const QString id = o.value(QStringLiteral("id")).toString();
        const int port = o.value(QStringLiteral("port")).toInt();
        if (id.isEmpty() || id == m_id || port < 1 || port > 65535)
            continue;
        QString ip = from.toString();
        if (from.protocol() == QAbstractSocket::IPv4Protocol)
            ip = QHostAddress(from.toIPv4Address()).toString();
        Peer p;
        p.id = id;
        p.name = o.value(QStringLiteral("name")).toString();
        p.ip = ip;
        p.port = port;
        p.osName = o.value(QStringLiteral("os")).toString();
        p.hostname = o.value(QStringLiteral("hostname")).toString().trimmed();
        p.lastSeen = QDateTime::currentDateTime();
        upsert(p);
    }
}

void Discovery::prune()
{
    const QDateTime now = QDateTime::currentDateTime();
    bool dirty = false;
    for (int i = m_peers.size() - 1; i >= 0; --i) {
        if (m_peers.at(i).manual || !m_peers.at(i).lastSeen.isValid())
            continue;
        if (m_peers.at(i).lastSeen.msecsTo(now) > 60000) {
            m_peers.removeAt(i);
            dirty = true;
        }
    }
    if (dirty)
        emit changed();
}
