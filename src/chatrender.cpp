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
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QUrl>

// —— 本模块自用（不依赖 uiicons；体积格式复用 fmtutil）——

static QString htmlEsc(const QString &s)
{
    return s.toHtmlEscaped();
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

static QString letterAvatarHtml(const QString &name, const QString &bg)
{
    const QString ch = avatarInitial(name);
    // 参考图比例：略放大，与更大气泡协调
    const int logical = 44;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logical * dpr, logical * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
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
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
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
    // Qt 富文本无圆角：整块绘成深色圆角卡，复制链仍用 HTML
    const int maxW = 420;
    const int padX = 14;
    const int padY = 12;
    const qreal radius = 14.0;
    QFont headFont = qApp->font();
    headFont.setPixelSize(13);
    QFont codeFont(QStringLiteral("Consolas"));
    if (!codeFont.exactMatch())
        codeFont = QFont(QStringLiteral("Courier New"));
    codeFont.setPixelSize(14);
    codeFont.setStyleHint(QFont::Monospace);
    QFontMetrics headFm(headFont);
    QFontMetrics codeFm(codeFont);
    const QRect codeBound = codeFm.boundingRect(QRect(0, 0, maxW - padX * 2, 10000),
                                               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                               code);
    const int contentW = qMax(160, qMin(maxW - padX * 2, qMax(headFm.horizontalAdvance(lang) + 48, codeBound.width())));
    const int headH = headFm.height();
    const int gap = 8;
    const int contentH = headH + gap + qMax(codeFm.height(), codeBound.height());
    const int innerW = contentW + padX * 2;
    const int innerH = contentH + padY * 2;
    const int logicalW = innerW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 0.5, kShadowPad + 0.5, innerW - 1.0, innerH - 1.0);
    paintSoftShadow(p, box, radius);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#1e293b")));
    p.drawRoundedRect(box, radius, radius);
    p.setFont(headFont);
    p.setPen(QColor(QStringLiteral("#94a3b8")));
    p.drawText(QRect(kShadowPad + padX, kShadowPad + padY, contentW, headH),
               Qt::AlignLeft | Qt::AlignVCenter, lang);
    p.setFont(codeFont);
    p.setPen(QColor(QStringLiteral("#e2e8f0")));
    p.drawText(QRect(kShadowPad + padX, kShadowPad + padY + headH + gap, contentW, codeBound.height()),
               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
               code);
    const QString img = pixmapToImgHtml(pm);
    return QString::fromUtf8(
               u8"%1<br/><a href=\"%2\" style=\"text-decoration:none;\">"
               u8"<font color=\"#93c5fd\" size=\"3\">复制</font></a>")
        .arg(img, href);
}

static QString metaLine(const QString &who, const QString &time, qint64 rttMs, bool failed)
{
    QString mid = htmlEsc(who) + QStringLiteral(" ") + htmlEsc(time);
    if (failed)
        return mid + QString::fromUtf8(u8" <font color=\"#dc2626\" size=\"3\">发送失败</font>");
    if (rttMs >= 0) {
        const QString ms = (rttMs < 1) ? QStringLiteral("<1") : QString::number(rttMs);
        mid += QString::fromUtf8(u8" <font color=\"#16a34a\" size=\"3\">✓✓ 已送达 - %1ms</font>").arg(ms);
    }
    return QStringLiteral("<font color=\"#64748b\" size=\"3\">%1</font>").arg(mid);
}

