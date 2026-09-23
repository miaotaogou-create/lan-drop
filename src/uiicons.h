#ifndef UIICONS_H
#define UIICONS_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

class QPushButton;

enum ChromeIcon {
    IconSettings = 0,
    IconMinimize,
    IconMaximize,
    IconRestore,
    IconClose
};

QIcon makeChromeIcon(ChromeIcon kind, const QColor &color);
QPixmap makeGlobeBadge(int logical = 36);
QPixmap makeRadioLogo(int logical = 36);
QPixmap renderSvgIcon(const QString &resPath, int logical = 16);
QPixmap makeLaptopIcon(int logical = 16);
QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical = 44);
QPushButton *toolLinkBtn(const QString &svgRes, const QString &text, const QString &objectName);
QPixmap makeStatusDot(bool ok, int logical = 7);
QPixmap makeChatBubbleIcon(int logical = 14);
QPixmap makeFileDocIcon(int logical = 14);
QPixmap makeCheckCircleIcon(int logical = 14);
QPixmap loadSvgPixmap(const QString &path, int logical);
QPushButton *chromeBtn(ChromeIcon kind, const QString &objectName, const QString &tip);

// 聊天气泡 HTML 仍用首字头像
QString avatarInitial(const QString &name);

#endif
