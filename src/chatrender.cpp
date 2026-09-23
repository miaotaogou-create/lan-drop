#include "chatrender.h"

#include "fmtutil.h"

#include <QApplication>
#include <QBuffer>
#include <QColor>
#include <QCryptographicHash>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>

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
    QString body = textBubbleImgHtml(m.text, out);
    if (!m.text.isEmpty()) {
        const QString href = QStringLiteral("landrop://copy/")
            + QString::fromLatin1(m.text.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
        body += QString::fromUtf8(
                    u8"<br/><a href=\"%1\" style=\"text-decoration:none;\">"
                    u8"<font color=\"#64748b\" size=\"1\">复制</font></a>")
                    .arg(href);
    }
    return renderMsgRow(out, head, body, avatar);
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
    QString actions;
    if (!pathB64.isEmpty()) {
        const QString openHref = QStringLiteral("landrop://open/") + pathB64;
        const QString revealHref = QStringLiteral("landrop://reveal/") + pathB64;
        const QString copyPathHref = QStringLiteral("landrop://copypath/") + pathB64;
        if (out) {
            actions = QString::fromUtf8(
                          u8"<a href=\"%1\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#2563eb\" size=\"2\">打开文件</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%2\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">打开所在目录</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%3\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">复制路径</font></a>")
                          .arg(openHref, revealHref, copyPathHref);
        } else {
            actions = QString::fromUtf8(
                          u8"<a href=\"%1\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#2563eb\" size=\"2\">打开文件</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%2\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">打开所在目录</font></a>"
                          u8"&nbsp;&nbsp;"
                          u8"<a href=\"%3\" style=\"text-decoration:none;\">"
                          u8"<font color=\"#64748b\" size=\"2\">复制路径</font></a>"
                          u8"&nbsp;&nbsp;<font color=\"#94a3b8\" size=\"1\">局域网直传 · 已存入下载目录</font>")
                          .arg(openHref, revealHref, copyPathHref);
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
        ? (out ? QString::fromUtf8(u8"<font color=\"#2563eb\" size=\"2\">发送中 %1%</font>").arg(pct)
               : QString::fromUtf8(u8"<font color=\"#ea580c\" size=\"2\">接收中 %1%</font>").arg(pct))
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
            body += QString::fromUtf8(
                        u8"&nbsp;&nbsp;<a href=\"%1\" style=\"text-decoration:none;\">"
                        u8"<font color=\"#2563eb\" size=\"2\">重发剩余 %2</font></a>")
                        .arg(batchHref)
                        .arg(all.size());
        }
    }
    return QStringLiteral(
               "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"6\"><tr><td align=\"center\">"
               "<table cellspacing=\"0\" cellpadding=\"6\" bgcolor=\"%1\">"
               "<tr><td><font color=\"%2\" size=\"2\">%3</font></td></tr></table>"
               "</td></tr></table>")
        .arg(bg, fg, body);
}

QString renderChatHtml(const QVector<ChatMsg> &msgs)
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
