#ifndef CHATRENDER_H
#define CHATRENDER_H

#include "chatmsg.h"

#include <QString>
#include <QVector>

QString renderChatHtml(const QVector<ChatMsg> &msgs);
QString renderFilesHtml(const QVector<ChatMsg> &msgs);
int countFiles(const QVector<ChatMsg> &msgs);
QString fileSha256Short(const QString &path);
// 侧栏无设备 / 无匹配空态白卡（与主区空态同语言）
QString renderSidebarEmptyHintHtml(bool noMatch);

#endif // CHATRENDER_H
