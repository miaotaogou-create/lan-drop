#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

// 与原先配置文件字段一致：deviceName、port、discoverPort、downloadDir。
struct Settings {
    QString deviceName;
    int port = 8848;
    int discoverPort = 8850;
    QString downloadDir = QStringLiteral("./downloads");

    static QString filePath();
    static Settings defaults();
    static Settings load();
    bool save() const;
};

#endif
