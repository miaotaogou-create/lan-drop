#include "uiicons.h"

#include "files.h"

#include <QApplication>
#include <QFont>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QRectF>
#include <QSize>
#include <QSvgRenderer>
#include <QtMath>

enum DeviceKind {
    DevLaptop = 0,
    DevPhone,
    DevTablet
};

QIcon makeChromeIcon(ChromeIcon kind, const QColor &color)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int logical = 16;
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF r(2.5, 2.5, 11.0, 11.0);
    switch (kind) {
    case IconMinimize:
        p.drawLine(QPointF(3.5, 8.0), QPointF(12.5, 8.0));
        break;
    case IconMaximize:
        p.drawRect(QRectF(3.5, 3.5, 9.0, 9.0));
        break;
    case IconRestore:
        p.drawRect(QRectF(5.0, 3.0, 7.5, 7.5));
        p.fillRect(QRectF(3.0, 5.5, 7.5, 7.5), Qt::white);
        p.drawRect(QRectF(3.0, 5.5, 7.5, 7.5));
        break;
    case IconClose:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    case IconSettings: {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(8.0, 8.0), 2.2, 2.2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(8.0, 8.0), 4.6, 4.6);
        for (int i = 0; i < 6; ++i) {
            const qreal a = i * 3.14159265 / 3.0;
            const qreal c = qCos(a);
            const qreal s = qSin(a);
            p.drawLine(QPointF(8.0 + c * 5.2, 8.0 + s * 5.2),
                       QPointF(8.0 + c * 7.0, 8.0 + s * 7.0));
        }
        break;
    }
    }
    return QIcon(pm);
}

QPixmap makeGlobeBadge(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#2563eb")));
    p.drawRoundedRect(QRectF(0, 0, logical, logical), 8, 8);
    QPen pen(Qt::white, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal pad = 8.0;
    const QRectF box(pad, pad, logical - pad * 2, logical - pad * 2);
    p.drawEllipse(box);
    p.drawEllipse(QRectF(logical * 0.36, pad, logical * 0.28, logical - pad * 2));
    p.drawLine(QPointF(pad + 1, logical * 0.40), QPointF(logical - pad - 1, logical * 0.40));
    p.drawLine(QPointF(pad + 1, logical * 0.60), QPointF(logical - pad - 1, logical * 0.60));
    return pm;
}

QPixmap makeRadioLogo(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#2563eb")));
    p.drawRoundedRect(QRectF(0, 0, logical, logical), 10, 10);

    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(logical / 2.0, logical / 2.0), 2.4, 2.4);

    // Qt 弧：0°在时钟 3 点，逆时针。左右弧不得跨过 12/6 点，否则会像 Wi‑Fi 上下波纹。
    QPen pen(Qt::white, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF c(logical / 2.0, logical / 2.0);
    for (int i = 0; i < 2; ++i) {
        const qreal r = 6.5 + i * 4.5;
        const QRectF box(c.x() - r, c.y() - r, r * 2, r * 2);
        p.drawArc(box, -50 * 16, 100 * 16);  // 右侧
        p.drawArc(box, 130 * 16, 100 * 16);  // 左侧
    }
    return pm;
}

QString avatarInitial(const QString &name)
{
    for (int i = 0; i < name.size(); ++i) {
        if (!name.at(i).isSpace())
            return QString(name.at(i).toUpper());
    }
    return QStringLiteral("?");
}

static QColor avatarColorForName(const QString &name)
{
    static const char *kColors[] = {
        "#f97316", "#2563eb", "#059669", "#7c3aed", "#db2777", "#0891b2", "#ca8a04"
    };
    const uint h = qHash(name.isEmpty() ? QStringLiteral("?") : name);
    return QColor(QString::fromLatin1(kColors[h % 7]));
}

static void paintDeviceGlyph(QPainter &p, DeviceKind kind, const QRectF &box, const QColor &color)
{
    QPen pen(color, 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal x = box.x();
    const qreal y = box.y();
    const qreal w = box.width();
    const qreal h = box.height();
    switch (kind) {
    case DevPhone:
        p.drawRoundedRect(QRectF(x + w * 0.28, y + h * 0.08, w * 0.44, h * 0.84), 1.5, 1.5);
        p.drawLine(QPointF(x + w * 0.40, y + h * 0.78), QPointF(x + w * 0.60, y + h * 0.78));
        break;
    case DevTablet:
        p.drawRoundedRect(QRectF(x + w * 0.12, y + h * 0.18, w * 0.76, h * 0.64), 1.8, 1.8);
        p.drawLine(QPointF(x + w * 0.42, y + h * 0.72), QPointF(x + w * 0.58, y + h * 0.72));
        break;
    case DevLaptop:
    default:
        p.drawRoundedRect(QRectF(x + w * 0.14, y + h * 0.18, w * 0.72, h * 0.48), 1.2, 1.2);
        p.drawLine(QPointF(x + w * 0.06, y + h * 0.72), QPointF(x + w * 0.94, y + h * 0.72));
        p.drawLine(QPointF(x + w * 0.22, y + h * 0.72), QPointF(x + w * 0.30, y + h * 0.86));
        p.drawLine(QPointF(x + w * 0.78, y + h * 0.72), QPointF(x + w * 0.70, y + h * 0.86));
        p.drawLine(QPointF(x + w * 0.30, y + h * 0.86), QPointF(x + w * 0.70, y + h * 0.86));
        break;
    }
}

QPixmap makeLaptopIcon(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    paintDeviceGlyph(p, DevLaptop, QRectF(0, 0, logical, logical), QColor(QStringLiteral("#2563eb")));
    return pm;
}

QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(avatarColorForName(name));
    p.drawRoundedRect(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0), 10.0, 10.0);
    QFont font = qApp->font();
    font.setPixelSize(qMax(14, logical * 2 / 5));
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, logical, logical), Qt::AlignCenter, avatarInitial(name));
    // 右下角设备类型小标（笔记本/手机/平板）
    const int badge = qMax(14, logical * 14 / 44);
    const QRectF badgeRect(logical - badge - 1.0, logical - badge - 1.0, badge, badge);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#dbeafe")));
    p.drawEllipse(badgeRect);
    DeviceKind kind = DevLaptop;
    const int k = deviceKindFromOs(osName);
    if (k == 1)
        kind = DevPhone;
    else if (k == 2)
        kind = DevTablet;
    paintDeviceGlyph(p, kind, badgeRect.adjusted(2.5, 2.5, -2.5, -2.5),
                     QColor(QStringLiteral("#2563eb")));
    return pm;
}

