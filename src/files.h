#ifndef FILES_H
#define FILES_H

#include <QString>

class QFile;

// 只留文件名。空串表示拒绝（路径穿越或非法字符）。
QString safeFileName(const QString &raw);

// 在目录里新建文件，同名不覆盖。失败时 *out 为空。
QString createUniqueFile(const QString &dir, const QString &filename, QFile *out);

// 把 src 文件复制到 dir，安全文件名 + 重名加后缀。失败返回空。
QString copyFileIntoDir(const QString &dir, const QString &srcPath);

bool isVirtualIfaceName(const QString &name);

// 在 root 下解析仅含文件名的 name；拒绝穿越。不存在或非普通文件返回空。
QString resolveSharedFile(const QString &root, const QString &name);

// 由 IPv4 与掩码算广播地址。掩码无效时返回空。
QString broadcastAddress(const QString &ipv4, const QString &mask);

// 0 笔记本，1 手机，2 平板（按对端 os 字段启发式）
int deviceKindFromOs(const QString &osName);

// 把链路速率（Mbps，未知为负数）格式化成「2.5 GbE 网线」这类文案。
QString formatLinkLabel(qint64 mbps, bool wifi);

// 本机对外报告的系统类型：windows / linux / arm-linux
QString localOsTag();

// 列表/副行展示用短标签（完整词，避免 linux→linu）
QString osDisplayLabel(const QString &osName);

#endif
