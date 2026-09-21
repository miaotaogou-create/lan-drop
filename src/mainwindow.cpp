#include "mainwindow.h"

#include "discovery.h"
#include "httpserver.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QtMath>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

enum ChromeIcon {
    IconSettings = 0,
    IconMinimize,
    IconMaximize,
    IconRestore,
    IconClose
};

static QIcon makeChromeIcon(ChromeIcon kind, const QColor &color)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int logical = 16;
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF r(2.5, 2.5, 11.0, 11.0);
    switch (kind) {
    case IconMinimize:
        p.drawLine(QPointF(3.5, 8.0), QPointF(12.5, 8.0));
        break;
    case IconMaximize:
        p.drawRect(QRectF(3.5, 3.5, 9.0, 9.0));
        break;
    case IconRestore:
        p.drawRect(QRectF(5.0, 3.0, 7.5, 7.5));
        p.fillRect(QRectF(3.0, 5.5, 7.5, 7.5), Qt::white);
        p.drawRect(QRectF(3.0, 5.5, 7.5, 7.5));
        break;
    case IconClose:
        p.drawLine(QPointF(4.0, 4.0), QPointF(12.0, 12.0));
        p.drawLine(QPointF(12.0, 4.0), QPointF(4.0, 12.0));
        break;
    case IconSettings: {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(8.0, 8.0), 2.2, 2.2);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(8.0, 8.0), 4.6, 4.6);
        for (int i = 0; i < 6; ++i) {
            const qreal a = i * 3.14159265 / 3.0;
            const qreal c = qCos(a);
            const qreal s = qSin(a);
            p.drawLine(QPointF(8.0 + c * 5.2, 8.0 + s * 5.2),
                       QPointF(8.0 + c * 7.0, 8.0 + s * 7.0));
        }
        break;
    }
    }
    return QIcon(pm);
}

static QPixmap makeRadioLogo(int logical = 36)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#2563eb")));
    p.drawRoundedRect(QRectF(0, 0, logical, logical), 10, 10);

    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(logical / 2.0, logical / 2.0), 2.2, 2.2);

    QPen pen(Qt::white, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF c(logical / 2.0, logical / 2.0);
    for (int i = 0; i < 2; ++i) {
        const qreal r = 6.0 + i * 4.5;
        // 左右各两道弧，像广播符号
        p.drawArc(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), 40 * 16, 100 * 16);
        p.drawArc(QRectF(c.x() - r, c.y() - r, r * 2, r * 2), 220 * 16, 100 * 16);
    }
    return pm;
}

static QPixmap makeLaptopIcon(int logical = 16)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(QStringLiteral("#2563eb")), 1.5);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(2.5, 3.0, 11.0, 7.5), 1.2, 1.2);
    p.drawLine(QPointF(1.5, 12.5), QPointF(14.5, 12.5));
    p.drawLine(QPointF(4.0, 12.5), QPointF(5.0, 14.0));
    p.drawLine(QPointF(12.0, 12.5), QPointF(11.0, 14.0));
    p.drawLine(QPointF(5.0, 14.0), QPointF(11.0, 14.0));
    return pm;
}

static QPushButton *chromeBtn(ChromeIcon kind, const QString &objectName, const QString &tip)
{
    QPushButton *b = new QPushButton;
    b->setObjectName(objectName);
    b->setFixedSize(40, 32);
    b->setFocusPolicy(Qt::NoFocus);
    b->setFlat(true);
    b->setCursor(Qt::ArrowCursor);
    b->setToolTip(tip);
    b->setIcon(makeChromeIcon(kind, QColor(QStringLiteral("#475569"))));
    b->setIconSize(QSize(16, 16));
    return b;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromUtf8(u8"局域快传"));
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    resize(1180, 720);
    setMinimumSize(900, 560);
    statusBar()->hide();

    m_disc = new Discovery(this);
    m_http = new HttpServer(this);
    m_nam = new QNetworkAccessManager(this);
    connect(m_disc, SIGNAL(changed()), this, SLOT(refreshPeers()));
    connect(m_http, SIGNAL(textArrived(QString,QString,QString,int,QString)),
            this, SLOT(onText(QString,QString,QString,int,QString)));
    connect(m_http, SIGNAL(fileArrived(QString,QString,QString,qint64)),
            this, SLOT(onFile(QString,QString,QString,qint64)));

    buildUi();
    applyStyle();

    QTimer *tick = new QTimer(this);
    connect(tick, SIGNAL(timeout()), this, SLOT(refreshPeers()));
    tick->start(1000);
    boot();
}

