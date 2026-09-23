#include "mainwindow.h"

#include "discovery.h"
#include "files.h"
#include "httpserver.h"
#include "qrcodegen.hpp"

#include <QApplication>
#include <QAction>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCursor>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <functional>
#include <QFrame>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QtMath>
#include <QPushButton>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QSystemTrayIcon>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QTime>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

enum ChromeIcon {
    IconSettings = 0,
    IconMinimize,
    IconMaximize,
    IconRestore,
    IconClose
};

static QIcon makeChromeIcon(ChromeIcon kind, const QColor &color)
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

static QPixmap makeGlobeBadge(int logical = 36)
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

static QPixmap makeRadioLogo(int logical = 36)
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

enum DeviceKind {
    DevLaptop = 0,
    DevPhone,
    DevTablet
};

static QPixmap renderSvgIcon(const QString &resPath, int logical = 16);

static QString avatarInitial(const QString &name)
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

static QPixmap makeLaptopIcon(int logical = 16)
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

static QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical = 44)
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

static QPixmap renderSvgIcon(const QString &resPath, int logical)
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

static QPushButton *toolLinkBtn(const QString &svgRes, const QString &text, const QString &objectName)
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

static QPixmap makeStatusDot(bool ok, int logical = 7)
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

static QPixmap makeChatBubbleIcon(int logical = 14)
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

static QPixmap makeFileDocIcon(int logical = 14)
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

static QPixmap makeCheckCircleIcon(int logical = 14)
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

static QPixmap loadSvgPixmap(const QString &path, int logical)
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

static QString nowClock()
{
    return QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
}

static QString htmlEsc(const QString &s)
{
    return s.toHtmlEscaped();
}

static QString humanBytesChat(qint64 n)
{
    if (n < 1024)
        return QString::number(n) + QStringLiteral(" B");
    if (n < 1024 * 1024)
        return QString::number(n / 1024.0, 'f', 1) + QStringLiteral(" KB");
    return QString::number(n / 1024.0 / 1024.0, 'f', 1) + QStringLiteral(" MB");
}

static QString fileSha256Short(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    QCryptographicHash h(QCryptographicHash::Sha256);
    if (!h.addData(&f))
        return QString();
    const QByteArray hex = h.result().toHex();
    return QString::fromLatin1(hex.left(16)) + QStringLiteral("...");
}

// Qt 富文本几乎不渲染 table 的 border-radius，头像/气泡改绘成 PNG 再嵌入。
static QString pixmapToImgHtml(const QPixmap &pm)
{
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    pm.save(&buf, "PNG");
    const qreal dpr = pm.devicePixelRatio();
    const int w = qMax(1, qRound(pm.width() / dpr));
    const int h = qMax(1, qRound(pm.height() / dpr));
    return QStringLiteral("<img src=\"data:image/png;base64,%1\" width=\"%2\" height=\"%3\"/>")
        .arg(QString::fromLatin1(bytes.toBase64()))
        .arg(w)
        .arg(h);
}

static QString letterAvatarHtml(const QString &name, const QString &bg)
{
    const QString ch = avatarInitial(name);
    const int logical = 40;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logical * dpr, logical * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(bg));
    // 参考图：大号圆角方头像（非直角小块）
    p.drawRoundedRect(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0), 10.0, 10.0);
    QFont font = qApp->font();
    font.setPixelSize(18);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, logical, logical), Qt::AlignCenter, ch);
    return pixmapToImgHtml(pm);
}

static QString faceName(const ChatMsg &m)
{
    return m.face.trimmed().isEmpty() ? m.who : m.face;
}

static QString textBubbleImgHtml(const QString &text, bool out)
{
    const int maxContentW = 340;
    const int padX = 14;
    const int padY = 10;
    const qreal radius = 12.0;
    QFont font = qApp->font();
    font.setPixelSize(14);
    QFontMetrics fm(font);
    const QRect textBound = fm.boundingRect(QRect(0, 0, maxContentW, 10000),
                                           Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                           text);
    const int contentW = qMax(24, qMin(maxContentW, textBound.width()));
    const int contentH = qMax(fm.height(), textBound.height());
    const int logicalW = contentW + padX * 2;
    const int logicalH = contentH + padY * 2;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(0.5, 0.5, logicalW - 1.0, logicalH - 1.0);
    if (out) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#2563eb")));
        p.drawRoundedRect(box, radius, radius);
        p.setPen(Qt::white);
    } else {
        p.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
        p.setBrush(Qt::white);
        p.drawRoundedRect(box, radius, radius);
        p.setPen(QColor(QStringLiteral("#0f172a")));
    }
    p.setFont(font);
    p.drawText(QRect(padX, padY, contentW, contentH),
               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
               text);
    return pixmapToImgHtml(pm);
}

static bool splitCodeFence(const QString &text, QString *lang, QString *body)
{
    const QString t = text;
    if (!t.startsWith(QStringLiteral("```")))
        return false;
    int nl = t.indexOf(QLatin1Char('\n'));
    if (nl < 0)
        return false;
    QString head = t.mid(3, nl - 3).trimmed();
    if (head.isEmpty())
        head = QStringLiteral("text");
    int end = t.lastIndexOf(QStringLiteral("```"));
    if (end <= nl)
        return false;
    *lang = head;
    *body = t.mid(nl + 1, end - nl - 1);
    if (body->endsWith(QLatin1Char('\n')))
        body->chop(1);
    return true;
}

static QString renderCodeBlock(const QString &lang, const QString &code)
{
    const QString href = QStringLiteral("landrop://copy/")
        + QString::fromLatin1(code.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return QStringLiteral(
               "<table cellspacing=\"0\" cellpadding=\"8\" bgcolor=\"#1e293b\" width=\"420\">"
               "<tr><td>"
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\"><tr>"
               "<td><font color=\"#94a3b8\" size=\"2\">%1</font></td>"
               "<td align=\"right\"><a href=\"%2\" style=\"color:#93c5fd;text-decoration:none;\">"
               "<font color=\"#93c5fd\" size=\"2\">%4</font></a></td>"
               "</tr></table>"
               "<pre style=\"margin:6px 0 0 0;\"><font color=\"#e2e8f0\" face=\"Consolas, Courier New, monospace\" size=\"2\">%3</font></pre>"
               "</td></tr></table>")
        .arg(htmlEsc(lang), href, htmlEsc(code), QString::fromUtf8(u8"复制"));
}

static QString metaLine(const QString &who, const QString &time, qint64 rttMs, bool failed)
{
    QString mid = htmlEsc(who) + QStringLiteral(" ") + htmlEsc(time);
    if (failed)
        return mid + QString::fromUtf8(u8" <font color=\"#dc2626\" size=\"2\">发送失败</font>");
    if (rttMs >= 0) {
        const QString ms = (rttMs < 1) ? QStringLiteral("<1") : QString::number(rttMs);
        mid += QString::fromUtf8(u8" <font color=\"#16a34a\" size=\"2\">✓✓ 已送达 - %1ms</font>").arg(ms);
    }
    return QStringLiteral("<font color=\"#64748b\" size=\"2\">%1</font>").arg(mid);
}

// 参考图：头像与名字顶对齐；气泡在名字下方、相对头像斜对角偏下（勿把头像贴气泡底边）。
static QString renderMsgRow(bool out, const QString &meta, const QString &body, const QString &avatarHtml)
{
    if (out) {
        return QStringLiteral(
                   "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\"><tr>"
                   "<td></td>"
                   "<td align=\"right\" valign=\"top\">"
                   "<div align=\"right\">%1</div>"
                   "<div style=\"margin-top:6px;\" align=\"right\">%2</div>"
                   "</td>"
                   "<td width=\"48\" valign=\"top\">%3</td>"
                   "</tr></table>")
            .arg(meta, body, avatarHtml);
    }
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\"><tr>"
               "<td width=\"48\" valign=\"top\">%1</td>"
               "<td align=\"left\" valign=\"top\">"
               "<div>%2</div>"
               "<div style=\"margin-top:6px;\">%3</div>"
               "</td>"
               "<td></td>"
               "</tr></table>")
        .arg(avatarHtml, meta, body);
}

static QString renderTextBubble(const ChatMsg &m)
{
    QString lang;
    QString code;
    const bool out = (m.type == ChatMsg::OutText);
    const QString avatar = letterAvatarHtml(
        faceName(m), out ? QStringLiteral("#2563eb") : QStringLiteral("#f97316"));
    const QString head = metaLine(m.who, m.time, out ? m.rttMs : -1, false);
    if (splitCodeFence(m.text, &lang, &code))
        return renderMsgRow(out, head, renderCodeBlock(lang, code), avatar);
    return renderMsgRow(out, head, textBubbleImgHtml(m.text, out), avatar);
}

static QString renderFileCard(const ChatMsg &m)
{
    const bool out = (m.type == ChatMsg::OutFile);
    const bool pending = out && m.progressPct >= 0;
    const QString size = humanBytesChat(m.size);
    QString sha = m.sha256;
    if (!pending && sha.isEmpty() && !m.path.isEmpty())
        sha = fileSha256Short(m.path);
    const QString pathB64 = (!pending && !m.path.isEmpty())
        ? QString::fromLatin1(m.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding))
        : QString();
    QString actions;
    if (!pathB64.isEmpty()) {
        const QString openHref = QStringLiteral("landrop://open/") + pathB64;
        const QString revealHref = QStringLiteral("landrop://reveal/") + pathB64;
        if (out) {
            actions = QString::fromUtf8(
                          u8"<a href=\"%1\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#2563eb\" size=\"2\">打开文件</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%2\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">打开所在目录</font></a>")
                          .arg(openHref, revealHref);
        } else {
            actions = QString::fromUtf8(
                          u8"<a href=\"%1\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#2563eb\" size=\"2\">打开文件</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%2\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">打开所在目录</font></a>"
                          u8"&nbsp;&nbsp;<font color=\"#94a3b8\" size=\"1\">局域网直传 · 已存入下载目录</font>")
                          .arg(openHref, revealHref);
        }
    } else if (pending) {
        actions = QString::fromUtf8(u8"<font color=\"#94a3b8\" size=\"2\">局域网直传</font>");
    } else {
        actions = QString::fromUtf8(u8"<font color=\"#94a3b8\" size=\"2\">局域网直传</font>");
    }
    const int pct = pending ? qBound(0, 100, m.progressPct) : 100;
    const int rest = 100 - pct;
    QString bar;
    if (pct <= 0) {
        bar = QString::fromUtf8(
            u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#e2e8f0\">"
            u8"<tr><td height=\"6\"></td></tr></table>");
    } else if (rest <= 0) {
        bar = QString::fromUtf8(
            u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" bgcolor=\"#2563eb\">"
            u8"<tr><td height=\"6\"></td></tr></table>");
    } else {
        bar = QString::fromUtf8(
                 u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\"><tr>"
                 u8"<td width=\"%1%\" bgcolor=\"#2563eb\">"
                 u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">"
                 u8"<tr><td height=\"6\"></td></tr></table></td>"
                 u8"<td width=\"%2%\" bgcolor=\"#e2e8f0\">"
                 u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">"
                 u8"<tr><td height=\"6\"></td></tr></table></td>"
                 u8"</tr></table>")
                 .arg(pct)
                 .arg(rest);
    }
    const QString status = pending
        ? QString::fromUtf8(u8"<font color=\"#2563eb\" size=\"2\">发送中 %1%</font>").arg(pct)
        : QString::fromUtf8(u8"<font color=\"#16a34a\" size=\"2\">✓✓ 传输完成 (已落盘)</font>");
    const QString shaLine = (!pending && !sha.isEmpty())
        ? QStringLiteral("<br/><font color=\"#94a3b8\" size=\"1\">SHA256: %1</font>").arg(htmlEsc(sha))
        : QString();
    const QString card =
        QString::fromUtf8(
            u8"<table cellspacing=\"0\" cellpadding=\"10\" bgcolor=\"#ffffff\" width=\"360\" "
            u8"style=\"border:1px solid #e2e8f0;\">"
            u8"<tr><td>"
            u8"<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\"><tr>"
            u8"<td width=\"36\" valign=\"top\"><table cellpadding=\"4\" bgcolor=\"#ede9fe\">"
            u8"<tr><td><font color=\"#7c3aed\" size=\"2\"><b>FILE</b></font></td></tr></table></td>"
            u8"<td>"
            u8"<font color=\"#0f172a\" size=\"3\"><b>%1</b></font><br/>"
            u8"<font color=\"#94a3b8\" size=\"2\">%2</font>"
            u8"</td></tr></table>"
            u8"%3"
            u8"%4"
            u8"%5"
            u8"<br/>%6"
            u8"</td></tr></table>")
            .arg(htmlEsc(m.text), size, bar, status, shaLine, actions);
    const QString head = metaLine(m.who, m.time, pending ? -1 : m.rttMs, false);
    const QString avatar = letterAvatarHtml(
        faceName(m), out ? QStringLiteral("#2563eb") : QStringLiteral("#f97316"));
    return renderMsgRow(out, head, card, avatar);
}

static QString renderSystem(const ChatMsg &m)
{
    const bool fail = (m.type == ChatMsg::Fail);
    const QString bg = fail ? QStringLiteral("#fef2f2") : QStringLiteral("#fffbeb");
    const QString fg = fail ? QStringLiteral("#b91c1c") : QStringLiteral("#b45309");
    QString body = htmlEsc(m.text);
    if (fail && !m.path.isEmpty()) {
        const QString href = QStringLiteral("landrop://retry/")
            + QString::fromLatin1(m.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
        body += QString::fromUtf8(
                    u8"&nbsp;&nbsp;<a href=\"%1\" style=\"text-decoration:none;\">"
                    u8"<font color=\"#2563eb\" size=\"2\">重试</font></a>")
                    .arg(href);
    }
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr><td align=\"center\">"
               "<table cellspacing=\"0\" cellpadding=\"6\" bgcolor=\"%1\">"
               "<tr><td><font color=\"%2\" size=\"2\">%3</font></td></tr></table>"
               "</td></tr></table>")
        .arg(bg, fg, body);
}

static QString renderChatHtml(const QVector<ChatMsg> &msgs)
{
    QString html = QStringLiteral(
        "<html><body style=\"margin:0;padding:8px;background:#f1f5f9;\">");
    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg &m = msgs.at(i);
        html += QStringLiteral("<div style=\"margin:10px 0;\">");
        switch (m.type) {
        case ChatMsg::OutText:
        case ChatMsg::InText:
            html += renderTextBubble(m);
            break;
        case ChatMsg::OutFile:
        case ChatMsg::InFile:
            html += renderFileCard(m);
            break;
        case ChatMsg::System:
        case ChatMsg::Fail:
            html += renderSystem(m);
            break;
        }
        html += QStringLiteral("</div>");
    }
    html += QStringLiteral("</body></html>");
    return html;
}

static int countFiles(const QVector<ChatMsg> &msgs)
{
    int n = 0;
    for (int i = 0; i < msgs.size(); ++i) {
        if (msgs.at(i).type == ChatMsg::InFile || msgs.at(i).type == ChatMsg::OutFile)
            ++n;
    }
    return n;
}

static QString renderFilesHtml(const QVector<ChatMsg> &msgs)
{
    QString html = QStringLiteral(
        "<html><body style=\"margin:0;padding:8px;background:#f8fafc;\">");
    int n = 0;
    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg &m = msgs.at(i);
        if (m.type != ChatMsg::InFile && m.type != ChatMsg::OutFile)
            continue;
        html += QStringLiteral("<div style=\"margin:10px 0;\">");
        html += renderFileCard(m);
        html += QStringLiteral("</div>");
        ++n;
    }
    if (n == 0) {
        html += QStringLiteral(
            "<p align=\"center\"><font color=\"#94a3b8\">还没有与该对端的文件传输</font></p>");
    }
    html += QStringLiteral("</body></html>");
    return html;
}

static QPushButton *chromeBtn(ChromeIcon kind, const QString &objectName, const QString &tip)
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

// 网页共享 / 聊天区共用：本地文件与文件夹顶层文件
static QStringList localSendPathsFromUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    for (int i = 0; i < urls.size(); ++i) {
        if (!urls.at(i).isLocalFile())
            continue;
        const QString p = urls.at(i).toLocalFile();
        const QFileInfo fi(p);
        if (fi.isFile()) {
            paths.append(fi.absoluteFilePath());
        } else if (fi.isDir()) {
            const QFileInfoList kids = QDir(p).entryInfoList(
                QDir::Files | QDir::Readable, QDir::Name);
            for (int k = 0; k < kids.size(); ++k)
                paths.append(kids.at(k).absoluteFilePath());
        }
    }
    return paths;
}

