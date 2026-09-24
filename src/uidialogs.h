#ifndef UIDIALOGS_H
#define UIDIALOGS_H

#include <QString>
#include <QStringList>

class QWidget;
class QMenu;
class QAction;

void applyFloatingShadow(QWidget *w);
void styleAppMenu(QMenu *menu);
// 危险菜单项：红字 + 浅红悬停；返回的 QAction 可与 menu.exec 比较
QAction *addDangerMenuAction(QMenu *menu, const QString &text);

// 表单弹窗共用头脚/字段/主次钮；ns 对应 objectName 前缀（如 addPeer、settings）
QString formDialogChromeQss(const QString &ns,
                            const QString &primarySuffix = QStringLiteral("Ok"),
                            const QString &secondarySuffix = QStringLiteral("Cancel"));
// 次要钮补充（如 sharePause / shareCloseWin）
QString formDialogSecondaryBtnQss(const QStringList &objectIds);

// 主窗轻 dim；返回的控件在弹窗关闭后 delete
QWidget *showDialogDim(QWidget *anchor);

void appInfo(QWidget *parent, const QString &text);
void appWarn(QWidget *parent, const QString &text);

// 确认：accept 为确认钮文案；cancel 为取消；defaultAccept=false 时默认焦点在取消；
// danger=true 时确认钮为红（危险操作）
bool appConfirm(QWidget *parent, const QString &text,
                const QString &acceptText = QString(),
                const QString &cancelText = QString(),
                bool defaultAccept = false,
                bool danger = false);

// 多选：返回点中的按钮下标；取消/关窗返回 -1
int appChoice(QWidget *parent, const QString &text, const QStringList &labels,
              int defaultIndex = -1);

#endif
