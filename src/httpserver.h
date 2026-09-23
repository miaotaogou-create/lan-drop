#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QList>
#include <QString>

class QTcpServer;

// 对端客户端：/api/info、/api/inbox、/api/upload。浏览器兜底：/share/
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);

    bool listen(int port);
    void close();
    void setInfo(const QString &id, const QString &name, int port);
    void setDownloadDir(const QString &dir);
    void setShareDir(const QString &dir);
    QString shareDir() const { return m_shareDir; }
    // 中止正在落盘的接收（删未完成文件并发 fileReceiveFailed）
    void abortActiveReceives();

signals:
    void textArrived(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text);
    void fileReceiving(const QString &ip, const QString &name, const QString &path, qint64 expectBytes);
    void fileProgress(const QString &ip, const QString &path, qint64 received, qint64 expectBytes);
    void fileArrived(const QString &ip, const QString &name, const QString &path, qint64 size);
    void fileReceiveFailed(const QString &ip, const QString &path);

private slots:
    void onNew();
    void onReady();
    void onGone();

private:
    struct Conn;
    Conn *connOf(QObject *o);
    void takeBytes(Conn *c);
    void finish(Conn *c, int code, const QByteArray &json);
    void finishHtml(Conn *c, int code, const QByteArray &html);
    void finishFile(Conn *c, const QString &absPath);
    void fail(Conn *c, int code, const QString &msg);
    bool tryShare(Conn *c);

    QTcpServer *m_srv = 0;
    QList<Conn *> m_conns;
    QString m_id;
    QString m_name;
    int m_port = 8848;
    QString m_downloadDir;
    QString m_shareDir;
};

#endif
