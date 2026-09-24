#include "uiicons.h"

#include "files.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPushButton>
#include <QRectF>
#include <QSize>
#include <QSvgRenderer>
#include <QtMath>

namespace {
qreal s_uiDpr = 0;

qreal uiDpr()
{
    if (s_uiDpr > 0)
        return s_uiDpr;
    if (qApp)
        return qMax<qreal>(1.0, qApp->devicePixelRatio());
    return 1.0;
}

QPixmap makeDprPixmap(int logicalW, int logicalH = -1)
{
    if (logicalH < 0)
        logicalH = logicalW;
    const qreal dpr = uiDpr();
    const int pxW = qMax(1, qCeil(logicalW * dpr));
    const int pxH = qMax(1, qCeil(logicalH * dpr));
    QPixmap pm(pxW, pxH);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    return pm;
}
} // namespace

void setUiIconDevicePixelRatio(qreal dpr)
{
    s_uiDpr = qMax<qreal>(1.0, dpr);
}

enum DeviceKind {
    DevLaptop = 0,
    DevPhone,
    DevTablet
};

QIcon makeChromeIcon(ChromeIcon kind, const QColor &color)
{
    // 与顶栏 SVG（folder/settings）统一为 18 逻辑像素，线重接近 Lucide stroke-2
    const int logical = 18;
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    switch (kind) {
    case IconMinimize:
        // 横线略加长，避免在同行里显得过轻过小
        p.drawLine(QPointF(4.0, 9.0), QPointF(14.0, 9.0));
        break;
    case IconMaximize:
        p.drawRect(QRectF(4.0, 4.0, 10.0, 10.0));
        break;
    case IconRestore:
        p.drawRect(QRectF(6.0, 3.5, 8.0, 8.0));
        p.fillRect(QRectF(3.5, 6.5, 8.0, 8.0), Qt::white);
        p.drawRect(QRectF(3.5, 6.5, 8.0, 8.0));
        break;
    case IconClose:
        p.drawLine(QPointF(4.5, 4.5), QPointF(13.5, 13.5));
        p.drawLine(QPointF(13.5, 4.5), QPointF(4.5, 13.5));
        break;
    case IconSettings: {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(9.0, 9.0), 2.4, 2.4);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(9.0, 9.0), 5.0, 5.0);
        for (int i = 0; i < 6; ++i) {
            const qreal a = i * 3.14159265 / 3.0;
            const qreal c = qCos(a);
            const qreal s = qSin(a);
            p.drawLine(QPointF(9.0 + c * 5.6, 9.0 + s * 5.6),
                       QPointF(9.0 + c * 7.6, 9.0 + s * 7.6));
        }
        break;
    }
    }
    return QIcon(pm);
}

QPixmap makeGlobeBadge(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
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
    QPixmap pm = makeDprPixmap(logical);
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
    // 更亮更干净的 Fluent 色阶，避免姜黄发脏
    static const char *kColors[] = {
        "#F59E0B", "#3B82F6", "#10B981", "#8B5CF6", "#EC4899", "#06B6D4", "#F97316"
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
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    paintDeviceGlyph(p, DevLaptop, QRectF(0, 0, logical, logical), QColor(QStringLiteral("#2563eb")));
    return pm;
}

QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical)
{
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setPen(Qt::NoPen);
    const QColor base = avatarColorForName(name);
    QLinearGradient grad(0, 0, 0, logical);
    grad.setColorAt(0.0, base.lighter(112));
    grad.setColorAt(1.0, base.darker(108));
    p.setBrush(grad);
    p.drawRoundedRect(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0), 11.0, 11.0);
    QFont font = qApp->font();
    font.setPixelSize(qMax(14, logical * 2 / 5));
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, logical, logical), Qt::AlignCenter, avatarInitial(name));
    // 右下角设备角标：白环隔离 + 浅蓝底，避免黏在头像上
    const int badge = qMax(14, logical * 14 / 44);
    const QRectF badgeRect(logical - badge - 1.0, logical - badge - 1.0, badge, badge);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(badgeRect.adjusted(-1.5, -1.5, 1.5, 1.5));
    p.setBrush(QColor(QStringLiteral("#eff6ff")));
    p.drawEllipse(badgeRect);
    DeviceKind kind = DevLaptop;
    const int k = deviceKindFromOs(osName);
    if (k == 1)
        kind = DevPhone;
    else if (k == 2)
        kind = DevTablet;
    paintDeviceGlyph(p, kind, badgeRect.adjusted(2.8, 2.8, -2.8, -2.8),
                     QColor(QStringLiteral("#2563eb")));
    return pm;
}