void MainWindow::buildUi()
{
    QWidget *root = new QWidget;
    root->setObjectName(QStringLiteral("root"));
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    // —— 自绘顶栏：品牌 + 本机信息 + 操作 + 窗口按钮 ——
    m_titleBar = new QWidget;
    m_titleBar->setObjectName(QStringLiteral("titleBar"));
    m_titleBar->setFixedHeight(64);
    m_titleBar->installEventFilter(this);
    QHBoxLayout *titleLay = new QHBoxLayout(m_titleBar);
    titleLay->setContentsMargins(16, 0, 8, 0);
    titleLay->setSpacing(12);

    QLabel *logo = new QLabel;
    logo->setObjectName(QStringLiteral("logo"));
    logo->setFixedSize(36, 36);
    logo->setPixmap(makeRadioLogo(36));

    QVBoxLayout *brandCol = new QVBoxLayout;
    brandCol->setSpacing(2);
    brandCol->setContentsMargins(0, 0, 0, 0);
    QLabel *brand = new QLabel(QString::fromUtf8(u8"局域快传"));
    brand->setObjectName(QStringLiteral("brand"));
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("statusOnline"));
    m_statusLabel->setTextFormat(Qt::RichText);
    brandCol->addWidget(brand);
    brandCol->addWidget(m_statusLabel);

    QHBoxLayout *brandRow = new QHBoxLayout;
    brandRow->setSpacing(10);
    brandRow->setContentsMargins(0, 0, 0, 0);
    brandRow->addWidget(logo);
    brandRow->addLayout(brandCol);

    m_hostPill = new QWidget;
    m_hostPill->setObjectName(QStringLiteral("hostPill"));
    QHBoxLayout *pillLay = new QHBoxLayout(m_hostPill);
    pillLay->setContentsMargins(10, 6, 12, 6);
    pillLay->setSpacing(6);
    QLabel *hostIcon = new QLabel;
    hostIcon->setFixedSize(16, 16);
    hostIcon->setPixmap(makeLaptopIcon(16));
    QLabel *hostTag = new QLabel(QString::fromUtf8(u8"本机:"));
    hostTag->setObjectName(QStringLiteral("hostTag"));
    m_hostName = new QLabel;
    m_hostName->setObjectName(QStringLiteral("hostName"));
    m_hostIp = new QLabel;
    m_hostIp->setObjectName(QStringLiteral("hostIp"));
    pillLay->addWidget(hostIcon);
    pillLay->addWidget(hostTag);
    pillLay->addWidget(m_hostName);
    pillLay->addWidget(m_hostIp);

    QPushButton *shareBtn = new QPushButton(QString::fromUtf8(u8"网页共享 (HTTP)"));
    shareBtn->setObjectName(QStringLiteral("shareBtn"));
    shareBtn->setCursor(Qt::PointingHandCursor);
    connect(shareBtn, SIGNAL(clicked()), this, SLOT(webShareSoon()));

    QPushButton *setBtn = chromeBtn(IconSettings, QStringLiteral("iconBtn"), QString::fromUtf8(u8"设置"));
    setBtn->setCursor(Qt::PointingHandCursor);
    connect(setBtn, SIGNAL(clicked()), this, SLOT(editSettings()));

    QPushButton *minBtn = chromeBtn(IconMinimize, QStringLiteral("minBtn"), QString::fromUtf8(u8"最小化"));
    m_maxBtn = chromeBtn(IconMaximize, QStringLiteral("maxBtn"), QString::fromUtf8(u8"最大化"));
    QPushButton *closeBtn = chromeBtn(IconClose, QStringLiteral("closeBtn"), QString::fromUtf8(u8"关闭"));
    connect(minBtn, SIGNAL(clicked()), this, SLOT(minimizeWin()));
    connect(m_maxBtn, SIGNAL(clicked()), this, SLOT(toggleMax()));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeWin()));

    titleLay->addLayout(brandRow);
    titleLay->addStretch(1);
    titleLay->addWidget(m_hostPill);
    titleLay->addWidget(shareBtn);
    titleLay->addWidget(setBtn);
    titleLay->addSpacing(6);
    titleLay->addWidget(minBtn);
    titleLay->addWidget(m_maxBtn);
    titleLay->addWidget(closeBtn);

    // —— 主体：左设备列表 + 右会话 ——
    QWidget *body = new QWidget;
    body->setObjectName(QStringLiteral("body"));
    QHBoxLayout *bodyLay = new QHBoxLayout(body);
    bodyLay->setContentsMargins(0, 0, 0, 0);
    bodyLay->setSpacing(0);

    QWidget *side = new QWidget;
    side->setObjectName(QStringLiteral("side"));
    side->setFixedWidth(300);
    QVBoxLayout *sideLay = new QVBoxLayout(side);
    sideLay->setContentsMargins(16, 14, 16, 14);
    sideLay->setSpacing(10);

    QHBoxLayout *sideHead = new QHBoxLayout;
    QLabel *sideTitle = new QLabel(QString::fromUtf8(u8"附近在线设备"));
    sideTitle->setObjectName(QStringLiteral("sideTitle"));
    m_peerCount = new QLabel(QStringLiteral("0"));
    m_peerCount->setObjectName(QStringLiteral("peerCount"));
    QPushButton *addBtn = new QPushButton(QString::fromUtf8(u8"+ 加 IP"));
    addBtn->setObjectName(QStringLiteral("addBtn"));
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, SIGNAL(clicked()), this, SLOT(addPeer()));
    sideHead->addWidget(sideTitle);
    sideHead->addWidget(m_peerCount);
    sideHead->addStretch(1);
    sideHead->addWidget(addBtn);

    m_search = new QLineEdit;
    m_search->setObjectName(QStringLiteral("search"));
    m_search->setPlaceholderText(QString::fromUtf8(u8"搜索设备名称或 IP..."));
    connect(m_search, SIGNAL(textChanged(QString)), this, SLOT(filterPeers(QString)));

    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("peerList"));
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_list, SIGNAL(currentRowChanged(int)), this, SLOT(showChat()));

    QLabel *manual = new QLabel(QString::fromUtf8(u8"<a href='add' style='color:#2563eb;text-decoration:none;'>+ 手动输入 IP 连接</a>"));
    manual->setObjectName(QStringLiteral("manualLink"));
    manual->setTextFormat(Qt::RichText);
    manual->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(manual, SIGNAL(linkActivated(QString)), this, SLOT(addPeer()));

    sideLay->addLayout(sideHead);
    sideLay->addWidget(m_search);
    sideLay->addWidget(m_list, 1);
    sideLay->addWidget(manual, 0, Qt::AlignHCenter);

    QWidget *right = new QWidget;
    right->setObjectName(QStringLiteral("right"));
    QVBoxLayout *rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);

    m_pages = new QStackedWidget;
    m_emptyHint = new QLabel(QString::fromUtf8(u8"还没有对端。同一网段等待发现，或点左侧「加 IP」。"));
    m_emptyHint->setObjectName(QStringLiteral("emptyHint"));
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setWordWrap(true);

    QWidget *chatPage = new QWidget;
    QVBoxLayout *chatLay = new QVBoxLayout(chatPage);
    chatLay->setContentsMargins(0, 0, 0, 0);
    chatLay->setSpacing(0);
    m_chat = new QTextEdit;
    m_chat->setObjectName(QStringLiteral("chat"));
    m_chat->setReadOnly(true);
    m_chat->setFrameShape(QFrame::NoFrame);

    m_composer = new QWidget;
    m_composer->setObjectName(QStringLiteral("composer"));
    QHBoxLayout *compLay = new QHBoxLayout(m_composer);
    compLay->setContentsMargins(16, 12, 16, 12);
    compLay->setSpacing(8);
    m_input = new QLineEdit;
    m_input->setObjectName(QStringLiteral("input"));
    m_input->setPlaceholderText(QString::fromUtf8(u8"输入文字或命令，回车发送"));
    QPushButton *fileBtn = new QPushButton(QString::fromUtf8(u8"发送文件"));
    fileBtn->setObjectName(QStringLiteral("secondaryBtn"));
    fileBtn->setCursor(Qt::PointingHandCursor);
    QPushButton *sendBtn = new QPushButton(QString::fromUtf8(u8"发送"));
    sendBtn->setObjectName(QStringLiteral("primaryBtn"));
    sendBtn->setCursor(Qt::PointingHandCursor);
    connect(fileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(sendBtn, SIGNAL(clicked()), this, SLOT(sendText()));
    connect(m_input, SIGNAL(returnPressed()), this, SLOT(sendText()));
    compLay->addWidget(m_input, 1);
    compLay->addWidget(fileBtn);
    compLay->addWidget(sendBtn);

    chatLay->addWidget(m_chat, 1);
    chatLay->addWidget(m_composer);

    m_pages->addWidget(m_emptyHint);
    m_pages->addWidget(chatPage);
    rightLay->addWidget(m_pages, 1);

    bodyLay->addWidget(side);
    bodyLay->addWidget(right, 1);

    rootLay->addWidget(m_titleBar);
    rootLay->addWidget(body, 1);
    setCentralWidget(root);
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "#root { background: #f8fafc; border: 1px solid #cbd5e1; }"
        "#titleBar { background: #ffffff; border-bottom: 1px solid #e2e8f0; }"
        "#logo { background: transparent; border: none; }"
        "#brand { color: #0f172a; font-size: 16px; font-weight: 700; }"
        "#statusOnline { color: #64748b; font-size: 11px; }"
        "#hostPill { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; }"
        "#hostTag { color: #64748b; font-size: 12px; }"
        "#hostName { color: #0f172a; font-size: 12px; font-weight: 600; }"
        "#hostIp { color: #94a3b8; font-size: 12px; font-family: Consolas, 'Courier New', monospace; }"
        "#shareBtn { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 8px; color: #1d4ed8;"
        " padding: 6px 12px; font-size: 12px; font-weight: 600; }"
        "#shareBtn:hover { background: #dbeafe; }"
        "#iconBtn { background: transparent; border: 1px solid transparent; border-radius: 8px; padding: 0; }"
        "#iconBtn:hover { background: #f1f5f9; border-color: #e2e8f0; }"
        "#minBtn, #maxBtn, #closeBtn { background: transparent; border: none; border-radius: 6px; padding: 0; }"
        "#minBtn:hover, #maxBtn:hover { background: #f1f5f9; }"
        "#closeBtn:hover { background: #ef4444; }"
        "#side { background: #ffffff; border-right: 1px solid #e2e8f0; }"
        "#sideTitle { color: #0f172a; font-size: 13px; font-weight: 600; }"
        "#peerCount { background: #ecfdf5; color: #047857; border-radius: 8px; padding: 1px 7px;"
        " font-size: 11px; font-weight: 700; }"
        "#addBtn { background: #eff6ff; border: 1px solid #bfdbfe; border-radius: 8px; color: #1d4ed8;"
        " padding: 4px 10px; font-size: 12px; font-weight: 600; }"
        "#addBtn:hover { background: #dbeafe; }"
        "#search { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; padding: 8px 12px;"
        " color: #0f172a; selection-background-color: #bfdbfe; }"
        "#peerList { background: transparent; outline: none; }"
        "#peerList::item { background: transparent; border: 1px solid transparent; border-radius: 10px;"
        " padding: 10px 8px; margin: 2px 0; color: #0f172a; }"
        "#peerList::item:hover { background: #f8fafc; border-color: #e2e8f0; }"
        "#peerList::item:selected { background: #eff6ff; border-color: #bfdbfe; color: #1e3a8a; }"
        "#manualLink { font-size: 12px; padding: 6px; }"
        "#right { background: #ffffff; }"
        "#emptyHint { color: #94a3b8; font-size: 14px; padding: 40px; background: #ffffff; }"
        "#chat { background: #ffffff; color: #0f172a; font-size: 13px; padding: 16px; }"
        "#composer { background: #ffffff; border-top: 1px solid #e2e8f0; }"
        "#input { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; padding: 10px 12px;"
        " color: #0f172a; }"
        "#primaryBtn { background: #2563eb; border: none; border-radius: 8px; color: white;"
        " padding: 8px 16px; font-weight: 600; }"
        "#primaryBtn:hover { background: #1d4ed8; }"
        "#secondaryBtn { background: #ffffff; border: 1px solid #cbd5e1; border-radius: 8px; color: #334155;"
        " padding: 8px 12px; }"
        "#secondaryBtn:hover { background: #f8fafc; }"
    ));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_titleBar) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            toggleMax();
            return true;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton && !isMaximized()) {
                m_dragging = true;
                m_dragOrigin = me->globalPos() - frameGeometry().topLeft();
                return true;
            }
        }
        if (event->type() == QEvent::MouseMove && m_dragging) {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            move(me->globalPos() - m_dragOrigin);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateChrome();
}

