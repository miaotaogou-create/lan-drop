#include "settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>

static QString configDir()
{
#ifdef Q_OS_WIN
    // 与 Go 的 UserConfigDir 对齐：%AppData%\lan-drop
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/lan-drop");
#else
    return QDir::homePath() + QStringLiteral("/.config/lan-drop");
#endif
}

QString Settings::filePath()
{
    const QString preferred = configDir() + QStringLiteral("/settings.json");
    if (QFile::exists(preferred))
        return preferred;
    const QString local = QStringLiteral("data/settings.json");
    if (QFile::exists(local))
        return local;
    if (QDir().mkpath(configDir()))
        return preferred;
    return local;
}

Settings Settings::defaults()
{
    Settings s;
    s.deviceName = QHostInfo::localHostName();
    if (s.deviceName.trimmed().isEmpty())
        s.deviceName = QString::fromUtf8(u8"局域快传");
    s.port = 8848;
    s.discoverPort = 8850;
    s.downloadDir = QStringLiteral("./downloads");
    s.transferThreads = 8;
    s.nudgeEnabled = true;
    s.soundNotification = true;
    s.closeToTray = true;
    s.alwaysOnTop = false;
    s.runAtStartup = false;
    return s;
}

static int clampPort(int p, int fallback)
{
    if (p < 1 || p > 65535)
        return fallback;
    return p;
}

static int clampThreads(int n, int fallback)
{
    if (n < 1 || n > 32)
        return fallback;
    return n;
}

static QList<ManualPeerEntry> readManualPeers(const QJsonObject &o)
{
    QList<ManualPeerEntry> out;
    if (!o.contains(QStringLiteral("manualPeers")) || !o.value(QStringLiteral("manualPeers")).isArray())
        return out;
    const QJsonArray arr = o.value(QStringLiteral("manualPeers")).toArray();
    QSet<QString> seen;
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonObject row = arr.at(i).toObject();
        ManualPeerEntry e;
        e.ip = row.value(QStringLiteral("ip")).toString().trimmed();
        if (e.ip.isEmpty() || e.ip.contains(QLatin1Char(' ')))
            continue;
        e.port = clampPort(row.value(QStringLiteral("port")).toInt(), 8848);
        e.alias = row.value(QStringLiteral("alias")).toString().trimmed();
        e.os = row.value(QStringLiteral("os")).toString().trimmed();
        e.tag = row.value(QStringLiteral("tag")).toString().trimmed();
        const QString key = e.ip + QLatin1Char(':') + QString::number(e.port);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        out.append(e);
    }
    return out;
}

static QJsonArray writeManualPeers(const QList<ManualPeerEntry> &list)
{
    QJsonArray arr;
    QSet<QString> seen;
    for (int i = 0; i < list.size(); ++i) {
        const ManualPeerEntry &e = list.at(i);
        const QString ip = e.ip.trimmed();
        if (ip.isEmpty() || ip.contains(QLatin1Char(' ')))
            continue;
        const int port = clampPort(e.port, 8848);
        const QString key = ip + QLatin1Char(':') + QString::number(port);
        if (seen.contains(key))
            continue;
        seen.insert(key);
        QJsonObject row;
        row.insert(QStringLiteral("ip"), ip);
        row.insert(QStringLiteral("port"), port);
        if (!e.alias.trimmed().isEmpty())
            row.insert(QStringLiteral("alias"), e.alias.trimmed());
        if (!e.os.trimmed().isEmpty())
            row.insert(QStringLiteral("os"), e.os.trimmed());
        if (!e.tag.trimmed().isEmpty())
            row.insert(QStringLiteral("tag"), e.tag.trimmed());
        arr.append(row);
    }
    return arr;
}

