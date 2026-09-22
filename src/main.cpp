#include "mainwindow.h"
#include "selfcheck.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QLibraryInfo>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <stdio.h>
#include <windows.h>
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
            return runSelfCheck();
        }
    }

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
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
