QT += widgets network
CONFIG += c++11
TEMPLATE = app
TARGET = landrop

SOURCES += \
    src/main.cpp \
    src/settings.cpp \
    src/discovery.cpp \
    src/httpserver.cpp \
    src/files.cpp \
    src/mainwindow.cpp \
    src/selfcheck.cpp

HEADERS += \
    src/settings.h \
    src/discovery.h \
    src/httpserver.h \
    src/files.h \
    src/mainwindow.h \
    src/selfcheck.h

win32: QMAKE_CXXFLAGS += /utf-8

# 安装包把 Qt 库放在可执行文件旁的 lib 里，启动脚本也会设置 LD_LIBRARY_PATH
unix: QMAKE_RPATHDIR += \$\$ORIGIN/lib