class ShareDropFilter : public QObject
{
public:
    explicit ShareDropFilter(QObject *parent = 0) : QObject(parent) {}
    std::function<void(const QStringList &)> onFiles;
    std::function<void(bool)> onActive; // 拖入/拖出高亮（可选）

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        Q_UNUSED(watched);
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *de = static_cast<QDragEnterEvent *>(event);
            if (de->mimeData() && de->mimeData()->hasUrls()) {
                de->acceptProposedAction();
                ++m_depth;
                if (m_depth == 1 && onActive)
                    onActive(true);
                return true;
            }
        }
        if (event->type() == QEvent::DragMove) {
            QDragMoveEvent *de = static_cast<QDragMoveEvent *>(event);
            if (de->mimeData() && de->mimeData()->hasUrls()) {
                de->acceptProposedAction();
                return true;
            }
        }
        if (event->type() == QEvent::DragLeave) {
            m_depth = qMax(0, m_depth - 1);
            if (m_depth == 0 && onActive)
                onActive(false);
            return false;
        }
        if (event->type() == QEvent::Drop) {
            QDropEvent *de = static_cast<QDropEvent *>(event);
            m_depth = 0;
            if (onActive)
                onActive(false);
            QStringList paths;
            if (de->mimeData())
                paths = localSendPathsFromUrls(de->mimeData()->urls());
            if (!paths.isEmpty() && onFiles) {
                onFiles(paths);
                de->acceptProposedAction();
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    int m_depth = 0;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8(u8"局域快传"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    resize(1180, 720);
    setMinimumSize(900, 560);
    statusBar()->hide();

    m_disc = new Discovery(this);
    m_http = new HttpServer(this);
    m_nam = new QNetworkAccessManager(this);
    connect(m_disc, SIGNAL(changed()), this, SLOT(refreshPeers()));
    connect(m_http, SIGNAL(textArrived(QString,QString,QString,int,QString)),
            this, SLOT(onText(QString,QString,QString,int,QString)));
    connect(m_http, SIGNAL(fileArrived(QString,QString,QString,qint64)),
            this, SLOT(onFile(QString,QString,QString,qint64)));

    buildUi();
    applyStyle();
    setupChatDrop();

    m_chatSaveTimer = new QTimer(this);
    m_chatSaveTimer->setSingleShot(true);
    connect(m_chatSaveTimer, SIGNAL(timeout()), this, SLOT(flushChatHistory()));

    QTimer *tick = new QTimer(this);
    connect(tick, SIGNAL(timeout()), this, SLOT(refreshPeers()));
    tick->start(1000);
    QTimer *ping = new QTimer(this);
    connect(ping, SIGNAL(timeout()), this, SLOT(measurePing()));
    ping->start(2000);
    setupTray();
    boot();
}

void MainWindow::buildUi()
{
    QWidget *root = new QWidget;
    root->setObjectName(QStringLiteral("root"));
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // —— 自绘顶栏：品牌 + 本机信息 + 操作 + 窗口按钮 ——
    m_titleBar = new QWidget;
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(64);
    m_titleBar->installEventFilter(this);
    QHBoxLayout *titleLay = new QHBoxLayout(m_titleBar);
    titleLay->setContentsMargins(16, 0, 8, 0);
    titleLay->setSpacing(12);

    QLabel *logo = new QLabel;
    logo->setObjectName(QStringLiteral("logo"));
    logo->setFixedSize(36, 36);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(makeRadioLogo(36));

    // 参考图：两行文字块高度与图标齐平（顶对齐标题、底对齐状态行）
    QWidget *brandWrap = new QWidget;
    brandWrap->setObjectName(QStringLiteral("brandWrap"));
    brandWrap->setFixedHeight(36);
    QVBoxLayout *brandCol = new QVBoxLayout(brandWrap);
    brandCol->setSpacing(1);
    brandCol->setContentsMargins(0, 0, 0, 0);
    QLabel *brand = new QLabel(QString::fromUtf8(u8"局域快传"));
    brand->setObjectName(QStringLiteral("brand"));
    brand->setFixedHeight(20);
    brand->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QWidget *statusWrap = new QWidget;
    statusWrap->setFixedHeight(15);
    QHBoxLayout *statusRow = new QHBoxLayout(statusWrap);
    statusRow->setContentsMargins(0, 0, 0, 0);
    statusRow->setSpacing(5);
    m_statusDot = new QLabel;
    m_statusDot->setFixedSize(7, 7);
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("statusOnline"));
    m_statusLabel->setTextFormat(Qt::PlainText);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusRow->addWidget(m_statusDot, 0, Qt::AlignVCenter);
    statusRow->addWidget(m_statusLabel, 0, Qt::AlignVCenter);
    statusRow->addStretch(1);
    brandCol->addWidget(brand, 0, Qt::AlignLeft | Qt::AlignTop);
    brandCol->addWidget(statusWrap, 0, Qt::AlignLeft | Qt::AlignBottom);

    QHBoxLayout *brandRow = new QHBoxLayout;
    brandRow->setSpacing(10);
    brandRow->setContentsMargins(0, 0, 0, 0);
    brandRow->addWidget(logo, 0, Qt::AlignVCenter);
    brandRow->addWidget(brandWrap, 0, Qt::AlignVCenter);

    m_hostPill = new QWidget;
    m_hostPill->setObjectName(QStringLiteral("hostPill"));
    m_hostPill->setFixedHeight(28);
    m_hostPill->setCursor(Qt::PointingHandCursor);
    m_hostPill->setToolTip(QString::fromUtf8(u8"点击复制本机 IP:端口"));
    m_hostPill->installEventFilter(this);
    QHBoxLayout *pillLay = new QHBoxLayout(m_hostPill);
    pillLay->setContentsMargins(10, 0, 12, 0);
    pillLay->setSpacing(5);
    QLabel *hostIcon = new QLabel;
    hostIcon->setFixedSize(16, 16);
    hostIcon->setPixmap(makeLaptopIcon(16));
    hostIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    QLabel *hostTag = new QLabel(QString::fromUtf8(u8"本机:"));
    hostTag->setObjectName(QStringLiteral("hostTag"));
    hostTag->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_hostName = new QLabel;
    m_hostName->setObjectName(QStringLiteral("hostName"));
    m_hostName->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_hostIp = new QLabel;
    m_hostIp->setObjectName(QStringLiteral("hostIp"));
    m_hostIp->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    pillLay->addWidget(hostIcon, 0, Qt::AlignVCenter);
    pillLay->addWidget(hostTag, 0, Qt::AlignVCenter);
    pillLay->addWidget(m_hostName, 0, Qt::AlignVCenter);
    pillLay->addWidget(m_hostIp, 0, Qt::AlignVCenter);

    m_shareBtn = new QPushButton;
    m_shareBtn->setObjectName(QStringLiteral("shareBtn"));
    m_shareBtn->setCursor(Qt::PointingHandCursor);
    connect(m_shareBtn, SIGNAL(clicked()), this, SLOT(openShare()));
    refreshShareBtn();

    QPushButton *dlBtn = chromeBtn(IconSettings, QStringLiteral("iconBtn"),
                                   QString::fromUtf8(u8"打开下载目录"));
    dlBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/folder-plus.svg"), 20)));
    dlBtn->setIconSize(QSize(20, 20));
    dlBtn->setCursor(Qt::PointingHandCursor);
    connect(dlBtn, SIGNAL(clicked()), this, SLOT(openDownloadDir()));

    QPushButton *setBtn = chromeBtn(IconSettings, QStringLiteral("iconBtn"), QString::fromUtf8(u8"设置"));
    setBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/settings.svg"), 20)));
    setBtn->setIconSize(QSize(20, 20));
    setBtn->setCursor(Qt::PointingHandCursor);
    connect(setBtn, SIGNAL(clicked()), this, SLOT(editSettings()));

    QPushButton *minBtn = chromeBtn(IconMinimize, QStringLiteral("minBtn"), QString::fromUtf8(u8"最小化"));
    m_maxBtn = chromeBtn(IconMaximize, QStringLiteral("maxBtn"), QString::fromUtf8(u8"最大化"));
    QPushButton *closeBtn = chromeBtn(IconClose, QStringLiteral("closeBtn"), QString::fromUtf8(u8"关闭"));
    connect(minBtn, SIGNAL(clicked()), this, SLOT(minimizeWin()));
    connect(m_maxBtn, SIGNAL(clicked()), this, SLOT(toggleMax()));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeWin()));

    // 左右等宽，胶囊落在窗口水平正中
    QWidget *leftZone = new QWidget;
    QHBoxLayout *leftLay = new QHBoxLayout(leftZone);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);
    leftLay->addLayout(brandRow);
    leftLay->addStretch(1);

    QWidget *rightZone = new QWidget;
    QHBoxLayout *chromeLay = new QHBoxLayout(rightZone);
    chromeLay->setContentsMargins(0, 0, 0, 0);
    chromeLay->setSpacing(12);
    chromeLay->addStretch(1);
    chromeLay->addWidget(m_shareBtn);
    chromeLay->addWidget(dlBtn);
    chromeLay->addWidget(setBtn);
    chromeLay->addSpacing(6);
    chromeLay->addWidget(minBtn);
    chromeLay->addWidget(m_maxBtn);
    chromeLay->addWidget(closeBtn);

    titleLay->addWidget(leftZone, 1);
    titleLay->addWidget(m_hostPill, 0, Qt::AlignVCenter);
    titleLay->addWidget(rightZone, 1);

    // —— 主体：左设备列表 + 右会话 ——
    QWidget *body = new QWidget;
    body->setObjectName(QStringLiteral("body"));
    QHBoxLayout *bodyLay = new QHBoxLayout(body);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(0);

    QWidget *side = new QWidget;
    m_side = side;
    side->setObjectName(QStringLiteral("side"));
    side->setMinimumWidth(220);
    side->setMaximumWidth(480);
    QVBoxLayout *sideLay = new QVBoxLayout(side);
    sideLay->setContentsMargins(16, 14, 16, 14);
    sideLay->setSpacing(10);

    QHBoxLayout *sideHead = new QHBoxLayout;
    QLabel *sideTitle = new QLabel(QString::fromUtf8(u8"附近在线设备"));
    sideTitle->setObjectName(QStringLiteral("sideTitle"));
    m_peerCount = new QLabel(QStringLiteral("0"));
    m_peerCount->setObjectName(QStringLiteral("peerCount"));
    QPushButton *addBtn = new QPushButton(QString::fromUtf8(u8"+ 加 IP"));
    addBtn->setObjectName(QStringLiteral("addBtn"));
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, SIGNAL(clicked()), this, SLOT(addPeer()));
    sideHead->addWidget(sideTitle);
    sideHead->addWidget(m_peerCount);
    sideHead->addStretch(1);
    sideHead->addWidget(addBtn);

    m_search = new QLineEdit;
    m_search->setObjectName(QStringLiteral("search"));
    m_search->setPlaceholderText(QString::fromUtf8(u8"搜索设备名称或 IP..."));
    connect(m_search, SIGNAL(textChanged(QString)), this, SLOT(filterPeers(QString)));

    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("peerList"));
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setIconSize(QSize(44, 44));
    m_list->setSpacing(2);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, SIGNAL(currentRowChanged(int)), this, SLOT(showChat()));
    connect(m_list, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(peerListContextMenu(QPoint)));
    m_list->installEventFilter(this);

    sideLay->addLayout(sideHead);
    sideLay->addWidget(m_search);
    sideLay->addWidget(m_list, 1);

    QWidget *right = new QWidget;
    right->setObjectName(QStringLiteral("right"));
    QVBoxLayout *rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    m_pages = new QStackedWidget;
    m_emptyHint = new QLabel(QString::fromUtf8(u8"还没有对端。同一网段等待发现，或点左侧「加 IP」。"));
    m_emptyHint->setObjectName(QStringLiteral("emptyHint"));
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setWordWrap(true);

    QWidget *chatPage = new QWidget;
    m_chatPage = chatPage;
    QVBoxLayout *chatLay = new QVBoxLayout(chatPage);
    chatLay->setContentsMargins(0, 0, 0, 0);
    chatLay->setSpacing(0);

    m_peerHeader = new QWidget;
    m_peerHeader->setObjectName(QStringLiteral("peerHeader"));
    m_peerHeader->setFixedHeight(64);
    QHBoxLayout *peerHeadLay = new QHBoxLayout(m_peerHeader);
    peerHeadLay->setContentsMargins(16, 8, 16, 8);
    peerHeadLay->setSpacing(12);

    m_peerAvatar = new QLabel;
    m_peerAvatar->setObjectName(QStringLiteral("peerAvatar"));
    m_peerAvatar->setFixedSize(44, 44);

    QVBoxLayout *peerInfoCol = new QVBoxLayout;
    peerInfoCol->setContentsMargins(0, 0, 0, 0);
    peerInfoCol->setSpacing(3);
    QHBoxLayout *peerTitleRow = new QHBoxLayout;
    peerTitleRow->setContentsMargins(0, 0, 0, 0);
    peerTitleRow->setSpacing(8);
    m_peerName = new QLabel;
    m_peerName->setObjectName(QStringLiteral("peerName"));
    m_peerOnlineDot = new QLabel;
    m_peerOnlineDot->setFixedSize(8, 8);
    m_peerAddr = new QLabel;
    m_peerAddr->setObjectName(QStringLiteral("peerAddr"));
    m_peerAddr->setCursor(Qt::PointingHandCursor);
    m_peerAddr->setToolTip(QString::fromUtf8(u8"点击复制对端 IP:端口"));
    m_peerAddr->installEventFilter(this);
    peerTitleRow->addWidget(m_peerName, 0, Qt::AlignVCenter);
    peerTitleRow->addWidget(m_peerOnlineDot, 0, Qt::AlignVCenter);
    peerTitleRow->addWidget(m_peerAddr, 0, Qt::AlignVCenter);
    peerTitleRow->addStretch(1);
    m_peerMeta = new QLabel;
    m_peerMeta->setObjectName(QStringLiteral("peerMeta"));
    peerInfoCol->addLayout(peerTitleRow);
    peerInfoCol->addWidget(m_peerMeta);

    m_tabChat = new QPushButton(QString::fromUtf8(u8"即时聊天"));
    m_tabChat->setObjectName(QStringLiteral("sessionTab"));
    m_tabChat->setCursor(Qt::PointingHandCursor);
    m_tabChat->setFocusPolicy(Qt::NoFocus);
    m_tabChat->setIcon(QIcon(makeChatBubbleIcon(14)));
    m_tabChat->setIconSize(QSize(14, 14));
    m_tabFiles = new QPushButton(QString::fromUtf8(u8"文件传输 (0)"));
    m_tabFiles->setObjectName(QStringLiteral("sessionTab"));
    m_tabFiles->setCursor(Qt::PointingHandCursor);
    m_tabFiles->setFocusPolicy(Qt::NoFocus);
    m_tabFiles->setIcon(QIcon(makeFileDocIcon(14)));
    m_tabFiles->setIconSize(QSize(14, 14));
    connect(m_tabChat, SIGNAL(clicked()), this, SLOT(showChatTab()));
    connect(m_tabFiles, SIGNAL(clicked()), this, SLOT(showFilesTab()));

    peerHeadLay->addWidget(m_peerAvatar, 0, Qt::AlignVCenter);
    peerHeadLay->addLayout(peerInfoCol, 1);
    peerHeadLay->addWidget(m_tabChat, 0, Qt::AlignVCenter);
    peerHeadLay->addWidget(m_tabFiles, 0, Qt::AlignVCenter);

    m_connBannerHost = new QWidget;
    m_connBannerHost->setObjectName(QStringLiteral("connBannerHost"));
    m_connBannerHost->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout *bannerHostLay = new QHBoxLayout(m_connBannerHost);
    bannerHostLay->setContentsMargins(16, 10, 16, 8);
    bannerHostLay->setSpacing(0);
    // QFrame + 半高圆角：QWidget 上 border-radius:999 在 Windows 常画成直角
    QFrame *banner = new QFrame;
    m_connBanner = banner;
    banner->setObjectName(QStringLiteral("connBanner"));
    banner->setFrameShape(QFrame::NoFrame);
    banner->setAttribute(Qt::WA_StyledBackground, true);
    banner->setFixedHeight(32);
    QHBoxLayout *bannerLay = new QHBoxLayout(banner);
    bannerLay->setContentsMargins(14, 0, 16, 0);
    bannerLay->setSpacing(8);
    QLabel *bannerIcon = new QLabel;
    bannerIcon->setFixedSize(16, 16);
    bannerIcon->setPixmap(loadSvgPixmap(QStringLiteral(":/icons/shield-check.svg"), 16));
    m_connBannerText = new QLabel;
    m_connBannerText->setObjectName(QStringLiteral("connBannerText"));
    m_connBannerText->setTextFormat(Qt::RichText);
    bannerLay->addWidget(bannerIcon, 0, Qt::AlignVCenter);
    bannerLay->addWidget(m_connBannerText, 0, Qt::AlignVCenter);
    bannerHostLay->addStretch(1);
    bannerHostLay->addWidget(banner, 0, Qt::AlignCenter);
    bannerHostLay->addStretch(1);

    QWidget *chatBody = new QWidget;
    QVBoxLayout *chatBodyLay = new QVBoxLayout(chatBody);
    chatBodyLay->setContentsMargins(0, 0, 0, 0);
    chatBodyLay->setSpacing(0);
    m_chatHost = new QWidget;
    m_chatHost->setObjectName(QStringLiteral("chatHost"));
    QVBoxLayout *chatHostLay = new QVBoxLayout(m_chatHost);
    chatHostLay->setContentsMargins(0, 0, 0, 0);
    chatHostLay->setSpacing(0);
    m_chat = new QTextBrowser;
    m_chat->setObjectName(QStringLiteral("chat"));
    m_chat->setReadOnly(true);
    m_chat->setFrameShape(QFrame::NoFrame);
    m_chat->setOpenExternalLinks(false);
    m_chat->setOpenLinks(false);
    connect(m_chat, SIGNAL(anchorClicked(QUrl)), this, SLOT(onChatAnchor(QUrl)));
    chatHostLay->addWidget(m_chat);
    m_jumpBottomBtn = new QPushButton(m_chatHost);
    m_jumpBottomBtn->setObjectName(QStringLiteral("jumpBottomBtn"));
    m_jumpBottomBtn->setCursor(Qt::PointingHandCursor);
    m_jumpBottomBtn->setFocusPolicy(Qt::NoFocus);
    m_jumpBottomBtn->setText(QString::fromUtf8(u8"有新消息 ↓"));
    m_jumpBottomBtn->hide();
    connect(m_jumpBottomBtn, SIGNAL(clicked()), this, SLOT(jumpChatToBottom()));
    connect(m_chat->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        syncJumpBottomBtn();
    });
    m_chatHost->installEventFilter(this);

    m_composer = new QWidget;
    m_composer->setObjectName(QStringLiteral("composer"));
    QVBoxLayout *compCol = new QVBoxLayout(m_composer);
    compCol->setContentsMargins(16, 10, 16, 14);
    compCol->setSpacing(8);
    m_progress = new QLabel;
    m_progress->setObjectName(QStringLiteral("progress"));
    m_progress->hide();

    QHBoxLayout *toolLay = new QHBoxLayout;
    toolLay->setContentsMargins(0, 0, 0, 0);
    toolLay->setSpacing(4);
    QPushButton *fileBtn = toolLinkBtn(QStringLiteral(":/icons/paperclip.svg"),
                                       QString::fromUtf8(u8"发送文件"), QStringLiteral("toolBtn"));
    QPushButton *folderBtn = toolLinkBtn(QStringLiteral(":/icons/folder-plus.svg"),
                                         QString::fromUtf8(u8"发送文件夹"), QStringLiteral("toolBtn"));
    QPushButton *nudgeBtn = toolLinkBtn(QStringLiteral(":/icons/zap.svg"),
                                        QString::fromUtf8(u8"抖动窗口"), QStringLiteral("toolBtn"));
    connect(fileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(folderBtn, SIGNAL(clicked()), this, SLOT(sendFolder()));
    connect(nudgeBtn, SIGNAL(clicked()), this, SLOT(nudgePeer()));
    // Qt 富文本不画 span 边框，键帽用独立标签才能看出描边
    auto hintBit = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("inputHint"));
        return l;
    };
    auto keycap = [](const QString &text) {
        QLabel *k = new QLabel(text);
        k->setObjectName(QStringLiteral("keycap"));
        return k;
    };
    QWidget *hintRow = new QWidget;
    QHBoxLayout *hintLay = new QHBoxLayout(hintRow);
    hintLay->setContentsMargins(0, 0, 0, 0);
    hintLay->setSpacing(4);
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"按")));
    hintLay->addWidget(keycap(QStringLiteral("Enter")));
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"发送，")));
    hintLay->addWidget(keycap(QStringLiteral("Shift+Enter")));
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"换行")));
    toolLay->addWidget(fileBtn);
    toolLay->addWidget(folderBtn);
    toolLay->addWidget(nudgeBtn);
    toolLay->addStretch(1);
    toolLay->addWidget(hintRow);

    m_inputShell = new QWidget;
    m_inputShell->setObjectName(QStringLiteral("inputShell"));
    QVBoxLayout *shellLay = new QVBoxLayout(m_inputShell);
    shellLay->setContentsMargins(12, 10, 10, 10);
    shellLay->setSpacing(4);
    m_input = new QPlainTextEdit;
    m_input->setObjectName(QStringLiteral("input"));
    m_input->setFrameShape(QFrame::NoFrame);
    m_input->setFixedHeight(72);
    m_input->setTabChangesFocus(true);
    m_input->installEventFilter(this);
    m_sendBtn = new QPushButton;
    m_sendBtn->setObjectName(QStringLiteral("sendFab"));
    m_sendBtn->setFixedSize(34, 34);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFocusPolicy(Qt::NoFocus);
    m_sendBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/send.svg"), 18)));
    m_sendBtn->setIconSize(QSize(18, 18));
    connect(m_sendBtn, SIGNAL(clicked()), this, SLOT(sendText()));
    QHBoxLayout *sendRow = new QHBoxLayout;
    sendRow->setContentsMargins(0, 0, 0, 0);
    sendRow->addStretch(1);
    sendRow->addWidget(m_sendBtn);
    shellLay->addWidget(m_input, 1);
    shellLay->addLayout(sendRow);

    compCol->addWidget(m_progress);
    compCol->addLayout(toolLay);
    compCol->addWidget(m_inputShell);

    chatBodyLay->addWidget(m_chatHost, 1);
    chatBodyLay->addWidget(m_composer);

    QWidget *filesPage = new QWidget;
    QVBoxLayout *filesLay = new QVBoxLayout(filesPage);
    filesLay->setContentsMargins(0, 0, 0, 0);
    filesLay->setSpacing(0);
    m_fileLive = new QLabel;
    m_fileLive->setObjectName(QStringLiteral("progress"));
    m_fileLive->setContentsMargins(16, 10, 16, 0);
    m_fileLive->hide();
    m_files = new QTextBrowser;
    m_files->setObjectName(QStringLiteral("filesView"));
    m_files->setReadOnly(true);
    m_files->setFrameShape(QFrame::NoFrame);
    m_files->setOpenExternalLinks(false);
    m_files->setOpenLinks(false);
    connect(m_files, SIGNAL(anchorClicked(QUrl)), this, SLOT(onChatAnchor(QUrl)));
    filesLay->addWidget(m_fileLive);
    filesLay->addWidget(m_files, 1);

    m_sessionStack = new QStackedWidget;
    m_sessionStack->setObjectName(QStringLiteral("sessionStack"));
    m_sessionStack->addWidget(chatBody);
    m_sessionStack->addWidget(filesPage);

    chatLay->addWidget(m_peerHeader);
    chatLay->addWidget(m_connBannerHost);
    chatLay->addWidget(m_sessionStack, 1);

    m_chatDropHint = new QFrame(chatPage);
    m_chatDropHint->setObjectName(QStringLiteral("chatDropHint"));
    m_chatDropHint->setAttribute(Qt::WA_StyledBackground, true);
    m_chatDropHint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_chatDropHint->hide();
    QVBoxLayout *dropHintLay = new QVBoxLayout(m_chatDropHint);
    dropHintLay->setContentsMargins(24, 24, 24, 24);
    m_chatDropHintLabel = new QLabel;
    m_chatDropHintLabel->setObjectName(QStringLiteral("chatDropHintLabel"));
    m_chatDropHintLabel->setAlignment(Qt::AlignCenter);
    m_chatDropHintLabel->setWordWrap(true);
    dropHintLay->addStretch(1);
    dropHintLay->addWidget(m_chatDropHintLabel, 0, Qt::AlignCenter);
    dropHintLay->addStretch(1);

    m_pages->addWidget(m_emptyHint);
    m_pages->addWidget(chatPage);
    rightLay->addWidget(m_pages, 1);

    m_bodySplit = new QSplitter(Qt::Horizontal);
    m_bodySplit->setObjectName(QStringLiteral("bodySplit"));
    m_bodySplit->setHandleWidth(4);
    m_bodySplit->setChildrenCollapsible(false);
    m_bodySplit->addWidget(side);
    m_bodySplit->addWidget(right);
    m_bodySplit->setStretchFactor(0, 0);
    m_bodySplit->setStretchFactor(1, 1);
    bodyLay->addWidget(m_bodySplit);

    rootLay->addWidget(m_titleBar);
    rootLay->addWidget(body, 1);
    setCentralWidget(root);
    setSessionTab(0);
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "#root { background: #f8fafc; border: 1px solid #cbd5e1; }"
        "#titleBar { background: #ffffff; border-bottom: 1px solid #e2e8f0; }"
        "#logo { background: transparent; border: none; padding: 0; margin: 0; }"
        "#brandWrap { background: transparent; }"
        "#brand { color: #0f172a; font-size: 15px; font-weight: 700; padding: 0; margin: 0; }"
        "#statusOnline { color: #64748b; font-size: 11px; padding: 0; margin: 0; }"
        "#hostPill { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; }"
        "#hostPill:hover { background: #eff6ff; border-color: #93c5fd; }"
        "#hostTag { color: #64748b; font-size: 12px; }"
        "#hostName { color: #0f172a; font-size: 12px; font-weight: 600; }"
        "#hostIp { color: #94a3b8; font-size: 12px; font-family: Consolas, 'Courier New', monospace; }"
        "#shareBtn { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 8px; color: #1d4ed8;"
        " padding: 6px 12px; font-size: 12px; font-weight: 600; }"
        "#shareBtn:hover { background: #dbeafe; }"
        "#iconBtn { background: transparent; border: 1px solid transparent; border-radius: 8px; padding: 0; }"
        "#iconBtn:hover { background: #f1f5f9; border-color: #e2e8f0; }"
        "#minBtn, #maxBtn, #closeBtn { background: transparent; border: none; border-radius: 6px; padding: 0; }"
        "#minBtn:hover, #maxBtn:hover { background: #f1f5f9; }"
        "#closeBtn:hover { background: #ef4444; }"
        "#side { background: #ffffff; border-right: none; }"
        "#bodySplit::handle:horizontal { background: #e2e8f0; width: 4px; }"
        "#bodySplit::handle:horizontal:hover { background: #93c5fd; }"
        "#sideTitle { color: #0f172a; font-size: 13px; font-weight: 600; }"
        "#peerCount { background: #ecfdf5; color: #047857; border-radius: 8px; padding: 1px 7px;"
        " font-size: 11px; font-weight: 700; }"
        "#addBtn { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 8px; color: #1d4ed8;"
        " padding: 4px 10px; font-size: 12px; font-weight: 600; }"
        "#addBtn:hover { background: #dbeafe; }"
        "#search { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; padding: 8px 12px;"
        " color: #0f172a; selection-background-color: #bfdbfe; }"
        "#peerList { background: transparent; outline: none; }"
        "#peerList::item { background: transparent; border: 1px solid transparent; border-radius: 10px;"
        " padding: 8px 8px; margin: 2px 0; color: #0f172a; }"
        "#peerList::item:hover { background: #f8fafc; border-color: #e2e8f0; }"
        "#peerList::item:selected { background: #eff6ff; border-color: #bfdbfe; color: #1e3a8a; }"
        "#right { background: #f1f5f9; }"
        "#emptyHint { color: #94a3b8; font-size: 14px; padding: 40px; background: #f1f5f9; }"
        "#peerHeader { background: #ffffff; border-bottom: 1px solid #e2e8f0; }"
        "#peerName { color: #0f172a; font-size: 14px; font-weight: 700; }"
        "#peerAddr { color: #64748b; font-size: 11px; font-family: Consolas, 'Courier New', monospace;"
        " background: #f1f5f9; border-radius: 6px; padding: 2px 8px; }"
        "#peerAddr:hover { color: #1d4ed8; background: #dbeafe; }"
        "#peerMeta { color: #94a3b8; font-size: 11px; }"
        "#sessionTab { background: #ffffff; border: 1px solid #bfdbfe; border-radius: 10px;"
        " color: #1d4ed8; padding: 6px 12px; font-size: 12px; font-weight: 600; }"
        "#sessionTab:hover { background: #eff6ff; }"
        "#sessionTabActive { background: #eff6ff; border: 1px solid #93c5fd; border-radius: 10px;"
        " color: #1e40af; padding: 6px 12px; font-size: 12px; font-weight: 700; }"
        "#sessionTabActive:hover { background: #dbeafe; }"
        "#connBannerHost { background: #f1f5f9; }"
        "#connBanner { background-color: #ffffff; border: 1px solid #e2e8f0;"
        " border-radius: 16px; }"
        "#connBannerText { color: #64748b; font-size: 12px; background: transparent; }"
        "#filesView { background: #f1f5f9; border: none; }"
        "#chat { background: #f1f5f9; color: #0f172a; font-size: 13px; padding: 8px 12px; border: none; }"
        "#chatHost { background: #f1f5f9; }"
        "#jumpBottomBtn { background: #1e293b; color: #f8fafc; border: none; border-radius: 16px;"
        " padding: 6px 14px; font-size: 12px; font-weight: 600; }"
        "#jumpBottomBtn:hover { background: #334155; }"
        "#composer { background: #ffffff; border-top: 1px solid #e2e8f0; }"
        "#chatDropHint { background: rgba(239, 246, 255, 220); border: 2px dashed #3b82f6; border-radius: 12px; }"
        "#chatDropHintLabel { color: #1d4ed8; font-size: 15px; font-weight: 600; }"
        "#progress { color: #1d4ed8; font-size: 12px; }"
        "#toolBtn { background: transparent; border: none; color: #475569; font-size: 12px;"
        " padding: 4px 8px; border-radius: 6px; }"
        "#toolBtn:hover { background: #f1f5f9; color: #0f172a; }"
        "#inputHint { color: #94a3b8; font-size: 12px; background: transparent; border: none; }"
        "#keycap { color: #475569; background: #f8fafc; border: 1px solid #94a3b8;"
        " border-radius: 4px; padding: 1px 6px; font-size: 11px; font-weight: 600; }"
        "#inputShell { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }"
        "#input { background: transparent; border: none; color: #0f172a; font-size: 13px;"
        " padding: 0; selection-background-color: #bfdbfe; }"
        "#sendFab { background: #93c5fd; border: none; border-radius: 8px; padding: 0; }"
        "#sendFab:hover { background: #60a5fa; }"
        "#sendFab:pressed { background: #3b82f6; }"
        "#primaryBtn { background: #2563eb; border: none; border-radius: 8px; color: white;"
        " padding: 8px 16px; font-weight: 600; }"
        "#primaryBtn:hover { background: #1d4ed8; }"
        "#secondaryBtn { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 8px; color: #334155;"
        " padding: 8px 12px; }"
        "#secondaryBtn:hover { background: #f8fafc; }"
    ));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_chatPage && watched == m_chatPage && event->type() == QEvent::Resize
        && m_chatDropHint && m_chatDropHint->isVisible()) {
        setChatDropHint(true);
    }
    if (m_chatHost && watched == m_chatHost && event->type() == QEvent::Resize) {
        placeJumpBottomBtn();
    }
    if (m_hostPill && watched == m_hostPill
        && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            copyLocalAddr();
            return true;
        }
    }
    if (m_peerAddr && watched == m_peerAddr
        && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            copyPeerAddr();
            return true;
        }
    }
    if (m_trayToast && watched == m_trayToast
        && event->type() == QEvent::MouseButtonPress) {
        showFromTrayNotify();
        return true;
    }
    if ((watched == m_input || watched == m_chat) && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        const bool pasteMod = (ke->modifiers() & Qt::ControlModifier)
            || (ke->modifiers() & Qt::MetaModifier);
        if (pasteMod && ke->key() == Qt::Key_V) {
            if (tryPasteClipboardFiles())
                return true;
        }
        if (watched == m_input
            && (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
            && !(ke->modifiers() & Qt::ShiftModifier)) {
            sendText();
            return true;
        }
    }
    if (watched == m_list && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
            removeSelectedManualPeer();
            return true;
        }
    }
    if (watched == m_titleBar) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            toggleMax();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && !isMaximized()) {
                m_dragging = true;
                m_dragOrigin = me->globalPos() - frameGeometry().topLeft();
                return true;
            }
        }
        if (event->type() == QEvent::MouseMove && m_dragging) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            move(me->globalPos() - m_dragOrigin);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateChrome();
}

