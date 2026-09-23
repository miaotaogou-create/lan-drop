#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chatmsg.h"
#include "settings.h"

#include <QHash>
#include <QMainWindow>
#include <QPoint>
#include <QPointer>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QVector>

class Discovery;
class HttpServer;
class QAction;
class QCloseEvent;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QPlainTextEdit;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTextBrowser;
class QTimer;
class QUrl;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

    static QString chatHistoryFilePath();
    static bool saveChatHistoryToFile(const QString &path, const QHash<QString, QVector<ChatMsg> > &log);
    static QHash<QString, QVector<ChatMsg> > loadChatHistoryFromFile(const QString &path);

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
    void onFileReceiving(const QString &ip, const QString &name, const QString &path, qint64 expectBytes);
    void onFileProgress(const QString &ip, const QString &path, qint64 received, qint64 expectBytes);
    void onFile(const QString &ip, const QString &name, const QString &path, qint64 size);
    void onFileReceiveFailed(const QString &ip, const QString &path);
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
    void clearSelectedPeerChat();
    void editSelectedManualPeer();
    void showFromTray();
    void showFromTrayNotify();
    void quitApp();
    void hideTrayToast();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void jumpChatToBottom();
    void copyLocalAddr();
    void copyPeerAddr();
    void flushChatHistory();
    void cancelUpload();
    void clearUploadQueue();
    void toggleTraySound(bool on);

private:
    void boot();
    void buildUi();
    void applyStyle();
    void setupTray();
    void setupChatDrop();
    void setupPeerListDrop();
    void hideToTray();
    void setChatDropHint(bool on);
    void enqueueDroppedPaths(const QStringList &paths, bool fromFolder = false);
    bool tryPasteClipboardFiles();
    bool tryPasteClipboardImage();
    void revealInFolder(const QString &path);
    void maybeTrayNotify(const QString &title, const QString &body, const QString &peerKey = QString());
    void showTrayToast(const QString &title, const QString &body);
    void selectPeerByKey(const QString &key);
    void clearUnread(const QString &key);
    void appendMsg(const QString &key, const ChatMsg &msg);
    void refreshChatHtml(bool forceBottom = false);
    void refreshFilesView();
    bool isChatNearBottom() const;
    void markChatNewBelowIfAway();
    void syncJumpBottomBtn();
    void placeJumpBottomBtn();
    void updateChrome();
    void updateEmpty();
    void updateHostPill();
    void updatePeerSession();
    void setSessionTab(int index);
    void setStatusOnline(const QString &text, bool ok);
    void setProgress(const QString &text);
    void setUploadProgressText(const QString &filename, int pct = -1);
    void setRecvProgressText(const QString &filename, int pct = -1);
    void applyAlwaysOnTop();
    void noteBusyUpload(const QString &hint = QString());
    void maybeWarnOfflinePeer();
    bool currentPeerOnline() const;
    void noteFail(const QString &key, QNetworkReply *rep, const QString &retryPath = QString(),
                  const QStringList &morePaths = QStringList());
    void refreshShareBtn();
    void updateInputPlaceholder();
    void shakeWindow();
    void playNotifySound();
    void persistManualPeers();
    void persistWindowGeometry();
    void applyWindowGeometry();
    void applySideWidth();
    void loadChatHistory();
    void scheduleSaveChatHistory();
    void startUpload(const QString &path, bool fromQueue);
    void pumpUploadQueue();
    void enqueueMoreUploads(const QStringList &paths, bool announceFolder = false);
    void syncCancelUploadBtn();
    void updateUploadProgress(const QString &key, int msgIndex, int pct);
    void finishUploadMsg(const QString &key, int msgIndex, qint64 rttMs, const QString &sha);
    void dropUploadMsg(const QString &key, int msgIndex);
    QString peerSessionKey(const QString &ip) const;
    int findPendingInFile(const QString &key, const QString &path) const;
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
    QSplitter *m_bodySplit = 0;
    QWidget *m_side = 0;
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
    QWidget *m_chatHost = 0;
    QPushButton *m_jumpBottomBtn = 0;
    bool m_chatNewBelow = false;
    QTextBrowser *m_files = 0;
    QLabel *m_fileLive = 0;
    QWidget *m_fileLiveHost = 0;
    QPlainTextEdit *m_input = 0;
    QPushButton *m_sendBtn = 0;
    QLabel *m_progress = 0;
    QPushButton *m_cancelUploadBtn = 0;
    QPushButton *m_cancelUploadBtnFiles = 0;
    QPushButton *m_clearQueueBtn = 0;
    QPushButton *m_clearQueueBtnFiles = 0;
    QPushButton *m_shareBtn = 0;
    QPushButton *m_maxBtn = 0;
    QWidget *m_composer = 0;
    QWidget *m_inputShell = 0;
    QWidget *m_chatPage = 0;
    QFrame *m_chatDropHint = 0;
    QLabel *m_chatDropHintLabel = 0;

    QHash<QString, QVector<ChatMsg> > m_log;
    QHash<QString, int> m_unread; // 对端 ip:port → 未读条数
    QStringList m_uploadQueue;
    bool m_uploading = false;
    bool m_uploadCanceling = false;
    QPointer<QNetworkReply> m_activeUploadReply;
    QString m_uploadCurrentName;
    int m_uploadLastPct = -1;
    qint64 m_uploadLastUiMs = 0;
    qint64 m_uploadBytesMark = 0;
    qint64 m_uploadMsMark = 0;
    double m_uploadSpeedBps = 0;
    qint64 m_uploadRemainBytes = -1;
    QString m_recvCurrentName;
    qint64 m_recvBytesMark = 0;
    qint64 m_recvMsMark = 0;
    double m_recvSpeedBps = 0;
    qint64 m_recvRemainBytes = -1;
    bool m_pingBusy = false;
    QString m_pingKey;
    QString m_pingText;
    QPoint m_dragOrigin;
    bool m_dragging = false;
    QSystemTrayIcon *m_tray = 0;
    QAction *m_traySoundAct = 0;
    bool m_forceQuit = false;
    bool m_trayHintShown = false;
    QFrame *m_trayToast = 0;
    QLabel *m_trayToastTitle = 0;
    QLabel *m_trayToastBody = 0;
    QTimer *m_trayToastTimer = 0;
    QTimer *m_chatSaveTimer = 0;
    QString m_trayNotifyKey; // 最近一条收件提示对应的对端 ip:port；空=勿跳转
    QString m_offlineWarnedKey; // 本轮离线提示已发过的对端 key
};

#endif
