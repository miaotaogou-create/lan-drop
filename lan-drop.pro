QT += widgets network svg
CONFIG += c++11
TEMPLATE = app
TARGET = landrop

SOURCES += \
    src/main.cpp \
    src/settings.cpp \
    src/discovery.cpp \
    src/httpserver.cpp \
    src/files.cpp \
    src/fmtutil.cpp \
    src/uiicons.cpp \
    src/chatrender.cpp \
    src/ziputil.cpp \
    src/qrcodegen.cpp \
    src/windowchrome.cpp \
    src/mainwindow.cpp \
    src/selfcheck.cpp

HEADERS += \
    src/settings.h \
    src/discovery.h \
    src/httpserver.h \
    src/files.h \
    src/fmtutil.h \
    src/uiicons.h \
    src/chatmsg.h \
    src/chatrender.h \
    src/ziputil.h \
    src/qrcodegen.hpp \
    src/windowchrome.h \
    src/mainwindow.h \
    src/selfcheck.h

RESOURCES += icons/icons.qrc

# MSVC 没有 BOM 时容易误判 UTF-8 源文件；另：不要对中文用 QStringLiteral，
# 它会展开成 u"" "中文"，在 VS2017 上拼接会把字编坏，请写 QString::fromUtf8(u8"...")。
win32 {
    LIBS += -liphlpapi -lws2_32 -luser32 -lwinmm
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CXXFLAGS += /wd4819
}

# 安装包把 Qt 库放在可执行文件旁的 lib 里，启动脚本也会设置 LD_LIBRARY_PATH
unix: QMAKE_RPATHDIR += \$\$ORIGIN/lib