void MainWindow::updateChrome()
{
    if (!m_maxBtn)
        return;
    const bool maxed = isMaximized();
    m_maxBtn->setIcon(makeChromeIcon(maxed ? IconRestore : IconMaximize, QColor(QStringLiteral("#475569"))));
    m_maxBtn->setToolTip(maxed ? QString::fromUtf8(u8"还原") : QString::fromUtf8(u8"最大化"));
}

void MainWindow::minimizeWin()
{
    showMinimized();
}

void MainWindow::toggleMax()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
    updateChrome();
}

void MainWindow::closeWin()
{
    close();
}

void MainWindow::webShareSoon()
{
    QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                             QString::fromUtf8(u8"网页共享会在后续版本提供。现在可以用左侧设备列表互传文字和文件。"));
}

void MainWindow::updateHostPill()
{
    if (!m_hostName || !m_hostIp)
        return;
    m_hostName->setText(m_settings.deviceName);
    m_hostIp->setText(QLatin1Char('(') + localIpText() + QLatin1Char(')'));
}

void MainWindow::setStatusOnline(const QString &text, bool ok)
{
    if (!m_statusLabel)
        return;
    const QString dot = ok ? QStringLiteral("#22c55e") : QStringLiteral("#f59e0b");
    m_statusLabel->setText(
        QStringLiteral("<span style=\"color:%1;font-size:10px;\">●</span>"
                       "&nbsp;<span style=\"color:#64748b;\">%2</span>")
            .arg(dot, text.toHtmlEscaped()));
}

