#include "mainwindow.h"

#include "discovery.h"
#include "files.h"
#include "httpserver.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QtMath>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSvgRenderer>
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

static QPixmap makeGlobeBadge(int logical = 36)
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
    p.drawRoundedRect(QRectF(0, 0, logical, logical), 8, 8);
    QPen pen(Qt::white, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal pad = 8.0;
    const QRectF box(pad, pad, logical - pad * 2, logical - pad * 2);
    p.drawEllipse(box);
    p.drawEllipse(QRectF(logical * 0.36, pad, logical * 0.28, logical - pad * 2));
    p.drawLine(QPointF(pad + 1, logical * 0.40), QPointF(logical - pad - 1, logical * 0.40));
    p.drawLine(QPointF(pad + 1, logical * 0.60), QPointF(logical - pad - 1, logical * 0.60));
    return pm;
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
    p.drawEllipse(QPointF(logical / 2.0, logical / 2.0), 2.4, 2.4);

    // Qt 弧：0°在时钟 3 点，逆时针。左右弧不得跨过 12/6 点，否则会像 Wi‑Fi 上下波纹。
    QPen pen(Qt::white, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QPointF c(logical / 2.0, logical / 2.0);
    for (int i = 0; i < 2; ++i) {
        const qreal r = 6.5 + i * 4.5;
        const QRectF box(c.x() - r, c.y() - r, r * 2, r * 2);
        p.drawArc(box, -50 * 16, 100 * 16);  // 右侧
        p.drawArc(box, 130 * 16, 100 * 16);  // 左侧
    }
    return pm;
}

enum DeviceKind {
    DevLaptop = 0,
    DevPhone,
    DevTablet
};

static QPixmap renderSvgIcon(const QString &resPath, int logical = 16);

static QString avatarSvgForOs(const QString &osName)
{
    const QString o = osName.toLower();
    if (o.contains(QLatin1String("android")) || o.contains(QLatin1String("ios"))
        || o.contains(QLatin1String("iphone")) || o.contains(QLatin1String("ipad"))
        || o.contains(QLatin1String("phone")))
        return QStringLiteral(":/avatars/avatar_iphone.svg");
    if (o.contains(QLatin1String("arm")) || o.contains(QLatin1String("aarch"))
        || o.contains(QLatin1String("raspberry")))
        return QStringLiteral(":/avatars/avatar_raspberrypi.svg");
    if (o.contains(QLatin1String("linux")) || o.contains(QLatin1String("ubuntu"))
        || o.contains(QLatin1String("kylin")))
        return QStringLiteral(":/avatars/avatar_ubuntu.svg");
    return QStringLiteral(":/avatars/avatar_windows.svg");
}

static void paintDeviceGlyph(QPainter &p, DeviceKind kind, const QRectF &box, const QColor &color)
{
    QPen pen(color, 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const qreal x = box.x();
    const qreal y = box.y();
    const qreal w = box.width();
    const qreal h = box.height();
    switch (kind) {
    case DevPhone:
        p.drawRoundedRect(QRectF(x + w * 0.28, y + h * 0.08, w * 0.44, h * 0.84), 1.5, 1.5);
        p.drawLine(QPointF(x + w * 0.40, y + h * 0.78), QPointF(x + w * 0.60, y + h * 0.78));
        break;
    case DevTablet:
        p.drawRoundedRect(QRectF(x + w * 0.12, y + h * 0.18, w * 0.76, h * 0.64), 1.8, 1.8);
        p.drawLine(QPointF(x + w * 0.42, y + h * 0.72), QPointF(x + w * 0.58, y + h * 0.72));
        break;
    case DevLaptop:
    default:
        p.drawRoundedRect(QRectF(x + w * 0.14, y + h * 0.18, w * 0.72, h * 0.48), 1.2, 1.2);
        p.drawLine(QPointF(x + w * 0.06, y + h * 0.72), QPointF(x + w * 0.94, y + h * 0.72));
        p.drawLine(QPointF(x + w * 0.22, y + h * 0.72), QPointF(x + w * 0.30, y + h * 0.86));
        p.drawLine(QPointF(x + w * 0.78, y + h * 0.72), QPointF(x + w * 0.70, y + h * 0.86));
        p.drawLine(QPointF(x + w * 0.30, y + h * 0.86), QPointF(x + w * 0.70, y + h * 0.86));
        break;
    }
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
    paintDeviceGlyph(p, DevLaptop, QRectF(0, 0, logical, logical), QColor(QStringLiteral("#2563eb")));
    return pm;
}

static QPixmap makePeerAvatar(const QString &name, const QString &osName, int logical = 44)
{
    Q_UNUSED(name);
    return renderSvgIcon(avatarSvgForOs(osName), logical);
}

static QPixmap renderSvgIcon(const QString &resPath, int logical)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QSvgRenderer renderer(resPath);
    if (!renderer.isValid())
        return pm;
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p, QRectF(0, 0, logical, logical));
    return pm;
}

static QPushButton *toolLinkBtn(const QString &svgRes, const QString &text, const QString &objectName)
{
    QPushButton *b = new QPushButton(text);
    b->setObjectName(objectName);
    b->setCursor(Qt::PointingHandCursor);
    b->setFlat(true);
    b->setFocusPolicy(Qt::NoFocus);
    b->setIcon(QIcon(renderSvgIcon(svgRes, 16)));
    b->setIconSize(QSize(16, 16));
    return b;
}

static QPixmap makeStatusDot(bool ok, int logical = 7)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(ok ? QColor(QStringLiteral("#22c55e")) : QColor(QStringLiteral("#f59e0b")));
    p.drawEllipse(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0));
    return pm;
}

static QPixmap makeChatBubbleIcon(int logical = 14)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(QStringLiteral("#2563eb")), 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(1.5, 1.5, logical - 3.5, logical - 5.5), 2.5, 2.5);
    p.drawLine(QPointF(4.0, logical - 3.0), QPointF(6.5, logical - 5.5));
    return pm;
}

static QPixmap makeFileDocIcon(int logical = 14)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(QStringLiteral("#2563eb")), 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    const QRectF body(3.0, 1.5, logical - 6.0, logical - 3.0);
    p.drawRoundedRect(body, 1.5, 1.5);
    p.drawLine(QPointF(5.5, 5.0), QPointF(logical - 5.5, 5.0));
    p.drawLine(QPointF(5.5, 8.0), QPointF(logical - 5.5, 8.0));
    p.drawLine(QPointF(5.5, 11.0), QPointF(logical - 7.0, 11.0));
    return pm;
}

