#include "mainwindow.h"
#include "selfcheck.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <stdio.h>
#include <windows.h>
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT) - 4)
#endif
#endif

static void useChineseFont(QApplication &app)
{
    const QStringList prefer = QStringList()
        << QStringLiteral("Microsoft YaHei")
        << QString::fromUtf8(u8"微软雅黑")
        << QStringLiteral("Noto Sans CJK SC")
        << QStringLiteral("Source Han Sans SC")
        << QStringLiteral("WenQuanYi Micro Hei")
        << QString::fromUtf8(u8"文泉驿微米黑");
    const QStringList fams = QFontDatabase().families();
    for (int i = 0; i < prefer.size(); ++i) {
        if (fams.contains(prefer.at(i))) {
            app.setFont(QFont(prefer.at(i), 10));
            return;
        }
    }
}

#ifdef Q_OS_WIN
// 主副屏缩放不同时，未声明 Per-Monitor V2 会被系统位图拉伸 → 发虚
static void enablePerMonitorDpiV2()
{
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32)
        return;
    typedef BOOL(WINAPI *SetCtxFn)(DPI_AWARENESS_CONTEXT);
    SetCtxFn setCtx = reinterpret_cast<SetCtxFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setCtx)
        setCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}
#endif

static void applyHighDpiAttrs()
{
#ifdef Q_OS_WIN
    enablePerMonitorDpiV2();
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    // 保留 1.25 等分数倍率，避免被四舍五入成 1 导致位图发虚
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--self-check") == 0) {
#ifdef Q_OS_WIN
            AttachConsole(ATTACH_PARENT_PROCESS);
            FILE *out = 0;
            freopen_s(&out, "CONOUT$", "w", stdout);
            freopen_s(&out, "CONOUT$", "w", stderr);
#endif
            // 聊天气泡渲染依赖 qApp（字体/DPR），自检也要有 Application
            applyHighDpiAttrs();
            QApplication app(argc, argv);
            return runSelfCheck();
        }
    }

    applyHighDpiAttrs();
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("lan-drop"));
    QApplication::setOrganizationName(QStringLiteral("lan-drop"));

    QTranslator qtLang;
    const QString trPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if (qtLang.load(QStringLiteral("qt_zh_CN"), trPath))
        app.installTranslator(&qtLang);
    useChineseFont(app);
    QApplication::setStyle(QStringLiteral("Fusion"));
    // 关主窗默认进托盘，不随最后窗口关闭而退出
    QApplication::setQuitOnLastWindowClosed(false);

    MainWindow w;
    w.show();
    return app.exec();
}
