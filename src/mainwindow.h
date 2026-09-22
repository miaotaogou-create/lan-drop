#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"

#include <QHash>
#include <QMainWindow>
#include <QPoint>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QVector>

class Discovery;
class HttpServer;
class QCloseEvent;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTextBrowser;
class QTimer;
class QUrl;
class QWidget;

struct ChatMsg {
    enum Type { OutText = 0, InText, OutFile, InFile, System, Fail };
    int type = OutText;
    QString who;
    QString face; // 头像取首字；空则用 who（发出侧 who=「我」时填本机设备名）
    QString text;
    QString path;
    qint64 size = 0;
    qint64 rttMs = -1;
    QString sha256;
    QString time;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void refreshPeers();
    void showChat();
    void sendText();
    void sendFile();
    void sendFolder();
    void nudgePeer();
    void addPeer();
    void probePeer();
    void editSettings();
    void filterPeers(const QString &text);
    void onText(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text);
    void onFile(const QString &ip, const QString &name, const QString &path, qint64 size);
    void minimizeWin();
    void toggleMax();
    void closeWin();
    void openShare();
    void openDownloadDir();
    void showChatTab();
    void showFilesTab();
    void onChatAnchor(const QUrl &url);
    void measurePing();
    void peerListContextMenu(const QPoint &pos);
    void removeSelectedManualPeer();
    void showFromTray();
    void showFromTrayNotify();
    void quitApp();
    void hideTrayToast();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void boot();
    void buildUi();
    void applyStyle();
    void setupTray();
    void setupChatDrop();
    void enqueueDroppedPaths(const QStringList &paths);
    bool tryPasteClipboardFiles();
    void maybeTrayNotify(const QString &title, const QString &body, const QString &peerKey = QString());
    void showTrayToast(const QString &title, const QString &body);
    void selectPeerByKey(const QString &key);
    void clearUnread(const QString &key);
    void appendMsg(const QString &key, const ChatMsg &msg);
    void refreshChatHtml();
    void refreshFilesView();
    void updateChrome();
    void updateEmpty();
    void updateHostPill();
    void updatePeerSession();
    void setSessionTab(int index);
    void setStatusOnline(const QString &text, bool ok);
    void setProgress(const QString &text);
    void noteFail(const QString &key, QNetworkReply *rep);
    void refreshShareBtn();
    void updateInputPlaceholder();
    void shakeWindow();
    void playNotifySound();
    void persistManualPeers();
    void startUpload(const QString &path, bool fromQueue);
    void pumpUploadQueue();
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
    QWidget *m_peerHeader = 0;
    QLabel *m_peerAvatar = 0;
    QLabel *m_peerName = 0;
    QLabel *m_peerOnlineDot = 0;
    QLabel *m_peerAddr = 0;
    QLabel *m_peerMeta = 0;
    QPushButton *m_tabChat = 0;
    QPushButton *m_tabFiles = 0;
    QWidget *m_connBannerHost = 0;
    QWidget *m_connBanner = 0;
    QLabel *m_connBannerText = 0;
    QStackedWidget *m_sessionStack = 0;
    QTextBrowser *m_chat = 0;
    QTextBrowser *m_files = 0;
    QLabel *m_fileLive = 0;
    QPlainTextEdit *m_input = 0;
    QPushButton *m_sendBtn = 0;
    QLabel *m_progress = 0;
    QPushButton *m_shareBtn = 0;
    QPushButton *m_maxBtn = 0;
    QWidget *m_composer = 0;
    QWidget *m_inputShell = 0;

    QHash<QString, QVector<ChatMsg> > m_log;
    QHash<QString, int> m_unread; // 对端 ip:port → 未读条数
    QStringList m_uploadQueue;
    bool m_uploading = false;
    bool m_pingBusy = false;
    QString m_pingKey;
    QString m_pingText;
    QPoint m_dragOrigin;
    bool m_dragging = false;
    QSystemTrayIcon *m_tray = 0;
    bool m_forceQuit = false;
    bool m_trayHintShown = false;
    QFrame *m_trayToast = 0;
    QLabel *m_trayToastTitle = 0;
    QLabel *m_trayToastBody = 0;
    QTimer *m_trayToastTimer = 0;
    QString m_trayNotifyKey; // 最近一条收件提示对应的对端 ip:port；空=勿跳转
};

#endif