static QPixmap makeCheckCircleIcon(int logical = 14)
{
    const int dpr = qMax(1, qRound(qApp->devicePixelRatio()));
    const int px = logical * dpr;
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(QStringLiteral("#059669")));
    p.drawEllipse(QRectF(0.5, 0.5, logical - 1.0, logical - 1.0));
    QPen pen(Qt::white, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(3.5, logical * 0.52);
    path.lineTo(5.8, logical * 0.70);
    path.lineTo(logical - 3.2, logical * 0.32);
    p.drawPath(path);
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
    QHBoxLayout *statusRow = new QHBoxLayout;
    statusRow->setContentsMargins(0, 0, 0, 0);
    statusRow->setSpacing(5);
    m_statusDot = new QLabel;
    m_statusDot->setFixedSize(7, 7);
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("statusOnline"));
    m_statusLabel->setTextFormat(Qt::PlainText);
    statusRow->addWidget(m_statusDot, 0, Qt::AlignVCenter);
    statusRow->addWidget(m_statusLabel, 0, Qt::AlignVCenter);
    statusRow->addStretch(1);
    brandCol->addWidget(brand);
    brandCol->addLayout(statusRow);

    QHBoxLayout *brandRow = new QHBoxLayout;
    brandRow->setSpacing(10);
    brandRow->setContentsMargins(0, 0, 0, 0);
    brandRow->addWidget(logo);
    brandRow->addLayout(brandCol);

    m_hostPill = new QWidget;
    m_hostPill->setObjectName(QStringLiteral("hostPill"));
    m_hostPill->setFixedHeight(28);
    QHBoxLayout *pillLay = new QHBoxLayout(m_hostPill);
    pillLay->setContentsMargins(10, 0, 12, 0);
    pillLay->setSpacing(5);
    QLabel *hostIcon = new QLabel;
    hostIcon->setFixedSize(16, 16);
    hostIcon->setPixmap(makeLaptopIcon(16));
    QLabel *hostTag = new QLabel(QString::fromUtf8(u8"本机:"));
    hostTag->setObjectName(QStringLiteral("hostTag"));
    m_hostName = new QLabel;
    m_hostName->setObjectName(QStringLiteral("hostName"));
    m_hostIp = new QLabel;
    m_hostIp->setObjectName(QStringLiteral("hostIp"));
    pillLay->addWidget(hostIcon, 0, Qt::AlignVCenter);
    pillLay->addWidget(hostTag, 0, Qt::AlignVCenter);
    pillLay->addWidget(m_hostName, 0, Qt::AlignVCenter);
    pillLay->addWidget(m_hostIp, 0, Qt::AlignVCenter);

    m_shareBtn = new QPushButton;
    m_shareBtn->setObjectName(QStringLiteral("shareBtn"));
    m_shareBtn->setCursor(Qt::PointingHandCursor);
    connect(m_shareBtn, SIGNAL(clicked()), this, SLOT(openShare()));
    refreshShareBtn();

    QPushButton *setBtn = chromeBtn(IconSettings, QStringLiteral("iconBtn"), QString::fromUtf8(u8"设置"));
    setBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/settings.svg"), 20)));
    setBtn->setIconSize(QSize(20, 20));
    setBtn->setCursor(Qt::PointingHandCursor);
    connect(setBtn, SIGNAL(clicked()), this, SLOT(editSettings()));

    QPushButton *minBtn = chromeBtn(IconMinimize, QStringLiteral("minBtn"), QString::fromUtf8(u8"最小化"));
    m_maxBtn = chromeBtn(IconMaximize, QStringLiteral("maxBtn"), QString::fromUtf8(u8"最大化"));
    QPushButton *closeBtn = chromeBtn(IconClose, QStringLiteral("closeBtn"), QString::fromUtf8(u8"关闭"));
    connect(minBtn, SIGNAL(clicked()), this, SLOT(minimizeWin()));
    connect(m_maxBtn, SIGNAL(clicked()), this, SLOT(toggleMax()));
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeWin()));

    // 左右等宽，胶囊落在窗口水平正中
    QWidget *leftZone = new QWidget;
    QHBoxLayout *leftLay = new QHBoxLayout(leftZone);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);
    leftLay->addLayout(brandRow);
    leftLay->addStretch(1);

    QWidget *rightZone = new QWidget;
    QHBoxLayout *chromeLay = new QHBoxLayout(rightZone);
    chromeLay->setContentsMargins(0, 0, 0, 0);
    chromeLay->setSpacing(12);
    chromeLay->addStretch(1);
    chromeLay->addWidget(m_shareBtn);
    chromeLay->addWidget(setBtn);
    chromeLay->addSpacing(6);
    chromeLay->addWidget(minBtn);
    chromeLay->addWidget(m_maxBtn);
    chromeLay->addWidget(closeBtn);

    titleLay->addWidget(leftZone, 1);
    titleLay->addWidget(m_hostPill, 0, Qt::AlignVCenter);
    titleLay->addWidget(rightZone, 1);

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
    m_list->setIconSize(QSize(44, 44));
    m_list->setSpacing(2);
    connect(m_list, SIGNAL(currentRowChanged(int)), this, SLOT(showChat()));

    sideLay->addLayout(sideHead);
    sideLay->addWidget(m_search);
    sideLay->addWidget(m_list, 1);

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

    m_peerHeader = new QWidget;
    m_peerHeader->setObjectName(QStringLiteral("peerHeader"));
    m_peerHeader->setFixedHeight(64);
    QHBoxLayout *peerHeadLay = new QHBoxLayout(m_peerHeader);
    peerHeadLay->setContentsMargins(16, 8, 16, 8);
    peerHeadLay->setSpacing(12);

    m_peerAvatar = new QLabel;
    m_peerAvatar->setObjectName(QStringLiteral("peerAvatar"));
    m_peerAvatar->setFixedSize(44, 44);

    QVBoxLayout *peerInfoCol = new QVBoxLayout;
    peerInfoCol->setContentsMargins(0, 0, 0, 0);
    peerInfoCol->setSpacing(3);
    QHBoxLayout *peerTitleRow = new QHBoxLayout;
    peerTitleRow->setContentsMargins(0, 0, 0, 0);
    peerTitleRow->setSpacing(8);
    m_peerName = new QLabel;
    m_peerName->setObjectName(QStringLiteral("peerName"));
    m_peerOnlineDot = new QLabel;
    m_peerOnlineDot->setFixedSize(8, 8);
    m_peerAddr = new QLabel;
    m_peerAddr->setObjectName(QStringLiteral("peerAddr"));
    peerTitleRow->addWidget(m_peerName, 0, Qt::AlignVCenter);
    peerTitleRow->addWidget(m_peerOnlineDot, 0, Qt::AlignVCenter);
    peerTitleRow->addWidget(m_peerAddr, 0, Qt::AlignVCenter);
    peerTitleRow->addStretch(1);
    m_peerMeta = new QLabel;
    m_peerMeta->setObjectName(QStringLiteral("peerMeta"));
    peerInfoCol->addLayout(peerTitleRow);
    peerInfoCol->addWidget(m_peerMeta);

    m_tabChat = new QPushButton(QString::fromUtf8(u8"即时聊天"));
    m_tabChat->setObjectName(QStringLiteral("sessionTab"));
    m_tabChat->setCursor(Qt::PointingHandCursor);
    m_tabChat->setFocusPolicy(Qt::NoFocus);
    m_tabChat->setIcon(QIcon(makeChatBubbleIcon(14)));
    m_tabChat->setIconSize(QSize(14, 14));
    m_tabFiles = new QPushButton(QString::fromUtf8(u8"文件传输 (0)"));
    m_tabFiles->setObjectName(QStringLiteral("sessionTab"));
    m_tabFiles->setCursor(Qt::PointingHandCursor);
    m_tabFiles->setFocusPolicy(Qt::NoFocus);
    m_tabFiles->setIcon(QIcon(makeFileDocIcon(14)));
    m_tabFiles->setIconSize(QSize(14, 14));
    connect(m_tabChat, SIGNAL(clicked()), this, SLOT(showChatTab()));
    connect(m_tabFiles, SIGNAL(clicked()), this, SLOT(showFilesTab()));

    peerHeadLay->addWidget(m_peerAvatar, 0, Qt::AlignVCenter);
    peerHeadLay->addLayout(peerInfoCol, 1);
    peerHeadLay->addWidget(m_tabChat, 0, Qt::AlignVCenter);
    peerHeadLay->addWidget(m_tabFiles, 0, Qt::AlignVCenter);

    m_connBanner = new QWidget;
    m_connBanner->setObjectName(QStringLiteral("connBanner"));
    QHBoxLayout *bannerLay = new QHBoxLayout(m_connBanner);
    bannerLay->setContentsMargins(16, 6, 16, 6);
    bannerLay->setSpacing(8);
    QLabel *bannerIcon = new QLabel;
    bannerIcon->setFixedSize(14, 14);
    bannerIcon->setPixmap(makeCheckCircleIcon(14));
    m_connBannerText = new QLabel;
    m_connBannerText->setObjectName(QStringLiteral("connBannerText"));
    m_connBannerText->setWordWrap(true);
    bannerLay->addWidget(bannerIcon, 0, Qt::AlignVCenter);
    bannerLay->addWidget(m_connBannerText, 1, Qt::AlignVCenter);

    QWidget *chatBody = new QWidget;
    QVBoxLayout *chatBodyLay = new QVBoxLayout(chatBody);
    chatBodyLay->setContentsMargins(0, 0, 0, 0);
    chatBodyLay->setSpacing(0);
    m_chat = new QTextEdit;
    m_chat->setObjectName(QStringLiteral("chat"));
    m_chat->setReadOnly(true);
    m_chat->setFrameShape(QFrame::NoFrame);

    m_composer = new QWidget;
    m_composer->setObjectName(QStringLiteral("composer"));
    QVBoxLayout *compCol = new QVBoxLayout(m_composer);
    compCol->setContentsMargins(16, 10, 16, 14);
    compCol->setSpacing(8);
    m_progress = new QLabel;
    m_progress->setObjectName(QStringLiteral("progress"));
    m_progress->hide();

    QHBoxLayout *toolLay = new QHBoxLayout;
    toolLay->setContentsMargins(0, 0, 0, 0);
    toolLay->setSpacing(4);
    QPushButton *fileBtn = toolLinkBtn(QStringLiteral(":/icons/paperclip.svg"),
                                       QString::fromUtf8(u8"发送文件"), QStringLiteral("toolBtn"));
    QPushButton *folderBtn = toolLinkBtn(QStringLiteral(":/icons/folder-plus.svg"),
                                         QString::fromUtf8(u8"发送文件夹"), QStringLiteral("toolBtn"));
    QPushButton *nudgeBtn = toolLinkBtn(QStringLiteral(":/icons/zap.svg"),
                                        QString::fromUtf8(u8"抖动窗口"), QStringLiteral("toolBtn"));
    connect(fileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(folderBtn, SIGNAL(clicked()), this, SLOT(sendFolder()));
    connect(nudgeBtn, SIGNAL(clicked()), this, SLOT(nudgePeer()));
    // Qt 富文本不画 span 边框，键帽用独立标签才能看出描边
    auto hintBit = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("inputHint"));
        return l;
    };
    auto keycap = [](const QString &text) {
        QLabel *k = new QLabel(text);
        k->setObjectName(QStringLiteral("keycap"));
        return k;
    };
    QWidget *hintRow = new QWidget;
    QHBoxLayout *hintLay = new QHBoxLayout(hintRow);
    hintLay->setContentsMargins(0, 0, 0, 0);
    hintLay->setSpacing(4);
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"按")));
    hintLay->addWidget(keycap(QStringLiteral("Enter")));
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"发送，")));
    hintLay->addWidget(keycap(QStringLiteral("Shift+Enter")));
    hintLay->addWidget(hintBit(QString::fromUtf8(u8"换行")));
    toolLay->addWidget(fileBtn);
    toolLay->addWidget(folderBtn);
    toolLay->addWidget(nudgeBtn);
    toolLay->addStretch(1);
    toolLay->addWidget(hintRow);

    m_inputShell = new QWidget;
    m_inputShell->setObjectName(QStringLiteral("inputShell"));
    QVBoxLayout *shellLay = new QVBoxLayout(m_inputShell);
    shellLay->setContentsMargins(12, 10, 10, 10);
    shellLay->setSpacing(4);
    m_input = new QPlainTextEdit;
    m_input->setObjectName(QStringLiteral("input"));
    m_input->setFrameShape(QFrame::NoFrame);
    m_input->setFixedHeight(72);
    m_input->setTabChangesFocus(true);
    m_input->installEventFilter(this);
    m_sendBtn = new QPushButton;
    m_sendBtn->setObjectName(QStringLiteral("sendFab"));
    m_sendBtn->setFixedSize(34, 34);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFocusPolicy(Qt::NoFocus);
    m_sendBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/send.svg"), 18)));
    m_sendBtn->setIconSize(QSize(18, 18));
    connect(m_sendBtn, SIGNAL(clicked()), this, SLOT(sendText()));
    QHBoxLayout *sendRow = new QHBoxLayout;
    sendRow->setContentsMargins(0, 0, 0, 0);
    sendRow->addStretch(1);
    sendRow->addWidget(m_sendBtn);
    shellLay->addWidget(m_input, 1);
    shellLay->addLayout(sendRow);

    compCol->addWidget(m_progress);
    compCol->addLayout(toolLay);
    compCol->addWidget(m_inputShell);

    chatBodyLay->addWidget(m_chat, 1);
    chatBodyLay->addWidget(m_composer);

    QLabel *filesPlaceholder = new QLabel(QString::fromUtf8(
        u8"文件传输列表将在后续版本实现。\n本轮只对齐「文件传输」页壳。"));
    filesPlaceholder->setObjectName(QStringLiteral("filesPlaceholder"));
    filesPlaceholder->setAlignment(Qt::AlignCenter);
    filesPlaceholder->setWordWrap(true);

    m_sessionStack = new QStackedWidget;
    m_sessionStack->setObjectName(QStringLiteral("sessionStack"));
    m_sessionStack->addWidget(chatBody);
    m_sessionStack->addWidget(filesPlaceholder);

    chatLay->addWidget(m_peerHeader);
    chatLay->addWidget(m_connBanner);
    chatLay->addWidget(m_sessionStack, 1);

    m_pages->addWidget(m_emptyHint);
    m_pages->addWidget(chatPage);
    rightLay->addWidget(m_pages, 1);

    bodyLay->addWidget(side);
    bodyLay->addWidget(right, 1);

    rootLay->addWidget(m_titleBar);
    rootLay->addWidget(body, 1);
    setCentralWidget(root);
    setSessionTab(0);
}