void MainWindow::boot()
{
    m_settings = Settings::load();
    m_id = deviceId();
    QDir().mkpath(m_settings.downloadDir);
    m_disc->setIdentity(m_id, m_settings.deviceName, m_settings.port);
    m_http->setInfo(m_id, m_settings.deviceName, m_settings.port);
    m_http->setDownloadDir(m_settings.downloadDir);
    const bool httpOk = m_http->listen(m_settings.port);
    const bool discOk = m_disc->start(m_settings.discoverPort);
    updateHostPill();
    if (!httpOk) {
        setStatusOnline(QString::fromUtf8(u8"传输端口占用，请在设置里改端口"), false);
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"),
                             QString::fromUtf8(u8"端口 %1 被占用，其他电脑连不上这台机器。请在设置里改端口后重启。").arg(m_settings.port));
    } else if (!discOk) {
        setStatusOnline(QString::fromUtf8(u8"在线 · 发现端口占用，仍可手动加 IP"), false);
    } else {
        setStatusOnline(QString::fromUtf8(u8"在线 · 局域网自动发现中"), true);
    }
    refreshPeers();
    showChat();
    updateChrome();
}

QString MainWindow::localIpText() const
{
    const QStringList ips = localIpv4();
    if (ips.isEmpty())
        return QString::fromUtf8(u8"—");
    return ips.first();
}

