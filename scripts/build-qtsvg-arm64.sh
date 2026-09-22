#!/bin/sh
set -eu
SRC=/tmp/qtsvg-5.14.2
BUILD=/tmp/qtsvg-build
ARM=/opt/Qt5.14.2-arm64
HOST=/opt/Qt5.14.2-host
HDR=/tmp/include/QtSvg

if [ ! -d "$SRC" ]; then
  cd /tmp
  curl -L --fail -o qtsvg-5.14.2.tar.gz https://github.com/qt/qtsvg/archive/refs/tags/v5.14.2.tar.gz
  tar xf qtsvg-5.14.2.tar.gz
fi

if [ ! -d "$HDR" ]; then
  perl "$HOST/bin/syncqt.pl" -version 5.14.2 "$SRC"
fi

mkdir -p "$ARM/include/QtSvg"
cp -a "$HDR/." "$ARM/include/QtSvg/"
# syncqt 生成的头可能是相对链接，再拷一份实文件兜底
cp -a "$SRC"/src/svg/*.h "$ARM/include/QtSvg/" 2>/dev/null || true

rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"
# 让构建能找到刚同步的头
export CPLUS_INCLUDE_PATH="$ARM/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
"$HOST/bin/qmake" "$SRC" -spec linux-aarch64-gnu-g++ \
  "INCLUDEPATH+=$ARM/include" "INCLUDEPATH+=$ARM/include/QtSvg"
make -j"$(nproc)"

SO=$(find "$BUILD" -name 'libQt5Svg.so.5.14.2' | head -1)
test -n "$SO"
cp -a "$(dirname "$SO")"/libQt5Svg.so* "$ARM/lib/"

cat > "$HOST/mkspecs/modules/qt_lib_svg.pri" << 'EOF'
QT.svg.VERSION = 5.14.2
QT.svg.name = QtSvg
QT.svg.module = Qt5Svg
QT.svg.libs = $$QT_MODULE_LIB_BASE
QT.svg.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtSvg
QT.svg.frameworks =
QT.svg.bins = $$QT_MODULE_BIN_BASE
QT.svg.depends = core gui
QT.svg.uses =
QT.svg.module_config = v2
QT.svg.DEFINES = QT_SVG_LIB
QT_MODULES += svg
EOF
mkdir -p "$ARM/mkspecs/modules"
cp "$HOST/mkspecs/modules/qt_lib_svg.pri" "$ARM/mkspecs/modules/qt_lib_svg.pri"

ls -la "$ARM/lib"/libQt5Svg.so*
echo "QtSvg OK"
