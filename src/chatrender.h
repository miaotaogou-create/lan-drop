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
// discoverOk=false：发现端口异常，脚注强调手动加 IP
QString renderSidebarEmptyHintHtml(bool noMatch, bool discoverOk = true);
// 主区未选设备 / 搜无结果空态白卡（与侧栏同语言，略宽）
QString renderMainEmptyHintHtml(bool noMatch, const QString &query = QString(),
                                bool discoverOk = true);

#endif // CHATRENDER_H
