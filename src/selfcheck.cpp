#include "selfcheck.h"

#include "files.h"

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
    if (safeFileName(QStringLiteral("说明.txt")) != QStringLiteral("说明.txt"))
        return fail("chinese name");
    if (!isVirtualIfaceName(QStringLiteral("vEthernet (WSL)")))
        return fail("virtual nic");
    if (isVirtualIfaceName(QStringLiteral("以太网")))
        return fail("real nic");
    if (broadcastAddress(QStringLiteral("10.0.0.5"), QStringLiteral("255.255.255.0")) != QLatin1String("10.0.0.255"))
        return fail("broadcast");
    if (!broadcastAddress(QStringLiteral("10.0.0.1"), QStringLiteral("255.255.255.255")).isEmpty())
        return fail("host route");
    std::printf("self-check ok\n");
    return 0;
}
