#include "fmtutil.h"

QString humanBytes(qint64 n)
{
    if (n < 1024)
        return QString::number(n) + QStringLiteral(" B");
    if (n < 1024 * 1024)
        return QString::number(n / 1024.0, 'f', 1) + QStringLiteral(" KB");
    return QString::number(n / 1024.0 / 1024.0, 'f', 1) + QStringLiteral(" MB");
}

QString humanBytesChat(qint64 n)
{
    if (n < 1024)
        return QString::number(n) + QStringLiteral(" B");
    if (n < 1024 * 1024)
        return QString::number(n / 1024.0, 'f', 1) + QStringLiteral(" KB");
    return QString::number(n / 1024.0 / 1024.0, 'f', 1) + QStringLiteral(" MB");
}

QString formatEta(qint64 remainBytes, double bps)
{
    if (remainBytes <= 0 || bps < 8 * 1024)
        return QString();
    const qint64 sec = qint64(double(remainBytes) / bps + 0.5);
    if (sec < 3)
        return QString();
    if (sec < 60)
        return QString::fromUtf8(u8"约 %1 秒").arg(sec);
    const qint64 min = (sec + 30) / 60;
    if (min < 60)
        return QString::fromUtf8(u8"约 %1 分").arg(min);
    return QString::fromUtf8(u8"约 %1 小时").arg((min + 30) / 60);
}

QString formatDurationMs(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    if (ms < 1)
        return QStringLiteral("<1ms");
    if (ms < 1000)
        return QString::number(ms) + QStringLiteral("ms");
    if (ms < 60000)
        return QString::number(ms / 1000.0, 'f', 1) + QStringLiteral("s");
    const qint64 minutes = ms / 60000;
    const qint64 seconds = (ms % 60000) / 1000;
    return QString::number(minutes) + QStringLiteral("m ") + QString::number(seconds)
        + QStringLiteral("s");
}
