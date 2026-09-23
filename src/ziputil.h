#ifndef ZIPUTIL_H
#define ZIPUTIL_H

#include <QString>
#include <QStringList>

// 校验目录并准备输出路径（删旧 zip、建目录）
bool prepareZipOutput(const QString &dirPath, const QString &zipPath, QString *errorOut = 0);

// 供 QProcess 使用的 tar 参数（需先 prepareZipOutput）
QStringList zipTarArguments(const QString &dirPath, const QString &zipPath);

// 同步打包（自检/小目录）；大目录请异步 QProcess
bool zipDirectory(const QString &dirPath, const QString &zipPath, QString *errorOut = 0);

#endif
