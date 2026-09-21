#include "httpserver.h"

#include "files.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

struct HttpServer::Conn {
    QTcpSocket *sock = 0;
    QByteArray buf;
    bool headDone = false;
    QString method;
    QString path;
    qint64 contentLength = 0;
    qint64 seen = 0;
    bool multipart = false;
    QByteArray boundary;
    int phase = 0; // 0 找第一段边界，1 段头，2 段正文
    QByteArray hold;
    QFile *file = 0;
    bool isFile = false;
    QString fileName;
    qint64 fileSize = 0;
    QByteArray jsonBody;
    bool replied = false;
    QString peerIp;
};

static QString peerIpOf(QTcpSocket *sock)
{
    QHostAddress a = sock->peerAddress();
    if (a.protocol() == QAbstractSocket::IPv6Protocol && a.toIPv4Address())
        a = QHostAddress(a.toIPv4Address());
    if (a.protocol() == QAbstractSocket::IPv4Protocol)
        return QHostAddress(a.toIPv4Address()).toString();
    return a.toString();
}

static QString headerValue(const QByteArray &headers, const QByteArray &key)
{
    const QList<QByteArray> lines = headers.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines.at(i).trimmed();
        if (line.toLower().startsWith(key.toLower())) {
            const int colon = line.indexOf(':');
            if (colon > 0)
                return QString::fromUtf8(line.mid(colon + 1).trimmed());
        }
    }
    return QString();
}

static QString fileNameFromDisposition(const QString &disp)
{
    const int star = disp.indexOf(QLatin1String("filename*="), 0, Qt::CaseInsensitive);
    if (star >= 0) {
        QString v = disp.mid(star + 10).trimmed();
        const int semi = v.indexOf(QLatin1Char(';'));
        if (semi >= 0)
            v = v.left(semi).trimmed();
        const int tick = v.indexOf(QLatin1String("''"));
        if (tick >= 0)
            v = v.mid(tick + 2);
        v = QUrl::fromPercentEncoding(v.toUtf8());
        return safeFileName(v);
    }
    int at = disp.indexOf(QLatin1String("filename="), 0, Qt::CaseInsensitive);
    if (at < 0)
        return QString();
    QString v = disp.mid(at + 9).trimmed();
    const int semi = v.indexOf(QLatin1Char(';'));
    if (semi >= 0)
        v = v.left(semi).trimmed();
    if (v.startsWith(QLatin1Char('"')) && v.endsWith(QLatin1Char('"')) && v.size() >= 2)
        v = v.mid(1, v.size() - 2);
    return safeFileName(v);
}

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
    , m_srv(new QTcpServer(this))
{
    connect(m_srv, SIGNAL(newConnection()), this, SLOT(onNew()));
}

bool HttpServer::listen(int port)
{
    close();
    m_port = port;
    return m_srv->listen(QHostAddress::Any, quint16(port));
}

void HttpServer::close()
{
    m_srv->close();
}

void HttpServer::setInfo(const QString &id, const QString &name, int port)
{
    m_id = id;
    m_name = name;
    m_port = port;
}

void HttpServer::setDownloadDir(const QString &dir)
{
    m_downloadDir = dir;
}

void HttpServer::setShareDir(const QString &dir)
{
    m_shareDir = dir.trimmed();
}

HttpServer::Conn *HttpServer::connOf(QObject *o)
{
    QTcpSocket *sock = qobject_cast<QTcpSocket *>(o);
    if (!sock)
        return 0;
    const quintptr p = sock->property("conn").value<quintptr>();
    return reinterpret_cast<Conn *>(p);
}

