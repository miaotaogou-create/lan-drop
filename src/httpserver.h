#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QString>

class QTcpServer;

// 对端只需要这三个接口：/api/info、/api/inbox、/api/upload。
class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);

    bool listen(int port);
    void close();
    void setInfo(const QString &id, const QString &name, int port);
    void setDownloadDir(const QString &dir);

signals:
    void textArrived(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text);
    void fileArrived(const QString &ip, const QString &name, const QString &path, qint64 size);

private slots:
    void onNew();
    void onReady();
    void onGone();

private:
    struct Conn;
    Conn *connOf(QObject *o);
    void takeBytes(Conn *c);
    void finish(Conn *c, int code, const QByteArray &json);
    void fail(Conn *c, int code, const QString &msg);

    QTcpServer *m_srv = 0;
    QString m_id;
    QString m_name;
    int m_port = 8848;
    QString m_downloadDir;
};

#endif
