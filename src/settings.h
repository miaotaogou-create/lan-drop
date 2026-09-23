#ifndef SETTINGS_H
#define SETTINGS_H

#include <QList>
#include <QString>
#include <QStringList>

// 手动「+ 加 IP」节点；写入 settings.json 的 manualPeers
struct ManualPeerEntry {
    QString ip;
    int port = 8848;
    QString alias;
    QString os;
    QString tag; // 部门 / 标签；空则不写盘
};

// deviceName、port、discoverPort、downloadDir 为既有字段；
// transferThreads / nudgeEnabled / soundNotification 为设置页扩展。
struct Settings {
    QString deviceName;
    int port = 8848;
    int discoverPort = 8850;
    QString downloadDir = QStringLiteral("./downloads");
    int transferThreads = 8; // ponytail: 仅入库；上传仍是单队列，以后再接线程池
    bool nudgeEnabled = true;
    bool soundNotification = true; // 收文字/文件、本机发文件完成时播提示音；关则静音
    bool closeToTray = true; // 关窗进托盘；false=关窗退出
    bool alwaysOnTop = false; // 主窗口置顶
    QString soundFile; // 自定义 wav；空=系统提示音
    QString preferredLocalIp; // 空=自动取 localIpv4 首项
    QList<ManualPeerEntry> manualPeers;
    QStringList pinnedPeers; // 置顶对端 ip:port
    // 窗口几何；windowW/H≤0 表示未记忆，启动用默认尺寸
    int windowX = 0;
    int windowY = 0;
    int windowW = 0;
    int windowH = 0;
    bool windowMaximized = false;
    int sideWidth = 0; // ≤0 表示未记忆，启动用 300
    QString lastPeer; // 最近选中的对端 ip:port；空=未记忆

    static QString filePath();
    static Settings defaults();
    static Settings load();
    static Settings loadFromFile(const QString &path);
    bool save() const;
    bool saveToFile(const QString &path) const;
};

#endif