// 参考图：头像与名字顶对齐；气泡在名字下方、相对头像斜对角偏下（勿把头像贴气泡底边）。
static QString renderMsgRow(bool out, const QString &meta, const QString &body, const QString &avatarHtml)
{
    if (out) {
        return QStringLiteral(
                   "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr>"
                   "<td></td>"
                   "<td align=\"right\" valign=\"top\">"
                   "<div align=\"right\">%1</div>"
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
               "<div>%2</div>"
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
    const QString head = metaLine(m.who, m.time, out ? m.rttMs : -1, false);
    if (splitCodeFence(m.text, &lang, &code))
        return renderMsgRow(out, head, renderCodeBlock(lang, code), avatar);
    QString body = textBubbleImgHtml(m.text, out);
    if (!m.text.isEmpty()) {
        const QString href = QStringLiteral("landrop://copy/")
            + QString::fromLatin1(m.text.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
        body += QString::fromUtf8(
                    u8"<br/><a href=\"%1\" style=\"text-decoration:none;\">"
                    u8"<font color=\"#2563eb\" size=\"3\">复制</font></a>")
                    .arg(href);
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
    const QByteArray keySrc = (fi.absoluteFilePath() + QLatin1Char('|')
                               + QString::number(fi.size()) + QLatin1Char('|')
                               + QString::number(fi.lastModified().toMSecsSinceEpoch()))
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
    if (!scaled.save(out, "PNG"))
        return QString();
    return out;
}

static QString fileCardShellImgHtml(const QString &fileName, const QString &sizeLabel,
                                    bool asImage, bool pending, bool out, int pct,
                                    const QString &shaShort)
{
    const int cardW = 320;
    const int pad = 14;
    const qreal radius = 16.0;
    const int badgeW = 40;
    const int badgeH = 28;
    const int gap = 10;
    const int barH = 8;
    QFont nameFont = qApp->font();
    nameFont.setPixelSize(15);
    nameFont.setBold(true);
    QFont metaFont = qApp->font();
    metaFont.setPixelSize(12);
    QFontMetrics nameFm(nameFont);
    QFontMetrics metaFm(metaFont);
    const int nameMaxW = cardW - pad * 2 - badgeW - gap;
    const QString elided = nameFm.elidedText(fileName, Qt::ElideMiddle, nameMaxW);
    const int nameH = nameFm.height();
    const int metaH = metaFm.height();
    const int topH = qMax(badgeH, nameH + 4 + metaH);
    const int statusH = metaH;
    const int shaH = (!pending && !shaShort.isEmpty()) ? (4 + metaH) : 0;
    const int innerH = pad + topH + gap + barH + 8 + statusH + shaH + pad;
    const int logicalW = cardW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    const QRectF box(kShadowPad + 1.0, kShadowPad + 1.0, cardW - 2.0, innerH - 2.0);
    paintSoftShadow(p, box, radius);
    p.setPen(QPen(QColor(QStringLiteral("#e2e8f0")), 1.0));
    p.setBrush(Qt::white);
    p.drawRoundedRect(box, radius, radius);

    const QColor badgeBg = asImage ? QColor(QStringLiteral("#ecfdf5"))
                                   : QColor(QStringLiteral("#ede9fe"));
    const QColor badgeFg = asImage ? QColor(QStringLiteral("#047857"))
                                   : QColor(QStringLiteral("#7c3aed"));
    const QString badge = asImage ? QStringLiteral("IMG") : QStringLiteral("FILE");
    const qreal ox = kShadowPad;
    const qreal oy = kShadowPad;
    const QRectF badgeRect(ox + pad, oy + pad + (topH - badgeH) / 2.0, badgeW, badgeH);
    p.setPen(Qt::NoPen);
    p.setBrush(badgeBg);
    p.drawRoundedRect(badgeRect, 8.0, 8.0);
    QFont badgeFont = qApp->font();
    badgeFont.setPixelSize(11);
    badgeFont.setBold(true);
    p.setFont(badgeFont);
    p.setPen(badgeFg);
    p.drawText(badgeRect, Qt::AlignCenter, badge);

    const int textX = int(ox) + pad + badgeW + gap;
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
        p.setBrush(QColor(QStringLiteral("#2563eb")));
        p.drawRoundedRect(fill, barH / 2.0, barH / 2.0);
    }

    QString status;
    QColor statusColor;
    if (pending) {
        status = out ? QString::fromUtf8(u8"发送中 %1%").arg(pct)
                     : QString::fromUtf8(u8"接收中 %1%").arg(pct);
        statusColor = out ? QColor(QStringLiteral("#2563eb")) : QColor(QStringLiteral("#ea580c"));
    } else {
        status = QString::fromUtf8(u8"传输完成 · 已落盘");
        statusColor = QColor(QStringLiteral("#16a34a"));
    }
    const int statusY = barY + barH + 8;
    p.setFont(metaFont);
    p.setPen(statusColor);
    p.drawText(QRect(int(ox) + pad, statusY, cardW - pad * 2, statusH), Qt::AlignLeft | Qt::AlignVCenter,
               status);
    if (shaH > 0) {
        p.setPen(QColor(QStringLiteral("#94a3b8")));
        p.drawText(QRect(int(ox) + pad, statusY + statusH + 4, cardW - pad * 2, metaH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("SHA256: %1").arg(shaShort));
    }
    return pixmapToImgHtml(pm);
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
    QString actions;
    if (!pathB64.isEmpty()) {
        openHref = QStringLiteral("landrop://open/") + pathB64;
        const QString revealHref = QStringLiteral("landrop://reveal/") + pathB64;
        const QString copyPathHref = QStringLiteral("landrop://copypath/") + pathB64;
        actions = QString::fromUtf8(
                      u8"<a href=\"%1\" style=\"text-decoration:none;\">"
                      u8"<font color=\"#2563eb\" size=\"3\">打开文件</font></a>"
                      u8"&nbsp;&nbsp;"
                      u8"<a href=\"%2\" style=\"text-decoration:none;\">"
                      u8"<font color=\"#3b82f6\" size=\"3\">打开所在目录</font></a>"
                      u8"&nbsp;&nbsp;"
                      u8"<a href=\"%3\" style=\"text-decoration:none;\">"
                      u8"<font color=\"#475569\" size=\"3\">复制路径</font></a>")
                      .arg(openHref, revealHref, copyPathHref);
        if (!out) {
            actions += QString::fromUtf8(
                u8"&nbsp;&nbsp;<font color=\"#94a3b8\" size=\"2\">局域网直传 · 已存入下载目录</font>");
        }
    } else {
        actions = QString::fromUtf8(u8"<font color=\"#94a3b8\" size=\"3\">局域网直传</font>");
    }
    const int pct = pending ? qBound(0, 100, m.progressPct) : 100;
    const bool asImage = isImageFileName(m.text) || isImageFileName(m.path);
    QString shell = fileCardShellImgHtml(m.text, size, asImage, pending, out, pct,
                                         pending ? QString() : sha);
    if (!openHref.isEmpty()) {
        shell = QStringLiteral("<a href=\"%1\" style=\"text-decoration:none;\">%2</a>")
                    .arg(openHref, shell);
    }
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
    const QString card = shell + thumbHtml + QStringLiteral("<br/>") + actions;
    const QString head = metaLine(m.who, m.time, pending ? -1 : m.rttMs, false);
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
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
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
            links = QString::fromUtf8(
                        u8"<br/><a href=\"%1\" style=\"text-decoration:none;\">"
                        u8"<font color=\"#2563eb\" size=\"3\">重发</font></a>")
                        .arg(href);
        } else {
            const QString href = QStringLiteral("landrop://retry/")
                + QString::fromLatin1(m.path.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
            links = QString::fromUtf8(
                        u8"<br/><a href=\"%1\" style=\"text-decoration:none;\">"
                        u8"<font color=\"#2563eb\" size=\"3\">重试</font></a>")
                        .arg(href);
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
                links += QString::fromUtf8(
                             u8"&nbsp;&nbsp;<a href=\"%1\" style=\"text-decoration:none;\">"
                             u8"<font color=\"#2563eb\" size=\"3\">重发剩余 %2</font></a>")
                             .arg(batchHref)
                             .arg(all.size());
            }
        }
    }
    const QString capsule = systemCapsuleImgHtml(m.text, fail);
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr><td align=\"center\">"
               "%1%2"
               "</td></tr></table>")
        .arg(capsule, links);
}

static QString emptyGuideCardImgHtml(const QString &title, const QString &body)
{
    const int cardW = 360;
    const int pad = 28;
    const qreal radius = 16.0;
    QFont titleFont = qApp->font();
    titleFont.setPixelSize(17);
    titleFont.setBold(true);
    QFont bodyFont = qApp->font();
    bodyFont.setPixelSize(13);
    QFontMetrics titleFm(titleFont);
    QFontMetrics bodyFm(bodyFont);
    const QRect bodyBound = bodyFm.boundingRect(QRect(0, 0, cardW - pad * 2, 10000),
                                               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                               body);
    const int titleH = titleFm.height();
    const int bodyH = qMax(bodyFm.height() * 2, bodyBound.height());
    const int innerH = pad + titleH + 12 + bodyH + pad;
    const int logicalW = cardW + kShadowPad * 2;
    const int logicalH = innerH + kShadowPad * 2;
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    QPixmap pm(logicalW * dpr, logicalH * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
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
    p.setFont(bodyFont);
    p.setPen(QColor(QStringLiteral("#64748b")));
    p.drawText(QRect(kShadowPad + pad, kShadowPad + pad + titleH + 12, cardW - pad * 2, bodyH),
               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, body);
    return pixmapToImgHtml(pm);
}

QString renderChatHtml(const QVector<ChatMsg> &msgs)
{
    QString html = QStringLiteral(
        "<html><body style=\"margin:0;padding:8px;background:#f1f5f9;\">");
    bool hasUserContent = false;
    for (int i = 0; i < msgs.size(); ++i) {
        const ChatMsg &m = msgs.at(i);
        html += QStringLiteral("<div style=\"margin:14px 0;\">");
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
        const QString card = emptyGuideCardImgHtml(
            QString::fromUtf8(u8"发消息，或把文件拖到这里"),
            QString::fromUtf8(u8"Enter 发送 · Shift+Enter 换行\n支持拖放文件与粘贴截图"));
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
        html += QStringLiteral("<div style=\"margin:14px 0;\">");
        html += renderFileCard(m);
        html += QStringLiteral("</div>");
        ++n;
    }
    if (n == 0) {
        const QString card = emptyGuideCardImgHtml(
            QString::fromUtf8(u8"还没有文件传输"),
            QString::fromUtf8(
                u8"把文件拖到聊天区，或点左下角附件发送。\n传完后会在这里列出记录。"));
        html += QStringLiteral(
                    "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"48\"><tr>"
                    "<td align=\"center\">%1</td></tr></table>")
                    .arg(card);
    }
    html += QStringLiteral("</body></html>");
    return html;
}