void MainWindow::updateChrome()
{
    if (!m_maxBtn)
        return;
    const bool maxed = isMaximized();
    m_maxBtn->setIcon(makeChromeIcon(maxed ? IconRestore : IconMaximize, QColor(QStringLiteral("#475569"))));
    m_maxBtn->setToolTip(maxed ? QString::fromUtf8(u8"还原") : QString::fromUtf8(u8"最大化"));
}

void MainWindow::minimizeWin()
{
    showMinimized();
}

void MainWindow::toggleMax()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
    updateChrome();
}

void MainWindow::closeWin()
{
    close();
}

void MainWindow::openDownloadDir()
{
    QString dir = m_settings.downloadDir.trimmed();
    if (dir.isEmpty())
        dir = QStringLiteral("./downloads");
    QDir().mkpath(dir);
    const QString abs = QFileInfo(dir).absoluteFilePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(abs))) {
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"),
                             QString::fromUtf8(u8"无法打开下载目录：\n%1").arg(abs));
    }
}

void MainWindow::setupTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon(makeRadioLogo(32)));
    m_tray->setToolTip(QString::fromUtf8(u8"局域快传 · 后台接收中"));
    QMenu *menu = new QMenu(this);
    QAction *showAct = menu->addAction(QString::fromUtf8(u8"显示主窗口"));
    QAction *dlAct = menu->addAction(QString::fromUtf8(u8"打开下载目录"));
    menu->addSeparator();
    QAction *quitAct = menu->addAction(QString::fromUtf8(u8"退出局域快传"));
    connect(showAct, SIGNAL(triggered()), this, SLOT(showFromTray()));
    connect(dlAct, SIGNAL(triggered()), this, SLOT(openDownloadDir()));
    connect(quitAct, SIGNAL(triggered()), this, SLOT(quitApp()));
    m_tray->setContextMenu(menu);
    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(onTrayActivated(QSystemTrayIcon::ActivationReason)));
    connect(m_tray, SIGNAL(messageClicked()), this, SLOT(showFromTrayNotify()));
    m_tray->show();
}