bool MainWindow::currentPeer(QString *ip, int *port, QString *name) const
{
    QListWidgetItem *it = m_list->currentItem();
    if (!it || it->isHidden())
        return false;
    if (ip)
        *ip = it->data(Qt::UserRole).toString();
    if (port)
        *port = it->data(Qt::UserRole + 1).toInt();
    if (name)
        *name = it->data(Qt::UserRole + 2).toString();
    return !it->data(Qt::UserRole).toString().isEmpty();
}

QString MainWindow::currentKey() const
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return QString();
    return ip + QLatin1Char(':') + QString::number(port);
}

void MainWindow::filterPeers(const QString &text)
{
    const QString q = text.trimmed().toLower();
    for (int i = 0; i < m_list->count(); ++i) {
        QListWidgetItem *it = m_list->item(i);
        if (q.isEmpty()) {
            it->setHidden(false);
            continue;
        }
        const QString hay = (it->text() + QLatin1Char(' ') + it->data(Qt::UserRole).toString()).toLower();
        it->setHidden(!hay.contains(q));
    }
    updateEmpty();
}

void MainWindow::updateEmpty()
{
    int visible = 0;
    for (int i = 0; i < m_list->count(); ++i) {
        if (!m_list->item(i)->isHidden())
            ++visible;
    }
    m_peerCount->setText(QString::number(visible));
    const bool hasPeer = currentPeer(0, 0, 0);
    m_pages->setCurrentIndex(hasPeer ? 1 : 0);
    m_composer->setEnabled(hasPeer);
}

