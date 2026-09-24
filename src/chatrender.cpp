#include "chatrender.h"

#include "fmtutil.h"

#include <QApplication>
#include <QBuffer>
#include <QColor>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSvgRenderer>
#include <QtMath>
#include <QUrl>

// —— 本模块自用（不依赖 uiicons；体积格式复用 fmtutil）——

// qApp->devicePixelRatio() 取的是全机最高屏；主 125%/副 100% 时副屏也会按 1.25 错画。
static qreal s_chatDpr = 0.0;

void setChatRenderDevicePixelRatio(qreal dpr)
{
    s_chatDpr = dpr;
}

static qreal chatDpr()
{
    if (s_chatDpr > 0.05)
        return s_chatDpr;
    return qMax(1.0, qApp->devicePixelRatio());
}

static QPixmap makeDprPixmap(int logicalW, int logicalH)
{
    const qreal dpr = chatDpr();
    QPixmap pm(qMax(1, qCeil(logicalW * dpr)), qMax(1, qCeil(logicalH * dpr)));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    return pm;
}

static QString avatarInitial(const QString &name)
{
    for (int i = 0; i < name.size(); ++i) {
        if (!name.at(i).isSpace())
            return QString(name.at(i).toUpper());
    }
    return QStringLiteral("?");
}

QString fileSha256Short(const QString &path)
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

// 位图卡片四周留白画轻阴影（Qt 富文本无 CSS box-shadow）
static const int kShadowPad = 5;

static void paintSoftShadow(QPainter &p, const QRectF &box, qreal radius)
{
    p.setPen(Qt::NoPen);
    for (int i = 3; i >= 1; --i) {
        QColor c(15, 23, 42, 6 + i * 5);
        p.setBrush(c);
        p.drawRoundedRect(box.translated(0.0, qreal(i) * 0.7), radius, radius);
    }
}

static QString actionChipHtml(const QString &href, const QString &label, const QString &title,
                              bool primary);

static QString letterAvatarHtml(const QString &name, const QString &bg)
{
    const QString ch = avatarInitial(name);
    // 参考图比例：略放大，与更大气泡协调
    const int logical = 44;
    QPixmap pm = makeDprPixmap(logical, logical);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(bg));
    p.drawRoundedRect(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0), 12.0, 12.0);
    QFont font = qApp->font();
    font.setPixelSize(20);
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
    // 对齐参考图：更大字号/留白 + 更柔和圆角（无尾巴）+ 轻阴影
    const int maxContentW = 400;
    const int padX = 18;
    const int padY = 13;
    const qreal radius = 16.0;
    QFont font = qApp->font();
    font.setPixelSize(16);
    QFontMetrics fm(font);
    const QRect textBound = fm.boundingRect(QRect(0, 0, maxContentW, 10000),
                                           Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                           text);
    const int contentW = qMax(28, qMin(maxContentW, textBound.width()));
    const int contentH = qMax(fm.height(), textBound.height());
    const int innerW = contentW + padX * 2;
    const int innerH = contentH + padY * 2;
    const int logicalW = innerW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    QPixmap pm = makeDprPixmap(logicalW, logicalH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 0.5, kShadowPad + 0.5, innerW - 1.0, innerH - 1.0);
    paintSoftShadow(p, box, radius);
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
    p.drawText(QRect(kShadowPad + padX, kShadowPad + padY, contentW, contentH),
               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
               text);
    return pixmapToImgHtml(pm);
}