void MainWindow::showFromTray()
{
    hideTrayToast();
    showNormal();
    raise();
    activateWindow();
    clearUnread(currentKey());
}

void MainWindow::showFromTrayNotify()
{
    const QString key = m_trayNotifyKey;
    showFromTray();
    if (!key.isEmpty()) {
        selectPeerByKey(key);
        setSessionTab(0);
    }
}

void MainWindow::selectPeerByKey(const QString &key)
{
    if (key.isEmpty() || !m_list)
        return;
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        if (!it || it->isHidden())
            continue;
        const QString k = it->data(Qt::UserRole).toString() + QLatin1Char(':')
            + QString::number(it->data(Qt::UserRole + 1).toInt());
        if (k == key) {
            m_list->setCurrentRow(i);
            return;
        }
    }
}

void MainWindow::clearUnread(const QString &key)
{
    if (key.isEmpty())
        return;
    if (m_unread.remove(key) > 0)
        refreshPeers();
}

void MainWindow::hideTrayToast()
{
    if (m_trayToast)
        m_trayToast->hide();
}

void MainWindow::showTrayToast(const QString &title, const QString &body)
{
    if (!m_trayToast) {
        m_trayToast = new QFrame(0, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        m_trayToast->setObjectName(QStringLiteral("trayToast"));
        m_trayToast->setAttribute(Qt::WA_ShowWithoutActivating);
        m_trayToast->setFixedWidth(320);
        m_trayToast->setCursor(Qt::PointingHandCursor);
        QVBoxLayout *lay = new QVBoxLayout(m_trayToast);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(4);
        m_trayToastTitle = new QLabel(m_trayToast);
        m_trayToastTitle->setObjectName(QStringLiteral("trayToastTitle"));
        m_trayToastTitle->setWordWrap(true);
        m_trayToastTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_trayToastBody = new QLabel(m_trayToast);
        m_trayToastBody->setObjectName(QStringLiteral("trayToastBody"));
        m_trayToastBody->setWordWrap(true);
        m_trayToastBody->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(m_trayToastTitle);
        lay->addWidget(m_trayToastBody);
        m_trayToast->setStyleSheet(QStringLiteral(
            "#trayToast { background: #0f172a; border: 1px solid #334155; border-radius: 10px; }"
            "#trayToastTitle { color: #f8fafc; font-size: 13px; font-weight: 600; }"
            "#trayToastBody { color: #cbd5e1; font-size: 12px; }"));
        m_trayToast->installEventFilter(this);
        m_trayToastTimer = new QTimer(this);
        m_trayToastTimer->setSingleShot(true);
        connect(m_trayToastTimer, SIGNAL(timeout()), this, SLOT(hideTrayToast()));
    }
    m_trayToastTitle->setText(title);
    m_trayToastBody->setText(body);
    m_trayToast->adjustSize();
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect ag = screen->availableGeometry();
        m_trayToast->move(ag.right() - m_trayToast->width() - 16,
                          ag.bottom() - m_trayToast->height() - 16);
    }
    m_trayToast->show();
    m_trayToast->raise();
    m_trayToastTimer->start(5000);
}

void MainWindow::maybeTrayNotify(const QString &title, const QString &body, const QString &peerKey)
{
    if (!m_tray || isVisible())
        return;
    m_trayNotifyKey = peerKey;
    // Win10/11 与部分桌面会吞掉 showMessage；自绘右下角提示作可靠出口
    showTrayToast(title, body);
}

void MainWindow::quitApp()
{
    persistWindowGeometry();
    flushChatHistory();
    m_forceQuit = true;
    hideTrayToast();
    if (m_tray) {
        m_tray->hide();
    }
    qApp->quit();
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        showFromTray();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    persistWindowGeometry();
    flushChatHistory();
    if (m_forceQuit || !m_tray) {
        event->accept();
        qApp->quit();
        return;
    }
    event->ignore();
    hide();
    if (!m_trayHintShown) {
        m_trayHintShown = true;
        m_trayNotifyKey.clear(); // 首次提示不跳会话
        showTrayToast(QString::fromUtf8(u8"局域快传"),
                      QString::fromUtf8(u8"已在托盘运行，可继续接收文件。右键托盘图标可退出。"));
    }
}

void MainWindow::refreshShareBtn()
{
    if (!m_shareBtn)
        return;
    m_shareBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/globe.svg"), 16)));
    m_shareBtn->setIconSize(QSize(16, 16));
    if (m_http && !m_http->shareDir().isEmpty())
        m_shareBtn->setText(QString::fromUtf8(u8"共享中…"));
    else
        m_shareBtn->setText(QString::fromUtf8(u8"网页共享 (HTTP)"));
}

static QPixmap makeQrPixmap(const QString &text, int logical)
{
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
        text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int n = qr.getSize();
    QImage raw(n, n, QImage::Format_RGB32);
    raw.fill(Qt::white);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (qr.getModule(x, y))
                raw.setPixel(x, y, qRgb(15, 23, 42));
        }
    }
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int quiet = logical / 12;
    QPixmap pm(logical * dpr, logical * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::white);
    QPainter p(&pm);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(QRect(quiet, quiet, logical - quiet * 2, logical - quiet * 2), raw);
    return pm;
}

static QString humanBytes(qint64 n)
{
    if (n < 1024)
        return QString::number(n) + QStringLiteral(" B");
    if (n < 1024 * 1024)
        return QString::number(n / 1024.0, 'f', 1) + QStringLiteral(" KB");
    return QString::number(n / 1024.0 / 1024.0, 'f', 1) + QStringLiteral(" MB");
}

void MainWindow::openShare()
{    QDialog dlg(this);
    dlg.setObjectName(QStringLiteral("shareDlg"));
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setModal(true);
    dlg.setFixedWidth(760);
    dlg.setStyleSheet(QStringLiteral(
        "#shareDlg { background: transparent; }"
        "#shareRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#shareHead { background: #ffffff; border-bottom: 1px solid #e2e8f0;"
        " border-top-left-radius: 16px; border-top-right-radius: 16px; }"
        "#shareTitle { color: #0f172a; font-size: 16px; font-weight: 700; }"
        "#shareSub { color: #64748b; font-size: 12px; }"
        "#shareClose { background: transparent; border: none; border-radius: 6px; padding: 0; }"
        "#shareClose:hover { background: #e2e8f0; }"
        "#shareStatus { border-radius: 11px; padding: 2px 10px; font-size: 12px; font-weight: 600; }"
        "#shareStatus[on=\"true\"] { color: #16a34a; background: #f0fdf4; border: 1px solid #86efac; }"
        "#shareStatus[on=\"false\"] { color: #64748b; background: #f8fafc; border: 1px solid #e2e8f0; }"
        "#shareUrlCard { background: #ffffff; border: 1px solid #bfdbfe; border-radius: 12px; }"
        "#shareQrCard { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }"
        "#shareUrlLab { color: #2563eb; font-size: 12px; font-weight: 600; }"
        "#shareUrl { color: #2563eb; font-size: 22px; font-weight: 700; }"
        "#shareMeta { color: #94a3b8; font-size: 12px; }"
        "#shareGhost { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #334155; padding: 6px 12px; font-weight: 600; }"
        "#shareGhost:hover { background: #f8fafc; }"
        "#sharePrimary { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 6px 14px; font-weight: 600; }"
        "#sharePrimary:hover { background: #1d4ed8; }"
        "#shareSecTitle { color: #0f172a; font-size: 13px; font-weight: 700; }"
        "#shareCount { color: #64748b; background: #f1f5f9; border-radius: 10px; padding: 1px 8px; font-size: 12px; }"
        "#shareHint { color: #94a3b8; font-size: 12px; }"
        "#shareFile { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; }"
        "#shareFileName { color: #0f172a; font-size: 13px; font-weight: 600; }"
        "#shareDel { background: transparent; border: none; border-radius: 6px; padding: 0; }"
        "#shareDel:hover { background: #fee2e2; }"
        "#shareDrop { background: #f8fafc; border: 1px dashed #cbd5e1; border-radius: 10px; color: #94a3b8; }"
        "#shareDrop:hover { background: #eff6ff; border-color: #93c5fd; color: #2563eb; }"
        "#shareFoot { border-top: 1px solid #e2e8f0; background: #ffffff;"
        " border-bottom-left-radius: 16px; border-bottom-right-radius: 16px; }"
        "#sharePause { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #334155; padding: 8px 14px; font-weight: 600; }"
        "#sharePause:hover { background: #f8fafc; }"
        "#shareCloseWin { background: #0f172a; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 8px 16px; font-weight: 600; }"
        "#shareCloseWin:hover { background: #1e293b; }"));

    const QString ip = localIpText();
    const QString url = QStringLiteral("http://%1:%2/share/").arg(ip).arg(m_settings.port);
    QString lastDir = m_http ? m_http->shareDir() : QString();

    QWidget *root = new QWidget(&dlg);
    root->setObjectName(QStringLiteral("shareRoot"));
    QVBoxLayout *dlgLay = new QVBoxLayout(&dlg);
    dlgLay->setContentsMargins(0, 0, 0, 0);
    dlgLay->addWidget(root);
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    QWidget *head = new QWidget;
    head->setObjectName(QStringLiteral("shareHead"));
    QHBoxLayout *headLay = new QHBoxLayout(head);
    headLay->setContentsMargins(20, 16, 12, 16);
    headLay->setSpacing(12);
    QLabel *badge = new QLabel;
    badge->setPixmap(loadSvgPixmap(QStringLiteral(":/icons/share-badge.svg"), 36));
    badge->setFixedSize(36, 36);
    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setSpacing(10);
    QLabel *title = new QLabel(QString::fromUtf8(u8"本机 HTTP 网页共享服务"));
    title->setObjectName(QStringLiteral("shareTitle"));
    QLabel *status = new QLabel;
    status->setObjectName(QStringLiteral("shareStatus"));
    titleRow->addWidget(title, 0, Qt::AlignVCenter);
    titleRow->addWidget(status, 0, Qt::AlignVCenter);
    titleRow->addStretch(1);
    QLabel *sub = new QLabel(QString::fromUtf8(
        u8"局域网内任意手机、平板或电脑，打开浏览器或扫码即可下载本机共享的内容"));
    sub->setObjectName(QStringLiteral("shareSub"));
    sub->setWordWrap(true);
    titleCol->addLayout(titleRow);
    titleCol->addWidget(sub);
    QPushButton *xBtn = new QPushButton;
    xBtn->setObjectName(QStringLiteral("shareClose"));
    xBtn->setFixedSize(28, 28);
    xBtn->setCursor(Qt::PointingHandCursor);
    xBtn->setFocusPolicy(Qt::NoFocus);
    xBtn->setIcon(makeChromeIcon(IconClose, QColor(QStringLiteral("#94a3b8"))));
    xBtn->setIconSize(QSize(14, 14));
    connect(xBtn, SIGNAL(clicked()), &dlg, SLOT(reject()));
    headLay->addWidget(badge, 0, Qt::AlignTop);
    headLay->addLayout(titleCol, 1);
    headLay->addWidget(xBtn, 0, Qt::AlignTop);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 16, 20, 12);
    bodyLay->setSpacing(16);

    QHBoxLayout *cards = new QHBoxLayout;
    cards->setSpacing(12);
    QFrame *urlCard = new QFrame;
    urlCard->setObjectName(QStringLiteral("shareUrlCard"));
    QVBoxLayout *urlLay = new QVBoxLayout(urlCard);
    urlLay->setContentsMargins(16, 14, 16, 14);
    urlLay->setSpacing(8);
    QLabel *urlLab = new QLabel(QString::fromUtf8(u8"局域网访问地址（浏览器直接输入）："));
    urlLab->setObjectName(QStringLiteral("shareUrlLab"));
    QLabel *urlText = new QLabel(url);
    urlText->setObjectName(QStringLiteral("shareUrl"));
    urlText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QLabel *meta = new QLabel(QString::fromUtf8(u8"绑定网卡: %1 · 端口: %2 (同一局域网下均可访问)")
                                  .arg(ip)
                                  .arg(m_settings.port));
    meta->setObjectName(QStringLiteral("shareMeta"));
    meta->setWordWrap(true);
    QHBoxLayout *urlBtns = new QHBoxLayout;
    urlBtns->setSpacing(8);
    QPushButton *copyBtn = new QPushButton(QString::fromUtf8(u8"复制网址"));
    copyBtn->setObjectName(QStringLiteral("shareGhost"));
    copyBtn->setCursor(Qt::PointingHandCursor);
    QPushButton *openBtn = new QPushButton(QString::fromUtf8(u8"本机浏览器自测"));
    openBtn->setObjectName(QStringLiteral("sharePrimary"));
    openBtn->setCursor(Qt::PointingHandCursor);
    urlBtns->addWidget(copyBtn);
    urlBtns->addWidget(openBtn);
    urlBtns->addStretch(1);
    urlLay->addWidget(urlLab);
    urlLay->addWidget(urlText);
    urlLay->addWidget(meta);
    urlLay->addStretch(1);
    urlLay->addLayout(urlBtns);

    QFrame *qrCard = new QFrame;
    qrCard->setObjectName(QStringLiteral("shareQrCard"));
    QVBoxLayout *qrLay = new QVBoxLayout(qrCard);
    qrLay->setContentsMargins(12, 12, 12, 12);
    qrLay->setSpacing(6);
    QLabel *qr = new QLabel;
    qr->setPixmap(makeQrPixmap(url, 168));
    qr->setFixedSize(168, 168);
    qr->setAlignment(Qt::AlignCenter);
    QLabel *qrCap = new QLabel(QString::fromUtf8(u8"手机 / 平板扫码"));
    qrCap->setObjectName(QStringLiteral("shareUrlLab"));
    qrCap->setAlignment(Qt::AlignCenter);
    QLabel *qrSub = new QLabel(QString::fromUtf8(u8"无需安装 App，相机扫码即可下载"));
    qrSub->setObjectName(QStringLiteral("shareMeta"));
    qrSub->setAlignment(Qt::AlignCenter);
    qrLay->addStretch(1);
    qrLay->addWidget(qr, 0, Qt::AlignHCenter);
    qrLay->addWidget(qrCap);
    qrLay->addWidget(qrSub);
    qrLay->addStretch(1);
    cards->addWidget(urlCard, 3);
    cards->addWidget(qrCard, 2);

    QHBoxLayout *listHead = new QHBoxLayout;
    QLabel *listTitle = new QLabel(QString::fromUtf8(u8"当前共享的文件列表"));
    listTitle->setObjectName(QStringLiteral("shareSecTitle"));
    QLabel *count = new QLabel;
    count->setObjectName(QStringLiteral("shareCount"));
    QLabel *dragHint = new QLabel(QString::fromUtf8(u8"支持拖动文件到此处"));
    dragHint->setObjectName(QStringLiteral("shareHint"));
    QPushButton *addBtn = new QPushButton(QString::fromUtf8(u8"+ 添加文件到共享"));
    addBtn->setObjectName(QStringLiteral("sharePrimary"));
    addBtn->setCursor(Qt::PointingHandCursor);
    listHead->addWidget(listTitle);
    listHead->addWidget(count);
    listHead->addWidget(dragHint);
    listHead->addStretch(1);
    listHead->addWidget(addBtn);

    QWidget *listHost = new QWidget;
    QVBoxLayout *listLay = new QVBoxLayout(listHost);
    listLay->setContentsMargins(0, 0, 0, 0);
    listLay->setSpacing(6);
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidget(listHost);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setFixedHeight(132);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QPushButton *dropBtn = new QPushButton(QString::fromUtf8(u8"可直接将文件拖动到此区域添加，或点击此处选择文件"));
    dropBtn->setObjectName(QStringLiteral("shareDrop"));
    dropBtn->setCursor(Qt::PointingHandCursor);
    dropBtn->setFixedHeight(56);
    bodyLay->addLayout(cards);
    bodyLay->addLayout(listHead);
    bodyLay->addWidget(scroll);
    bodyLay->addWidget(dropBtn);

    QWidget *foot = new QWidget;
    foot->setObjectName(QStringLiteral("shareFoot"));
    QHBoxLayout *footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(20, 12, 20, 16);
    QPushButton *pauseBtn = new QPushButton;
    pauseBtn->setObjectName(QStringLiteral("sharePause"));
    pauseBtn->setCursor(Qt::PointingHandCursor);
    pauseBtn->setIcon(QIcon(loadSvgPixmap(QStringLiteral(":/icons/power.svg"), 16)));
    pauseBtn->setIconSize(QSize(16, 16));
    QLabel *footHint = new QLabel(QString::fromUtf8(u8"仅局域网有效，随开随关"));
    footHint->setObjectName(QStringLiteral("shareHint"));
    QPushButton *closeWin = new QPushButton(QString::fromUtf8(u8"关闭窗口"));
    closeWin->setObjectName(QStringLiteral("shareCloseWin"));
    closeWin->setCursor(Qt::PointingHandCursor);
    footLay->addWidget(pauseBtn);
    footLay->addSpacing(12);
    footLay->addWidget(footHint);
    footLay->addStretch(1);
    footLay->addWidget(closeWin);
    connect(closeWin, SIGNAL(clicked()), &dlg, SLOT(accept()));

    rootLay->addWidget(head);
    rootLay->addWidget(body, 1);
    rootLay->addWidget(foot);

    auto paintStatus = [&]() {
        const bool on = m_http && !m_http->shareDir().isEmpty();
        status->setText(on ? QString::fromUtf8(u8"● HTTP 服务运行中")
                           : QString::fromUtf8(u8"● HTTP 服务已暂停"));
        status->setProperty("on", on);
        status->style()->unpolish(status);
        status->style()->polish(status);
        pauseBtn->setText(on ? QString::fromUtf8(u8"暂停 HTTP 服务")
                             : QString::fromUtf8(u8"开启 HTTP 服务"));
    };
    std::function<void()> reloadFiles;
    reloadFiles = [&]() {
        while (QLayoutItem *it = listLay->takeAt(0)) {
            delete it->widget();
            delete it;
        }
        const QString dir = m_http ? m_http->shareDir() : QString();
        QStringList names;
        if (!dir.isEmpty() && QDir(dir).exists()) {
            const QFileInfoList infos = QDir(dir).entryInfoList(
                QDir::Files | QDir::Readable, QDir::Name);
            for (int i = 0; i < infos.size(); ++i) {
                const QString abs = infos.at(i).absoluteFilePath();
                QWidget *row = new QWidget;
                row->setObjectName(QStringLiteral("shareFile"));
                QHBoxLayout *rowLay = new QHBoxLayout(row);
                rowLay->setContentsMargins(10, 8, 10, 8);
                QLabel *name = new QLabel(infos.at(i).fileName());
                name->setObjectName(QStringLiteral("shareFileName"));
                QLabel *sz = new QLabel(humanBytes(infos.at(i).size()));
                sz->setObjectName(QStringLiteral("shareMeta"));
                QPushButton *del = new QPushButton;
                del->setObjectName(QStringLiteral("shareDel"));
                del->setFixedSize(28, 28);
                del->setCursor(Qt::PointingHandCursor);
                del->setFocusPolicy(Qt::NoFocus);
                del->setIcon(makeChromeIcon(IconClose, QColor(QStringLiteral("#94a3b8"))));
                del->setIconSize(QSize(12, 12));
                del->setToolTip(QString::fromUtf8(u8"从共享中移除"));
                connect(del, &QPushButton::clicked, &dlg, [abs, &reloadFiles]() {
                    QFile::remove(abs);
                    reloadFiles();
                });
                rowLay->addWidget(name, 1);
                rowLay->addWidget(sz);
                rowLay->addWidget(del);
                listLay->addWidget(row);
                names << infos.at(i).fileName();
            }
        }
        listLay->addStretch(1);
        count->setText(QString::fromUtf8(u8"%1 个").arg(names.size()));
        paintStatus();
    };
    auto ensureShareDir = [&]() -> QString {
        if (m_http && !m_http->shareDir().isEmpty() && QDir(m_http->shareDir()).exists()) {
            lastDir = m_http->shareDir();
            return lastDir;
        }
        if (!lastDir.isEmpty() && QDir(lastDir).exists()) {
            m_http->setShareDir(lastDir);
            refreshShareBtn();
            return lastDir;
        }
        QString d = QDir(m_settings.downloadDir).filePath(QStringLiteral("lan-drop-share"));
        if (!QDir().mkpath(d)) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"无法创建共享目录"));
            return QString();
        }
        lastDir = QFileInfo(d).absoluteFilePath();
        m_http->setShareDir(lastDir);
        refreshShareBtn();
        return lastDir;
    };
    auto addFilesToShare = [&](const QStringList &paths) {
        if (paths.isEmpty())
            return;
        const QString dir = ensureShareDir();
        if (dir.isEmpty())
            return;
        int ok = 0;
        for (int i = 0; i < paths.size(); ++i) {
            if (!copyFileIntoDir(dir, paths.at(i)).isEmpty())
                ++ok;
        }
        reloadFiles();
        if (ok == 0) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"没有文件被加入共享（可能无权复制或路径无效）"));
        }
    };
    auto pickFiles = [&]() {
        const QStringList picked = QFileDialog::getOpenFileNames(
            &dlg, QString::fromUtf8(u8"选择要共享的文件"));
        addFilesToShare(picked);
    };
    connect(copyBtn, &QPushButton::clicked, &dlg, [url]() {
        QApplication::clipboard()->setText(url);
    });
    connect(openBtn, &QPushButton::clicked, &dlg, [url]() {
        QDesktopServices::openUrl(QUrl(url));
    });
    connect(addBtn, &QPushButton::clicked, &dlg, pickFiles);
    connect(dropBtn, &QPushButton::clicked, &dlg, pickFiles);
    ShareDropFilter *dropFilter = new ShareDropFilter(&dlg);
    dropFilter->onFiles = addFilesToShare;
    dropBtn->setAcceptDrops(true);
    dropBtn->installEventFilter(dropFilter);
    scroll->setAcceptDrops(true);
    scroll->installEventFilter(dropFilter);
    listHost->setAcceptDrops(true);
    listHost->installEventFilter(dropFilter);
    connect(pauseBtn, &QPushButton::clicked, &dlg, [&]() {
        if (!m_http)
            return;
        if (!m_http->shareDir().isEmpty()) {
            lastDir = m_http->shareDir();
            m_http->setShareDir(QString());
            refreshShareBtn();
            reloadFiles();
            return;
        }
        if (!lastDir.isEmpty() && QDir(lastDir).exists()) {
            m_http->setShareDir(lastDir);
            refreshShareBtn();
            reloadFiles();
            return;
        }
        pickFiles();
    });
    reloadFiles();
    dlg.exec();
    refreshShareBtn();
}