Settings Settings::loadFromFile(const QString &path)
{
    Settings s = defaults();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return s;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    const QString name = o.value(QStringLiteral("deviceName")).toString().trimmed();
    if (!name.isEmpty())
        s.deviceName = name;
    s.port = clampPort(o.value(QStringLiteral("port")).toInt(), s.port);
    s.discoverPort = clampPort(o.value(QStringLiteral("discoverPort")).toInt(), s.discoverPort);
    const QString dir = o.value(QStringLiteral("downloadDir")).toString().trimmed();
    if (!dir.isEmpty())
        s.downloadDir = dir;
    if (o.contains(QStringLiteral("transferThreads")))
        s.transferThreads = clampThreads(o.value(QStringLiteral("transferThreads")).toInt(), s.transferThreads);
    if (o.contains(QStringLiteral("nudgeEnabled")))
        s.nudgeEnabled = o.value(QStringLiteral("nudgeEnabled")).toBool();
    if (o.contains(QStringLiteral("soundNotification")))
        s.soundNotification = o.value(QStringLiteral("soundNotification")).toBool();
    if (o.contains(QStringLiteral("closeToTray")))
        s.closeToTray = o.value(QStringLiteral("closeToTray")).toBool();
    if (o.contains(QStringLiteral("alwaysOnTop")))
        s.alwaysOnTop = o.value(QStringLiteral("alwaysOnTop")).toBool();
    if (o.contains(QStringLiteral("runAtStartup")))
        s.runAtStartup = o.value(QStringLiteral("runAtStartup")).toBool();
    s.soundFile = o.value(QStringLiteral("soundFile")).toString().trimmed();
    s.preferredLocalIp = o.value(QStringLiteral("preferredLocalIp")).toString().trimmed();
    s.manualPeers = readManualPeers(o);
    if (o.contains(QStringLiteral("windowX")))
        s.windowX = o.value(QStringLiteral("windowX")).toInt();
    if (o.contains(QStringLiteral("windowY")))
        s.windowY = o.value(QStringLiteral("windowY")).toInt();
    if (o.contains(QStringLiteral("windowW")))
        s.windowW = o.value(QStringLiteral("windowW")).toInt();
    if (o.contains(QStringLiteral("windowH")))
        s.windowH = o.value(QStringLiteral("windowH")).toInt();
    if (o.contains(QStringLiteral("windowMaximized")))
        s.windowMaximized = o.value(QStringLiteral("windowMaximized")).toBool();
    if (o.contains(QStringLiteral("sideWidth")))
        s.sideWidth = o.value(QStringLiteral("sideWidth")).toInt();
    s.lastPeer = o.value(QStringLiteral("lastPeer")).toString().trimmed();
    s.pinnedPeers.clear();
    if (o.contains(QStringLiteral("pinnedPeers")) && o.value(QStringLiteral("pinnedPeers")).isArray()) {
        const QJsonArray arr = o.value(QStringLiteral("pinnedPeers")).toArray();
        QSet<QString> seen;
        for (int i = 0; i < arr.size(); ++i) {
            const QString k = arr.at(i).toString().trimmed();
            if (k.isEmpty() || !k.contains(QLatin1Char(':')) || seen.contains(k))
                continue;
            seen.insert(k);
            s.pinnedPeers.append(k);
        }
    }
    return s;
}

Settings Settings::load()
{
    return loadFromFile(filePath());
}

bool Settings::saveToFile(const QString &path) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject o;
    o.insert(QStringLiteral("deviceName"), deviceName.trimmed().isEmpty() ? defaults().deviceName : deviceName.trimmed());
    o.insert(QStringLiteral("port"), clampPort(port, 8848));
    o.insert(QStringLiteral("discoverPort"), clampPort(discoverPort, 8850));
    o.insert(QStringLiteral("downloadDir"), downloadDir.trimmed().isEmpty() ? QStringLiteral("./downloads") : downloadDir.trimmed());
    o.insert(QStringLiteral("transferThreads"), clampThreads(transferThreads, 8));
    o.insert(QStringLiteral("nudgeEnabled"), nudgeEnabled);
    o.insert(QStringLiteral("soundNotification"), soundNotification);
    o.insert(QStringLiteral("closeToTray"), closeToTray);
    o.insert(QStringLiteral("alwaysOnTop"), alwaysOnTop);
    o.insert(QStringLiteral("runAtStartup"), runAtStartup);
    if (!soundFile.trimmed().isEmpty())
        o.insert(QStringLiteral("soundFile"), soundFile.trimmed());
    if (!preferredLocalIp.trimmed().isEmpty())
        o.insert(QStringLiteral("preferredLocalIp"), preferredLocalIp.trimmed());
    o.insert(QStringLiteral("manualPeers"), writeManualPeers(manualPeers));
    o.insert(QStringLiteral("windowX"), windowX);
    o.insert(QStringLiteral("windowY"), windowY);
    o.insert(QStringLiteral("windowW"), windowW);
    o.insert(QStringLiteral("windowH"), windowH);
    o.insert(QStringLiteral("windowMaximized"), windowMaximized);
    o.insert(QStringLiteral("sideWidth"), sideWidth);
    if (!lastPeer.trimmed().isEmpty())
        o.insert(QStringLiteral("lastPeer"), lastPeer.trimmed());
    if (!pinnedPeers.isEmpty()) {
        QJsonArray pins;
        for (int i = 0; i < pinnedPeers.size(); ++i)
            pins.append(pinnedPeers.at(i));
        o.insert(QStringLiteral("pinnedPeers"), pins);
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

bool Settings::save() const
{
    return saveToFile(filePath());
}

QString Settings::resolvedDownloadDir() const
{
    QString dir = downloadDir.trimmed();
    if (dir.isEmpty())
        dir = QStringLiteral("./downloads");

    const QString abs = QDir::cleanPath(
        QFileInfo(dir).isAbsolute() ? dir : QDir::current().absoluteFilePath(dir));

    auto canWrite = [](const QString &path) -> bool {
        if (!QDir().mkpath(path))
            return false;
        const QString probe = path + QStringLiteral("/.landrop-wtest");
        QFile f(probe);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        f.close();
        QFile::remove(probe);
        return true;
    };

    if (canWrite(abs))
        return abs;

    // AppImage / Enigma 等：程序目录只读时落到家目录
    const QString fallback = QDir::homePath() + QStringLiteral("/landrop/downloads");
    QDir().mkpath(fallback);
    return QDir::cleanPath(fallback);
}
