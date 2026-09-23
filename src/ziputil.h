#ifndef ZIPUTIL_H
#define ZIPUTIL_H

#include <QString>

// 用系统 tar 将目录打成 zip（Windows 10+ / 多数 Linux 可用）
bool zipDirectory(const QString &dirPath, const QString &zipPath, QString *errorOut = 0);

#endif