void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "#root { background: #f8fafc; border: 1px solid #cbd5e1; }"
        "#titleBar { background: #ffffff; border-bottom: 1px solid #e2e8f0; }"
        "#logo { background: transparent; border: none; }"
        "#brand { color: #0f172a; font-size: 16px; font-weight: 700; }"
        "#statusOnline { color: #64748b; font-size: 11px; }"
        "#hostPill { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; }"
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
        " padding: 8px 8px; margin: 2px 0; color: #0f172a; }"
        "#peerList::item:hover { background: #f8fafc; border-color: #e2e8f0; }"
        "#peerList::item:selected { background: #eff6ff; border-color: #bfdbfe; color: #1e3a8a; }"
        "#right { background: #ffffff; }"
        "#emptyHint { color: #94a3b8; font-size: 14px; padding: 40px; background: #ffffff; }"
        "#peerHeader { background: #ffffff; border-bottom: 1px solid #e2e8f0; }"
        "#peerName { color: #0f172a; font-size: 14px; font-weight: 700; }"
        "#peerAddr { color: #64748b; font-size: 11px; font-family: Consolas, 'Courier New', monospace;"
        " background: #f1f5f9; border-radius: 6px; padding: 2px 8px; }"
        "#peerMeta { color: #94a3b8; font-size: 11px; }"
        "#sessionTab { background: #ffffff; border: 1px solid #bfdbfe; border-radius: 10px;"
        " color: #1d4ed8; padding: 6px 12px; font-size: 12px; font-weight: 600; }"
        "#sessionTab:hover { background: #eff6ff; }"
        "#sessionTabActive { background: #eff6ff; border: 1px solid #93c5fd; border-radius: 10px;"
        " color: #1e40af; padding: 6px 12px; font-size: 12px; font-weight: 700; }"
        "#sessionTabActive:hover { background: #dbeafe; }"
        "#connBanner { background: #ecfdf5; border-bottom: 1px solid #a7f3d0; }"
        "#connBannerText { color: #047857; font-size: 12px; }"
        "#filesPlaceholder { color: #94a3b8; font-size: 13px; padding: 40px; background: #f8fafc; }"
        "#chat { background: #ffffff; color: #0f172a; font-size: 13px; padding: 16px; }"
        "#composer { background: #ffffff; border-top: 1px solid #e2e8f0; }"
        "#progress { color: #1d4ed8; font-size: 12px; }"
        "#toolBtn { background: transparent; border: none; color: #475569; font-size: 12px;"
        " padding: 4px 8px; border-radius: 6px; }"
        "#toolBtn:hover { background: #f1f5f9; color: #0f172a; }"
        "#inputHint { color: #94a3b8; font-size: 12px; background: transparent; border: none; }"
        "#keycap { color: #475569; background: #f8fafc; border: 1px solid #94a3b8;"
        " border-radius: 4px; padding: 1px 6px; font-size: 11px; font-weight: 600; }"
        "#inputShell { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }"
        "#input { background: transparent; border: none; color: #0f172a; font-size: 13px;"
        " padding: 0; selection-background-color: #bfdbfe; }"
        "#sendFab { background: #93c5fd; border: none; border-radius: 8px; padding: 0; }"
        "#sendFab:hover { background: #60a5fa; }"
        "#sendFab:pressed { background: #3b82f6; }"
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
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
            && !(ke->modifiers() & Qt::ShiftModifier)) {
            sendText();
            return true;
        }
    }
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

