#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

// deviceName、port、discoverPort、downloadDir 为既有字段；
// transferThreads / nudgeEnabled / soundNotification 为设置页扩展。
struct Settings {
    QString deviceName;
    int port = 8848;
    int discoverPort = 8850;
    QString downloadDir = QStringLiteral("./downloads");
    int transferThreads = 8; // ponytail: 仅入库；上传仍是单队列，以后再接线程池
    bool nudgeEnabled = true;
    bool soundNotification = true; // 收文字/文件时播系统提示音；关则静音

    static QString filePath();
    static Settings defaults();
    static Settings load();
    bool save() const;
};

#endif
