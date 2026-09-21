#include "mainwindow.h"

#include "discovery.h"
#include "httpserver.h"

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("局域快传"));
    resize(1080, 680);

    m_list = new QListWidget;
    m_chat = new QTextEdit;
    m_chat->setReadOnly(true);
    m_input = new QLineEdit;
    m_input->setPlaceholderText(QStringLiteral("输入文字或命令，回车发送"));

    QPushButton *addBtn = new QPushButton(QStringLiteral("添加节点"));
    QPushButton *probeBtn = new QPushButton(QStringLiteral("探测"));
    QPushButton *setBtn = new QPushButton(QStringLiteral("设置"));
    QPushButton *fileBtn = new QPushButton(QStringLiteral("发送文件"));
    QPushButton *sendBtn = new QPushButton(QStringLiteral("发送"));

    QWidget *left = new QWidget;
    QVBoxLayout *leftLay = new QVBoxLayout(left);
    leftLay->addWidget(new QLabel(QStringLiteral("局域网设备")));
    leftLay->addWidget(m_list, 1);
    QHBoxLayout *leftBtns = new QHBoxLayout;
    leftBtns->addWidget(addBtn);
    leftBtns->addWidget(probeBtn);
    leftBtns->addWidget(setBtn);
    leftLay->addLayout(leftBtns);

    QWidget *right = new QWidget;
    QVBoxLayout *rightLay = new QVBoxLayout(right);
    rightLay->addWidget(m_chat, 1);
    QHBoxLayout *row = new QHBoxLayout;
    row->addWidget(m_input, 1);
    row->addWidget(fileBtn);
    row->addWidget(sendBtn);
    rightLay->addLayout(row);

    QSplitter *split = new QSplitter;
    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(1, 1);
    left->setMinimumWidth(260);
    setCentralWidget(split);

    connect(addBtn, SIGNAL(clicked()), this, SLOT(addPeer()));
    connect(probeBtn, SIGNAL(clicked()), this, SLOT(probePeer()));
    connect(setBtn, SIGNAL(clicked()), this, SLOT(editSettings()));
    connect(fileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
    connect(sendBtn, SIGNAL(clicked()), this, SLOT(sendText()));
    connect(m_input, SIGNAL(returnPressed()), this, SLOT(sendText()));
    connect(m_list, SIGNAL(currentRowChanged(int)), this, SLOT(showChat()));

    m_disc = new Discovery(this);
    m_http = new HttpServer(this);
    m_nam = new QNetworkAccessManager(this);
    connect(m_disc, SIGNAL(changed()), this, SLOT(refreshPeers()));
    connect(m_http, SIGNAL(textArrived(QString,QString,QString,int,QString)),
            this, SLOT(onText(QString,QString,QString,int,QString)));
    connect(m_http, SIGNAL(fileArrived(QString,QString,QString,qint64)),
            this, SLOT(onFile(QString,QString,QString,qint64)));

    QTimer *tick = new QTimer(this);
    connect(tick, SIGNAL(timeout()), this, SLOT(refreshPeers()));
    tick->start(1000);
    boot();
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
    setWindowTitle(QStringLiteral("局域快传 - %1").arg(m_settings.deviceName));
    QString st = QStringLiteral("本机 %1    传输 %2    发现 %3    下载 %4")
                     .arg(m_settings.deviceName)
                     .arg(m_settings.port)
                     .arg(m_settings.discoverPort)
                     .arg(m_settings.downloadDir);
    if (!httpOk)
        st += QStringLiteral("    传输端口占用");
    if (!discOk)
        st += QStringLiteral("    发现端口占用，仍可手动加 IP");
    statusBar()->showMessage(st);
    if (!httpOk)
        QMessageBox::warning(this, QStringLiteral("局域快传"),
                             QStringLiteral("端口 %1 被占用，其他电脑连不上这台机器。请在设置里改端口后重启。").arg(m_settings.port));
    refreshPeers();
    showChat();
}