void HttpServer::onNew()
{
    while (m_srv->hasPendingConnections()) {
        QTcpSocket *sock = m_srv->nextPendingConnection();
        Conn *c = new Conn;
        c->sock = sock;
        c->peerIp = peerIpOf(sock);
        sock->setProperty("conn", QVariant::fromValue<quintptr>(reinterpret_cast<quintptr>(c)));
        connect(sock, SIGNAL(readyRead()), this, SLOT(onReady()));
        connect(sock, SIGNAL(disconnected()), this, SLOT(onGone()));
        if (sock->bytesAvailable())
            c->buf.append(sock->readAll());
        QTimer::singleShot(0, this, [this, sock]() {
            Conn *pending = connOf(sock);
            if (pending)
                takeBytes(pending);
        });
    }
}

void HttpServer::onGone()
{
    Conn *c = connOf(sender());
    if (!c)
        return;
    if (c->file) {
        const QString path = c->file->fileName();
        c->file->close();
        if (!c->replied)
            QFile::remove(path);
        delete c->file;
        c->file = 0;
    }
    c->sock->setProperty("conn", QVariant());
    c->sock->deleteLater();
    delete c;
}

void HttpServer::finish(Conn *c, int code, const QByteArray &json)
{
    if (!c || c->replied || !c->sock)
        return;
    c->replied = true;
    QByteArray reason = "OK";
    if (code == 400)
        reason = "Bad Request";
    else if (code == 404)
        reason = "Not Found";
    else if (code == 500)
        reason = "Error";
    QByteArray msg = "HTTP/1.1 " + QByteArray::number(code) + " " + reason + "\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: " + QByteArray::number(json.size()) + "\r\n"
        "Connection: close\r\n\r\n";
    msg += json;
    c->sock->write(msg);
    c->sock->disconnectFromHost();
}

void HttpServer::finishHtml(Conn *c, int code, const QByteArray &html)
{
    if (!c || c->replied || !c->sock)
        return;
    c->replied = true;
    QByteArray reason = code == 200 ? "OK" : (code == 404 ? "Not Found" : "Error");
    QByteArray msg = "HTTP/1.1 " + QByteArray::number(code) + " " + reason + "\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: " + QByteArray::number(html.size()) + "\r\n"
        "Connection: close\r\n\r\n";
    msg += html;
    c->sock->write(msg);
    c->sock->disconnectFromHost();
}

void HttpServer::finishFile(Conn *c, const QString &absPath)
{
    if (!c || c->replied || !c->sock)
        return;
    QFile f(absPath);
    if (!f.open(QIODevice::ReadOnly)) {
        fail(c, 500, QString::fromUtf8(u8"打不开文件"));
        return;
    }
    const QString name = QFileInfo(absPath).fileName();
    const QByteArray utfName = QUrl::toPercentEncoding(name);
    const QByteArray asciiName = name.toLatin1();
    QByteArray disp = "attachment; filename=\"";
    disp += asciiName.isEmpty() ? QByteArray("file") : asciiName;
    disp += "\"; filename*=UTF-8''";
    disp += utfName;
    c->replied = true;
    QByteArray head = "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Disposition: " + disp + "\r\n"
        "Content-Length: " + QByteArray::number(f.size()) + "\r\n"
        "Connection: close\r\n\r\n";
    c->sock->write(head);
    // 流式读盘，不把整文件塞进内存
    while (!f.atEnd()) {
        const QByteArray chunk = f.read(64 * 1024);
        if (chunk.isEmpty())
            break;
        if (c->sock->write(chunk) != chunk.size())
            break;
        c->sock->flush();
    }
    c->sock->disconnectFromHost();
}