QPixmap makePeerListAvatar(const QString &name, const QString &osName, int unread, int logical,
                           bool pinned)
{
    QPixmap base = makePeerAvatar(name, osName, logical);
    if (unread <= 0 && !pinned)
        return base;
    QPixmap pm = base;
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    if (pinned) {
        // 左下角小钉：一眼识别置顶
        const QRectF pin(2.0, logical - 16.0, 14.0, 14.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#2563eb")));
        p.drawEllipse(pin);
        p.setPen(QPen(Qt::white, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(pin.x() + 3.5, pin.y() + 2.5, 7.0, 7.0));
        p.drawLine(QPointF(pin.center().x(), pin.y() + 9.0),
                   QPointF(pin.center().x(), pin.y() + 11.5));
    }
    if (unread > 0) {
        const QString label = unread > 99 ? QStringLiteral("99+") : QString::number(unread);
        QFont font = qApp->font();
        font.setPixelSize(unread > 99 ? 8 : 10);
        font.setBold(true);
        p.setFont(font);
        QFontMetrics fm(font);
        const int tw = fm.horizontalAdvance(label);
        const int pillW = qMax(16, tw + 8);
        const int pillH = 16;
        const QRectF pill(logical - pillW + 2.0, -2.0, pillW, pillH);
        p.setPen(QPen(Qt::white, 2.0));
        p.setBrush(QColor(QStringLiteral("#ef4444")));
        p.drawRoundedRect(pill, pillH / 2.0, pillH / 2.0);
        p.setPen(Qt::white);
        p.drawText(pill, Qt::AlignCenter, label);
    }
    return pm;
}

QPixmap renderSvgIcon(const QString &resPath, int logical)
{
    QPixmap pm = makeDprPixmap(logical);
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
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(ok ? QColor(QStringLiteral("#22c55e")) : QColor(QStringLiteral("#f59e0b")));
    p.drawEllipse(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0));
    return pm;
}

QPixmap makeAlertTriangleIcon(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal s = logical;
    QPainterPath tri;
    tri.moveTo(s * 0.50, s * 0.12);
    tri.lineTo(s * 0.90, s * 0.86);
    tri.lineTo(s * 0.10, s * 0.86);
    tri.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#d97706")));
    p.drawPath(tri);
    QPen bar(Qt::white, qMax(1.2, s * 0.10));
    bar.setCapStyle(Qt::RoundCap);
    p.setPen(bar);
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(s * 0.50, s * 0.36), QPointF(s * 0.50, s * 0.58));
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(s * 0.50, s * 0.72), s * 0.055, s * 0.055);
    return pm;
}