bool MainWindow::currentPeer(QString *ip, int *port, QString *name) const
{
    QListWidgetItem *it = m_list->currentItem();
    if (!it)
        return false;
    if (ip)
        *ip = it->data(Qt::UserRole).toString();
    if (port)
        *port = it->data(Qt::UserRole + 1).toInt();
    if (name)
        *name = it->text();
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

void MainWindow::refreshPeers()
{
    const QString keep = currentKey();
    m_list->blockSignals(true);
    m_list->clear();
    const QList<Peer> list = m_disc->peers();
    int row = -1;
    for (int i = 0; i < list.size(); ++i) {
        const Peer &p = list.at(i);
        const QString flag = p.online() ? QStringLiteral("在线") : QStringLiteral("离线");
        const QString manual = p.manual ? QStringLiteral("  手动") : QString();
        QListWidgetItem *it = new QListWidgetItem(
            QStringLiteral("%1    %2:%3    %4%5").arg(p.label()).arg(p.ip).arg(p.port).arg(flag).arg(manual));
        it->setData(Qt::UserRole, p.ip);
        it->setData(Qt::UserRole + 1, p.port);
        m_list->addItem(it);
        if (p.key() == keep)
            row = m_list->count() - 1;
    }
    if (row >= 0)
        m_list->setCurrentRow(row);
    else if (m_list->count() > 0 && keep.isEmpty())
        m_list->setCurrentRow(0);
    m_list->blockSignals(false);
    if (row < 0 && !keep.isEmpty())
        showChat();
}

void MainWindow::showChat()
{
    m_chat->setPlainText(m_log.value(currentKey()).join(QStringLiteral("\n")));
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
    const int port = fromPort > 0 ? fromPort : 8848;
    m_disc->touch(ip, port, fromId, fromName, QString());
    const QString who = fromName.trimmed().isEmpty() ? ip : fromName.trimmed();
    note(ip + QLatin1Char(':') + QString::number(port), QStringLiteral("%1：%2").arg(who).arg(text));
}

void MainWindow::onFile(const QString &ip, const QString &name, const QString &path, qint64 size)
{
    Q_UNUSED(path);
    Peer known;
    int port = 8848;
    if (m_disc->find(ip, 8848, &known))
        port = known.port;
    note(ip + QLatin1Char(':') + QString::number(port),
         QStringLiteral("收到文件 %1（%2 字节）").arg(name).arg(size));
    statusBar()->showMessage(QStringLiteral("已保存 %1").arg(name), 5000);
}

void MainWindow::sendText()
{
    const QString text = m_input->text();
    if (text.trimmed().isEmpty())
        return;
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0)) {
        statusBar()->showMessage(QStringLiteral("先选一台设备"));
        return;
    }
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
            statusBar()->showMessage(QStringLiteral("发送失败，对端无响应"));
            return;
        }
        note(key, QStringLiteral("%1：%2").arg(mine).arg(sent));
        m_input->clear();
    });
}

void MainWindow::sendFile()
{
    QString ip;
    int port = 0;
    if (!currentPeer(&ip, &port, 0)) {
        statusBar()->showMessage(QStringLiteral("先选一台设备"));
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择要发送的文件"));
    if (path.isEmpty())
        return;
    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        statusBar()->showMessage(QStringLiteral("打不开文件"));
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
    statusBar()->showMessage(QStringLiteral("正在发送 %1").arg(filename));
    connect(rep, &QNetworkReply::uploadProgress, this, [this, filename](qint64 sent, qint64 total) {
        if (total > 0)
            statusBar()->showMessage(QStringLiteral("正在发送 %1  %2%").arg(filename).arg(sent * 100 / total));
    });
    const QString key = currentKey();
    connect(rep, &QNetworkReply::finished, this, [this, rep, key, filename]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError || rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() >= 300) {
            statusBar()->showMessage(QStringLiteral("发送失败，对端未保存"));
            return;
        }
        note(key, QStringLiteral("已发送文件 %1").arg(filename));
        statusBar()->showMessage(QStringLiteral("已发送 %1").arg(filename), 5000);
    });
}