void MainWindow::refreshShareBtn()
{
    if (!m_shareBtn)
        return;
    if (m_http && !m_http->shareDir().isEmpty())
        m_shareBtn->setText(QString::fromUtf8(u8"共享中…"));
    else
        m_shareBtn->setText(QString::fromUtf8(u8"网页共享 (HTTP)"));
}

void MainWindow::openShare()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"网页共享"));
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setMinimumWidth(420);

    QLabel *hint = new QLabel(QString::fromUtf8(
        u8"选一个本机目录，同网段用浏览器打开下面的地址即可下载文件（只列顶层文件）。"));
    hint->setWordWrap(true);

    QLineEdit *dirEdit = new QLineEdit(m_http ? m_http->shareDir() : QString());
    dirEdit->setPlaceholderText(QString::fromUtf8(u8"尚未选择共享目录"));
    dirEdit->setReadOnly(true);
    QPushButton *pick = new QPushButton(QString::fromUtf8(u8"选择目录"));
    connect(pick, &QPushButton::clicked, &dlg, [dirEdit, &dlg]() {
        const QString picked = QFileDialog::getExistingDirectory(
            &dlg, QString::fromUtf8(u8"选择要共享的目录"), dirEdit->text());
        if (!picked.isEmpty())
            dirEdit->setText(picked);
    });

    const QString url = QStringLiteral("http://%1:%2/share/")
                            .arg(localIpText())
                            .arg(m_settings.port);
    QLineEdit *urlEdit = new QLineEdit(url);
    urlEdit->setReadOnly(true);

    QPushButton *copy = new QPushButton(QString::fromUtf8(u8"复制链接"));
    connect(copy, &QPushButton::clicked, &dlg, [urlEdit]() {
        QApplication::clipboard()->setText(urlEdit->text());
    });

    QPushButton *start = new QPushButton(QString::fromUtf8(u8"开启共享"));
    QPushButton *stop = new QPushButton(QString::fromUtf8(u8"停止共享"));
    QPushButton *close = new QPushButton(QString::fromUtf8(u8"关闭"));
    connect(start, &QPushButton::clicked, &dlg, [this, dirEdit, &dlg]() {
        const QString dir = dirEdit->text().trimmed();
        if (dir.isEmpty() || !QDir(dir).exists()) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"请先选择有效目录"));
            return;
        }
        m_http->setShareDir(dir);
        refreshShareBtn();
        QMessageBox::information(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"已开启网页共享。"));
    });
    connect(stop, &QPushButton::clicked, &dlg, [this]() {
        m_http->setShareDir(QString());
        refreshShareBtn();
    });
    connect(close, SIGNAL(clicked()), &dlg, SLOT(accept()));

    QHBoxLayout *dirRow = new QHBoxLayout;
    dirRow->addWidget(dirEdit, 1);
    dirRow->addWidget(pick);
    QHBoxLayout *urlRow = new QHBoxLayout;
    urlRow->addWidget(urlEdit, 1);
    urlRow->addWidget(copy);
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(start);
    btns->addWidget(stop);
    btns->addWidget(close);

    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addWidget(hint);
    lay->addLayout(dirRow);
    lay->addWidget(new QLabel(QString::fromUtf8(u8"访问地址")));
    lay->addLayout(urlRow);
    lay->addLayout(btns);
    dlg.exec();
    refreshShareBtn();
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
    if (m_statusDot)
        m_statusDot->setPixmap(makeStatusDot(ok));
    if (m_statusLabel)
        m_statusLabel->setText(text);
}

