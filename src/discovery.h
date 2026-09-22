#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

class QTimer;
class QUdpSocket;

struct Peer {
    QString id;
    QString name;
    QString ip;
    int port = 8848;
    QString osName;
    QString hostname;
    QString alias;
    bool manual = false;
    QDateTime lastSeen;

    bool online() const;
    QString label() const;
    QString key() const;
};

class Discovery : public QObject
{
    Q_OBJECT
public:
    explicit Discovery(QObject *parent = 0);

    void setIdentity(const QString &id, const QString &name, int httpPort);
    bool start(int discoverPort);
    void stop();

    QList<Peer> peers() const;
    Peer addManual(const QString &ip, int port, const QString &alias,
                   const QString &osName = QString());
    bool removeManual(const QString &ip, int port);
    void touch(const QString &ip, int port, const QString &id, const QString &name,
               const QString &osName, const QString &hostname = QString());
    bool find(const QString &ip, int port, Peer *out) const;

signals:
    void changed();

private slots:
    void announce();
    void onDatagram();
    void prune();

private:
    void upsert(const Peer &in);

    QString m_id;
    QString m_name;
    int m_httpPort = 8848;
    int m_port = 0;
    QUdpSocket *m_sock = 0;
    QTimer *m_announce = 0;
    QTimer *m_prune = 0;
    QList<Peer> m_peers;
    bool m_loggedSendFail = false;
};

QString deviceId();
QStringList localIpv4();
QString localLinkLabel();
QString localHostName();

#endif