void MainWindow::addPeer()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("添加节点"));
    QLineEdit *ip = new QLineEdit;
    ip->setPlaceholderText(QStringLiteral("192.168.1.10"));
    QLineEdit *port = new QLineEdit(QStringLiteral("8848"));
    QLineEdit *alias = new QLineEdit;
    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("IP"), ip);
    form->addRow(QStringLiteral("端口"), port);
    form->addRow(QStringLiteral("备注"), alias);
    QPushButton *ok = new QPushButton(QStringLiteral("确定"));
    QPushButton *cancel = new QPushButton(QStringLiteral("取消"));
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
        QMessageBox::warning(this, QStringLiteral("局域快传"), QStringLiteral("IP 或端口无效"));
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
    if (!currentPeer(&ip, &port, 0)) {
        statusBar()->showMessage(QStringLiteral("先选一台设备"));
        return;
    }
    QNetworkReply *rep = m_nam->get(QNetworkRequest(QUrl(QStringLiteral("http://%1:%2/api/info").arg(ip).arg(port))));
    connect(rep, &QNetworkReply::finished, this, [this, rep, ip, port]() {
        rep->deleteLater();
        if (rep->error() != QNetworkReply::NoError) {
            statusBar()->showMessage(QStringLiteral("连不上 %1:%2").arg(ip).arg(port));
            return;
        }
        const QJsonObject o = QJsonDocument::fromJson(rep->readAll()).object();
        m_disc->touch(ip, o.value(QStringLiteral("port")).toInt() > 0 ? o.value(QStringLiteral("port")).toInt() : port,
                      o.value(QStringLiteral("id")).toString(),
                      o.value(QStringLiteral("name")).toString(),
                      o.value(QStringLiteral("os")).toString());
        statusBar()->showMessage(QStringLiteral("已连通 %1").arg(o.value(QStringLiteral("name")).toString()), 4000);
    });
}

void MainWindow::editSettings()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("设置"));
    QLineEdit *name = new QLineEdit(m_settings.deviceName);
    QLineEdit *port = new QLineEdit(QString::number(m_settings.port));
    QLineEdit *dport = new QLineEdit(QString::number(m_settings.discoverPort));
    QLineEdit *dir = new QLineEdit(m_settings.downloadDir);
    QPushButton *browse = new QPushButton(QStringLiteral("浏览"));
    connect(browse, &QPushButton::clicked, &dlg, [dir, &dlg]() {
        const QString picked = QFileDialog::getExistingDirectory(&dlg, QStringLiteral("下载目录"), dir->text());
        if (!picked.isEmpty())
            dir->setText(picked);
    });
    QHBoxLayout *dirRow = new QHBoxLayout;
    dirRow->addWidget(dir, 1);
    dirRow->addWidget(browse);
    QFormLayout *form = new QFormLayout;
    form->addRow(QStringLiteral("本机名称"), name);
    form->addRow(QStringLiteral("传输端口"), port);
    form->addRow(QStringLiteral("发现端口"), dport);
    form->addRow(QStringLiteral("下载目录"), dirRow);
    QPushButton *ok = new QPushButton(QStringLiteral("保存"));
    QPushButton *cancel = new QPushButton(QStringLiteral("取消"));
    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    btns->addWidget(ok);
    btns->addWidget(cancel);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addLayout(form);
    lay->addWidget(new QLabel(QStringLiteral("改端口会马上重新监听。若提示占用，换一个端口。")));
    lay->addLayout(btns);
    connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
    if (dlg.exec() != QDialog::Accepted)
        return;
    bool ok1 = false, ok2 = false;
    const int p = port->text().trimmed().toInt(&ok1);
    const int dp = dport->text().trimmed().toInt(&ok2);
    if (name->text().trimmed().isEmpty() || !ok1 || !ok2 || p < 1 || p > 65535 || dp < 1 || dp > 65535) {
        QMessageBox::warning(this, QStringLiteral("局域快传"), QStringLiteral("名称或端口无效"));
        return;
    }
    m_settings.deviceName = name->text().trimmed();
    m_settings.port = p;
    m_settings.discoverPort = dp;
    m_settings.downloadDir = dir->text().trimmed();
    if (!m_settings.save()) {
        QMessageBox::warning(this, QStringLiteral("局域快传"), QStringLiteral("保存设置失败"));
        return;
    }
    boot();
}