void MainWindow::boot()
{
    m_settings = Settings::load();
    m_id = deviceId();
    QDir().mkpath(m_settings.downloadDir);
    m_disc->setIdentity(m_id, m_settings.deviceName, m_settings.port);
    m_http->setInfo(m_id, m_settings.deviceName, m_settings.port);
    m_http->setDownloadDir(m_settings.downloadDir);
    const QByteArray envShare = qgetenv("LANDROP_SHARE_DIR");
    if (!envShare.isEmpty()) {
        const QString dir = QString::fromLocal8Bit(envShare);
        if (QDir(dir).exists())
            m_http->setShareDir(dir);
    }
    const bool httpOk = m_http->listen(m_settings.port);
    const bool discOk = m_disc->start(m_settings.discoverPort);
    updateHostPill();
    refreshShareBtn();
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
    updateInputPlaceholder();
    if (hasPeer)
        updatePeerSession();
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
        const QString osTag = p.osName.trimmed().isEmpty()
            ? QString()
            : (QStringLiteral("  ·  ") + p.osName);
        QListWidgetItem *it = new QListWidgetItem(
            QStringLiteral("%1\n%2:%3  %4%5%6")
                .arg(p.label()).arg(p.ip).arg(p.port).arg(flag).arg(manual).arg(osTag));
        it->setIcon(QIcon(makePeerAvatar(p.label(), p.osName, 44)));
        it->setSizeHint(QSize(0, 60));
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

void MainWindow::setSessionTab(int index)
{
    if (!m_sessionStack)
        return;
    const int i = (index == 1) ? 1 : 0;
    m_sessionStack->setCurrentIndex(i);
    if (m_tabChat) {
        m_tabChat->setObjectName(i == 0 ? QStringLiteral("sessionTabActive")
                                          : QStringLiteral("sessionTab"));
        m_tabChat->style()->unpolish(m_tabChat);
        m_tabChat->style()->polish(m_tabChat);
        m_tabChat->update();
    }
    if (m_tabFiles) {
        m_tabFiles->setObjectName(i == 1 ? QStringLiteral("sessionTabActive")
                                           : QStringLiteral("sessionTab"));
        m_tabFiles->style()->unpolish(m_tabFiles);
        m_tabFiles->style()->polish(m_tabFiles);
        m_tabFiles->update();
    }
}

void MainWindow::showChatTab()
{
    setSessionTab(0);
}

void MainWindow::showFilesTab()
{
    setSessionTab(1);
}

void MainWindow::updatePeerSession()
{
    if (!m_peerHeader || !m_connBanner)
        return;
    QString ip;
    int port = 0;
    QString name;
    if (!currentPeer(&ip, &port, &name))
        return;

    Peer peer;
    const bool found = m_disc && m_disc->find(ip, port, &peer);
    if (!found) {
        peer.ip = ip;
        peer.port = port;
        peer.name = name;
    }
    const QString label = peer.label().isEmpty() ? name : peer.label();
    const QString addr = QStringLiteral("%1:%2").arg(ip).arg(port);
    const QString osTag = peer.osName.trimmed().isEmpty()
        ? QString::fromUtf8(u8"未知系统")
        : peer.osName.trimmed();

    m_peerAvatar->setPixmap(makePeerAvatar(label, peer.osName, 44));
    m_peerName->setText(label);
    m_peerOnlineDot->setPixmap(makeStatusDot(found ? peer.online() : true, 8));
    m_peerAddr->setText(addr);
    m_peerMeta->setText(QString::fromUtf8(u8"%1  ·  Ping —  ·  链路 —").arg(osTag));
    m_connBannerText->setText(
        QString::fromUtf8(u8"已建立局域网直连：%1 (%2)").arg(label).arg(addr));
    if (m_tabFiles)
        m_tabFiles->setText(QString::fromUtf8(u8"文件传输 (0)"));
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
    updatePeerSession();
    updateInputPlaceholder();
}

void MainWindow::setProgress(const QString &text)
{
    if (!m_progress)
        return;
    if (text.isEmpty()) {
        m_progress->clear();
        m_progress->hide();
        return;
    }
    m_progress->setText(text);
    m_progress->show();
}

void MainWindow::noteFail(const QString &key, QNetworkReply *rep)
{
    const int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString why = QString::fromUtf8(rep->readAll()).trimmed();
    if (why.isEmpty())
        why = rep->errorString();
    if (code >= 400)
        why = QString::number(code) + QLatin1Char(' ') + why;
    note(key, QString::fromUtf8(u8"发送失败：%1").arg(why));
    setProgress(QString());
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
    const QString key = ip + QLatin1Char(':') + QString::number(port);
    if (text == QLatin1String("__landrop_nudge__")) {
        const QString who = fromName.trimmed().isEmpty() ? ip : fromName.trimmed();
        note(key, QString::fromUtf8(u8"%1 抖动了窗口").arg(who));
        shakeWindow();
        return;
    }
    const QString who = fromName.trimmed().isEmpty() ? ip : fromName.trimmed();
    note(key, QString::fromUtf8(u8"%1：%2").arg(who).arg(text));
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
    if (!m_input)
        return;
    const QString text = m_input->toPlainText();
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
        if (rep->error() != QNetworkReply::NoError) {
            noteFail(key, rep);
            return;
        }
        note(key, QString::fromUtf8(u8"%1：%2").arg(mine).arg(sent));
        m_input->clear();
    });
}

