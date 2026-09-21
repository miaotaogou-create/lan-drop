#include "settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

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
        s.deviceName = QStringLiteral("局域快传");
    s.port = 8848;
    s.discoverPort = 8850;
    s.downloadDir = QStringLiteral("./downloads");
    return s;
}

static int clampPort(int p, int fallback)
{
    if (p < 1 || p > 65535)
        return fallback;
    return p;
}

Settings Settings::load()
{
    Settings s = defaults();
    QFile f(filePath());
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
    return s;
}

bool Settings::save() const
{
    const QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject o;
    o.insert(QStringLiteral("deviceName"), deviceName.trimmed().isEmpty() ? defaults().deviceName : deviceName.trimmed());
    o.insert(QStringLiteral("port"), clampPort(port, 8848));
    o.insert(QStringLiteral("discoverPort"), clampPort(discoverPort, 8850));
    o.insert(QStringLiteral("downloadDir"), downloadDir.trimmed().isEmpty() ? QStringLiteral("./downloads") : downloadDir.trimmed());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}