QPixmap makeChatBubbleIcon(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
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

QPixmap makeSearchIcon(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal s = logical;
    QPen pen(QColor(QStringLiteral("#94a3b8")), qMax(1.4, s * 0.10));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal r = s * 0.28;
    const QPointF c(s * 0.42, s * 0.42);
    p.drawEllipse(c, r, r);
    p.drawLine(QPointF(c.x() + r * 0.72, c.y() + r * 0.72),
               QPointF(s * 0.82, s * 0.82));
    return pm;
}

QPixmap makeFileDocIcon(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
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
    QPixmap pm = makeDprPixmap(logical);
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

QPixmap makeTrashIcon(int logical)
{
    QPixmap pm = makeDprPixmap(logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal s = logical;
    QPen pen(QColor(QStringLiteral("#64748b")), qMax(1.3, s * 0.09));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(s * 0.28, s * 0.34), QPointF(s * 0.72, s * 0.34));
    p.drawLine(QPointF(s * 0.38, s * 0.34), QPointF(s * 0.42, s * 0.22));
    p.drawLine(QPointF(s * 0.62, s * 0.34), QPointF(s * 0.58, s * 0.22));
    p.drawLine(QPointF(s * 0.42, s * 0.22), QPointF(s * 0.58, s * 0.22));
    QPainterPath body;
    body.moveTo(s * 0.32, s * 0.38);
    body.lineTo(s * 0.36, s * 0.82);
    body.lineTo(s * 0.64, s * 0.82);
    body.lineTo(s * 0.68, s * 0.38);
    p.drawPath(body);
    p.drawLine(QPointF(s * 0.46, s * 0.46), QPointF(s * 0.46, s * 0.72));
    p.drawLine(QPointF(s * 0.54, s * 0.46), QPointF(s * 0.54, s * 0.72));
    return pm;
}

QPixmap loadSvgPixmap(const QString &path, int logical)
{
    QPixmap pm = makeDprPixmap(logical);
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
    b->setIconSize(QSize(18, 18));
    return b;
}

QIcon makePinIcon(bool pinned, const QColor &color)
{
    Q_UNUSED(color);
    // 成对 SVG：斜插镂空 / 垂直蓝实心；颜色写在矢量里，与齿轮线宽一致
    return QIcon(renderSvgIcon(pinned ? QStringLiteral(":/icons/pin-on.svg")
                                      : QStringLiteral(":/icons/pin-off.svg"),
                               18));
}

QPixmap makePeerStatusChip(const QString &text, const QColor &bg, const QColor &fg,
                           const QColor &border)
{
    QFont font = qApp->font();
    font.setPixelSize(10);
    font.setBold(true);
    QFontMetrics fm(font);
    const int padX = 6;
    const int padY = 2;
    const int innerH = qMax(16, fm.height() + padY * 2);
    const int innerW = fm.horizontalAdvance(text) + padX * 2;
    const qreal radius = 6.0;
    QPixmap pm = makeDprPixmap(innerW, innerH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(0.5, 0.5, innerW - 1.0, innerH - 1.0);
    p.setPen(QPen(border, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(box, radius, radius);
    p.setFont(font);
    p.setPen(fg);
    p.drawText(QRect(0, 0, innerW, innerH), Qt::AlignCenter, text);
    return pm;
}

QPixmap makeOsStatusChip(const QString &osName)
{
    const QString label = osDisplayLabel(osName);
    if (label.isEmpty())
        return QPixmap();
    const QString o = osName.trimmed().toLower();
    QColor bg(QStringLiteral("#f1f5f9"));
    QColor fg(QStringLiteral("#475569"));
    QColor border(QStringLiteral("#e2e8f0"));
    if (label == QLatin1String("Linux") || o.contains(QLatin1String("ubuntu"))
        || o.contains(QLatin1String("kylin"))) {
        bg = QColor(QStringLiteral("#fff7ed"));
        fg = QColor(QStringLiteral("#ea580c"));
        border = QColor(QStringLiteral("#fed7aa"));
    } else if (label == QLatin1String("ARM64") || o.contains(QLatin1String("arm"))) {
        bg = QColor(QStringLiteral("#ecfdf5"));
        fg = QColor(QStringLiteral("#059669"));
        border = QColor(QStringLiteral("#a7f3d0"));
    } else if (label == QLatin1String("Windows") || o.startsWith(QLatin1String("win"))) {
        bg = QColor(QStringLiteral("#eff6ff"));
        fg = QColor(QStringLiteral("#2563eb"));
        border = QColor(QStringLiteral("#bfdbfe"));
    }
    return makePeerStatusChip(label, bg, fg, border);
}
