#ifndef FMTUTIL_H
#define FMTUTIL_H

#include <QtGlobal>
#include <QString>

QString humanBytes(qint64 n);
QString humanBytesChat(qint64 n);
QString formatEta(qint64 remainBytes, double bps);
// 往返/耗时：260ms / 37.4s / 1m 15s
QString formatDurationMs(qint64 ms);

#endif