bool HttpServer::tryShare(Conn *c)
{
    if (c->method != QLatin1String("GET"))
        return false;
    if (c->path != QLatin1String("/share") && c->path != QLatin1String("/share/")
        && !c->path.startsWith(QLatin1String("/share/")))
        return false;

    if (m_shareDir.isEmpty() || !QDir(m_shareDir).exists()) {
        finishHtml(c, 404, QString::fromUtf8(
            u8"<!doctype html><meta charset=utf-8><title>局域快传</title><p>网页共享未开启。</p>").toUtf8());
        return true;
    }

    if (c->path == QLatin1String("/share") || c->path == QLatin1String("/share/")) {
        const QFileInfoList files = QDir(m_shareDir).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        QString html;
        html += QString::fromUtf8(u8"<!doctype html><meta charset=utf-8><title>");
        html += m_name.toHtmlEscaped();
        html += QString::fromUtf8(u8" - 网页共享</title>");
        html += QString::fromUtf8(u8"<style>body{font-family:sans-serif;max-width:640px;margin:40px auto;padding:0 16px;color:#0f172a}"
                                 "a{color:#2563eb;text-decoration:none}li{margin:8px 0;color:#475569}</style>");
        html += QString::fromUtf8(u8"<h1>");
        html += m_name.toHtmlEscaped();
        html += QString::fromUtf8(u8"</h1><p>共享目录文件（共 ");
        html += QString::number(files.size());
        html += QString::fromUtf8(u8" 个）</p><ul>");
        for (int i = 0; i < files.size(); ++i) {
            const QString fname = files.at(i).fileName();
            const QByteArray enc = QUrl::toPercentEncoding(fname);
            const QString href = QStringLiteral("/share/") + QString::fromUtf8(enc);
            html += QString::fromUtf8(u8"<li><a href=\"");
            html += href.toHtmlEscaped();
            html += QString::fromUtf8(u8"\">");
            html += fname.toHtmlEscaped();
            html += QString::fromUtf8(u8"</a> <span>（");
            html += QString::number(files.at(i).size());
            html += QString::fromUtf8(u8" 字节）</span></li>");
        }
        if (files.isEmpty())
            html += QString::fromUtf8(u8"<li>目录为空</li>");
        html += QString::fromUtf8(u8"</ul>");
        finishHtml(c, 200, html.toUtf8());
        return true;
    }

    const QString raw = c->path.mid(QStringLiteral("/share/").size());
    const QString name = QString::fromUtf8(QByteArray::fromPercentEncoding(raw.toUtf8()));
    const QString abs = resolveSharedFile(m_shareDir, name);
    if (abs.isEmpty()) {
        finishHtml(c, 404, QString::fromUtf8(
            u8"<!doctype html><meta charset=utf-8><title>局域快传</title><p>没有这个文件。</p>").toUtf8());
        return true;
    }
    finishFile(c, abs);
    return true;
}

