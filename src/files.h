#ifndef FILES_H
#define FILES_H

#include <QString>

class QFile;

// 只留文件名。空串表示拒绝（路径穿越或非法字符）。
QString safeFileName(const QString &raw);

// 在目录里新建文件，同名不覆盖。失败时 *out 为空。
QString createUniqueFile(const QString &dir, const QString &filename, QFile *out);

bool isVirtualIfaceName(const QString &name);

// 由 IPv4 与掩码算广播地址。掩码无效时返回空。
QString broadcastAddress(const QString &ipv4, const QString &mask);

#endif
