#include "files.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>

QString safeFileName(const QString &raw)
{
    QString name = raw;
    name.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const int slash = name.lastIndexOf(QLatin1Char('/'));
    name = (slash >= 0 ? name.mid(slash + 1) : name).trimmed();
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String(".."))
        return QString();
    if (name.size() > 255)
        return QString();
    const QString bad = QStringLiteral("/\\:*?\"<>|");
    for (int i = 0; i < name.size(); ++i) {
        if (bad.contains(name.at(i)))
            return QString();
    }
    return name;
}

QString createUniqueFile(const QString &dir, const QString &filename, QFile *out)
{
    if (!out)
        return QString();
    const QString name = safeFileName(filename);
    if (name.isEmpty())
        return QString();
    if (!QDir().mkpath(dir))
        return QString();
    const QString ext = QFileInfo(name).suffix();
    const QString base = ext.isEmpty() ? name : name.left(name.size() - ext.size() - 1);
    for (int i = 0; i < 10000; ++i) {
        const QString candidate = (i == 0)
            ? name
            : (ext.isEmpty() ? QStringLiteral("%1_%2").arg(base).arg(i + 1)
                             : QStringLiteral("%1_%2.%3").arg(base).arg(i + 1).arg(ext));
        const QString path = QDir(dir).filePath(candidate);
        out->setFileName(path);
        if (out->open(QIODevice::WriteOnly | QIODevice::NewOnly))
            return path;
    }
    return QString();
}

bool isVirtualIfaceName(const QString &name)
{
    const QString n = name.toLower();
    const char *hints[] = {
        "vmware", "virtualbox", "vbox", "hyper-v", "vethernet", "wsl",
        "loopback", "docker", "tap", "tun", "virtual", "虚拟", 0
    };
    for (int i = 0; hints[i]; ++i) {
        if (n.contains(QString::fromUtf8(hints[i])))
            return true;
    }
    return false;
}

QString broadcastAddress(const QString &ipv4, const QString &mask)
{
    QHostAddress ip(ipv4);
    QHostAddress m(mask);
    if (ip.protocol() != QAbstractSocket::IPv4Protocol || m.protocol() != QAbstractSocket::IPv4Protocol)
        return QString();
    const quint32 a = ip.toIPv4Address();
    const quint32 k = m.toIPv4Address();
    int ones = 0;
    for (int i = 31; i >= 0; --i) {
        if (k & (1u << i))
            ++ones;
        else
            break;
    }
    // /31、/32 没有可用广播地址
    if (ones >= 31)
        return QString();
    const quint32 b = a | ~k;
    return QHostAddress(b).toString();
}