void MainWindow::startUpload(const QString &path, bool fromQueue)
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0)) {
        m_uploading = false;
        m_uploadQueue.clear();
        setProgress(QString());
        return;
    }
    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        note(currentKey(), QString::fromUtf8(u8"发送失败：打不开 %1").arg(QFileInfo(path).fileName()));
        if (fromQueue)
            pumpUploadQueue();
        else {
            m_uploading = false;
            setProgress(QString());
        }
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
    m_uploading = true;
    setProgress(QString::fromUtf8(u8"正在发送 %1").arg(filename));
    connect(rep, &QNetworkReply::uploadProgress, this, [this, filename](qint64 sent, qint64 total) {
        if (total <= 0)
            return;
        const int pct = int(sent * 100 / total);
        setProgress(QString::fromUtf8(u8"正在发送 %1  %2%").arg(filename).arg(pct));
    });
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, filename, fromQueue]() {
        rep->deleteLater();
        const int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (rep->error() != QNetworkReply::NoError || code >= 300) {
            noteFail(key, rep);
            m_uploadQueue.clear();
            m_uploading = false;
            return;
        }
        note(key, QString::fromUtf8(u8"已发送文件 %1").arg(filename));
        if (fromQueue)
            pumpUploadQueue();
        else {
            m_uploading = false;
            setProgress(QString());
        }
    });
}

void MainWindow::pumpUploadQueue()
{
    if (m_uploadQueue.isEmpty()) {
        m_uploading = false;
        setProgress(QString());
        return;
    }
    const QString path = m_uploadQueue.takeFirst();
    startUpload(path, true);
}

void MainWindow::sendFile()
{
    if (m_uploading) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"正在发送中，请稍候。"));
        return;
    }
    if (!currentPeer(0, 0, 0))
        return;
    const QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8(u8"选择要发送的文件"));
    if (path.isEmpty())
        return;
    startUpload(path, false);
}

void MainWindow::sendFolder()
{
    if (m_uploading) {
        QMessageBox::information(this, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"正在发送中，请稍候。"));
        return;
    }
    if (!currentPeer(0, 0, 0))
        return;
    const QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8(u8"选择要发送的文件夹"));
    if (dir.isEmpty())
        return;
    const QFileInfoList files = QDir(dir).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    if (files.isEmpty()) {
        note(currentKey(), QString::fromUtf8(u8"文件夹为空，没有可发送的文件"));
        return;
    }
    m_uploadQueue.clear();
    for (int i = 0; i < files.size(); ++i)
        m_uploadQueue.append(files.at(i).absoluteFilePath());
    note(currentKey(), QString::fromUtf8(u8"开始发送文件夹（%1 个文件）").arg(m_uploadQueue.size()));
    pumpUploadQueue();
}

void MainWindow::nudgePeer()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0))
        return;
    QJsonObject o;
    o.insert(QStringLiteral("fromId"), m_id);
    o.insert(QStringLiteral("fromName"), m_settings.deviceName);
    o.insert(QStringLiteral("fromPort"), m_settings.port);
    o.insert(QStringLiteral("text"), QStringLiteral("__landrop_nudge__"));
    const QByteArray body = QJsonDocument(o).toJson(QJsonDocument::Compact);
    QNetworkRequest req(QUrl(QStringLiteral("http://%1:%2/api/inbox").arg(ip).arg(port)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/json; charset=utf-8")));
    QNetworkReply *rep = m_nam->post(req, body);
    const QString key = currentKey();
    connect(rep, &QNetworkReply::finished, this, [this, rep, key]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError) {
            noteFail(key, rep);
            return;
        }
        note(key, QString::fromUtf8(u8"已发送窗口抖动"));
        shakeWindow();
    });
}