void MainWindow::updateHostPill()
{
    if (!m_hostName || !m_hostIp)
        return;
    m_hostName->setText(m_settings.deviceName);
    m_hostIp->setText(QLatin1Char('(') + localIpText() + QLatin1Char(')'));
}

void MainWindow::copyLocalAddr()
{
    const QString ip = localIpText();
    if (ip.isEmpty() || ip == QString::fromUtf8(u8"—")) {
        QToolTip::showText(QCursor::pos(), QString::fromUtf8(u8"暂无可用 IP"), this);
        return;
    }
    const QString addr = ip + QLatin1Char(':') + QString::number(m_settings.port);
    QApplication::clipboard()->setText(addr);
    QToolTip::showText(QCursor::pos(), QString::fromUtf8(u8"已复制 %1").arg(addr), this);
}

void MainWindow::copyPeerAddr()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0) || ip.trimmed().isEmpty()) {
        QToolTip::showText(QCursor::pos(), QString::fromUtf8(u8"请先选择对端"), this);
        return;
    }
    if (port <= 0)
        port = 8848;
    const QString addr = ip.trimmed() + QLatin1Char(':') + QString::number(port);
    QApplication::clipboard()->setText(addr);
    QToolTip::showText(QCursor::pos(), QString::fromUtf8(u8"已复制 %1").arg(addr), this);
}

void MainWindow::setStatusOnline(const QString &text, bool ok)
{
    if (m_statusDot)
        m_statusDot->setPixmap(makeStatusDot(ok));
    if (m_statusLabel)
        m_statusLabel->setText(text);
}

void MainWindow::boot()
{
    m_settings = Settings::load();
    loadChatHistory();
    m_id = deviceId();
    QDir().mkpath(m_settings.downloadDir);
    m_disc->setIdentity(m_id, m_settings.deviceName, m_settings.port);
    m_http->setInfo(m_id, m_settings.deviceName, m_settings.port);
    m_http->setDownloadDir(m_settings.downloadDir);
    const QByteArray envShare = qgetenv("LANDROP_SHARE_DIR");
    if (!envShare.isEmpty()) {
        const QString dir = QString::fromLocal8Bit(envShare);
        if (QDir(dir).exists())
            m_http->setShareDir(dir);
    }
    const bool httpOk = m_http->listen(m_settings.port);
    const bool discOk = m_disc->start(m_settings.discoverPort);
    for (int i = 0; i < m_settings.manualPeers.size(); ++i) {
        const ManualPeerEntry &e = m_settings.manualPeers.at(i);
        m_disc->addManual(e.ip, e.port, e.alias, e.os);
    }
    updateHostPill();
    refreshShareBtn();
    if (!httpOk) {
        setStatusOnline(QString::fromUtf8(u8"传输端口占用，请在设置里改端口"), false);
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"),
                             QString::fromUtf8(u8"端口 %1 被占用，其他电脑连不上这台机器。请在设置里改端口后重启。").arg(m_settings.port));
    } else if (!discOk) {
        setStatusOnline(QString::fromUtf8(u8"在线 · 发现端口占用，仍可手动加 IP"), false);
    } else {
        setStatusOnline(QString::fromUtf8(u8"在线 · 局域网自动发现中"), true);
    }
    refreshPeers();
    showChat();
    updateChrome();
    applyWindowGeometry();
    applySideWidth();
}

void MainWindow::persistWindowGeometry()
{
    // 最大化时记还原矩形，否则记当前客户区几何
    const QRect geo = isMaximized() ? normalGeometry() : geometry();
    m_settings.windowX = geo.x();
    m_settings.windowY = geo.y();
    m_settings.windowW = geo.width();
    m_settings.windowH = geo.height();
    m_settings.windowMaximized = isMaximized();
    if (m_bodySplit && m_bodySplit->count() >= 1) {
        const QList<int> sizes = m_bodySplit->sizes();
        if (!sizes.isEmpty())
            m_settings.sideWidth = qBound(220, 480, sizes.at(0));
    }
    const QString peer = currentKey();
    if (!peer.isEmpty())
        m_settings.lastPeer = peer;
    m_settings.save();
}

void MainWindow::applyWindowGeometry()
{
    const int w = m_settings.windowW;
    const int h = m_settings.windowH;
    if (w >= minimumWidth() && h >= minimumHeight()) {
        const QRect want(m_settings.windowX, m_settings.windowY, w, h);
        bool onScreen = false;
        const QList<QScreen *> screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            if (screens.at(i)->availableGeometry().intersects(want.adjusted(32, 32, -32, -32))) {
                onScreen = true;
                break;
            }
        }
        if (onScreen || screens.isEmpty())
            setGeometry(want);
        else
            resize(w, h);
    }
    if (m_settings.windowMaximized)
        setWindowState(windowState() | Qt::WindowMaximized);
}

void MainWindow::applySideWidth()
{
    if (!m_bodySplit)
        return;
    int w = m_settings.sideWidth > 0 ? m_settings.sideWidth : 300;
    w = qBound(220, 480, w);
    const int total = qMax(m_bodySplit->width(), w + 400);
    QList<int> sizes;
    sizes << w << qMax(400, total - w);
    m_bodySplit->setSizes(sizes);
}

QString MainWindow::chatHistoryFilePath()
{
    return QFileInfo(Settings::filePath()).absolutePath() + QStringLiteral("/chat.json");
}

bool MainWindow::saveChatHistoryToFile(const QString &path, const QHash<QString, QVector<ChatMsg> > &log)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QJsonObject root;
    for (auto it = log.constBegin(); it != log.constEnd(); ++it) {
        QJsonArray arr;
        const QVector<ChatMsg> &msgs = it.value();
        for (int i = 0; i < msgs.size(); ++i) {
            const ChatMsg &m = msgs.at(i);
            if (m.type == ChatMsg::OutFile && m.progressPct >= 0)
                continue; // 不落盘进行中发送
            QJsonObject o;
            o.insert(QStringLiteral("type"), m.type);
            o.insert(QStringLiteral("who"), m.who);
            o.insert(QStringLiteral("face"), m.face);
            o.insert(QStringLiteral("text"), m.text);
            o.insert(QStringLiteral("path"), m.path);
            o.insert(QStringLiteral("size"), m.size);
            o.insert(QStringLiteral("rttMs"), m.rttMs);
            o.insert(QStringLiteral("sha256"), m.sha256);
            o.insert(QStringLiteral("time"), m.time);
            arr.append(o);
        }
        if (!arr.isEmpty())
            root.insert(it.key(), arr);
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

QHash<QString, QVector<ChatMsg> > MainWindow::loadChatHistoryFromFile(const QString &path)
{
    QHash<QString, QVector<ChatMsg> > out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return out;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!it.value().isArray())
            continue;
        const QJsonArray arr = it.value().toArray();
        QVector<ChatMsg> msgs;
        msgs.reserve(qMin(500, arr.size()));
        for (int i = 0; i < arr.size(); ++i) {
            const QJsonObject o = arr.at(i).toObject();
            ChatMsg m;
            m.type = o.value(QStringLiteral("type")).toInt();
            if (m.type < ChatMsg::OutText || m.type > ChatMsg::Fail)
                continue;
            m.who = o.value(QStringLiteral("who")).toString();
            m.face = o.value(QStringLiteral("face")).toString();
            m.text = o.value(QStringLiteral("text")).toString();
            m.path = o.value(QStringLiteral("path")).toString();
            m.size = static_cast<qint64>(o.value(QStringLiteral("size")).toDouble());
            m.rttMs = static_cast<qint64>(o.value(QStringLiteral("rttMs")).toDouble(-1));
            m.sha256 = o.value(QStringLiteral("sha256")).toString();
            m.time = o.value(QStringLiteral("time")).toString();
            m.progressPct = -1;
            msgs.append(m);
        }
        if (msgs.size() > 500)
            msgs = msgs.mid(msgs.size() - 500);
        if (!msgs.isEmpty())
            out.insert(it.key(), msgs);
    }
    return out;
}

void MainWindow::loadChatHistory()
{
    m_log = loadChatHistoryFromFile(chatHistoryFilePath());
}

void MainWindow::scheduleSaveChatHistory()
{
    if (!m_chatSaveTimer)
        return;
    m_chatSaveTimer->start(500);
}

void MainWindow::flushChatHistory()
{
    if (m_chatSaveTimer)
        m_chatSaveTimer->stop();
    saveChatHistoryToFile(chatHistoryFilePath(), m_log);
}

void MainWindow::persistManualPeers()
{
    if (!m_disc)
        return;
    QList<ManualPeerEntry> list;
    const QList<Peer> peers = m_disc->peers();
    for (int i = 0; i < peers.size(); ++i) {
        const Peer &p = peers.at(i);
        if (!p.manual || p.ip.trimmed().isEmpty())
            continue;
        ManualPeerEntry e;
        e.ip = p.ip.trimmed();
        e.port = p.port > 0 ? p.port : 8848;
        e.alias = p.alias.trimmed();
        e.os = p.osName.trimmed();
        list.append(e);
    }
    m_settings.manualPeers = list;
    m_settings.save();
}

void MainWindow::peerListContextMenu(const QPoint &pos)
{
    QListWidgetItem *it = m_list ? m_list->itemAt(pos) : 0;
    if (!it)
        return;
    m_list->setCurrentItem(it);
    const bool manual = it->data(Qt::UserRole + 3).toBool();
    QMenu menu(this);
    QAction *del = menu.addAction(QString::fromUtf8(u8"删除手动节点"));
    del->setEnabled(manual);
    if (!manual)
        del->setToolTip(QString::fromUtf8(u8"仅手动添加的节点可删除"));
    QAction *chosen = menu.exec(m_list->viewport()->mapToGlobal(pos));
    if (chosen == del)
        removeSelectedManualPeer();
}

void MainWindow::removeSelectedManualPeer()
{
    QListWidgetItem *it = m_list ? m_list->currentItem() : 0;
    if (!it || !m_disc)
        return;
    if (!it->data(Qt::UserRole + 3).toBool()) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"自动发现的节点不能从这里删除，离线后会自动消失。"));
        return;
    }
    const QString ip = it->data(Qt::UserRole).toString();
    const int port = it->data(Qt::UserRole + 1).toInt();
    const QString name = it->data(Qt::UserRole + 2).toString();
    const QString label = name.isEmpty() ? (ip + QLatin1Char(':') + QString::number(port)) : name;
    if (QMessageBox::question(this, QString::fromUtf8(u8"局域快传"),
                              QString::fromUtf8(u8"删除手动节点「%1」？\n重启后也不会再出现。").arg(label),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No)
        != QMessageBox::Yes) {
        return;
    }
    if (!m_disc->removeManual(ip, port))
        return;
    persistManualPeers();
    // changed 会触发 refreshPeers；若信号被挡住则手动刷
    refreshPeers();
}

QString MainWindow::localIpText() const
{
    const QStringList ips = localIpv4();
    if (ips.isEmpty())
        return QString::fromUtf8(u8"—");
    return ips.first();
}

bool MainWindow::currentPeer(QString *ip, int *port, QString *name) const
{
    QListWidgetItem *it = m_list->currentItem();
    if (!it || it->isHidden())
        return false;
    if (ip)
        *ip = it->data(Qt::UserRole).toString();
    if (port)
        *port = it->data(Qt::UserRole + 1).toInt();
    if (name)
        *name = it->data(Qt::UserRole + 2).toString();
    return !it->data(Qt::UserRole).toString().isEmpty();
}

QString MainWindow::currentKey() const
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return QString();
    return ip + QLatin1Char(':') + QString::number(port);
}

void MainWindow::filterPeers(const QString &text)
{
    const QString q = text.trimmed().toLower();
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        if (q.isEmpty()) {
            it->setHidden(false);
            continue;
        }
        const QString hay = (it->text() + QLatin1Char(' ') + it->data(Qt::UserRole).toString()).toLower();
        it->setHidden(!hay.contains(q));
    }
    updateEmpty();
}

