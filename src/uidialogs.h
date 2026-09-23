#ifndef UIDIALOGS_H
#define UIDIALOGS_H

#include <QString>
#include <QStringList>

class QWidget;
class QMenu;

void applyFloatingShadow(QWidget *w);
void styleAppMenu(QMenu *menu);

void appInfo(QWidget *parent, const QString &text);
void appWarn(QWidget *parent, const QString &text);

// 确认：accept 为确认钮文案；cancel 为取消；defaultAccept=false 时默认焦点在取消
bool appConfirm(QWidget *parent, const QString &text,
                const QString &acceptText = QString(),
                const QString &cancelText = QString(),
                bool defaultAccept = false);

// 多选：返回点中的按钮下标；取消/关窗返回 -1
// labels 从左到右为主操作优先；defaultIndex 为默认焦点（通常是取消）
int appChoice(QWidget *parent, const QString &text, const QStringList &labels,
              int defaultIndex = -1);

#endif