void MainWindow::shakeWindow()
{
    const QPoint origin = pos();
    QTimer *t = new QTimer(this);
    t->setInterval(28);
    QObject *guard = new QObject(t);
    guard->setProperty("step", 0);
    connect(t, &QTimer::timeout, this, [this, t, origin, guard]() {
        static const int offs[] = {12, -12, 9, -9, 6, -6, 3, -3, 0};
        const int step = guard->property("step").toInt();
        if (step >= 9) {
            move(origin);
            t->stop();
            t->deleteLater();
            return;
        }
        move(origin + QPoint(offs[step], 0));
        guard->setProperty("step", step + 1);
    });
    t->start();
}

void MainWindow::updateInputPlaceholder()
{
    if (!m_input)
        return;
    QString name;
    if (!currentPeer(0, 0, &name) || name.trimmed().isEmpty())
        m_input->setPlaceholderText(QString::fromUtf8(u8"输入文字或命令…"));
    else
        m_input->setPlaceholderText(QString::fromUtf8(u8"向 %1 发送消息...").arg(name));
}

void MainWindow::addPeer()
{
    QDialog dlg(this);
    dlg.setObjectName(QStringLiteral("addPeerDlg"));
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setAttribute(Qt::WA_TranslucentBackground, true);
    dlg.setModal(true);
    dlg.setFixedWidth(460);
    dlg.setStyleSheet(QStringLiteral(
        "#addPeerDlg { background: transparent; }"
        "#addPeerRoot { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; }"
        "#addPeerHead { background: #f8fafc; border-bottom: 1px solid #e2e8f0;"
        " border-top-left-radius: 16px; border-top-right-radius: 16px; }"
        "#addPeerTitle { color: #0f172a; font-size: 14px; font-weight: 700; }"
        "#addPeerSub { color: #64748b; font-size: 11px; }"
        "#addPeerClose { background: transparent; border: none; border-radius: 6px; padding: 0; color: #94a3b8; }"
        "#addPeerClose:hover { background: #e2e8f0; color: #334155; }"
        "#addPeerLabel { color: #334155; font-size: 12px; font-weight: 600; }"
        "#addPeerField { background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px;"
        " padding: 8px 10px; color: #0f172a; selection-background-color: #bfdbfe; }"
        "#addPeerField:focus { background: #ffffff; border-color: #3b82f6; }"
        "#addPeerProbe { background: transparent; border: none; color: #2563eb; font-size: 12px;"
        " font-weight: 600; text-align: left; padding: 0; }"
        "#addPeerProbe:hover { color: #1d4ed8; }"
        "#addPeerProbe:disabled { color: #93c5fd; }"
        "#addPeerProbeResult { color: #64748b; font-size: 11px; }"
        "#addPeerFoot { border-top: 1px solid #e2e8f0; }"
        "#addPeerCancel { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px;"
        " color: #475569; padding: 8px 14px; }"
        "#addPeerCancel:hover { background: #f8fafc; }"
        "#addPeerOk { background: #2563eb; border: none; border-radius: 8px; color: #ffffff;"
        " padding: 8px 16px; font-weight: 600; }"
        "#addPeerOk:hover { background: #1d4ed8; }"));

    QWidget *root = new QWidget(&dlg);
    root->setObjectName(QStringLiteral("addPeerRoot"));
    QVBoxLayout *dlgLay = new QVBoxLayout(&dlg);
    dlgLay->setContentsMargins(0, 0, 0, 0);
    dlgLay->addWidget(root);
    QVBoxLayout *rootLay = new QVBoxLayout(root);
    rootLay->setContentsMargins(0, 0, 0, 0);
    rootLay->setSpacing(0);

    QWidget *head = new QWidget;
    head->setObjectName(QStringLiteral("addPeerHead"));
    QHBoxLayout *headLay = new QHBoxLayout(head);
    headLay->setContentsMargins(20, 14, 12, 14);
    headLay->setSpacing(10);

    QLabel *globeBg = new QLabel;
    globeBg->setFixedSize(36, 36);
    globeBg->setPixmap(makeGlobeBadge(36));

    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->setContentsMargins(0, 0, 0, 0);
    titleCol->setSpacing(2);
    QLabel *title = new QLabel(QString::fromUtf8(u8"跨网段直连 / 手动添加节点"));
    title->setObjectName(QStringLiteral("addPeerTitle"));
    QLabel *sub = new QLabel(QString::fromUtf8(u8"当设备处于不同 VLAN、VPN 或禁用了 mDNS 广播时使用"));
    sub->setObjectName(QStringLiteral("addPeerSub"));
    sub->setWordWrap(true);
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    QPushButton *closeBtn = new QPushButton;
    closeBtn->setObjectName(QStringLiteral("addPeerClose"));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setIcon(makeChromeIcon(IconClose, QColor(QStringLiteral("#94a3b8"))));
    closeBtn->setIconSize(QSize(14, 14));
    connect(closeBtn, SIGNAL(clicked()), &dlg, SLOT(reject()));

    headLay->addWidget(globeBg, 0, Qt::AlignVCenter);
    headLay->addLayout(titleCol, 1);
    headLay->addWidget(closeBtn, 0, Qt::AlignTop);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLay = new QVBoxLayout(body);
    bodyLay->setContentsMargins(20, 16, 20, 12);
    bodyLay->setSpacing(12);

    auto fieldLabel = [](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setObjectName(QStringLiteral("addPeerLabel"));
        return l;
    };
    auto fieldEdit = [](const QString &text = QString(), const QString &ph = QString()) {
        QLineEdit *e = new QLineEdit(text);
        e->setObjectName(QStringLiteral("addPeerField"));
        if (!ph.isEmpty())
            e->setPlaceholderText(ph);
        return e;
    };

    QHBoxLayout *rowIp = new QHBoxLayout;
    rowIp->setSpacing(10);
    QVBoxLayout *ipCol = new QVBoxLayout;
    ipCol->setSpacing(4);
    QLineEdit *ip = fieldEdit(QString(), QStringLiteral("192.168.1.10"));
    ipCol->addWidget(fieldLabel(QString::fromUtf8(u8"目标 IP 地址")));
    ipCol->addWidget(ip);
    QVBoxLayout *portCol = new QVBoxLayout;
    portCol->setSpacing(4);
    QLineEdit *port = fieldEdit(QStringLiteral("8848"));
    port->setFixedWidth(110);
    portCol->addWidget(fieldLabel(QString::fromUtf8(u8"服务端口")));
    portCol->addWidget(port);
    rowIp->addLayout(ipCol, 1);
    rowIp->addLayout(portCol, 0);

    QVBoxLayout *aliasCol = new QVBoxLayout;
    aliasCol->setSpacing(4);
    QLineEdit *alias = fieldEdit(QString(), QString::fromUtf8(u8"例如：跨网段工控机"));
    aliasCol->addWidget(fieldLabel(QString::fromUtf8(u8"设备别名")));
    aliasCol->addWidget(alias);

    QHBoxLayout *rowOs = new QHBoxLayout;
    rowOs->setSpacing(10);
    QVBoxLayout *osCol = new QVBoxLayout;
    osCol->setSpacing(4);
    QComboBox *osBox = new QComboBox;
    osBox->setObjectName(QStringLiteral("addPeerField"));
    osBox->addItem(QStringLiteral("Windows PC"), QStringLiteral("windows"));
    osBox->addItem(QString::fromUtf8(u8"Linux (Ubuntu / 麒麟)"), QStringLiteral("linux"));
    osBox->addItem(QString::fromUtf8(u8"ARM64 Linux (工控/树莓派)"), QStringLiteral("arm-linux"));
    osBox->addItem(QString::fromUtf8(u8"iOS / Android 手机"), QStringLiteral("ios"));
    osBox->setCurrentIndex(2);
    osCol->addWidget(fieldLabel(QString::fromUtf8(u8"系统类型")));
    osCol->addWidget(osBox);
    QVBoxLayout *tagCol = new QVBoxLayout;
    tagCol->setSpacing(4);
    QLineEdit *tag = fieldEdit(QString(), QString::fromUtf8(u8"可选，本轮不入库"));
    tagCol->addWidget(fieldLabel(QString::fromUtf8(u8"部门 / 标签")));
    tagCol->addWidget(tag);
    rowOs->addLayout(osCol, 1);
    rowOs->addLayout(tagCol, 1);

    QHBoxLayout *probeRow = new QHBoxLayout;
    probeRow->setSpacing(8);
    QPushButton *probeBtn = new QPushButton(QString::fromUtf8(u8"测试目标端口连通性 (Ping /api/info)"));
    probeBtn->setObjectName(QStringLiteral("addPeerProbe"));
    probeBtn->setCursor(Qt::PointingHandCursor);
    probeBtn->setIcon(QIcon(renderSvgIcon(QStringLiteral(":/icons/zap.svg"), 14)));
    probeBtn->setIconSize(QSize(14, 14));
    probeBtn->setFocusPolicy(Qt::NoFocus);
    QLabel *probeResult = new QLabel;
    probeResult->setObjectName(QStringLiteral("addPeerProbeResult"));
    probeRow->addWidget(probeBtn, 0, Qt::AlignVCenter);
    probeRow->addWidget(probeResult, 1, Qt::AlignVCenter);

    bodyLay->addLayout(rowIp);
    bodyLay->addLayout(aliasCol);
    bodyLay->addLayout(rowOs);
    bodyLay->addLayout(probeRow);

    QWidget *foot = new QWidget;
    foot->setObjectName(QStringLiteral("addPeerFoot"));
    QHBoxLayout *footLay = new QHBoxLayout(foot);
    footLay->setContentsMargins(20, 12, 20, 16);
    footLay->setSpacing(8);
    QPushButton *cancel = new QPushButton(QString::fromUtf8(u8"取消"));
    cancel->setObjectName(QStringLiteral("addPeerCancel"));
    cancel->setCursor(Qt::PointingHandCursor);
    QPushButton *ok = new QPushButton(QString::fromUtf8(u8"+ 添加并连接"));
    ok->setObjectName(QStringLiteral("addPeerOk"));
    ok->setCursor(Qt::PointingHandCursor);
    ok->setDefault(true);
    footLay->addStretch(1);
    footLay->addWidget(cancel);
    footLay->addWidget(ok);

    rootLay->addWidget(head);
    rootLay->addWidget(body);
    rootLay->addWidget(foot);

    connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
    connect(probeBtn, &QPushButton::clicked, &dlg, [&]() {
        const QString host = ip->text().trimmed();
        bool portOk = false;
        const int p = port->text().trimmed().toInt(&portOk);
        if (host.isEmpty() || !portOk || p < 1 || p > 65535) {
            probeResult->setText(QString::fromUtf8(u8"请先填写有效的 IP 与端口"));
            return;
        }
        probeBtn->setEnabled(false);
        probeResult->setText(QString::fromUtf8(u8"正在探测…"));
        QElapsedTimer *timer = new QElapsedTimer;
        timer->start();
        QNetworkReply *rep = m_nam->get(
            QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(host).arg(p))));
        connect(rep, &QNetworkReply::finished, &dlg, [=]() {
            rep->deleteLater();
            const qint64 ms = timer->elapsed();
            delete timer;
            probeBtn->setEnabled(true);
            if (rep->error() != QNetworkReply::NoError) {
                probeResult->setText(QString::fromUtf8(u8"不通：%1").arg(rep->errorString()));
                return;
            }
            const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
            const QString name = o.value(QStringLiteral("name")).toString();
            probeResult->setText(QString::fromUtf8(u8"连通 %1 ms%2")
                                     .arg(ms)
                                     .arg(name.isEmpty() ? QString()
                                                         : (QStringLiteral(" · ") + name)));
        });
    });
    connect(ok, &QPushButton::clicked, &dlg, [&]() {
        const QString host = ip->text().trimmed();
        bool portOk = false;
        const int p = port->text().trimmed().toInt(&portOk);
        if (host.isEmpty() || host.contains(QLatin1Char(' ')) || !portOk || p < 1 || p > 65535) {
            QMessageBox::warning(&dlg, QString::fromUtf8(u8"局域快传"),
                                 QString::fromUtf8(u8"IP 或端口无效"));
            return;
        }
        Q_UNUSED(tag);
        const QString osName = osBox->currentData().toString();
        m_disc->addManual(host, p, alias->text(), osName);
        refreshPeers();
        for (int i = 0; i < m_list->count(); ++i) {
            QListWidgetItem *it = m_list->item(i);
            if (it->data(Qt::UserRole).toString() == host
                && it->data(Qt::UserRole + 1).toInt() == p) {
                m_list->setCurrentRow(i);
                break;
            }
        }
        probePeer();
        dlg.accept();
    });

    dlg.exec();
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