void MainWindow::refreshPeers()
{
    const QString keep = currentKey();
    const QString filter = m_search ? m_search->text() : QString();
    m_list->blockSignals(true);
    m_list->clear();
    const QList<Peer> list = m_disc->peers();
    int row = -1;
    for (int i = 0; i < list.size(); ++i) {
        const Peer &p = list.at(i);
        const QString flag = p.online() ? QString::fromUtf8(u8"在线") : QString::fromUtf8(u8"离线");
        const QString manual = p.manual ? QString::fromUtf8(u8" · 手动") : QString();
        QListWidgetItem *it = new QListWidgetItem(
            QStringLiteral("%1\n%2:%3  %4%5").arg(p.label()).arg(p.ip).arg(p.port).arg(flag).arg(manual));
        it->setData(Qt::UserRole, p.ip);
        it->setData(Qt::UserRole + 1, p.port);
        it->setData(Qt::UserRole + 2, p.label());
        m_list->addItem(it);
        if (p.key() == keep)
            row = m_list->count() - 1;
    }
    if (row >= 0)
        m_list->setCurrentRow(row);
    else if (m_list->count() > 0 && keep.isEmpty())
        m_list->setCurrentRow(0);
    m_list->blockSignals(false);
    filterPeers(filter);
    if (row < 0 && !keep.isEmpty())
        showChat();
    else
        updateEmpty();
    updateHostPill();
}

