#include "ziputil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

bool prepareZipOutput(const QString &dirPath, const QString &zipPath, QString *errorOut)
{
    const QFileInfo fi(dirPath);
    if (!fi.exists() || !fi.isDir()) {
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"目录不存在");
        return false;
    }
    if (fi.fileName().isEmpty()) {
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"目录名无效");
        return false;
    }
    const QString zipAbs = QFileInfo(zipPath).absoluteFilePath();
    QDir().mkpath(QFileInfo(zipAbs).absolutePath());
    QFile::remove(zipAbs);
    return true;
}

QStringList zipTarArguments(const QString &dirPath, const QString &zipPath)
{
    const QFileInfo fi(dirPath);
    const QString zipAbs = QFileInfo(zipPath).absoluteFilePath();
    return QStringList()
        << QStringLiteral("-a")
        << QStringLiteral("-cf")
        << QDir::toNativeSeparators(zipAbs)
        << QStringLiteral("-C")
        << QDir::toNativeSeparators(fi.absolutePath())
        << fi.fileName();
}

bool zipDirectory(const QString &dirPath, const QString &zipPath, QString *errorOut)
{
    if (!prepareZipOutput(dirPath, zipPath, errorOut))
        return false;

    QProcess p;
    p.setProgram(QStringLiteral("tar"));
    p.setArguments(zipTarArguments(dirPath, zipPath));
    p.start();
    if (!p.waitForStarted(5000)) {
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"本机找不到 tar，无法打包");
        return false;
    }
    if (!p.waitForFinished(10 * 60 * 1000)) {
        p.kill();
        p.waitForFinished(3000);
        QFile::remove(QFileInfo(zipPath).absoluteFilePath());
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"打包超时");
        return false;
    }
    const QString zipAbs = QFileInfo(zipPath).absoluteFilePath();
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0
        || !QFileInfo::exists(zipAbs) || QFileInfo(zipAbs).size() <= 0) {
        QFile::remove(zipAbs);
        if (errorOut) {
            const QString detail = QString::fromLocal8Bit(p.readAllStandardError()).trimmed();
            *errorOut = detail.isEmpty() ? QString::fromUtf8(u8"打包失败") : detail;
        }
        return false;
    }
    return true;
}

QString landropZipTempDir()
{
    return QDir::temp().filePath(QStringLiteral("landrop-zip"));
}

bool isLandropTempZip(const QString &path)
{
    if (path.isEmpty())
        return false;
    const QFileInfo fi(path);
    if (fi.suffix().toLower() != QLatin1String("zip"))
        return false;
    const QString dir = QFileInfo(landropZipTempDir()).absoluteFilePath();
    return QFileInfo(fi.absolutePath()).absoluteFilePath() == dir;
}

void removeLandropTempZip(const QString &path)
{
    if (!isLandropTempZip(path))
        return;
    QFile::remove(path);
}

void cleanupLandropZipTempDir()
{
    const QString dir = landropZipTempDir();
    QDir d(dir);
    if (!d.exists())
        return;
    const QFileInfoList files = d.entryInfoList(QStringList() << QStringLiteral("*.zip"),
                                                QDir::Files);
    for (int i = 0; i < files.size(); ++i)
        QFile::remove(files.at(i).absoluteFilePath());
}
