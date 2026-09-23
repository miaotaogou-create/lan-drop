#include "ziputil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

bool zipDirectory(const QString &dirPath, const QString &zipPath, QString *errorOut)
{
    const QFileInfo fi(dirPath);
    if (!fi.exists() || !fi.isDir()) {
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"目录不存在");
        return false;
    }
    const QString zipAbs = QFileInfo(zipPath).absoluteFilePath();
    QDir().mkpath(QFileInfo(zipAbs).absolutePath());
    QFile::remove(zipAbs);

    QProcess p;
    p.setProgram(QStringLiteral("tar"));
    p.setArguments(QStringList()
                   << QStringLiteral("-a")
                   << QStringLiteral("-cf")
                   << QDir::toNativeSeparators(zipAbs)
                   << QStringLiteral("-C")
                   << QDir::toNativeSeparators(fi.absolutePath())
                   << fi.fileName());
    p.start();
    if (!p.waitForStarted(5000)) {
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"本机找不到 tar，无法打包");
        return false;
    }
    // ponytail: 大目录可能较久；上限 10 分钟，超时再升级为后台任务
    if (!p.waitForFinished(10 * 60 * 1000)) {
        p.kill();
        p.waitForFinished(3000);
        QFile::remove(zipAbs);
        if (errorOut)
            *errorOut = QString::fromUtf8(u8"打包超时");
        return false;
    }
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
