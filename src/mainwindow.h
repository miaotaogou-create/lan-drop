#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"

#include <QHash>
#include <QMainWindow>
#include <QPoint>
#include <QStringList>

class Discovery;
class HttpServer;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QStackedWidget;
class QTextEdit;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void refreshPeers();
    void showChat();
    void sendText();
    void sendFile();
    void addPeer();
    void probePeer();
    void editSettings();
    void filterPeers(const QString &text);
    void onText(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text);
    void onFile(const QString &ip, const QString &name, const QString &path, qint64 size);
    void minimizeWin();
    void toggleMax();
    void closeWin();
    void webShareSoon();

private:
    void boot();
    void buildUi();
    void applyStyle();
    void note(const QString &key, const QString &line);
    void updateChrome();
    void updateEmpty();
    void updateHostPill();
    void setStatusOnline(const QString &text, bool ok);
    void setProgress(const QString &text);
    void noteFail(const QString &key, QNetworkReply *rep);
    QString currentKey() const;
    bool currentPeer(QString *ip, int *port, QString *name) const;
    QString localIpText() const;

    Settings m_settings;
    QString m_id;
    Discovery *m_disc = 0;
    HttpServer *m_http = 0;
    QNetworkAccessManager *m_nam = 0;

    QWidget *m_titleBar = 0;
    QWidget *m_hostPill = 0;
    QLabel *m_hostName = 0;
    QLabel *m_hostIp = 0;
    QLabel *m_statusDot = 0;
    QLabel *m_statusLabel = 0;
    QLabel *m_peerCount = 0;
    QLabel *m_emptyHint = 0;
    QLineEdit *m_search = 0;
    QListWidget *m_list = 0;
    QStackedWidget *m_pages = 0;
    QTextEdit *m_chat = 0;
    QLineEdit *m_input = 0;
    QLabel *m_progress = 0;
    QPushButton *m_maxBtn = 0;
    QWidget *m_composer = 0;

    QHash<QString, QStringList> m_log;
    QPoint m_dragOrigin;
    bool m_dragging = false;
};

#endif
