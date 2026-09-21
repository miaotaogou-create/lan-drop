#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"

#include <QHash>
#include <QMainWindow>
#include <QStringList>

class Discovery;
class HttpServer;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

private slots:
    void refreshPeers();
    void showChat();
    void sendText();
    void sendFile();
    void addPeer();
    void probePeer();
    void editSettings();
    void onText(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text);
    void onFile(const QString &ip, const QString &name, const QString &path, qint64 size);

private:
    void boot();
    void note(const QString &key, const QString &line);
    QString currentKey() const;
    bool currentPeer(QString *ip, int *port, QString *name) const;

    Settings m_settings;
    QString m_id;
    Discovery *m_disc = 0;
    HttpServer *m_http = 0;
    QNetworkAccessManager *m_nam = 0;
    QListWidget *m_list = 0;
    QTextEdit *m_chat = 0;
    QLineEdit *m_input = 0;
    QHash<QString, QStringList> m_log;
};

#endif
