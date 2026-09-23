#ifndef FMTUTIL_H
#define FMTUTIL_H

#include <QtGlobal>
#include <QString>

QString humanBytes(qint64 n);
QString humanBytesChat(qint64 n);
QString formatEta(qint64 remainBytes, double bps);

#endif