QPixmap renderSvgIcon(const QString &resPath, int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QSvgRenderer renderer(resPath);
    if (!renderer.isValid())
        return pm;
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p, QRectF(0, 0, logical, logical));
    return pm;
}

QPushButton *toolLinkBtn(const QString &svgRes, const QString &text, const QString &objectName)
{
    QPushButton *b = new QPushButton(text);
    b->setObjectName(objectName);
    b->setCursor(Qt::PointingHandCursor);
    b->setFlat(true);
    b->setFocusPolicy(Qt::NoFocus);
    b->setIcon(QIcon(renderSvgIcon(svgRes, 16)));
    b->setIconSize(QSize(16, 16));
    return b;
}

QPixmap makeStatusDot(bool ok, int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(ok ? QColor(QStringLiteral("#22c55e")) : QColor(QStringLiteral("#f59e0b")));
    p.drawEllipse(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0));
    return pm;
}

QPixmap makeChatBubbleIcon(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(QStringLiteral("#2563eb")), 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(1.5, 1.5, logical - 3.5, logical - 5.5), 2.5, 2.5);
    p.drawLine(QPointF(4.0, logical - 3.0), QPointF(6.5, logical - 5.5));
    return pm;
}

QPixmap makeFileDocIcon(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(QStringLiteral("#2563eb")), 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF body(3.0, 1.5, logical - 6.0, logical - 3.0);
    p.drawRoundedRect(body, 1.5, 1.5);
    p.drawLine(QPointF(5.5, 5.0), QPointF(logical - 5.5, 5.0));
    p.drawLine(QPointF(5.5, 8.0), QPointF(logical - 5.5, 8.0));
    p.drawLine(QPointF(5.5, 11.0), QPointF(logical - 7.0, 11.0));
    return pm;
}

QPixmap makeCheckCircleIcon(int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#059669")));
    p.drawEllipse(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0));
    QPen pen(Qt::white, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(3.5, logical * 0.52);
    path.lineTo(5.8, logical * 0.70);
    path.lineTo(logical - 3.2, logical * 0.32);
    p.drawPath(path);
    return pm;
}

QPixmap loadSvgPixmap(const QString &path, int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logical * dpr, logical * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QSvgRenderer r(path);
    if (r.isValid()) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        r.render(&p, QRectF(0, 0, logical, logical));
    }
    return pm;
}

QPushButton *chromeBtn(ChromeIcon kind, const QString &objectName, const QString &tip)
{
    QPushButton *b = new QPushButton;
    b->setObjectName(objectName);
    b->setFixedSize(40, 32);
    b->setFocusPolicy(Qt::NoFocus);
    b->setFlat(true);
    b->setCursor(Qt::ArrowCursor);
    b->setToolTip(tip);
    b->setIcon(makeChromeIcon(kind, QColor(QStringLiteral("#475569"))));
    b->setIconSize(QSize(16, 16));
    return b;
}