void HttpServer::fail(Conn *c, int code, const QString &msg)
{
    QJsonObject o;
    o.insert(QStringLiteral("error"), msg);
    finish(c, code, QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void HttpServer::onReady()
{
    Conn *c = connOf(sender());
    if (!c || c->replied)
        return;
    c->buf.append(c->sock->readAll());
    takeBytes(c);
}

void HttpServer::takeBytes(Conn *c)
{
    if (!c || c->replied)
        return;
    if (!c->headDone) {
        const int cut = c->buf.indexOf("\r\n\r\n");
        if (cut < 0) {
            if (c->buf.size() > 65536)
                fail(c, 400, QString::fromUtf8(u8"请求头过大"));
            return;
        }
        const QByteArray head = c->buf.left(cut);
        c->buf.remove(0, cut + 4);
        const int lineEnd = head.indexOf("\r\n");
        const QByteArray req = lineEnd >= 0 ? head.left(lineEnd) : head;
        const QList<QByteArray> parts = req.split(' ');
        if (parts.size() < 2) {
            fail(c, 400, QString::fromUtf8(u8"请求行无效"));
            return;
        }
        c->method = QString::fromLatin1(parts.at(0));
        c->path = QString::fromLatin1(parts.at(1));
        const int q = c->path.indexOf(QLatin1Char('?'));
        if (q >= 0)
            c->path = c->path.left(q);
        const QString len = headerValue(head, "content-length");
        c->contentLength = len.isEmpty() ? 0 : len.toLongLong();
        if (c->contentLength < 0 || c->contentLength > (qint64(8) << 30)) {
            fail(c, 400, QString::fromUtf8(u8"长度无效"));
            return;
        }
        const QString ctype = headerValue(head, "content-type");
        if (ctype.contains(QLatin1String("multipart/form-data"), Qt::CaseInsensitive)) {
            const int b = ctype.indexOf(QLatin1String("boundary="), 0, Qt::CaseInsensitive);
            if (b < 0) {
                fail(c, 400, QString::fromUtf8(u8"缺少 boundary"));
                return;
            }
            QString bound = ctype.mid(b + 9).trimmed();
            const int semi = bound.indexOf(QLatin1Char(';'));
            if (semi >= 0)
                bound = bound.left(semi).trimmed();
            if (bound.startsWith(QLatin1Char('"')) && bound.endsWith(QLatin1Char('"')) && bound.size() >= 2)
                bound = bound.mid(1, bound.size() - 2);
            c->boundary = bound.toUtf8();
            c->multipart = true;
            c->phase = 0;
        }
        c->headDone = true;
    }

    if (!c->headDone || c->replied)
        return;

    if (c->method == QLatin1String("GET") && c->path == QLatin1String("/api/info")) {
        QJsonObject o;
        o.insert(QStringLiteral("id"), m_id);
        o.insert(QStringLiteral("name"), m_name);
        o.insert(QStringLiteral("port"), m_port);
#ifdef Q_OS_WIN
        o.insert(QStringLiteral("os"), QStringLiteral("windows"));
#else
        o.insert(QStringLiteral("os"), QStringLiteral("linux"));
#endif
        QJsonArray ips;
        // localIpv4 在 discovery.cpp，这里直接再扫会重复。调用方通过名字已经够用，IP 列表给探测看。
        extern QStringList localIpv4();
        const QStringList list = localIpv4();
        for (int i = 0; i < list.size(); ++i)
            ips.append(list.at(i));
        o.insert(QStringLiteral("ips"), ips);
        finish(c, 200, QJsonDocument(o).toJson(QJsonDocument::Compact));
        return;
    }

    if (tryShare(c))
        return;

    if (!c->multipart) {
        if (c->jsonBody.size() + c->buf.size() > c->contentLength && c->contentLength >= 0)
            c->jsonBody.append(c->buf.left(int(c->contentLength - c->jsonBody.size())));
        else
            c->jsonBody.append(c->buf);
        c->buf.clear();
        if (c->jsonBody.size() < c->contentLength)
            return;
        if (c->method == QLatin1String("POST") && c->path == QLatin1String("/api/inbox")) {
            const QJsonObject o = QJsonDocument::fromJson(c->jsonBody).object();
            const QString text = o.value(QStringLiteral("text")).toString();
            if (text.trimmed().isEmpty()) {
                fail(c, 400, QString::fromUtf8(u8"文本不能为空"));
                return;
            }
            QString fromId = o.value(QStringLiteral("fromId")).toString();
            if (fromId.isEmpty())
                fromId = c->peerIp;
            emit textArrived(c->peerIp, fromId, o.value(QStringLiteral("fromName")).toString(),
                             o.value(QStringLiteral("fromPort")).toInt(), text);
            QJsonObject ok;
            ok.insert(QStringLiteral("ok"), true);
            finish(c, 200, QJsonDocument(ok).toJson(QJsonDocument::Compact));
            return;
        }
        fail(c, 404, QString::fromUtf8(u8"没有这个接口"));
        return;
    }

    if (c->method != QLatin1String("POST") || c->path != QLatin1String("/api/upload")) {
        fail(c, 404, QString::fromUtf8(u8"没有这个接口"));
        return;
    }

    // 流式拆 multipart，文件段直接落盘，不把整文件放进内存。
    const QByteArray mark = QByteArray("\r\n--") + c->boundary;
    const QByteArray first = QByteArray("--") + c->boundary;
    while (!c->replied) {
        if (c->phase == 0) {
            int at = c->buf.indexOf(first);
            if (at < 0) {
                if (c->buf.size() > first.size() + 4096)
                    c->buf = c->buf.right(first.size() + 8);
                if (c->seen + c->buf.size() > c->contentLength)
                    fail(c, 400, QString::fromUtf8(u8"找不到文件段"));
                return;
            }
            c->seen += at;
            c->buf.remove(0, at);
            const int nl = c->buf.indexOf('\n');
            if (nl < 0)
                return;
            c->seen += nl + 1;
            c->buf.remove(0, nl + 1);
            c->phase = 1;
            c->hold.clear();
            continue;
        }
        if (c->phase == 1) {
            c->hold.append(c->buf);
            c->buf.clear();
            const int cut = c->hold.indexOf("\r\n\r\n");
            if (cut < 0) {
                if (c->hold.size() > 65536)
                    fail(c, 400, QString::fromUtf8(u8"段头过大"));
                return;
            }
            const QByteArray ph = c->hold.left(cut);
            c->buf = c->hold.mid(cut + 4);
            c->hold.clear();
            c->seen += cut + 4;
            const QString disp = headerValue(ph, "content-disposition");
            c->fileName = fileNameFromDisposition(disp);
            c->isFile = disp.contains(QLatin1String("name=\"file\"")) || disp.contains(QLatin1String("name=file"));
            c->phase = 2;
            if (c->isFile) {
                if (c->fileName.isEmpty())
                    c->fileName = QString::fromUtf8(u8"未命名文件");
                c->file = new QFile;
                if (createUniqueFile(m_downloadDir, c->fileName, c->file).isEmpty()) {
                    delete c->file;
                    c->file = 0;
                    fail(c, 500, QString::fromUtf8(u8"保存文件失败"));
                    return;
                }
            }
            continue;
        }
        // phase 2
        int at = c->buf.indexOf(mark);
        if (at < 0) {
            const int keep = mark.size() + 4;
            if (c->buf.size() > keep) {
                const QByteArray chunk = c->buf.left(c->buf.size() - keep);
                c->buf.remove(0, chunk.size());
                c->seen += chunk.size();
                if (c->file) {
                    if (c->file->write(chunk) != chunk.size()) {
                        fail(c, 500, QString::fromUtf8(u8"写入失败"));
                        return;
                    }
                    c->fileSize += chunk.size();
                }
            }
            if (c->seen + c->buf.size() >= c->contentLength)
                fail(c, 400, QString::fromUtf8(u8"缺少文件"));
            return;
        }
        const QByteArray chunk = c->buf.left(at);
        if (c->file) {
            if (c->file->write(chunk) != chunk.size()) {
                fail(c, 500, QString::fromUtf8(u8"写入失败"));
                return;
            }
            c->file->flush();
            c->fileSize += chunk.size();
            const QString path = c->file->fileName();
            const QString base = QFileInfo(path).fileName();
            c->file->close();
            delete c->file;
            c->file = 0;
            emit fileArrived(c->peerIp, base, path, c->fileSize);
            QJsonObject ok;
            ok.insert(QStringLiteral("ok"), true);
            ok.insert(QStringLiteral("name"), base);
            ok.insert(QStringLiteral("size"), double(c->fileSize));
            ok.insert(QStringLiteral("path"), path);
            finish(c, 200, QJsonDocument(ok).toJson(QJsonDocument::Compact));
            return;
        }
        c->seen += at;
        c->buf.remove(0, at);
        // 不是文件段，跳过这段再找下一段
        const int nl = c->buf.indexOf('\n', mark.size());
        if (nl < 0)
            return;
        const bool last = c->buf.mid(mark.size(), 2) == "--";
        c->seen += nl + 1;
        c->buf.remove(0, nl + 1);
        if (last) {
            fail(c, 400, QString::fromUtf8(u8"缺少文件"));
            return;
        }
        c->phase = 1;
        c->hold.clear();
    }
}
