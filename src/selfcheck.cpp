#include "selfcheck.h"

#include "files.h"
#include "discovery.h"
#include "qrcodegen.hpp"
#include "settings.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <cstdio>

static int fail(const char *msg)
{
    std::fprintf(stderr, "self-check: %s\n", msg);
    return 1;
}

int runSelfCheck()
{
    if (safeFileName(QStringLiteral("../a/b.txt")) != QLatin1String("b.txt"))
        return fail("path escape");
    if (safeFileName(QStringLiteral("..")) == QLatin1String(".."))
        return fail("dotdot");
    if (safeFileName(QString::fromUtf8(u8"说明.txt")) != QString::fromUtf8(u8"说明.txt"))
        return fail("chinese name");
    {
        QDir tmp = QDir::temp();
        const QString root = tmp.filePath(QStringLiteral("landrop-share-check"));
        tmp.mkpath(QStringLiteral("landrop-share-check"));
        QFile f(QDir(root).filePath(QStringLiteral("a.txt")));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return fail("share tmp");
        f.write("x");
        f.close();
        if (resolveSharedFile(root, QStringLiteral("a.txt")).isEmpty())
            return fail("share ok");
        if (!resolveSharedFile(root, QStringLiteral("nope.txt")).isEmpty())
            return fail("share miss");
        QFile::remove(f.fileName());
        QDir().rmdir(root);
    }
    if (!isVirtualIfaceName(QStringLiteral("vEthernet (WSL)")))
        return fail("virtual nic");
    if (isVirtualIfaceName(QString::fromUtf8(u8"以太网")))
        return fail("real nic");
    if (broadcastAddress(QStringLiteral("10.0.0.5"), QStringLiteral("255.255.255.0")) != QLatin1String("10.0.0.255"))
        return fail("broadcast");
    if (!broadcastAddress(QStringLiteral("10.0.0.1"), QStringLiteral("255.255.255.255")).isEmpty())
        return fail("host route");
    if (deviceKindFromOs(QStringLiteral("windows")) != 0)
        return fail("kind laptop");
    if (deviceKindFromOs(QStringLiteral("android")) != 1)
        return fail("kind phone");
    if (deviceKindFromOs(QStringLiteral("iPadOS")) != 2)
        return fail("kind tablet");
    if (formatLinkLabel(2500, false) != QString::fromUtf8(u8"2.5 GbE 网线"))
        return fail("link 2.5");
    if (formatLinkLabel(1000, true) != QString::fromUtf8(u8"千兆 Wi-Fi"))
        return fail("link wifi");
    if (formatLinkLabel(-1, false) != QString::fromUtf8(u8"链路 —"))
        return fail("link unknown");
    {
        const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
            "http://192.168.1.108:8848/share/", qrcodegen::QrCode::Ecc::MEDIUM);
        if (qr.getSize() < 21 || !qr.getModule(0, 0) || !qr.getModule(qr.getSize() - 1, 0))
            return fail("qr");
    }
    {
        QCryptographicHash h(QCryptographicHash::Sha256);
        h.addData("test");
        if (h.result().toHex().left(8) != QByteArray("9f86d081"))
            return fail("sha256");
    }
    {
        QDir tmp = QDir::temp();
        const QString dir = tmp.filePath(QStringLiteral("landrop-copy-check"));
        const QString srcPath = tmp.filePath(QStringLiteral("landrop-copy-src.txt"));
        tmp.mkpath(QStringLiteral("landrop-copy-check"));
        QFile src(srcPath);
        if (!src.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return fail("copy src");
        src.write("hi");
        src.close();
        const QString once = copyFileIntoDir(dir, srcPath);
        if (once.isEmpty() || !QFile::exists(once))
            return fail("copy once");
        const QString twice = copyFileIntoDir(dir, srcPath);
        if (twice.isEmpty() || twice == once || !QFile::exists(twice))
            return fail("copy unique");
        QFile::remove(srcPath);
        QFile::remove(once);
        QFile::remove(twice);
        QDir().rmdir(dir);
    }
    if (localHostName().trimmed().isEmpty())
        return fail("hostname");
    {
        const QString tmpDir = QDir::temp().filePath(QStringLiteral("landrop-settings-check"));
        QDir().mkpath(tmpDir);
        const QString path = QDir(tmpDir).filePath(QStringLiteral("settings.json"));
        QFile::remove(path);
        Settings s = Settings::defaults();
        s.deviceName = QStringLiteral("check-host");
        ManualPeerEntry a;
        a.ip = QStringLiteral("10.9.8.7");
        a.port = 0; // 存盘夹到 8848
        a.alias = QString::fromUtf8(u8"跨网段");
        a.os = QStringLiteral("linux");
        ManualPeerEntry dup = a;
        dup.port = 8848; // 与夹紧后同键，应去重
        s.manualPeers << a << dup;
        if (!s.saveToFile(path))
            return fail("manualPeers save");
        const Settings loaded = Settings::loadFromFile(path);
        if (loaded.manualPeers.size() != 1)
            return fail("manualPeers dedupe");
        const ManualPeerEntry &e = loaded.manualPeers.at(0);
        if (e.ip != QLatin1String("10.9.8.7") || e.port != 8848)
            return fail("manualPeers addr");
        if (e.alias != QString::fromUtf8(u8"跨网段") || e.os != QLatin1String("linux"))
            return fail("manualPeers meta");
        // 窗口几何往返
        {
            Settings g = Settings::defaults();
            g.windowX = 120;
            g.windowY = 80;
            g.windowW = 1000;
            g.windowH = 640;
            g.windowMaximized = true;
            if (!g.saveToFile(path))
                return fail("window geom save");
            const Settings gl = Settings::loadFromFile(path);
            if (gl.windowX != 120 || gl.windowY != 80 || gl.windowW != 1000 || gl.windowH != 640
                || !gl.windowMaximized)
                return fail("window geom load");
        }
        // 侧栏宽度往返
        {
            Settings s2 = Settings::defaults();
            s2.sideWidth = 360;
            if (!s2.saveToFile(path))
                return fail("sideWidth save");
            if (Settings::loadFromFile(path).sideWidth != 360)
                return fail("sideWidth load");
        }
        // lastPeer 往返
        {
            Settings s3 = Settings::defaults();
            s3.lastPeer = QStringLiteral("10.0.0.8:8848");
            if (!s3.saveToFile(path))
                return fail("lastPeer save");
            if (Settings::loadFromFile(path).lastPeer != QLatin1String("10.0.0.8:8848"))
                return fail("lastPeer load");
        }
        // 缺字段也能加载
        {
            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return fail("manualPeers strip");
            f.write("{\"deviceName\":\"x\",\"port\":8848}\n");
            f.close();
        }
        if (!Settings::loadFromFile(path).manualPeers.isEmpty())
            return fail("manualPeers missing");
        if (Settings::loadFromFile(path).windowW != 0)
            return fail("window geom missing");
        if (Settings::loadFromFile(path).sideWidth != 0)
            return fail("sideWidth missing");
        if (!Settings::loadFromFile(path).lastPeer.isEmpty())
            return fail("lastPeer missing");
        QFile::remove(path);
        QDir().rmdir(tmpDir);
    }
    {
        Discovery disc;
        const Peer p = disc.addManual(QStringLiteral("10.1.2.3"), 8848,
                                      QString::fromUtf8(u8"别名"), QStringLiteral("linux"));
        if (!p.manual || p.ip != QLatin1String("10.1.2.3") || p.alias != QString::fromUtf8(u8"别名"))
            return fail("addManual");
        bool found = false;
        const QList<Peer> list = disc.peers();
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).manual && list.at(i).ip == QLatin1String("10.1.2.3")) {
                found = true;
                break;
            }
        }
        if (!found)
            return fail("addManual list");
        // 非手动节点不可删
        disc.touch(QStringLiteral("10.1.2.4"), 8848, QStringLiteral("id-auto"),
                   QStringLiteral("auto"), QStringLiteral("windows"));
        if (disc.removeManual(QStringLiteral("10.1.2.4"), 8848))
            return fail("removeManual auto");
        if (!disc.removeManual(QStringLiteral("10.1.2.3"), 8848))
            return fail("removeManual");
        const QList<Peer> after = disc.peers();
        for (int i = 0; i < after.size(); ++i) {
            if (after.at(i).ip == QLatin1String("10.1.2.3"))
                return fail("removeManual gone");
        }
        // 持久化：删后写盘再读应无该项
        const QString tmpDir = QDir::temp().filePath(QStringLiteral("landrop-rm-manual-check"));
        QDir().mkpath(tmpDir);
        const QString path = QDir(tmpDir).filePath(QStringLiteral("settings.json"));
        Settings s = Settings::defaults();
        ManualPeerEntry keep;
        keep.ip = QStringLiteral("10.9.9.9");
        keep.port = 8848;
        s.manualPeers << keep;
        if (!s.saveToFile(path))
            return fail("removeManual save");
        Settings loaded = Settings::loadFromFile(path);
        // 模拟删除后只留下空
        loaded.manualPeers.clear();
        if (!loaded.saveToFile(path))
            return fail("removeManual clear");
        if (!Settings::loadFromFile(path).manualPeers.isEmpty())
            return fail("removeManual persist");
        QFile::remove(path);
        QDir().rmdir(tmpDir);
    }
    std::printf("self-check ok\n");
    return 0;
}