void MainWindow::showChat()
{
    const QString key = currentKey();
    if (key.isEmpty()) {
        updateEmpty();
        return;
    }
    m_pages->setCurrentIndex(1);
    m_composer->setEnabled(true);
    m_chat->setPlainText(m_log.value(key).join(QStringLiteral("\n")));
}

void MainWindow::note(const QString &key, const QString &line)
{
    QStringList lines = m_log.value(key);
    lines.append(line);
    if (lines.size() > 500)
        lines = lines.mid(lines.size() - 500);
    m_log.insert(key, lines);
    if (key == currentKey())
        m_chat->append(line);
}

void MainWindow::onText(const QString &ip, const QString &fromId, const QString &fromName, int fromPort, const QString &text)
{
    Q_UNUSED(fromId);
    const int port = fromPort > 0 ? fromPort : 8848;
    m_disc->touch(ip, port, fromId, fromName, QString());
    const QString who = fromName.trimmed().isEmpty() ? ip : fromName.trimmed();
    note(ip + QLatin1Char(':') + QString::number(port), QString::fromUtf8(u8"%1：%2").arg(who).arg(text));
}

void MainWindow::onFile(const QString &ip, const QString &name, const QString &path, qint64 size)
{
    Q_UNUSED(path);
    Peer known;
    int port = 8848;
    if (m_disc->find(ip, 8848, &known))
        port = known.port;
    note(ip + QLatin1Char(':') + QString::number(port),
         QString::fromUtf8(u8"收到文件 %1（%2 字节）").arg(name).arg(size));
}

void MainWindow::sendText()
{
    const QString text = m_input->text();
    if (text.trimmed().isEmpty())
        return;
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QJsonObject o;
    o.insert(QStringLiteral("fromId"), m_id);
    o.insert(QStringLiteral("fromName"), m_settings.deviceName);
    o.insert(QStringLiteral("fromPort"), m_settings.port);
    o.insert(QStringLiteral("text"), text);
    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/inbox").arg(ip).arg(port)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/json; charset=utf-8")));
    QNetworkReply *rep = m_nam->post(req, body);
    const QString key = currentKey();
    const QString mine = m_settings.deviceName;
    const QString sent = text;
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, mine, sent]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError)
            return;
        note(key, QString::fromUtf8(u8"%1：%2").arg(mine).arg(sent));
        m_input->clear();
    });
}

void MainWindow::sendFile()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8(u8"选择要发送的文件"));
    if (path.isEmpty())
        return;
    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        return;
    }
    const QString filename = QFileInfo(path).fileName();
    QHttpMultiPart *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart part;
    const QByteArray disp = "form-data; name=\"file\"; filename=\"" + filename.toUtf8()
        + "\"; filename*=UTF-8''" + QUrl::toPercentEncoding(filename);
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(disp));
    part.setBodyDevice(file);
    file->setParent(multi);
    multi->append(part);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/upload").arg(ip).arg(port)));
    QNetworkReply *rep = m_nam->post(req, multi);
    multi->setParent(rep);
    const QString key = currentKey();
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, filename]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError || rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 300)
            return;
        note(key, QString::fromUtf8(u8"已发送文件 %1").arg(filename));
    });
}