void MainWindow::updateEmpty()
{
    int visible = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        if (!m_list->item(i)->isHidden())
            ++visible;
    }
    m_peerCount->setText(QString::number(visible));
    const bool hasPeer = currentPeer(0, 0, 0);
    m_pages->setCurrentIndex(hasPeer ? 1 : 0);
    m_composer->setEnabled(hasPeer);
    updateInputPlaceholder();
    if (hasPeer)
        updatePeerSession();
}

void MainWindow::refreshPeers()
{
    const QString keep = currentKey();
    QString want = keep;
    if (want.isEmpty())
        want = m_settings.lastPeer.trimmed();
    const QString filter = m_search ? m_search->text() : QString();
    m_list->blockSignals(true);
    m_list->clear();
    const QList<Peer> list = m_disc->peers();
    int row = -1;
    for (int i = 0; i < list.size(); ++i) {
        const Peer &p = list.at(i);
        const QString flag = p.online() ? QString::fromUtf8(u8"在线") : QString::fromUtf8(u8"离线");
        const QString manual = p.manual ? QString::fromUtf8(u8" · 手动") : QString();
        const QString osTag = p.osName.trimmed().isEmpty()
            ? QString()
            : (QStringLiteral("  ·  ") + p.osName);
        const int unread = m_unread.value(p.key(), 0);
        const QString unreadTag = (unread > 0)
            ? QString::fromUtf8(u8" · %1").arg(unread)
            : QString();
        QListWidgetItem *it = new QListWidgetItem(
            QStringLiteral("%1%2\n%3:%4  %5%6%7")
                .arg(p.label(), unreadTag, p.ip, QString::number(p.port), flag, manual, osTag));
        it->setIcon(QIcon(makePeerAvatar(p.label(), p.osName, 44)));
        it->setSizeHint(QSize(0, 60));
        it->setData(Qt::UserRole, p.ip);
        it->setData(Qt::UserRole + 1, p.port);
        it->setData(Qt::UserRole + 2, p.label());
        it->setData(Qt::UserRole + 3, p.manual);
        m_list->addItem(it);
        if (!want.isEmpty() && p.key() == want)
            row = m_list->count() - 1;
    }
    if (row >= 0)
        m_list->setCurrentRow(row);
    else if (m_list->count() > 0 && keep.isEmpty())
        m_list->setCurrentRow(0);
    const bool restoredLast = keep.isEmpty() && row >= 0;
    m_list->blockSignals(false);
    filterPeers(filter);
    // 周期性刷新：已有选中只 updateEmpty，避免每秒 forceBottom
    // 刚从 lastPeer 恢复或丢选中时要走 showChat
    if (restoredLast || (row < 0 && !keep.isEmpty()))
        showChat();
    else
        updateEmpty();
    updateHostPill();
}

void MainWindow::setSessionTab(int index)
{
    if (!m_sessionStack)
        return;
    const int i = (index == 1) ? 1 : 0;
    m_sessionStack->setCurrentIndex(i);
    if (m_tabChat) {
        m_tabChat->setObjectName(i == 0 ? QStringLiteral("sessionTabActive")
                                          : QStringLiteral("sessionTab"));
        m_tabChat->style()->unpolish(m_tabChat);
        m_tabChat->style()->polish(m_tabChat);
        m_tabChat->update();
    }
    if (m_tabFiles) {
        m_tabFiles->setObjectName(i == 1 ? QStringLiteral("sessionTabActive")
                                           : QStringLiteral("sessionTab"));
        m_tabFiles->style()->unpolish(m_tabFiles);
        m_tabFiles->style()->polish(m_tabFiles);
        m_tabFiles->update();
    }
}

void MainWindow::showChatTab()
{
    setSessionTab(0);
}

void MainWindow::showFilesTab()
{
    setSessionTab(1);
}

void MainWindow::updatePeerSession()
{
    if (!m_peerHeader || !m_connBannerHost)
        return;
    QString ip;
    int port = 0;
    QString name;
    if (!currentPeer(&ip, &port, &name))
        return;

    Peer peer;
    const bool found = m_disc && m_disc->find(ip, port, &peer);
    if (!found) {
        peer.ip = ip;
        peer.port = port;
        peer.name = name;
    }
    const QString label = peer.label().isEmpty() ? name : peer.label();
    const QString addr = QStringLiteral("%1:%2").arg(ip).arg(port);
    const QString hostTag = !peer.hostname.trimmed().isEmpty()
        ? peer.hostname.trimmed()
        : (!label.isEmpty() ? label : ip);
    m_peerAvatar->setPixmap(makePeerAvatar(label, peer.osName, 44));
    m_peerName->setText(label);
    m_peerOnlineDot->setPixmap(makeStatusDot(found ? peer.online() : true, 8));
    m_peerAddr->setText(addr);
    m_peerMeta->setText(QString::fromUtf8(u8"%1  ·  Ping %2  ·  %3")
                            .arg(hostTag)
                            .arg(m_pingKey == addr && !m_pingText.isEmpty()
                                     ? m_pingText
                                     : QString::fromUtf8(u8"—"))
                            .arg(localLinkLabel()));
    // 参考图：灰字前缀 + 加粗设备名 + 灰字地址
    m_connBannerText->setText(
        QString::fromUtf8(
            u8"<span style=\"color:#64748b;\">已建立局域网直连：</span>"
            "<span style=\"color:#0f172a;font-weight:700;\">%1</span>"
            "<span style=\"color:#64748b;\"> (%2)</span>")
            .arg(label.toHtmlEscaped())
            .arg(addr.toHtmlEscaped()));
    if (m_tabFiles)
        m_tabFiles->setText(QString::fromUtf8(u8"文件传输 (%1)")
                                .arg(countFiles(m_log.value(currentKey()))));
}

void MainWindow::showChat()
{
    const QString key = currentKey();
    if (key.isEmpty()) {
        updateEmpty();
        return;
    }
    if (m_settings.lastPeer != key) {
        m_settings.lastPeer = key;
        m_settings.save();
    }
    clearUnread(key);
    m_chatNewBelow = false;
    if (m_jumpBottomBtn)
        m_jumpBottomBtn->hide();
    m_pages->setCurrentIndex(1);
    m_composer->setEnabled(true);
    refreshChatHtml(true);
    refreshFilesView();
    updatePeerSession();
    updateInputPlaceholder();
    QTimer::singleShot(0, this, SLOT(measurePing()));
}

void MainWindow::setProgress(const QString &text)
{
    if (!m_progress)
        return;
    if (text.isEmpty()) {
        m_progress->clear();
        m_progress->hide();
        if (m_fileLive) {
            m_fileLive->clear();
            m_fileLive->hide();
        }
        return;
    }
    m_progress->setText(text);
    m_progress->show();
    if (m_fileLive) {
        m_fileLive->setText(text);
        m_fileLive->show();
    }
}

void MainWindow::noteFail(const QString &key, QNetworkReply *rep, const QString &retryPath)
{
    const int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString why = QString::fromUtf8(rep->readAll()).trimmed();
    if (why.isEmpty())
        why = rep->errorString();
    if (code >= 400)
        why = QString::number(code) + QLatin1Char(' ') + why;
    ChatMsg m;
    m.type = ChatMsg::Fail;
    m.text = QString::fromUtf8(u8"发送失败：%1").arg(why);
    m.path = retryPath;
    m.time = nowClock();
    appendMsg(key, m);
    setProgress(QString());
}

void MainWindow::appendMsg(const QString &key, const ChatMsg &msg)
{
    QVector<ChatMsg> lines = m_log.value(key);
    lines.append(msg);
    if (lines.size() > 500)
        lines = lines.mid(lines.size() - 500);
    m_log.insert(key, lines);
    scheduleSaveChatHistory();
    const bool viewing = (key == currentKey()) && isVisible();
    if (viewing) {
        markChatNewBelowIfAway();
        refreshChatHtml();
        refreshFilesView();
    } else if (msg.type == ChatMsg::InText || msg.type == ChatMsg::InFile) {
        m_unread[key] = m_unread.value(key, 0) + 1;
        refreshPeers();
    }
}

bool MainWindow::isChatNearBottom() const
{
    if (!m_chat)
        return true;
    QScrollBar *bar = m_chat->verticalScrollBar();
    if (!bar || bar->maximum() <= 0)
        return true;
    return (bar->maximum() - bar->value()) <= 80;
}

void MainWindow::markChatNewBelowIfAway()
{
    if (!isChatNearBottom())
        m_chatNewBelow = true;
}

void MainWindow::placeJumpBottomBtn()
{
    if (!m_jumpBottomBtn || !m_chatHost)
        return;
    m_jumpBottomBtn->adjustSize();
    const int bw = qMax(120, m_jumpBottomBtn->sizeHint().width() + 8);
    const int bh = 32;
    m_jumpBottomBtn->setFixedSize(bw, bh);
    const int x = qMax(8, (m_chatHost->width() - bw) / 2);
    const int y = qMax(8, m_chatHost->height() - bh - 14);
    m_jumpBottomBtn->move(x, y);
    m_jumpBottomBtn->raise();
}

void MainWindow::syncJumpBottomBtn()
{
    if (!m_jumpBottomBtn)
        return;
    if (isChatNearBottom()) {
        m_chatNewBelow = false;
        m_jumpBottomBtn->hide();
        return;
    }
    if (!m_chatNewBelow) {
        m_jumpBottomBtn->hide();
        return;
    }
    m_jumpBottomBtn->setText(QString::fromUtf8(u8"有新消息 ↓"));
    placeJumpBottomBtn();
    m_jumpBottomBtn->show();
    m_jumpBottomBtn->raise();
}

void MainWindow::jumpChatToBottom()
{
    m_chatNewBelow = false;
    if (m_jumpBottomBtn)
        m_jumpBottomBtn->hide();
    refreshChatHtml(true);
}

void MainWindow::refreshChatHtml(bool forceBottom)
{
    if (!m_chat)
        return;
    QScrollBar *bar = m_chat->verticalScrollBar();
    const int oldVal = bar ? bar->value() : 0;
    const int oldMax = bar ? bar->maximum() : 0;
    const bool stick = forceBottom || !bar || oldMax <= 0
        || (oldMax - oldVal) <= 80;

    m_chat->setHtml(renderChatHtml(m_log.value(currentKey())));

    if (!bar)
        return;
    if (stick) {
        bar->setValue(bar->maximum());
        QTextCursor c = m_chat->textCursor();
        c.movePosition(QTextCursor::End);
        m_chat->setTextCursor(c);
        m_chatNewBelow = false;
        if (m_jumpBottomBtn)
            m_jumpBottomBtn->hide();
    } else {
        bar->setValue(qBound(0, bar->maximum(), oldVal));
        syncJumpBottomBtn();
    }
}

void MainWindow::refreshFilesView()
{
    const QVector<ChatMsg> msgs = m_log.value(currentKey());
    if (m_files)
        m_files->setHtml(renderFilesHtml(msgs));
    if (m_tabFiles)
        m_tabFiles->setText(QString::fromUtf8(u8"文件传输 (%1)").arg(countFiles(msgs)));
}

void MainWindow::measurePing()
{
    if (m_pingBusy || !m_pages || m_pages->currentIndex() != 1)
        return;
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    m_pingBusy = true;
    QElapsedTimer *clock = new QElapsedTimer;
    clock->start();
    const QString key = ip + QLatin1Char(':') + QString::number(port);
    QNetworkReply *rep = m_nam->get(
        QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(ip).arg(port))));
    connect(rep, &QNetworkReply::finished, this, [this, rep, clock, key, ip, port]() {
        rep->deleteLater();
        const double ms = clock->nsecsElapsed() / 1000000.0;
        delete clock;
        m_pingBusy = false;
        m_pingKey = key;
        if (rep->error() != QNetworkReply::NoError) {
            m_pingText = QString::fromUtf8(u8"超时");
        } else {
            m_pingText = QString::number(ms, 'f', ms < 10.0 ? 1 : 0) + QStringLiteral("ms");
            const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
            const int p = o.value(QStringLiteral("port")).toInt() > 0
                ? o.value(QStringLiteral("port")).toInt() : port;
            m_disc->touch(ip, p,
                          o.value(QStringLiteral("id")).toString(),
                          o.value(QStringLiteral("name")).toString(),
                          o.value(QStringLiteral("os")).toString(),
                          o.value(QStringLiteral("hostname")).toString());
        }
        if (key == currentKey())
            updatePeerSession();
    });
}

void MainWindow::onChatAnchor(const QUrl &url)
{
    if (url.scheme() != QLatin1String("landrop"))
        return;
    const QByteArray raw = QByteArray::fromBase64(
        url.path().mid(1).toLatin1(), QByteArray::Base64UrlEncoding);
    if (url.host() == QLatin1String("copy")) {
        QApplication::clipboard()->setText(QString::fromUtf8(raw));
        return;
    }
    if (url.host() == QLatin1String("retry")) {
        const QString path = QString::fromUtf8(raw);
        if (path.isEmpty())
            return;
        if (m_uploading) {
            QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                     QString::fromUtf8(u8"请等待当前文件传完后再重试"));
            return;
        }
        if (!QFileInfo::exists(path)) {
            ChatMsg m;
            m.type = ChatMsg::Fail;
            m.text = QString::fromUtf8(u8"发送失败：文件不存在或已移动");
            m.path = path;
            m.time = nowClock();
            appendMsg(currentKey(), m);
            return;
        }
        startUpload(path, false);
        return;
    }
    if (url.host() == QLatin1String("open") || url.host() == QLatin1String("reveal")) {
        const QString path = QString::fromUtf8(raw);
        const QFileInfo fi(path);
        if (!fi.exists())
            return;
        if (url.host() == QLatin1String("open"))
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteFilePath()));
        else
            QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
    }
}

void MainWindow::onText(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text)
{
    Q_UNUSED(fromId);
    const int port = fromPort > 0 ? fromPort : 8848;
    m_disc->touch(ip, port, fromId, fromName, QString());
    const QString key = ip + QLatin1Char(':') + QString::number(port);
    const QString who = fromName.trimmed().isEmpty() ? ip : fromName.trimmed();
    ChatMsg m;
    m.time = nowClock();
    m.who = who;
    if (text == QLatin1String("__landrop_nudge__")) {
        m.type = ChatMsg::System;
        m.text = QString::fromUtf8(u8"%1 抖动了窗口").arg(who);
        appendMsg(key, m);
        if (m_settings.nudgeEnabled)
            shakeWindow();
        return;
    }
    m.type = ChatMsg::InText;
    m.text = text;
    appendMsg(key, m);
    playNotifySound();
    QString preview = text.trimmed();
    if (preview.size() > 80)
        preview = preview.left(80) + QString::fromUtf8(u8"…");
    maybeTrayNotify(QString::fromUtf8(u8"新消息 · %1").arg(who), preview, key);
}

void MainWindow::onFile(const QString &ip, const QString &name, const QString &path, qint64 size)
{
    Peer known;
    int port = 8848;
    if (m_disc->find(ip, 8848, &known))
        port = known.port;
    ChatMsg m;
    m.type = ChatMsg::InFile;
    m.who = known.name.trimmed().isEmpty() ? ip : known.name.trimmed();
    m.text = name;
    m.path = path;
    m.size = size;
    m.sha256 = fileSha256Short(path);
    m.time = nowClock();
    const QString key = ip + QLatin1Char(':') + QString::number(port);
    appendMsg(key, m);
    playNotifySound();
    maybeTrayNotify(QString::fromUtf8(u8"收到文件 · %1").arg(m.who),
                    QString::fromUtf8(u8"%1（%2）").arg(name).arg(humanBytesChat(size)),
                    key);
}

void MainWindow::sendText()
{
    if (!m_input)
        return;
    const QString text = m_input->toPlainText();
    if (text.trimmed().isEmpty())
        return;
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QJsonObject o;
    o.insert(QStringLiteral("fromId"), m_id);
    o.insert(QStringLiteral("fromName"), m_settings.deviceName);
    o.insert(QStringLiteral("fromPort"), m_settings.port);
    o.insert(QStringLiteral("text"), text);
    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/inbox").arg(ip).arg(port)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/json; charset=utf-8")));
    QElapsedTimer *clock = new QElapsedTimer;
    clock->start();
    QNetworkReply *rep = m_nam->post(req, body);
    const QString key = currentKey();
    const QString sent = text;
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, sent, clock]() {
        rep->deleteLater();
        const qint64 ms = clock->elapsed();
        delete clock;
        if (rep->error() != QNetworkReply::NoError) {
            noteFail(key, rep);
            return;
        }
        ChatMsg m;
        m.type = ChatMsg::OutText;
        m.who = QString::fromUtf8(u8"我");
        m.face = m_settings.deviceName;
        m.text = sent;
        m.rttMs = ms;
        m.time = nowClock();
        appendMsg(key, m);
        m_input->clear();
    });
}

