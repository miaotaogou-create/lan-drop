#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#ifdef Q_OS_WIN
static const char *kRunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const char *kValueName = "LanDrop";

bool Autostart::isEnabled()
{
    QSettings s(QString::fromLatin1(kRunKey), QSettings::NativeFormat);
    return s.contains(QString::fromLatin1(kValueName));
}

bool Autostart::setEnabled(bool on)
{
    QSettings s(QString::fromLatin1(kRunKey), QSettings::NativeFormat);
    if (on) {
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        if (exe.isEmpty())
            return false;
        s.setValue(QString::fromLatin1(kValueName), QStringLiteral("\"%1\"").arg(exe));
    } else {
        s.remove(QString::fromLatin1(kValueName));
    }
    s.sync();
    return s.status() == QSettings::NoError;
}
#else
static QString desktopPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/autostart");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/lan-drop.desktop");
}

bool Autostart::isEnabled()
{
    return QFile::exists(desktopPath());
}

bool Autostart::setEnabled(bool on)
{
    const QString path = desktopPath();
    if (!on)
        return !QFile::exists(path) || QFile::remove(path);
    const QString exe = QCoreApplication::applicationFilePath();
    if (exe.isEmpty())
        return false;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << QStringLiteral("[Desktop Entry]\n");
    out << QStringLiteral("Type=Application\n");
    out << QStringLiteral("Name=局域快传\n");
    out << QStringLiteral("Exec=\"%1\"\n").arg(exe);
    out << QStringLiteral("X-GNOME-Autostart-enabled=true\n");
    out.flush();
    return true;
}
#endif