void MainWindow::addPeer()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"添加节点"));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QLineEdit *ip = new QLineEdit;
    ip->setPlaceholderText(QStringLiteral("192.168.1.10"));
    QLineEdit *port = new QLineEdit(QStringLiteral("8848"));
    QLineEdit *alias = new QLineEdit;
    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("IP"), ip);
    form->addRow(QString::fromUtf8(u8"端口"), port);
    form->addRow(QString::fromUtf8(u8"备注"), alias);
    QPushButton *ok = new QPushButton(QString::fromUtf8(u8"确定"));
    QPushButton *cancel = new QPushButton(QString::fromUtf8(u8"取消"));
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(ok);
    btns->addWidget(cancel);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addLayout(form);
    lay->addLayout(btns);
    connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
    if (dlg.exec() != QDialog::Accepted)
        return;
    const QString host = ip->text().trimmed();
    bool portOk = false;
    int p = port->text().trimmed().toInt(&portOk);
    if (host.isEmpty() || host.contains(QLatin1Char(' ')) || !portOk || p < 1 || p > 65535) {
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"), QString::fromUtf8(u8"IP 或端口无效"));
        return;
    }
    m_disc->addManual(host, p, alias->text());
    refreshPeers();
    probePeer();
}

void MainWindow::probePeer()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QNetworkReply *rep = m_nam->get(QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(ip).arg(port))));
    connect(rep, &QNetworkReply::finished, this, [this, rep, ip, port]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"连不上 %1:%2").arg(ip).arg(port));
            return;
        }
        const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
        m_disc->touch(ip, o.value(QStringLiteral("port")).toInt() > 0 ? o.value(QStringLiteral("port")).toInt() : port,
                      o.value(QStringLiteral("id")).toString(),
                      o.value(QStringLiteral("name")).toString(),
                      o.value(QStringLiteral("os")).toString());
        refreshPeers();
    });
}

void MainWindow::editSettings()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"设置"));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QLineEdit *name = new QLineEdit(m_settings.deviceName);
    QLineEdit *port = new QLineEdit(QString::number(m_settings.port));
    QLineEdit *dport = new QLineEdit(QString::number(m_settings.discoverPort));
    QLineEdit *dir = new QLineEdit(m_settings.downloadDir);
    QPushButton *browse = new QPushButton(QString::fromUtf8(u8"浏览"));
    connect(browse, &QPushButton::clicked, &dlg, [dir, &dlg]() {
        const QString picked = QFileDialog::getExistingDirectory(&dlg, QString::fromUtf8(u8"下载目录"), dir->text());
        if (!picked.isEmpty())
            dir->setText(picked);
    });
    QHBoxLayout *dirRow = new QHBoxLayout;
    dirRow->addWidget(dir, 1);
    dirRow->addWidget(browse);
    QFormLayout *form = new QFormLayout;
    form->addRow(QString::fromUtf8(u8"本机名称"), name);
    form->addRow(QString::fromUtf8(u8"传输端口"), port);
    form->addRow(QString::fromUtf8(u8"发现端口"), dport);
    form->addRow(QString::fromUtf8(u8"下载目录"), dirRow);
    QPushButton *ok = new QPushButton(QString::fromUtf8(u8"保存"));
    QPushButton *cancel = new QPushButton(QString::fromUtf8(u8"取消"));
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(ok);
    btns->addWidget(cancel);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addLayout(form);
    lay->addWidget(new QLabel(QString::fromUtf8(u8"改端口会马上重新监听。若提示占用，换一个端口。")));
    lay->addLayout(btns);
    connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
    if (dlg.exec() != QDialog::Accepted)
        return;
    bool ok1 = false, ok2 = false;
    const int p = port->text().trimmed().toInt(&ok1);
    const int dp = dport->text().trimmed().toInt(&ok2);
    if (name->text().trimmed().isEmpty() || !ok1 || !ok2 || p < 1 || p > 65535 || dp < 1 || dp > 65535) {
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"), QString::fromUtf8(u8"名称或端口无效"));
        return;
    }
    m_settings.deviceName = name->text().trimmed();
    m_settings.port = p;
    m_settings.discoverPort = dp;
    m_settings.downloadDir = dir->text().trimmed();
    if (!m_settings.save()) {
        QMessageBox::warning(this, QString::fromUtf8(u8"局域快传"), QString::fromUtf8(u8"保存设置失败"));
        return;
    }
    boot();
}
