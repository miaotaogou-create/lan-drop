#ifndef CHATMSG_H
#define CHATMSG_H

#include <QString>
#include <QStringList>
#include <QtGlobal>

struct ChatMsg {
    enum Type { OutText = 0, InText, OutFile, InFile, System, Fail };
    int type = OutText;
    QString who;
    QString face; // 头像取首字；空则用 who（发出侧 who=「我」时填本机设备名）
    QString text;
    QString path;
    QStringList morePaths; // Fail：失败文件之外仍待重发的排队
    qint64 size = 0;
    qint64 rttMs = -1;
    QString sha256;
    QString time;
    int progressPct = -1; // -1=完成/非传输；0..100=收发进行中
};

#endif // CHATMSG_H
