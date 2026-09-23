#ifndef CHATRENDER_H
#define CHATRENDER_H

#include "chatmsg.h"

#include <QString>
#include <QVector>

QString renderChatHtml(const QVector<ChatMsg> &msgs);
QString renderFilesHtml(const QVector<ChatMsg> &msgs);
int countFiles(const QVector<ChatMsg> &msgs);
QString fileSha256Short(const QString &path);

#endif // CHATRENDER_H
