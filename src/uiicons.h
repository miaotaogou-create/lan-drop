#ifndef UIICONS_H
#define UIICONS_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QtGlobal>

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
// 将 SVG 内 #RRGGBB 统一替换为指定色后渲染（工具钮静音/悬停染色）
QPixmap renderSvgIconColored(const QString &resPath, int logical, const QColor &color);
QPixmap makeLaptopIcon(int logical = 16);
QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical = 44,
                       bool withOsBadge = false);
// unread>0 时在头像右上角画未读角标（99+ 封顶）
QPixmap makePeerListAvatar(const QString &name, const QString &osName, int unread,
                           int logical = 44, bool pinned = false);
QPushButton *toolLinkBtn(const QString &svgRes, const QString &text, const QString &objectName);
QPixmap makeStatusDot(bool ok, int logical = 7);
QPixmap makeAlertTriangleIcon(int logical = 16);
QPixmap makeChatBubbleIcon(int logical = 14);
QPixmap makeSearchIcon(int logical = 16);
QPixmap makeFileDocIcon(int logical = 14);
QPixmap makeCheckCircleIcon(int logical = 14);
QPixmap makeTrashIcon(int logical = 16, const QColor &color = QColor());
QPixmap makeLightningIcon(int logical = 12, const QColor &color = QColor());
QPixmap loadSvgPixmap(const QString &path, int logical);
QPushButton *chromeBtn(ChromeIcon kind, const QString &objectName, const QString &tip);
QIcon makePinIcon(bool pinned, const QColor &color);
QPixmap makePeerStatusChip(const QString &text, const QColor &bg, const QColor &fg,
                           const QColor &border);
// 按 OS 着色的右侧胶囊（Linux 暖橙 / ARM 绿 / Windows 蓝）
QPixmap makeOsStatusChip(const QString &osName);

// 自绘图标按窗口当前屏 DPR 出图（换屏后需重设并刷新）
void setUiIconDevicePixelRatio(qreal dpr);

// 聊天气泡 HTML 仍用首字头像
QString avatarInitial(const QString &name);

#endif