void MainWindow::startUpload(const QString &path, bool fromQueue)
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0)) {
        m_uploading = false;
        m_uploadQueue.clear();
        setProgress(QString());
        return;
    }
    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        ChatMsg m;
        m.type = ChatMsg::Fail;
        m.text = QString::fromUtf8(u8"发送失败：打不开 %1").arg(QFileInfo(path).fileName());
        m.path = path;
        m.time = nowClock();
        appendMsg(currentKey(), m);
        if (fromQueue)
            pumpUploadQueue();
        else {
            m_uploading = false;
            setProgress(QString());
        }
        return;
    }
    const QString filename = QFileInfo(path).fileName();
    const qint64 fsize = file->size();
    const QString sha = fileSha256Short(path);
    QHttpMultiPart *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    const QByteArray disp = "form-data; name=\"file\"; filename=\"" + filename.toUtf8()
        + "\"; filename*=UTF-8''" + QUrl::toPercentEncoding(filename);
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(disp));
    part.setBodyDevice(file);
    file->setParent(multi);
    multi->append(part);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/upload").arg(ip).arg(port)));
    QElapsedTimer *clock = new QElapsedTimer;
    clock->start();
    QNetworkReply *rep = m_nam->post(req, multi);
    multi->setParent(rep);
    const QString key = currentKey();
    ChatMsg pending;
    pending.type = ChatMsg::OutFile;
    pending.who = QString::fromUtf8(u8"我");
    pending.face = m_settings.deviceName;
    pending.text = filename;
    pending.path = path;
    pending.size = fsize;
    pending.sha256 = sha;
    pending.progressPct = 0;
    pending.time = nowClock();
    appendMsg(key, pending);
    const int msgIndex = m_log.value(key).size() - 1;
    m_uploading = true;
    m_uploadLastPct = -1;
    m_uploadLastUiMs = 0;
    setProgress(QString::fromUtf8(u8"正在发送 %1").arg(filename));
    connect(rep, &QNetworkReply::uploadProgress, this, [this, key, msgIndex, filename](qint64 sent, qint64 total) {
        if (total <= 0)
            return;
        const int pct = int(sent * 100 / total);
        setProgress(QString::fromUtf8(u8"正在发送 %1  %2%").arg(filename).arg(pct));
        updateUploadProgress(key, msgIndex, pct);
    });
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, msgIndex, fromQueue, path, clock]() {
        rep->deleteLater();
        const qint64 ms = clock->elapsed();
        delete clock;
        const int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (rep->error() != QNetworkReply::NoError || code >= 300) {
            dropUploadMsg(key, msgIndex);
            noteFail(key, rep, path);
            m_uploadQueue.clear();
            m_uploading = false;
            return;
        }
        QString sha;
        QVector<ChatMsg> lines = m_log.value(key);
        if (msgIndex >= 0 && msgIndex < lines.size())
            sha = lines.at(msgIndex).sha256;
        finishUploadMsg(key, msgIndex, ms, sha);
        if (fromQueue)
            pumpUploadQueue();
        else {
            m_uploading = false;
            setProgress(QString());
        }
    });
}

void MainWindow::updateUploadProgress(const QString &key, int msgIndex, int pct)
{
    pct = qBound(0, 100, pct);
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (pct < 100 && m_uploadLastPct >= 0 && pct - m_uploadLastPct < 2
        && now - m_uploadLastUiMs < 300)
        return;
    QVector<ChatMsg> lines = m_log.value(key);
    int idx = msgIndex;
    if (idx < 0 || idx >= lines.size() || lines.at(idx).type != ChatMsg::OutFile
        || lines.at(idx).progressPct < 0) {
        idx = -1;
        for (int i = lines.size() - 1; i >= 0; --i) {
            if (lines.at(i).type == ChatMsg::OutFile && lines.at(i).progressPct >= 0) {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0)
        return;
    ChatMsg &m = lines[idx];
    if (m.progressPct == pct)
        return;
    m.progressPct = pct;
    m_log.insert(key, lines);
    m_uploadLastPct = pct;
    m_uploadLastUiMs = now;
    if (key == currentKey()) {
        refreshChatHtml();
        refreshFilesView();
    }
}

void MainWindow::finishUploadMsg(const QString &key, int msgIndex, qint64 rttMs, const QString &sha)
{
    QVector<ChatMsg> lines = m_log.value(key);
    int idx = msgIndex;
    if (idx < 0 || idx >= lines.size() || lines.at(idx).type != ChatMsg::OutFile
        || lines.at(idx).progressPct < 0) {
        idx = -1;
        for (int i = lines.size() - 1; i >= 0; --i) {
            if (lines.at(i).type == ChatMsg::OutFile && lines.at(i).progressPct >= 0) {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0)
        return;
    ChatMsg &m = lines[idx];
    m.progressPct = -1;
    m.rttMs = rttMs;
    if (!sha.isEmpty())
        m.sha256 = sha;
    m_log.insert(key, lines);
    if (key == currentKey()) {
        markChatNewBelowIfAway();
        refreshChatHtml();
        refreshFilesView();
    }
    scheduleSaveChatHistory();
}

void MainWindow::dropUploadMsg(const QString &key, int msgIndex)
{
    QVector<ChatMsg> lines = m_log.value(key);
    int idx = msgIndex;
    if (idx < 0 || idx >= lines.size() || lines.at(idx).type != ChatMsg::OutFile
        || lines.at(idx).progressPct < 0) {
        idx = -1;
        for (int i = lines.size() - 1; i >= 0; --i) {
            if (lines.at(i).type == ChatMsg::OutFile && lines.at(i).progressPct >= 0) {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0)
        return;
    lines.removeAt(idx);
    m_log.insert(key, lines);
    if (key == currentKey()) {
        refreshChatHtml();
        refreshFilesView();
    }
    scheduleSaveChatHistory();
}

void MainWindow::pumpUploadQueue()
{
    if (m_uploadQueue.isEmpty()) {
        m_uploading = false;
        setProgress(QString());
        return;
    }
    const QString path = m_uploadQueue.takeFirst();
    startUpload(path, true);
}

void MainWindow::setupChatDrop()
{
    ShareDropFilter *filter = new ShareDropFilter(this);
    filter->onFiles = [this](const QStringList &paths) { enqueueDroppedPaths(paths); };
    filter->onActive = [this](bool on) { setChatDropHint(on); };
    // 只挂会话页：子控件不接 drop，事件落到 chatPage，避免进出子控件时高亮闪烁
    if (m_chatPage) {
        m_chatPage->setAcceptDrops(true);
        m_chatPage->installEventFilter(filter);
        m_chatPage->installEventFilter(this); // 同步遮罩几何
    }
    QWidget *inner[] = { m_chat, m_composer, m_inputShell, m_sessionStack, m_files };
    for (int i = 0; i < 5; ++i) {
        if (inner[i])
            inner[i]->setAcceptDrops(false);
    }
    if (m_chat)
        m_chat->installEventFilter(this); // Ctrl+V 粘贴发文件
}

void MainWindow::setChatDropHint(bool on)
{
    if (!m_chatDropHint || !m_sessionStack || !m_chatPage)
        return;
    if (!on) {
        m_chatDropHint->hide();
        return;
    }
    QString name;
    currentPeer(0, 0, &name);
    if (name.trimmed().isEmpty())
        name = QString::fromUtf8(u8"当前设备");
    if (m_chatDropHintLabel) {
        m_chatDropHintLabel->setText(
            QString::fromUtf8(u8"松手发送到 %1").arg(name.trimmed()));
    }
    const QRect r = m_sessionStack->geometry().adjusted(10, 10, -10, -10);
    m_chatDropHint->setGeometry(r);
    m_chatDropHint->show();
    m_chatDropHint->raise();
}

bool MainWindow::tryPasteClipboardFiles()
{
    const QMimeData *md = QApplication::clipboard()->mimeData();
    if (!md || !md->hasUrls())
        return false;
    const QStringList paths = localSendPathsFromUrls(md->urls());
    if (paths.isEmpty())
        return false;
    enqueueDroppedPaths(paths);
    return true;
}

void MainWindow::enqueueDroppedPaths(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    if (!currentPeer(0, 0, 0)) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"请先选择一台设备，再发送文件。"));
        return;
    }
    if (m_uploading) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"正在发送中，请稍候。"));
        return;
    }
    QStringList unique;
    for (int i = 0; i < paths.size(); ++i) {
        const QString p = paths.at(i);
        if (p.isEmpty() || unique.contains(p))
            continue;
        unique.append(p);
    }
    if (unique.isEmpty())
        return;
    if (unique.size() == 1) {
        startUpload(unique.first(), false);
        return;
    }
    m_uploadQueue = unique;
    {
        ChatMsg m;
        m.type = ChatMsg::System;
        m.text = QString::fromUtf8(u8"开始发送文件（%1 个）").arg(m_uploadQueue.size());
        m.time = nowClock();
        appendMsg(currentKey(), m);
    }
    pumpUploadQueue();
}

void MainWindow::sendFile()
{
    if (m_uploading) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"正在发送中，请稍候。"));
        return;
    }
    if (!currentPeer(0, 0, 0))
        return;
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8(u8"选择要发送的文件"));
    if (path.isEmpty())
        return;
    startUpload(path, false);
}

void MainWindow::sendFolder()
{
    if (m_uploading) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"正在发送中，请稍候。"));
        return;
    }
    if (!currentPeer(0, 0, 0))
        return;
    const QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8(u8"选择要发送的文件夹"));
    if (dir.isEmpty())
        return;
    const QFileInfoList files = QDir(dir).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    if (files.isEmpty()) {
        ChatMsg m;
        m.type = ChatMsg::System;
        m.text = QString::fromUtf8(u8"文件夹为空，没有可发送的文件");
        m.time = nowClock();
        appendMsg(currentKey(), m);
        return;
    }
    m_uploadQueue.clear();
    for (int i = 0; i < files.size(); ++i)
        m_uploadQueue.append(files.at(i).absoluteFilePath());
    {
        ChatMsg m;
        m.type = ChatMsg::System;
        m.text = QString::fromUtf8(u8"开始发送文件夹（%1 个文件）").arg(m_uploadQueue.size());
        m.time = nowClock();
        appendMsg(currentKey(), m);
    }
    pumpUploadQueue();
}

void MainWindow::nudgePeer()
{
    if (!m_settings.nudgeEnabled)
        return;
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QJsonObject o;
    o.insert(QStringLiteral("fromId"), m_id);
    o.insert(QStringLiteral("fromName"), m_settings.deviceName);
    o.insert(QStringLiteral("fromPort"), m_settings.port);
    o.insert(QStringLiteral("text"), QStringLiteral("__landrop_nudge__"));
    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/inbox").arg(ip).arg(port)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/json; charset=utf-8")));
    QNetworkReply *rep = m_nam->post(req, body);
    const QString key = currentKey();
    connect(rep, &QNetworkReply::finished, this, [this, rep, key]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError) {
            noteFail(key, rep);
            return;
        }
        ChatMsg m;
        m.type = ChatMsg::System;
        m.text = QString::fromUtf8(u8"已发送窗口抖动");
        m.time = nowClock();
        appendMsg(key, m);
        shakeWindow();
    });
}

void MainWindow::playNotifySound()
{
    if (!m_settings.soundNotification)
        return;
#ifdef Q_OS_WIN
    MessageBeep(MB_OK);
#else
    QApplication::beep();
#endif
}

void MainWindow::shakeWindow()
{
    const QPoint origin = pos();
    QTimer *t = new QTimer(this);
    t->setInterval(28);
    QObject *guard = new QObject(t);
    guard->setProperty("step", 0);
    connect(t, &QTimer::timeout, this, [this, t, origin, guard]() {
        static const int offs[] = {12, -12, 9, -9, 6, -6, 3, -3, 0};
        const int step = guard->property("step").toInt();
        if (step >= 9) {
            move(origin);
            t->stop();
            t->deleteLater();
            return;
        }
        move(origin + QPoint(offs[step], 0));
        guard->setProperty("step", step + 1);
    });
    t->start();
}

void MainWindow::updateInputPlaceholder()
{
    if (!m_input)
        return;
    QString name;
    if (!currentPeer(0, 0, &name) || name.trimmed().isEmpty())
        m_input->setPlaceholderText(QString::fromUtf8(u8"输入文字或命令…"));
    else
        m_input->setPlaceholderText(QString::fromUtf8(u8"向 %1 发送消息...").arg(name));
}

