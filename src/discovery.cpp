#include "discovery.h"

#include "files.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTimer>
#include <QUdpSocket>
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
        if (!in.alias.isEmpty())
            old.alias = in.alias;
        if (in.manual)
            old.manual = true;
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

Peer Discovery::addManual(const QString &ip, int port, const QString &alias)
{
    Peer p;
    p.ip = ip.trimmed();
    p.port = port > 0 ? port : 8848;
    p.alias = alias.trimmed();
    p.name = p.alias;
    p.manual = true;
    p.id = p.key();
    upsert(p);
    for (int i = 0; i < m_peers.size(); ++i) {
        if (m_peers.at(i).ip == p.ip && m_peers.at(i).port == p.port)
            return m_peers.at(i);
    }
    return p;
}

void Discovery::touch(const QString &ip, int port, const QString &id, const QString &name, const QString &osName)
{
    Peer p;
    p.ip = ip.trimmed();
    p.port = port > 0 ? port : 8848;
    p.id = id;
    p.name = name;
    p.osName = osName;
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
    o.insert(QStringLiteral("os"), QStringLiteral("linux"));
#endif
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