static bool splitCodeFence(const QString &text, QString *lang, QString *body)
{
    QString t = text.trimmed();
    t.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    t.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    if (!t.startsWith(QStringLiteral("```")))
        return false;
    int nl = t.indexOf(QLatin1Char('\n'));
    if (nl < 0)
        return false;
    QString head = t.mid(3, nl - 3).trimmed();
    // 允许 ```cpp 后带多余空格；空语言当 text
    if (head.isEmpty())
        head = QStringLiteral("text");
    // 去掉语言行里误带的尾部反引号
    if (head.endsWith(QStringLiteral("```")))
        head.chop(3);
    head = head.trimmed();
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

static bool looksLikeCommentLine(const QString &line)
{
    const QString s = line.trimmed();
    if (s.isEmpty())
        return false;
    if (s.startsWith(QLatin1Char('#')))
        return true;
    if (s.startsWith(QStringLiteral("//")))
        return true;
    if (s.startsWith(QStringLiteral("/*")) || s.startsWith(QStringLiteral("*"))
        || s.startsWith(QStringLiteral("*/")))
        return true;
    return false;
}

// 双层方框复制图标（对齐文档站代码块顶栏）
static void paintCopyGlyph(QPainter &p, const QRectF &r, const QColor &color)
{
    const qreal s = qMin(r.width(), r.height());
    if (s < 4.0)
        return;
    const QPointF o(r.center().x() - s * 0.5, r.center().y() - s * 0.5);
    QPen pen(color, qMax(1.15, s * 0.11));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal rr = qMax(1.2, s * 0.10);
    p.drawRoundedRect(QRectF(o.x() + s * 0.30, o.y() + s * 0.06, s * 0.58, s * 0.58), rr, rr);
    p.drawRoundedRect(QRectF(o.x() + s * 0.06, o.y() + s * 0.30, s * 0.58, s * 0.58), rr, rr);
}

static QString renderCodeBlock(const QString &lang, const QString &code)
{
    const QString href = QStringLiteral("landrop://copy/")
        + QString::fromLatin1(code.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    // Qt 富文本无可靠圆角：整块绘成深色圆角卡；顶栏浅于正文，「复制」带图标在右侧，整卡可点
    const int maxW = 420;
    const int padX = 14;
    const int headPadY = 8;
    const int codePadY = 12;
    const qreal radius = 14.0;
    const int iconSz = 14;
    const int iconTextGap = 4;
    const QString copyLabel = QString::fromUtf8(u8"复制");
    const QColor headBg(QStringLiteral("#334155"));
    const QColor bodyBg(QStringLiteral("#0f172a"));
    const QColor mutedFg(QStringLiteral("#94a3b8"));
    QFont headFont = qApp->font();
    headFont.setPixelSize(13);
    QFont copyFont = qApp->font();
    copyFont.setPixelSize(12);
    QFont codeFont(QStringLiteral("Consolas"));
    if (!codeFont.exactMatch())
        codeFont = QFont(QStringLiteral("Courier New"));
    codeFont.setPixelSize(14);
    codeFont.setStyleHint(QFont::Monospace);
    QFontMetrics headFm(headFont);
    QFontMetrics copyFm(copyFont);
    QFontMetrics codeFm(codeFont);
    const QStringList lines = code.split(QLatin1Char('\n'));
    int codeTextW = 0;
    for (int i = 0; i < lines.size(); ++i)
        codeTextW = qMax(codeTextW, codeFm.horizontalAdvance(lines.at(i)));
    const int codeTextH = qMax(codeFm.height(), lines.size() * codeFm.lineSpacing());
    const int copyTextW = copyFm.horizontalAdvance(copyLabel);
    const int copyClusterW = iconSz + iconTextGap + copyTextW;
    const int headTextH = qMax(headFm.height(), qMax(copyFm.height(), iconSz));
    const int headBandH = headTextH + headPadY * 2;
    const int contentW = qMax(160, qMin(maxW - padX * 2,
                                        qMax(headFm.horizontalAdvance(lang) + copyClusterW + 24,
                                             codeTextW)));
    const int codeBandH = codeTextH + codePadY * 2;
    const int innerW = contentW + padX * 2;
    const int innerH = headBandH + codeBandH;
    const int logicalW = innerW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    QPixmap pm = makeDprPixmap(logicalW, logicalH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 0.5, kShadowPad + 0.5, innerW - 1.0, innerH - 1.0);
    paintSoftShadow(p, box, radius);
    QPainterPath clip;
    clip.addRoundedRect(box, radius, radius);
    p.setClipPath(clip);
    p.fillRect(QRectF(box.left(), box.top(), box.width(), headBandH), headBg);
    p.fillRect(QRectF(box.left(), box.top() + headBandH, box.width(),
                       box.height() - headBandH),
               bodyBg);
    p.setClipping(false);

    const int headY = kShadowPad + headPadY;
    const int contentLeft = kShadowPad + padX;
    p.setFont(headFont);
    p.setPen(mutedFg);
    p.drawText(QRect(contentLeft, headY, contentW - copyClusterW - 12, headTextH),
               Qt::AlignLeft | Qt::AlignVCenter, lang);
    // 顶栏右侧：图标 +「复制」（整卡包在复制链里，故可点）
    const int copyRight = contentLeft + contentW;
    const int copyTextX = copyRight - copyTextW;
    const int iconX = copyTextX - iconTextGap - iconSz;
    paintCopyGlyph(p, QRectF(iconX, headY + (headTextH - iconSz) * 0.5, iconSz, iconSz), mutedFg);
    p.setFont(copyFont);
    p.setPen(mutedFg);
    p.drawText(QRect(copyTextX, headY, copyTextW, headTextH),
               Qt::AlignLeft | Qt::AlignVCenter, copyLabel);

    int y = kShadowPad + headBandH + codePadY;
    p.setFont(codeFont);
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        p.setPen(looksLikeCommentLine(line) ? mutedFg : QColor(QStringLiteral("#e2e8f0")));
        p.drawText(QRect(contentLeft, y, contentW, codeFm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, line);
        y += codeFm.lineSpacing();
    }
    const QString img = pixmapToImgHtml(pm);
    return QStringLiteral(
               "<a href=\"%1\" title=\"%2\" style=\"text-decoration:none;\">%3</a>")
        .arg(href, QString::fromUtf8(u8"点击复制"), img);
}

static QString metaBadgeImgHtml(const QString &text, const QColor &bg, const QColor &fg,
                                const QColor &border)
{
    QFont font = qApp->font();
    const QStringList prefer = QStringList()
        << QStringLiteral("Microsoft YaHei UI")
        << QStringLiteral("Microsoft YaHei")
        << QString::fromUtf8(u8"微软雅黑")
        << QStringLiteral("Segoe UI")
        << QStringLiteral("Noto Sans CJK SC");
    const QStringList fams = QFontDatabase().families();
    for (int i = 0; i < prefer.size(); ++i) {
        if (fams.contains(prefer.at(i))) {
            font.setFamily(prefer.at(i));
            break;
        }
    }
    font.setPixelSize(11);
    font.setBold(false);
    font.setStyleStrategy(QFont::PreferAntialias);
    QFontMetrics fm(font);
    const int padX = 8;
    const int padY = 3;
    const int innerH = qMax(20, fm.height() + padY * 2);
    const int innerW = fm.horizontalAdvance(text) + padX * 2;
    const qreal radius = innerH / 2.0;
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
    // 基线居中：中英混排比 AlignVCenter 更不容易高低错位
    const int baseline = (innerH + fm.ascent() - fm.descent()) / 2;
    p.drawText(padX, baseline, text);
    return pixmapToImgHtml(pm);
}

// 发出 meta 时间：等宽感数字 + 次级灰，避免 HTML font 发虚
static QString metaPlainImgHtml(const QString &text)
{
    QFont font = qApp->font();
    const QStringList prefer = QStringList()
        << QStringLiteral("Segoe UI")
        << QStringLiteral("Consolas")
        << QStringLiteral("Microsoft YaHei UI")
        << QStringLiteral("Microsoft YaHei");
    const QStringList fams = QFontDatabase().families();
    for (int i = 0; i < prefer.size(); ++i) {
        if (fams.contains(prefer.at(i))) {
            font.setFamily(prefer.at(i));
            break;
        }
    }
    font.setPixelSize(11);
    font.setStyleStrategy(QFont::PreferAntialias);
    QFontMetrics fm(font);
    const int padY = 2;
    const int innerH = qMax(16, fm.height() + padY * 2);
    const int innerW = qMax(1, fm.horizontalAdvance(text));
    QPixmap pm = makeDprPixmap(innerW, innerH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(font);
    p.setPen(QColor(QStringLiteral("#94a3b8")));
    const int baseline = (innerH + fm.ascent() - fm.descent()) / 2;
    p.drawText(0, baseline, text);
    return pixmapToImgHtml(pm);
}

static QString metaLine(const QString &who, const QString &time, qint64 rttMs, bool failed,
                        bool showSendState = false)
{
    const QString clock = who.isEmpty() ? time : (who + QLatin1Char(' ') + time);
    QString mid = metaPlainImgHtml(clock);
    QString badge;
    if (failed) {
        badge = metaBadgeImgHtml(QString::fromUtf8(u8"发送失败"),
                                 QColor(QStringLiteral("#fef2f2")),
                                 QColor(QStringLiteral("#dc2626")),
                                 QColor(QStringLiteral("#fecaca")));
    } else if (showSendState && rttMs < 0) {
        badge = metaBadgeImgHtml(QString::fromUtf8(u8"发送中…"),
                                 QColor(QStringLiteral("#f1f5f9")),
                                 QColor(QStringLiteral("#64748b")),
                                 QColor(QStringLiteral("#e2e8f0")));
    } else if (rttMs >= 0) {
        badge = metaBadgeImgHtml(
            QString::fromUtf8(u8"已送达 · %1").arg(formatDurationMs(rttMs)),
            QColor(QStringLiteral("#ecfdf5")),
            QColor(QStringLiteral("#059669")),
            QColor(QStringLiteral("#a7f3d0")));
    }
    if (!badge.isEmpty())
        mid += QStringLiteral("&nbsp;") + badge;
    return mid;
}

// 参考图：头像与名字顶对齐；气泡在名字下方、相对头像斜对角偏下（勿把头像贴气泡底边）。
static QString renderMsgRow(bool out, const QString &meta, const QString &body, const QString &avatarHtml)
{
    if (out) {
        return QStringLiteral(
                   "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr>"
                   "<td></td>"
                   "<td align=\"right\" valign=\"top\">"
                   "<div align=\"right\" style=\"margin-right:5px;\">%1</div>"
                   "<div style=\"margin-top:8px;\" align=\"right\">%2</div>"
                   "</td>"
                   "<td width=\"56\" valign=\"top\">%3</td>"
                   "</tr></table>")
            .arg(meta, body, avatarHtml);
    }
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr>"
               "<td width=\"56\" valign=\"top\">%1</td>"
               "<td align=\"left\" valign=\"top\">"
               "<div style=\"margin-left:5px;\">%2</div>"
               "<div style=\"margin-top:8px;\">%3</div>"
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
    const QString head = metaLine(out ? QString() : m.who, m.time, out ? m.rttMs : -1, false, out);
    if (splitCodeFence(m.text, &lang, &code))
        return renderMsgRow(out, head, renderCodeBlock(lang, code), avatar);
    QString img = textBubbleImgHtml(m.text, out);
    QString body = img;
    if (!m.text.isEmpty()) {
        const QString href = QStringLiteral("landrop://copy/")
            + QString::fromLatin1(m.text.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
        // 点气泡本体即可复制；不再挂重复「复制」胶囊
        body = QStringLiteral(
                   "<a href=\"%1\" title=\"%2\" style=\"text-decoration:none;\">%3</a>")
                   .arg(href,
                        QString::fromUtf8(u8"点击复制"),
                        img);
    }
    return renderMsgRow(out, head, body, avatar);
}

static bool isImageFileName(const QString &nameOrPath)
{
    const QString e = QFileInfo(nameOrPath).suffix().toLower();
    return e == QLatin1String("png") || e == QLatin1String("jpg") || e == QLatin1String("jpeg")
        || e == QLatin1String("gif") || e == QLatin1String("bmp") || e == QLatin1String("webp");
}

// 缩略落临时目录，避免大图塞进 HTML；>12MB 不解码
static QString imageThumbFile(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile() || fi.size() <= 0 || fi.size() > 12LL * 1024 * 1024)
        return QString();
    if (!isImageFileName(fi.fileName()))
        return QString();
    // |r2：圆角浅边版本，强制旧直角缓存失效
    const QByteArray keySrc = (fi.absoluteFilePath() + QLatin1Char('|')
                               + QString::number(fi.size()) + QLatin1Char('|')
                               + QString::number(fi.lastModified().toMSecsSinceEpoch())
                               + QStringLiteral("|r2"))
                                  .toUtf8();
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash(keySrc, QCryptographicHash::Sha1).toHex().left(16));
    const QString dir = QDir::temp().filePath(QStringLiteral("landrop-thumbs"));
    QDir().mkpath(dir);
    const QString out = QDir(dir).filePath(key + QStringLiteral(".png"));
    if (QFileInfo::exists(out))
        return out;
    QImage img(path);
    if (img.isNull())
        return QString();
    const QImage scaled = img.scaled(240, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaled.isNull())
        return QString();
    const int pad = 1;
    QImage canvas(scaled.width() + pad * 2, scaled.height() + pad * 2,
                  QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    QPainter tp(&canvas);
    tp.setRenderHint(QPainter::Antialiasing, true);
    tp.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF box(0.5, 0.5, canvas.width() - 1.0, canvas.height() - 1.0);
    QPainterPath clip;
    clip.addRoundedRect(box, 10.0, 10.0);
    tp.setClipPath(clip);
    tp.drawImage(pad, pad, scaled);
    tp.setClipping(false);
    tp.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
    tp.setBrush(Qt::NoBrush);
    tp.drawRoundedRect(box, 10.0, 10.0);
    tp.end();
    if (!canvas.save(out, "PNG"))
        return QString();
    return out;
}

static void paintFileTypeGlyph(QPainter &p, const QRectF &box, const QColor &color)
{
    QPen pen(color, qMax(1.4, box.width() * 0.08));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal x = box.x();
    const qreal y = box.y();
    const qreal w = box.width();
    const qreal h = box.height();
    // 折角文档
    QPainterPath doc;
    const qreal fold = w * 0.28;
    doc.moveTo(x + w * 0.18, y + h * 0.12);
    doc.lineTo(x + w * 0.72 - fold, y + h * 0.12);
    doc.lineTo(x + w * 0.82, y + h * 0.12 + fold);
    doc.lineTo(x + w * 0.82, y + h * 0.88);
    doc.lineTo(x + w * 0.18, y + h * 0.88);
    doc.closeSubpath();
    p.drawPath(doc);
    p.drawLine(QPointF(x + w * 0.72 - fold, y + h * 0.12),
               QPointF(x + w * 0.72 - fold, y + h * 0.12 + fold));
    p.drawLine(QPointF(x + w * 0.72 - fold, y + h * 0.12 + fold),
               QPointF(x + w * 0.82, y + h * 0.12 + fold));
    p.drawLine(QPointF(x + w * 0.28, y + h * 0.42), QPointF(x + w * 0.70, y + h * 0.42));
    p.drawLine(QPointF(x + w * 0.28, y + h * 0.55), QPointF(x + w * 0.70, y + h * 0.55));
    p.drawLine(QPointF(x + w * 0.28, y + h * 0.68), QPointF(x + w * 0.58, y + h * 0.68));
}

static void paintDoubleCheck(QPainter &p, qreal x, qreal y, qreal s, const QColor &color)
{
    QPen pen(color, qMax(1.5, s * 0.14));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    auto one = [&](qreal ox) {
        QPainterPath path;
        path.moveTo(ox, y + s * 0.52);
        path.lineTo(ox + s * 0.22, y + s * 0.72);
        path.lineTo(ox + s * 0.58, y + s * 0.28);
        p.drawPath(path);
    };
    one(x);
    one(x + s * 0.32);
}

static QPixmap cropLogical(const QPixmap &pm, const QRect &logicalRect)
{
    const qreal dpr = chatDpr();
    const QRect device(qRound(logicalRect.x() * dpr), qRound(logicalRect.y() * dpr),
                       qRound(logicalRect.width() * dpr), qRound(logicalRect.height() * dpr));
    QPixmap out = pm.copy(device.intersected(pm.rect()));
    out.setDevicePixelRatio(dpr);
    return out;
}

static QString linkedImg(const QString &href, const QString &title, const QPixmap &pm)
{
    if (href.isEmpty())
        return pixmapToImgHtml(pm);
    return QStringLiteral("<a href=\"%1\" title=\"%2\" style=\"text-decoration:none;\">%3</a>")
        .arg(href, title, pixmapToImgHtml(pm));
}

// 完成态操作区绘进白卡内；按行切片成可点链接，避免按钮落在气泡外
static QString fileCardShellImgHtml(const QString &fileName, const QString &sizeLabel,
                                    bool asImage, bool pending, bool out, int pct,
                                    const QString &shaShort, const QString &openHref,
                                    const QString &revealHref, const QString &copyPathHref)
{
    const int cardW = 320;
    const int pad = 14;
    const qreal radius = 16.0;
    const int icon = 40;
    const int gap = 12;
    const int barH = 6;
    const bool hasActions = !pending && !openHref.isEmpty();
    QFont nameFont = qApp->font();
    nameFont.setPixelSize(14);
    nameFont.setBold(true);
    QFont metaFont = qApp->font();
    metaFont.setPixelSize(12);
    QFont btnFont = qApp->font();
    btnFont.setPixelSize(12);
    btnFont.setBold(true);
    QFontMetrics nameFm(nameFont);
    QFontMetrics metaFm(metaFont);
    QFontMetrics btnFm(btnFont);
    const int nameMaxW = cardW - pad * 2 - icon - gap;
    const QString elided = nameFm.elidedText(fileName, Qt::ElideMiddle, nameMaxW);
    const int nameH = nameFm.height();
    const int metaH = metaFm.height();
    const int topH = qMax(icon, nameH + 4 + metaH);
    const int statusH = metaH;
    const int shaH = (!pending && !shaShort.isEmpty()) ? (6 + metaH) : 0;
    const int btnH = 30;
    const int actionGap = 10;
    const int footerH = hasActions && !out ? (6 + metaH) : 0;
    // 三钮同行 + 可选脚注
    const int actionBlock = hasActions ? (actionGap + btnH + footerH) : 0;
    const int bodyH = pad + topH + gap + barH + 10 + statusH + shaH;
    const int innerH = bodyH + actionBlock + pad;
    const int logicalW = cardW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    QPixmap pm = makeDprPixmap(logicalW, logicalH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QRectF box(kShadowPad + 1.0, kShadowPad + 1.0, cardW - 2.0, innerH - 2.0);
    paintSoftShadow(p, box, radius);
    p.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
    p.setBrush(Qt::white);
    p.drawRoundedRect(box, radius, radius);

    const qreal ox = kShadowPad;
    const qreal oy = kShadowPad;
    const QRectF iconRect(ox + pad, oy + pad + (topH - icon) / 2.0, icon, icon);
    p.setPen(Qt::NoPen);
    p.setBrush(asImage ? QColor(QStringLiteral("#ecfdf5")) : QColor(QStringLiteral("#eff6ff")));
    p.drawRoundedRect(iconRect, 10.0, 10.0);
    paintFileTypeGlyph(p, iconRect.adjusted(6, 6, -6, -6),
                       asImage ? QColor(QStringLiteral("#059669"))
                               : QColor(QStringLiteral("#4f46e5")));

    const int textX = int(ox) + pad + icon + gap;
    p.setFont(nameFont);
    p.setPen(QColor(QStringLiteral("#0f172a")));
    p.drawText(QRect(textX, int(oy) + pad, nameMaxW, nameH), Qt::AlignLeft | Qt::AlignVCenter, elided);
    p.setFont(metaFont);
    p.setPen(QColor(QStringLiteral("#94a3b8")));
    p.drawText(QRect(textX, int(oy) + pad + nameH + 4, nameMaxW, metaH), Qt::AlignLeft | Qt::AlignVCenter,
               sizeLabel);

    const int barY = int(oy) + pad + topH + gap;
    const QRectF track(ox + pad, barY, cardW - pad * 2, barH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#e2e8f0")));
    p.drawRoundedRect(track, barH / 2.0, barH / 2.0);
    const int fillW = qRound(track.width() * qBound(0, 100, pct) / 100.0);
    if (fillW > 0) {
        QRectF fill = track;
        fill.setWidth(qMax(barH, fillW));
        if (pending) {
            p.setBrush(QColor(QStringLiteral("#2563eb")));
            p.drawRoundedRect(fill, barH / 2.0, barH / 2.0);
        } else {
            QLinearGradient grad(fill.topLeft(), fill.topRight());
            grad.setColorAt(0.0, QColor(QStringLiteral("#3b82f6")));
            grad.setColorAt(1.0, QColor(QStringLiteral("#7c3aed")));
            p.setBrush(grad);
            p.drawRoundedRect(fill, barH / 2.0, barH / 2.0);
        }
    }

    const int statusY = barY + barH + 10;
    const int contentW = cardW - pad * 2;
    if (pending) {
        const QString status = out ? QString::fromUtf8(u8"发送中 %1%").arg(pct)
                                   : QString::fromUtf8(u8"接收中 %1%").arg(pct);
        const QColor statusColor = out ? QColor(QStringLiteral("#2563eb"))
                                       : QColor(QStringLiteral("#ea580c"));
        p.setFont(metaFont);
        p.setPen(statusColor);
        p.drawText(QRect(int(ox) + pad, statusY, contentW, statusH),
                   Qt::AlignLeft | Qt::AlignVCenter, status);
    } else {
        const QColor ok(QStringLiteral("#16a34a"));
        const qreal checkS = 14.0;
        paintDoubleCheck(p, ox + pad, statusY + (statusH - checkS) / 2.0, checkS, ok);
        QFont statusFont = metaFont;
        statusFont.setBold(true);
        p.setFont(statusFont);
        p.setPen(ok);
        p.drawText(QRect(int(ox) + pad + 22, statusY, contentW - 22, statusH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::fromUtf8(u8"传输完成 (已落盘)"));
    }

    int shaBottom = statusY + statusH;
    if (shaH > 0) {
        const int shaY = statusY + statusH + 6;
        const int shield = 14;
        QSvgRenderer shieldSvg(QStringLiteral(":/icons/shield-check.svg"));
        if (shieldSvg.isValid()) {
            shieldSvg.render(&p, QRectF(ox + pad, shaY + (metaH - shield) / 2.0, shield, shield));
        }
        p.setFont(metaFont);
        p.setPen(QColor(QStringLiteral("#94a3b8")));
        p.drawText(QRect(int(ox) + pad + shield + 6, shaY, contentW - shield - 6, metaH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("SHA256: %1").arg(shaShort));
        shaBottom = shaY + metaH;
    }

    QRect openRect;
    QRect revealRect;
    QRect copyRect;
    int actBottom = 0;
    if (hasActions) {
        const int actY = shaBottom + actionGap;
        const int chipGap = 6;
        const int chipPadX = 10;
        auto chipW = [&](const QString &t) { return btnFm.horizontalAdvance(t) + chipPadX * 2; };
        const QString openLabel = QString::fromUtf8(u8"打开");
        const QString revealLabel = QString::fromUtf8(u8"目录");
        const QString copyLabel = QString::fromUtf8(u8"复制路径");
        openRect = QRect(int(ox) + pad, actY, chipW(openLabel), btnH);
        revealRect = QRect(openRect.right() + chipGap, actY, chipW(revealLabel), btnH);
        copyRect = QRect(revealRect.right() + chipGap, actY, chipW(copyLabel), btnH);

        auto paintChip = [&](const QRect &r, const QString &label, bool primary) {
            p.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
            p.setBrush(primary ? QColor(QStringLiteral("#dbeafe")) : QColor(QStringLiteral("#f1f5f9")));
            p.drawRoundedRect(QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5), 8.0, 8.0);
            p.setFont(btnFont);
            p.setPen(primary ? QColor(QStringLiteral("#2563eb")) : QColor(QStringLiteral("#334155")));
            p.drawText(r, Qt::AlignCenter, label);
        };
        paintChip(openRect, openLabel, true);
        paintChip(revealRect, revealLabel, false);
        paintChip(copyRect, copyLabel, false);

        if (!out) {
            const int footY = actY + btnH + 6;
            p.setFont(metaFont);
            p.setPen(QColor(QStringLiteral("#94a3b8")));
            p.drawText(QRect(int(ox) + pad, footY, contentW, metaH), Qt::AlignLeft | Qt::AlignVCenter,
                       QString::fromUtf8(u8"局域网直传 · 已存入下载目录"));
        }
        actBottom = actY + btnH;
    }

    if (!hasActions)
        return pixmapToImgHtml(pm);

    const int bodyBottom = int(oy) + bodyH;
    const int rowTop = bodyBottom;
    const int rowBottom = actBottom;
    const int cardBottom = int(oy) + innerH;
    QString html = QStringLiteral("<table cellspacing=\"0\" cellpadding=\"0\" style=\"border-collapse:collapse;\">");
    html += QStringLiteral("<tr><td>")
        + linkedImg(openHref, QString::fromUtf8(u8"点击打开"),
                    cropLogical(pm, QRect(0, 0, logicalW, rowTop)))
        + QStringLiteral("</td></tr>");

    {
        const int rowH = rowBottom - rowTop;
        const int g1 = revealRect.x() - openRect.right();
        const int g2 = copyRect.x() - revealRect.right();
        html += QStringLiteral("<tr><td><table cellspacing=\"0\" cellpadding=\"0\"><tr>");
        html += QStringLiteral("<td>")
            + pixmapToImgHtml(cropLogical(pm, QRect(0, rowTop, openRect.x(), rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + linkedImg(openHref, QString::fromUtf8(u8"点击打开"),
                        cropLogical(pm, QRect(openRect.x(), rowTop, openRect.width(), rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + pixmapToImgHtml(cropLogical(pm, QRect(openRect.right(), rowTop, g1, rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + linkedImg(revealHref, QString::fromUtf8(u8"打开所在目录"),
                        cropLogical(pm, QRect(revealRect.x(), rowTop, revealRect.width(), rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + pixmapToImgHtml(cropLogical(pm, QRect(revealRect.right(), rowTop, g2, rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + linkedImg(copyPathHref, QString::fromUtf8(u8"点击复制路径"),
                        cropLogical(pm, QRect(copyRect.x(), rowTop, copyRect.width(), rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("<td>")
            + pixmapToImgHtml(cropLogical(pm, QRect(copyRect.right(), rowTop,
                                                    logicalW - copyRect.right(), rowH)))
            + QStringLiteral("</td>");
        html += QStringLiteral("</tr></table></td></tr>");
    }

    if (cardBottom > rowBottom) {
        html += QStringLiteral("<tr><td>")
            + pixmapToImgHtml(cropLogical(pm, QRect(0, rowBottom, logicalW, cardBottom - rowBottom)))
            + QStringLiteral("</td></tr>");
    }
    html += QStringLiteral("</table>");
    return html;
}

// Qt 富文本 border-radius 不可靠，操作胶囊绘成位图再嵌 <a>
static QString actionChipHtml(const QString &href, const QString &label, const QString &title,
                              bool primary)
{
    QFont font = qApp->font();
    font.setPixelSize(12);
    font.setBold(true);
    QFontMetrics fm(font);
    const int padX = 11;
    const int padY = 5;
    const int innerH = qMax(26, fm.height() + padY * 2);
    const int innerW = fm.horizontalAdvance(label) + padX * 2;
    const qreal radius = 8.0;
    QPixmap pm = makeDprPixmap(innerW, innerH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(0.5, 0.5, innerW - 1.0, innerH - 1.0);
    // 参考图：统一浅灰底；主操作蓝字，次要深灰字
    const QColor bg(QStringLiteral("#f1f5f9"));
    const QColor border(QStringLiteral("#e2e8f0"));
    const QColor fg = primary ? QColor(QStringLiteral("#2563eb")) : QColor(QStringLiteral("#334155"));
    p.setPen(QPen(border, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(box, radius, radius);
    p.setFont(font);
    p.setPen(fg);
    p.drawText(QRect(0, 0, innerW, innerH), Qt::AlignCenter, label);
    return QStringLiteral("<td style=\"padding:0;\">"
                          "<a href=\"%1\" title=\"%2\" style=\"text-decoration:none;\">%3</a></td>")
        .arg(href, title, pixmapToImgHtml(pm));
}

static QString renderFileCard(const ChatMsg &m)
{
    const bool out = (m.type == ChatMsg::OutFile);
    const bool pending = (m.type == ChatMsg::OutFile || m.type == ChatMsg::InFile) && m.progressPct >= 0;
    const QString size = humanBytesChat(m.size);
    QString sha = m.sha256;
    if (!pending && sha.isEmpty() && !m.path.isEmpty())
        sha = fileSha256Short(m.path);
    const QString pathB64 = (!pending && !m.path.isEmpty())
        ? QString::fromLatin1(m.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding))
        : QString();
    QString openHref;
    QString revealHref;
    QString copyPathHref;
    if (!pathB64.isEmpty()) {
        openHref = QStringLiteral("landrop://open/") + pathB64;
        revealHref = QStringLiteral("landrop://reveal/") + pathB64;
        copyPathHref = QStringLiteral("landrop://copypath/") + pathB64;
    }
    const int pct = pending ? qBound(0, 100, m.progressPct) : 100;
    const bool asImage = isImageFileName(m.text) || isImageFileName(m.path);
    QString shell = fileCardShellImgHtml(m.text, size, asImage, pending, out, pct,
                                         pending ? QString() : sha, openHref, revealHref,
                                         copyPathHref);
    QString thumbHtml;
    if (!pending && !m.path.isEmpty()) {
        const QString thumb = imageThumbFile(m.path);
        if (!thumb.isEmpty()) {
            const QString src = QUrl::fromLocalFile(thumb).toString();
            if (openHref.isEmpty())
                thumbHtml = QStringLiteral("<br/><img src=\"%1\" />").arg(src);
            else
                thumbHtml = QString::fromUtf8(
                                u8"<br/><a href=\"%1\" style=\"text-decoration:none;\">"
                                u8"<img src=\"%2\" /></a>")
                                .arg(openHref, src);
        }
    }
    const QString card = shell + thumbHtml;
    const QString head = metaLine(out ? QString() : m.who, m.time, pending ? -1 : m.rttMs, false, out);
    const QString avatar = letterAvatarHtml(
        faceName(m), out ? QStringLiteral("#2563eb") : QStringLiteral("#f97316"));
    return renderMsgRow(out, head, card, avatar);
}

static QString systemCapsuleImgHtml(const QString &text, bool fail)
{
    const int maxW = 420;
    const int padX = 14;
    const int padY = 8;
    const qreal radius = 14.0;
    QFont font = qApp->font();
    font.setPixelSize(13);
    QFontMetrics fm(font);
    const QRect bound = fm.boundingRect(QRect(0, 0, maxW - padX * 2, 10000),
                                        Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter,
                                        text);
    const int contentW = qMax(40, qMin(maxW - padX * 2, bound.width()));
    const int contentH = qMax(fm.height(), bound.height());
    const int innerW = contentW + padX * 2;
    const int innerH = contentH + padY * 2;
    const int logicalW = innerW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    QPixmap pm = makeDprPixmap(logicalW, logicalH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 0.5, kShadowPad + 0.5, innerW - 1.0, innerH - 1.0);
    paintSoftShadow(p, box, radius);
    const QColor bg = fail ? QColor(QStringLiteral("#fef2f2")) : QColor(QStringLiteral("#fffbeb"));
    const QColor border = fail ? QColor(QStringLiteral("#fecaca")) : QColor(QStringLiteral("#fde68a"));
    const QColor fg = fail ? QColor(QStringLiteral("#b91c1c")) : QColor(QStringLiteral("#b45309"));
    p.setPen(QPen(border, 1.0));
    p.setBrush(bg);
    p.drawRoundedRect(box, radius, radius);
    p.setFont(font);
    p.setPen(fg);
    p.drawText(QRect(kShadowPad + padX, kShadowPad + padY, contentW, contentH),
               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter, text);
    return pixmapToImgHtml(pm);
}

static QString renderSystem(const ChatMsg &m)
{
    const bool fail = (m.type == ChatMsg::Fail);
    QString links;
    if (fail && !m.path.isEmpty()) {
        if (m.path.startsWith(QLatin1String("text:"))) {
            const QString rawText = m.path.mid(5);
            const QString href = QStringLiteral("landrop://retrytext/")
                + QString::fromLatin1(rawText.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            links = QStringLiteral("<br/><table cellspacing=\"0\" cellpadding=\"0\"><tr>")
                + actionChipHtml(href, QString::fromUtf8(u8"重发"),
                                 QString::fromUtf8(u8"重新发送该消息"), true)
                + QStringLiteral("</tr></table>");
        } else {
            const QString href = QStringLiteral("landrop://retry/")
                + QString::fromLatin1(m.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            links = QStringLiteral("<br/><table cellspacing=\"0\" cellpadding=\"0\"><tr>")
                + actionChipHtml(href, QString::fromUtf8(u8"重试"),
                                 QString::fromUtf8(u8"重新发送该文件"), true);
            if (!m.morePaths.isEmpty()) {
                QStringList all;
                all << m.path;
                for (int i = 0; i < m.morePaths.size(); ++i) {
                    if (!m.morePaths.at(i).isEmpty() && !all.contains(m.morePaths.at(i)))
                        all.append(m.morePaths.at(i));
                }
                const QByteArray joined = all.join(QStringLiteral("\n")).toUtf8();
                const QString batchHref = QStringLiteral("landrop://retrybatch/")
                    + QString::fromLatin1(joined.toBase64(QByteArray::Base64UrlEncoding));
                links += QStringLiteral("<td width=\"6\"></td>")
                    + actionChipHtml(batchHref,
                                     QString::fromUtf8(u8"重发剩余 %1").arg(all.size()),
                                     QString::fromUtf8(u8"重发本文件及排队剩余"), false);
            }
            links += QStringLiteral("</tr></table>");
        }
    }
    const QString capsule = systemCapsuleImgHtml(m.text, fail);
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr><td align=\"center\">"
               "%1%2"
               "</td></tr></table>")
        .arg(capsule, links);
}

static void paintKeycap(QPainter &p, const QRectF &r, const QString &label)
{
    p.setPen(QPen(QColor(QStringLiteral("#94a3b8")), 1.0));
    p.setBrush(QColor(QStringLiteral("#f8fafc")));
    p.drawRoundedRect(r, 5.0, 5.0);
    QFont f = qApp->font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(QStringLiteral("#475569")));
    p.drawText(r, Qt::AlignCenter, label);
}

static QString emptyGuideCardImgHtml(const QString &title, const QStringList &keycaps,
                                     const QString &footer, int cardW = 360)
{
    const bool compact = cardW < 280;
    const int pad = compact ? 16 : 28;
    const qreal radius = compact ? 12.0 : 16.0;
    QFont titleFont = qApp->font();
    titleFont.setPixelSize(compact ? 14 : 17);
    titleFont.setBold(true);
    QFont footFont = qApp->font();
    footFont.setPixelSize(compact ? 11 : 12);
    QFontMetrics titleFm(titleFont);
    QFontMetrics footFm(footFont);
    const int titleH = titleFm.height();
    const int capH = compact ? 22 : 24;
    const int footMaxW = cardW - pad * 2;
    const QRect footBound = footer.isEmpty()
        ? QRect()
        : footFm.boundingRect(QRect(0, 0, footMaxW, 10000), Qt::TextWordWrap, footer);
    const int footH = footer.isEmpty() ? 0 : (footBound.height() + (compact ? 8 : 10));
    const int capsBlock = keycaps.isEmpty() ? 0 : (compact ? 10 : 12) + capH;
    const int innerH = pad + titleH + capsBlock + footH + pad;
    const int logicalW = cardW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    QPixmap pm = makeDprPixmap(logicalW, logicalH);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 1.0, kShadowPad + 1.0, cardW - 2.0, innerH - 2.0);
    paintSoftShadow(p, box, radius);
    p.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
    p.setBrush(Qt::white);
    p.drawRoundedRect(box, radius, radius);
    p.setFont(titleFont);
    p.setPen(QColor(QStringLiteral("#0f172a")));
    p.drawText(QRect(kShadowPad + pad, kShadowPad + pad, cardW - pad * 2, titleH),
               Qt::AlignLeft | Qt::AlignVCenter, title);
    int y = kShadowPad + pad + titleH + 12;
    if (!keycaps.isEmpty()) {
        QFont capFont = qApp->font();
        capFont.setPixelSize(11);
        capFont.setBold(true);
        QFontMetrics capFm(capFont);
        int x = kShadowPad + pad;
        for (int i = 0; i < keycaps.size(); ++i) {
            const QString lab = keycaps.at(i);
            const int w = qMax(36, capFm.horizontalAdvance(lab) + 16);
            if (x + w > kShadowPad + cardW - pad)
                break;
            paintKeycap(p, QRectF(x, y, w, capH), lab);
            x += w + 8;
        }
        y += capH + 10;
    }
    if (!footer.isEmpty()) {
        p.setFont(footFont);
        p.setPen(QColor(QStringLiteral("#94a3b8")));
        p.drawText(QRect(kShadowPad + pad, y, footMaxW, footBound.height()),
                   Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, footer);
    }
    return pixmapToImgHtml(pm);
}

QString renderChatHtml(const QVector<ChatMsg> &msgs)
{
    QString html = QStringLiteral(
        "<html><body style=\"margin:0;padding:8px;background:#f1f5f9;\">");
    bool hasUserContent = false;
    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg &m = msgs.at(i);
        html += QStringLiteral("<div style=\"margin:18px 0;\">");
        switch (m.type) {
        case ChatMsg::OutText:
        case ChatMsg::InText:
            hasUserContent = true;
            html += renderTextBubble(m);
            break;
        case ChatMsg::OutFile:
        case ChatMsg::InFile:
            hasUserContent = true;
            html += renderFileCard(m);
            break;
        case ChatMsg::System:
        case ChatMsg::Fail:
            html += renderSystem(m);
            break;
        }
        html += QStringLiteral("</div>");
    }
    if (!hasUserContent) {
        QStringList caps;
        caps << QString::fromUtf8(u8"发送")
             << QString::fromUtf8(u8"换行")
             << QString::fromUtf8(u8"粘贴");
        const QString card = emptyGuideCardImgHtml(
            QString::fromUtf8(u8"发消息，或把文件拖到这里"), caps,
            QString::fromUtf8(u8"回车 · Shift+回车 · Ctrl+V"));
        html += QStringLiteral(
                    "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"48\"><tr>"
                    "<td align=\"center\">%1</td></tr></table>")
                    .arg(card);
    }
    html += QStringLiteral("</body></html>");
    return html;
}

int countFiles(const QVector<ChatMsg> &msgs)
{
    int n = 0;
    for (int i = 0; i < msgs.size(); ++i) {
        if (msgs.at(i).type == ChatMsg::InFile || msgs.at(i).type == ChatMsg::OutFile)
            ++n;
    }
    return n;
}

QString renderFilesHtml(const QVector<ChatMsg> &msgs)
{
    QString html = QStringLiteral(
        "<html><body style=\"margin:0;padding:8px;background:#f1f5f9;\">");
    int n = 0;
    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg &m = msgs.at(i);
        if (m.type != ChatMsg::InFile && m.type != ChatMsg::OutFile)
            continue;
        html += QStringLiteral("<div style=\"margin:18px 0;\">");
        html += renderFileCard(m);
        html += QStringLiteral("</div>");
        ++n;
    }
    if (n == 0) {
        QStringList caps;
        caps << QString::fromUtf8(u8"附件")
             << QString::fromUtf8(u8"拖入")
             << QString::fromUtf8(u8"文件夹");
        const QString card = emptyGuideCardImgHtml(
            QString::fromUtf8(u8"还没有文件传输"), caps,
            QString::fromUtf8(u8"把文件拖到聊天区，或点左下角附件发送"));
        html += QStringLiteral(
                    "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"48\"><tr>"
                    "<td align=\"center\">%1</td></tr></table>")
                    .arg(card);
    }
    html += QStringLiteral("</body></html>");
    return html;
}

QString renderSidebarEmptyHintHtml(bool noMatch, bool discoverOk)
{
    if (noMatch) {
        return emptyGuideCardImgHtml(
            QString::fromUtf8(u8"没有匹配"),
            QStringList() << QString::fromUtf8(u8"Esc"),
            QString::fromUtf8(u8"清除搜索后再试"),
            200);
    }
    const QString foot = discoverOk
        ? QString::fromUtf8(u8"同网段等待发现；防火墙请放行 TCP 8848 与 UDP 8850")
        : QString::fromUtf8(u8"发现异常，请点「添加 IP」；并检查防火墙端口");
    return emptyGuideCardImgHtml(
        QString::fromUtf8(u8"暂无设备"),
        QStringList() << QString::fromUtf8(u8"添加 IP"),
        foot,
        200);
}

QString renderMainEmptyHintHtml(bool noMatch, const QString &query, bool discoverOk)
{
    if (noMatch) {
        QString foot = QString::fromUtf8(u8"可按 Esc 清除搜索，或改用名称 / IP / 标签");
        if (!query.trimmed().isEmpty()) {
            QString q = query.trimmed();
            if (q.size() > 20)
                q = q.left(18) + QString::fromUtf8(u8"…");
            foot = QString::fromUtf8(u8"没有匹配「%1」· Esc 清除").arg(q);
        }
        return emptyGuideCardImgHtml(
            QString::fromUtf8(u8"没有匹配的设备"),
            QStringList() << QString::fromUtf8(u8"Esc"),
            foot,
            360);
    }
    QStringList caps;
    caps << QString::fromUtf8(u8"添加 IP")
         << QString::fromUtf8(u8"本机")
         << QString::fromUtf8(u8"防火墙");
    const QString foot = discoverOk
        ? QString::fromUtf8(
              u8"① 两端同网段，放行 TCP 8848 / UDP 8850\n"
              u8"② 点顶栏「本机」复制地址发给对方\n"
              u8"③ 或点「添加 IP」手动添加后选中即可聊天")
        : QString::fromUtf8(
              u8"发现端口异常：仍可用「添加 IP」直连。\n"
              u8"请确认防火墙已放行 TCP 8848 与 UDP 8850，并点顶栏复制本机地址给对方。");
    return emptyGuideCardImgHtml(
        QString::fromUtf8(u8"还没有可聊的设备"), caps, foot, 400);
}