void MainWindow::addPeer()
{
    QDialog dlg(this);
    dlg.setObjectName(QStringLiteral("addPeerDlg"));
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setModal(true);
    dlg.setFixedWidth(460);
    dlg.setStyleSheet(QStringLiteral(
        "#addPeerDlg { background: transparent; }"
        "#addPeerRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#addPeerHead { background: #f8fafc; border-bottom: 1px solid #e2e8f0;"
        " border-top-left-radius: 16px; border-top-right-radius: 16px; }"
        "#addPeerTitle { color: #0f172a; font-size: 14px; font-weight: 700; }"
        "#addPeerSub { color: #64748b; font-size: 11px; }"
        "#addPeerClose { background: transparent; border: none; border-radius: 6px; padding: 0; color: #94a3b8; }"
        "#addPeerClose:hover { background: #e2e8f0; color: #334155; }"
        "#addPeerLabel { color: #334155; font-size: 12px; font-weight: 600; }"
        "#addPeerField { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
        " padding: 8px 10px; color: #0f172a; selection-background-color: #bfdbfe; }"
        "#addPeerField:focus { background: #ffffff; border-color: #3b82f6; }"
        "#addPeerCombo { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
        " padding: 7px 32px 7px 10px; color: #0f172a; min-height: 20px; }"
        "#addPeerCombo:hover { border-color: #cbd5e1; }"
        "#addPeerCombo:on { background: #ffffff; border-color: #3b82f6; }"
        "#addPeerCombo::drop-down { subcontrol-origin: padding; subcontrol-position: center right;"
        " width: 28px; border: none; background: transparent; }"
        "#addPeerCombo::down-arrow { image: url(:/icons/chevron-down.svg); width: 12px; height: 12px; }"
        "#addPeerCombo QAbstractItemView { background: #ffffff; border: 1px solid #e2e8f0;"
        " outline: 0; padding: 4px; selection-background-color: #eff6ff;"
        " selection-color: #1e3a8a; color: #0f172a; }"
        "#addPeerProbe { background: transparent; border: none; color: #2563eb; font-size: 12px;"
        " font-weight: 600; text-align: left; padding: 0; }"
        "#addPeerProbe:hover { color: #1d4ed8; }"
        "#addPeerProbe:disabled { color: #93c5fd; }"
        "#addPeerProbeResult { color: #64748b; font-size: 11px; }"
        "#addPeerFoot { border-top: 1px solid #e2e8f0; }"
        "#addPeerCancel { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #475569; padding: 8px 14px; }"
        "#addPeerCancel:hover { background: #f8fafc; }"
        "#addPeerOk { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 8px 16px; font-weight: 600; }"
        "#addPeerOk:hover { background: #1d4ed8; }"));

    QWidget *root = new QWidget(&dlg);
    root->setObjectName(QStringLiteral("addPeerRoot"));
    QVBoxLayout *dlgLay = new QVBoxLayout(&dlg);
    dlgLay->setContentsMargins(0, 0, 0, 0);
    dlgLay->addWidget(root);
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    QWidget *head = new QWidget;
    head->setObjectName(QStringLiteral("addPeerHead"));
    QHBoxLayout *headLay = new QHBoxLayout(head);
    headLay->setContentsMargins(20, 14, 12, 14);
    headLay->setSpacing(10);

    QLabel *globeBg = new QLabel;
    globeBg->setFixedSize(36, 36);
    globeBg->setPixmap(makeGlobeBadge(36));

    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->setContentsMargins(0, 0, 0, 0);
    titleCol->setSpacing(2);
    QLabel *title = new QLabel(QString::fromUtf8(u8"跨网段直连 / 手动添加节点"));
    title->setObjectName(QStringLiteral("addPeerTitle"));
    QLabel *sub = new QLabel(QString::fromUtf8(u8"当设备处于不同 VLAN、VPN 或禁用了 mDNS 广播时使用"));
    sub->setObjectName(QStringLiteral("addPeerSub"));
    sub->setWordWrap(true);
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    QPushButton *closeBtn = new QPushButton;
    closeBtn->setObjectName(QStringLiteral("addPeerClose"));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setIcon(makeChromeIcon(IconClose, QColor(QStringLiteral("#94a3b8"))));
    closeBtn->setIconSize(QSize(14, 14));
    connect(closeBtn, SIGNAL(clicked()), &dlg, SLOT(reject()));

    headLay->addWidget(globeBg, 0, Qt::AlignVCenter);
    headLay->addLayout(titleCol, 1);
    headLay->addWidget(closeBtn, 0, Qt::AlignTop);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 16, 20, 12);
    bodyLay->setSpacing(12);

    auto fieldLabel = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("addPeerLabel"));
        return l;
    };
    auto fieldEdit = [](const QString &text = QString(), const QString &ph = QString()) {
        QLineEdit *e = new QLineEdit(text);
        e->setObjectName(QStringLiteral("addPeerField"));
        if (!ph.isEmpty())
            e->setPlaceholderText(ph);
        return e;
    };

    QHBoxLayout *rowIp = new QHBoxLayout;
    rowIp->setSpacing(10);
    QVBoxLayout *ipCol = new QVBoxLayout;
    ipCol->setSpacing(4);
    QLineEdit *ip = fieldEdit(QString(), QStringLiteral("192.168.1.10"));
    ipCol->addWidget(fieldLabel(QString::fromUtf8(u8"目标 IP 地址")));
    ipCol->addWidget(ip);
    QVBoxLayout *portCol = new QVBoxLayout;
    portCol->setSpacing(4);
    QLineEdit *port = fieldEdit(QStringLiteral("8848"));
    port->setFixedWidth(110);
    portCol->addWidget(fieldLabel(QString::fromUtf8(u8"服务端口")));
    portCol->addWidget(port);
    rowIp->addLayout(ipCol, 1);
    rowIp->addLayout(portCol, 0);

    QVBoxLayout *aliasCol = new QVBoxLayout;
    aliasCol->setSpacing(4);
    QLineEdit *alias = fieldEdit(QString(), QString::fromUtf8(u8"例如：跨网段工控机"));
    aliasCol->addWidget(fieldLabel(QString::fromUtf8(u8"设备别名")));
    aliasCol->addWidget(alias);

    QHBoxLayout *rowOs = new QHBoxLayout;
    rowOs->setSpacing(10);
    QVBoxLayout *osCol = new QVBoxLayout;
    osCol->setSpacing(4);
    QComboBox *osBox = new QComboBox;
    osBox->setObjectName(QStringLiteral("addPeerCombo"));
    osBox->setEditable(false);
    osBox->setFocusPolicy(Qt::StrongFocus);
    // 避免 Windows 原生下拉条盖掉样式表
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
        osBox->setStyle(fusion);
    osBox->addItem(QStringLiteral("Windows PC"), QStringLiteral("windows"));
    osBox->addItem(QString::fromUtf8(u8"Linux (Ubuntu / 麒麟)"), QStringLiteral("linux"));
    osBox->addItem(QString::fromUtf8(u8"ARM64 Linux (工控/树莓派)"), QStringLiteral("arm-linux"));
    osBox->addItem(QString::fromUtf8(u8"iOS / Android 手机"), QStringLiteral("ios"));
    osBox->setCurrentIndex(2);
    osCol->addWidget(fieldLabel(QString::fromUtf8(u8"系统类型")));
    osCol->addWidget(osBox);
    QVBoxLayout *tagCol = new QVBoxLayout;
    tagCol->setSpacing(4);
    QLineEdit *tag = fieldEdit(QString(), QString::fromUtf8(u8"可选，本轮不入库"));
    tagCol->addWidget(fieldLabel(QString::fromUtf8(u8"部门 / 标签")));
    tagCol->addWidget(tag);
    rowOs->addLayout(osCol, 1);
    rowOs->addLayout(tagCol, 1);

    QHBoxLayout *probeRow = new QHBoxLayout;
    probeRow->setSpacing(8);
    QPushButton *probeBtn = new QPushButton(QString::fromUtf8(u8"测试目标端口连通性 (Ping /api/info)"));
    probeBtn->setObjectName(QStringLiteral("addPeerProbe"));
    probeBtn->setCursor(Qt::PointingHandCursor);
    probeBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/zap.svg"), 14)));
    probeBtn->setIconSize(QSize(14, 14));
    probeBtn->setFocusPolicy(Qt::NoFocus);
    QLabel *probeResult = new QLabel;
    probeResult->setObjectName(QStringLiteral("addPeerProbeResult"));
    probeRow->addWidget(probeBtn, 0, Qt::AlignVCenter);
    probeRow->addWidget(probeResult, 1, Qt::AlignVCenter);

    bodyLay->addLayout(rowIp);
    bodyLay->addLayout(aliasCol);
    bodyLay->addLayout(rowOs);
    bodyLay->addLayout(probeRow);

    QWidget *foot = new QWidget;
    foot->setObjectName(QStringLiteral("addPeerFoot"));
    QHBoxLayout *footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(20, 12, 20, 16);
    footLay->setSpacing(8);
    QPushButton *cancel = new QPushButton(QString::fromUtf8(u8"取消"));
    cancel->setObjectName(QStringLiteral("addPeerCancel"));
    cancel->setCursor(Qt::PointingHandCursor);
    QPushButton *ok = new QPushButton(QString::fromUtf8(u8"+ 添加并连接"));
    ok->setObjectName(QStringLiteral("addPeerOk"));
    ok->setCursor(Qt::PointingHandCursor);
    ok->setDefault(true);
    footLay->addStretch(1);
    footLay->addWidget(cancel);
    footLay->addWidget(ok);

    rootLay->addWidget(head);
    rootLay->addWidget(body);
    rootLay->addWidget(foot);

    connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
    connect(probeBtn, &QPushButton::clicked, &dlg, [&]() {
        const QString host = ip->text().trimmed();
        bool portOk = false;
        const int p = port->text().trimmed().toInt(&portOk);
        if (host.isEmpty() || !portOk || p < 1 || p > 65535) {
            probeResult->setText(QString::fromUtf8(u8"请先填写有效的 IP 与端口"));
            return;
        }
        probeBtn->setEnabled(false);
        probeResult->setText(QString::fromUtf8(u8"正在探测…"));
        QElapsedTimer *timer = new QElapsedTimer;
        timer->start();
        QNetworkReply *rep = m_nam->get(
            QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(host).arg(p))));
        connect(rep, &QNetworkReply::finished, &dlg, [=]() {
            rep->deleteLater();
            const qint64 ms = timer->elapsed();
            delete timer;
            probeBtn->setEnabled(true);
            if (rep->error() != QNetworkReply::NoError) {
                probeResult->setText(QString::fromUtf8(u8"不通：%1").arg(rep->errorString()));
                return;
            }
            const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
            const QString name = o.value(QStringLiteral("name")).toString();
            probeResult->setText(QString::fromUtf8(u8"连通 %1 ms%2")
                                     .arg(ms)
                                     .arg(name.isEmpty() ? QString()
                                                         : (QStringLiteral(" · ") + name)));
        });
    });
    connect(ok, &QPushButton::clicked, &dlg, [&]() {
        const QString host = ip->text().trimmed();
        bool portOk = false;
        const int p = port->text().trimmed().toInt(&portOk);
        if (host.isEmpty() || host.contains(QLatin1Char(' ')) || !portOk || p < 1 || p > 65535) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"IP 或端口无效"));
            return;
        }
        Q_UNUSED(tag);
        const QString osName = osBox->currentData().toString();
        m_disc->addManual(host, p, alias->text(), osName);
        persistManualPeers();
        refreshPeers();
        for (int i = 0; i < m_list->count(); ++i) {
            QListWidgetItem *it = m_list->item(i);
            if (it->data(Qt::UserRole).toString() == host
                && it->data(Qt::UserRole + 1).toInt() == p) {
                m_list->setCurrentRow(i);
                break;
            }
        }
        probePeer();
        dlg.accept();
    });

    dlg.exec();
}

void MainWindow::probePeer()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QNetworkReply *rep = m_nam->get(QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(ip).arg(port))));
    connect(rep, &QNetworkReply::finished, this, [this, rep, ip, port]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"连不上 %1:%2").arg(ip).arg(port));
            return;
        }
        const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
        m_disc->touch(ip, o.value(QStringLiteral("port")).toInt() > 0 ? o.value(QStringLiteral("port")).toInt() : port,
                      o.value(QStringLiteral("id")).toString(),
                      o.value(QStringLiteral("name")).toString(),
                      o.value(QStringLiteral("os")).toString(),
                      o.value(QStringLiteral("hostname")).toString());
        refreshPeers();
    });
}

void MainWindow::editSettings()
{
    QDialog dlg(this);
    dlg.setObjectName(QStringLiteral("settingsDlg"));
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setModal(true);
    dlg.setFixedWidth(460);
    dlg.setStyleSheet(QStringLiteral(
        "#settingsDlg { background: transparent; }"
        "#settingsRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#settingsHead { background: #f8fafc; border-bottom: 1px solid #e2e8f0;"
        " border-top-left-radius: 16px; border-top-right-radius: 16px; }"
        "#settingsTitle { color: #0f172a; font-size: 14px; font-weight: 700; }"
        "#settingsSub { color: #64748b; font-size: 11px; }"
        "#settingsClose { background: transparent; border: none; border-radius: 6px; padding: 0; }"
        "#settingsClose:hover { background: #e2e8f0; }"
        "#settingsLabel { color: #0f172a; font-size: 12px; font-weight: 700; }"
        "#settingsHint { color: #94a3b8; font-size: 11px; }"
        "#settingsField { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
        " padding: 8px 10px; color: #0f172a; selection-background-color: #bfdbfe; }"
        "#settingsField:focus { background: #ffffff; border-color: #3b82f6; }"
        "#settingsBrowse { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #334155; padding: 8px 12px; font-size: 12px; font-weight: 600; }"
        "#settingsBrowse:hover { background: #f8fafc; }"
        "#settingsSwitchLabel { color: #334155; font-size: 12px; font-weight: 600; }"
        "#settingsSwitchRow { border-top: 1px solid #e2e8f0; }"
        "#settingsCheck { spacing: 0; }"
        "#settingsCheck::indicator { width: 16px; height: 16px; border-radius: 4px;"
        " border: 1px solid #cbd5e1; background: #ffffff; }"
        "#settingsCheck::indicator:checked { background: #2563eb; border-color: #2563eb;"
        " image: url(:/icons/check.svg); }"
        "#settingsFoot { border-top: 1px solid #e2e8f0; background: #f8fafc;"
        " border-bottom-left-radius: 16px; border-bottom-right-radius: 16px; }"
        "#settingsSave { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 8px 18px; font-weight: 600; }"
        "#settingsSave:hover { background: #1d4ed8; }"));

    QWidget *root = new QWidget(&dlg);
    root->setObjectName(QStringLiteral("settingsRoot"));
    QVBoxLayout *dlgLay = new QVBoxLayout(&dlg);
    dlgLay->setContentsMargins(0, 0, 0, 0);
    dlgLay->addWidget(root);
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    QWidget *head = new QWidget;
    head->setObjectName(QStringLiteral("settingsHead"));
    QHBoxLayout *headLay = new QHBoxLayout(head);
    headLay->setContentsMargins(20, 14, 12, 14);
    headLay->setSpacing(10);

    QLabel *gearBadge = new QLabel;
    gearBadge->setFixedSize(36, 36);
    {
        const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
        QPixmap pm(36 * dpr, 36 * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#0f172a")));
        p.drawRoundedRect(QRectF(0, 0, 36, 36), 8, 8);
        QSvgRenderer r(QStringLiteral(":/icons/settings-white.svg"));
        if (r.isValid())
            r.render(&p, QRectF(8, 8, 20, 20));
        gearBadge->setPixmap(pm);
    }

    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->setContentsMargins(0, 0, 0, 0);
    titleCol->setSpacing(2);
    QLabel *title = new QLabel(QString::fromUtf8(u8"局域快传设置"));
    title->setObjectName(QStringLiteral("settingsTitle"));
    QLabel *sub = new QLabel(QString::fromUtf8(u8"设备名称、下载存储与接收提示"));
    sub->setObjectName(QStringLiteral("settingsSub"));
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    QPushButton *closeBtn = new QPushButton;
    closeBtn->setObjectName(QStringLiteral("settingsClose"));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setIcon(makeChromeIcon(IconClose, QColor(QStringLiteral("#94a3b8"))));
    closeBtn->setIconSize(QSize(14, 14));
    connect(closeBtn, SIGNAL(clicked()), &dlg, SLOT(reject()));

    headLay->addWidget(gearBadge, 0, Qt::AlignVCenter);
    headLay->addLayout(titleCol, 1);
    headLay->addWidget(closeBtn, 0, Qt::AlignTop);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 16, 20, 8);
    bodyLay->setSpacing(14);

    auto fieldLabel = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("settingsLabel"));
        return l;
    };
    auto fieldHint = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("settingsHint"));
        l->setWordWrap(true);
        return l;
    };
    auto fieldEdit = [](const QString &text) {
        QLineEdit *e = new QLineEdit(text);
        e->setObjectName(QStringLiteral("settingsField"));
        return e;
    };

    QVBoxLayout *nameCol = new QVBoxLayout;
    nameCol->setSpacing(4);
    QLineEdit *name = fieldEdit(m_settings.deviceName);
    nameCol->addWidget(fieldLabel(QString::fromUtf8(u8"本机设备名称")));
    nameCol->addWidget(name);
    nameCol->addWidget(fieldHint(QString::fromUtf8(u8"局域网内其他设备将显示此设备名称")));

    QHBoxLayout *rowPort = new QHBoxLayout;
    rowPort->setSpacing(12);
    QVBoxLayout *portCol = new QVBoxLayout;
    portCol->setSpacing(4);
    QLineEdit *port = fieldEdit(QString::number(m_settings.port));
    portCol->addWidget(fieldLabel(QString::fromUtf8(u8"本地 HTTP 监听端口")));
    portCol->addWidget(port);
    QVBoxLayout *thrCol = new QVBoxLayout;
    thrCol->setSpacing(4);
    QLineEdit *threads = fieldEdit(QString::number(m_settings.transferThreads));
    thrCol->addWidget(fieldLabel(QString::fromUtf8(u8"并发传输线程数")));
    thrCol->addWidget(threads);
    rowPort->addLayout(portCol, 1);
    rowPort->addLayout(thrCol, 1);

    QVBoxLayout *dirCol = new QVBoxLayout;
    dirCol->setSpacing(4);
    QLineEdit *dir = fieldEdit(m_settings.downloadDir);
    QPushButton *browse = new QPushButton(QString::fromUtf8(u8"浏览…"));
    browse->setObjectName(QStringLiteral("settingsBrowse"));
    browse->setCursor(Qt::PointingHandCursor);
    browse->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout *dirRow = new QHBoxLayout;
    dirRow->setContentsMargins(0, 0, 0, 0);
    dirRow->setSpacing(8);
    dirRow->addWidget(dir, 1);
    dirRow->addWidget(browse, 0);
    dirCol->addWidget(fieldLabel(QString::fromUtf8(u8"文件接收下载目录 (落盘路径)")));
    dirCol->addLayout(dirRow);
    dirCol->addWidget(fieldHint(QString::fromUtf8(u8"文件传输以 HTTP Stream 模式直接写盘，避免内存溢出")));
    connect(browse, &QPushButton::clicked, &dlg, [dir, &dlg]() {
        QString start = dir->text().trimmed();
        if (start.isEmpty() || !QDir(start).exists())
            start = QDir::homePath();
        const QString picked = QFileDialog::getExistingDirectory(
            &dlg, QString::fromUtf8(u8"选择下载目录"), start);
        if (!picked.isEmpty())
            dir->setText(QDir::toNativeSeparators(picked));
    });

    bodyLay->addLayout(nameCol);
    bodyLay->addLayout(rowPort);
    bodyLay->addLayout(dirCol);

    auto switchRow = [](const QString &text, bool checked) {
        QWidget *row = new QWidget;
        row->setObjectName(QStringLiteral("settingsSwitchRow"));
        QHBoxLayout *lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 12, 0, 12);
        lay->setSpacing(8);
        QLabel *lab = new QLabel(text);
        lab->setObjectName(QStringLiteral("settingsSwitchLabel"));
        QCheckBox *box = new QCheckBox;
        box->setObjectName(QStringLiteral("settingsCheck"));
        box->setChecked(checked);
        box->setCursor(Qt::PointingHandCursor);
        lay->addWidget(lab, 1);
        lay->addWidget(box, 0, Qt::AlignVCenter);
        return qMakePair(row, box);
    };
    const QPair<QWidget *, QCheckBox *> nudgePair =
        switchRow(QString::fromUtf8(u8"窗口轻颤与抖动提醒 (Nudge)"), m_settings.nudgeEnabled);
    const QPair<QWidget *, QCheckBox *> soundPair =
        switchRow(QString::fromUtf8(u8"新消息与传输完成通知声"), m_settings.soundNotification);
    QCheckBox *nudgeBox = nudgePair.second;
    QCheckBox *soundBox = soundPair.second;
    bodyLay->addWidget(nudgePair.first);
    bodyLay->addWidget(soundPair.first);

    QWidget *foot = new QWidget;
    foot->setObjectName(QStringLiteral("settingsFoot"));
    QHBoxLayout *footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(20, 12, 20, 16);
    QPushButton *save = new QPushButton(QString::fromUtf8(u8"保存并关闭"));
    save->setObjectName(QStringLiteral("settingsSave"));
    save->setCursor(Qt::PointingHandCursor);
    save->setDefault(true);
    footLay->addStretch(1);
    footLay->addWidget(save);

    rootLay->addWidget(head);
    rootLay->addWidget(body);
    rootLay->addWidget(foot);

    connect(save, &QPushButton::clicked, &dlg, [&]() {
        bool okPort = false;
        bool okThr = false;
        const int p = port->text().trimmed().toInt(&okPort);
        const int thr = threads->text().trimmed().toInt(&okThr);
        if (name->text().trimmed().isEmpty() || !okPort || p < 1 || p > 65535) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"名称或端口无效"));
            return;
        }
        if (!okThr || thr < 1 || thr > 32) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"并发线程数须在 1–32"));
            return;
        }
        m_settings.deviceName = name->text().trimmed();
        m_settings.port = p;
        m_settings.transferThreads = thr;
        m_settings.downloadDir = dir->text().trimmed();
        m_settings.nudgeEnabled = nudgeBox->isChecked();
        m_settings.soundNotification = soundBox->isChecked();
        if (!m_settings.save()) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"保存设置失败"));
            return;
        }
        boot();
        dlg.accept();
    });

    dlg.exec();
}
