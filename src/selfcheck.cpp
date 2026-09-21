#include "selfcheck.h"

#include "files.h"
#include "qrcodegen.hpp"

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
    std::printf("self-check ok\n");
    return 0;
}
