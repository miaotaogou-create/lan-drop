#!/bin/sh
# 在 WSL Ubuntu 20.04 里交叉编译 ARM64，并打成麒麟可直接解压运行的目录。
# Qt 5.14.2 和 aarch64 工具链已装在这台机器上，glibc 与麒麟 V10 SP1 同为 2.31。
set -eu
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/arm64"
DIST="$ROOT/dist/linux-arm64"
QT_HOST=/opt/Qt5.14.2-host
QT_LIB=/opt/Qt5.14.2-arm64
SYSROOT_LIB=/opt/arm64-rootfs/usr/lib/aarch64-linux-gnu

rm -rf "$BUILD"
mkdir -p "$BUILD" "$DIST/lib" "$DIST/plugins/platforms" "$DIST/plugins/platforminputcontexts" "$DIST/translations"
# 清掉上次打进去的库，避免残留断掉的符号链接
find "$DIST/lib" -mindepth 1 -delete
cd "$BUILD"
"$QT_HOST/bin/qmake" "$ROOT/lan-drop.pro" -spec linux-aarch64-gnu-g++ CONFIG+=release
make -j"$(nproc)"

cp -f "$BUILD/landrop" "$DIST/landrop"
aarch64-linux-gnu-strip "$DIST/landrop" || true

# cp -L：按动态链接器要的 soname 存成普通文件。直接 cp 符号链接，拷到别的机器上会断。
copy_lib() {
    src="$1"
    name=$(basename "$src")
    if [ ! -e "$src" ]; then
        echo "缺少库: $src" >&2
        exit 1
    fi
    cp -L "$src" "$DIST/lib/$name"
}

for so in libQt5Core.so.5 libQt5Gui.so.5 libQt5Widgets.so.5 libQt5Network.so.5 libQt5DBus.so.5 libQt5XcbQpa.so.5 libQt5Svg.so.5; do
    copy_lib "$QT_LIB/lib/$so"
done
# Qt 自己链进来、桌面不一定同版本的库。X11 / fontconfig / freetype 仍用麒麟系统的，中文字体才能被找到。
for so in libpcre2-16.so.0 libglib-2.0.so.0 libpcre.so.3 libpng16.so.16 libz.so.1; do
    copy_lib "$SYSROOT_LIB/$so"
done

cp -L "$QT_LIB/plugins/platforms/libqxcb.so" "$DIST/plugins/platforms/"
for plug in libibusplatforminputcontextplugin.so libcomposeplatforminputcontextplugin.so; do
    if [ -f "$QT_LIB/plugins/platforminputcontexts/$plug" ]; then
        cp -L "$QT_LIB/plugins/platforminputcontexts/$plug" "$DIST/plugins/platforminputcontexts/"
    fi
done

# 翻译文件与平台无关，用本机 Windows 那套 Qt 5.14.2 的 qm，文件对话框才是中文。
QM="/mnt/c/Qt/Qt5.14.2/5.14.2/msvc2017_64/translations/qt_zh_CN.qm"
if [ -f "$QM" ]; then
    cp -f "$QM" "$DIST/translations/qt_zh_CN.qm"
fi

cat > "$DIST/qt.conf" << 'EOF'
[Paths]
Prefix = .
Plugins = plugins
Translations = translations
EOF

cat > "$DIST/landrop.sh" << 'EOF'
#!/bin/sh
DIR=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"

# 自带包只有 ibus/compose；麒麟默认 fcitx。把系统 fcitx Qt 插件挂进包内目录，
# 否则 QT_PLUGIN_PATH 隔离后永远加载不到，文本框无法出中文。
FCITX_DST="$DIR/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so"
if [ ! -e "$FCITX_DST" ]; then
    for cand in \
        /usr/lib/aarch64-linux-gnu/qt5/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so \
        /usr/lib/qt5/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so \
        /usr/lib64/qt5/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so; do
        if [ -f "$cand" ]; then
            ln -sf "$cand" "$FCITX_DST" 2>/dev/null || cp -L "$cand" "$FCITX_DST" 2>/dev/null || true
            break
        fi
    done
fi

# SSH/快捷方式启动常丢桌面 IM 环境；有则不覆盖。
if [ -z "$QT_IM_MODULE" ]; then
    if [ -e "$FCITX_DST" ]; then
        export QT_IM_MODULE=fcitx
    elif [ -f "$DIR/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so" ]; then
        export QT_IM_MODULE=ibus
    fi
fi
if [ -z "$XMODIFIERS" ] && [ "$QT_IM_MODULE" = "fcitx" ]; then
    export XMODIFIERS=@im=fcitx
fi
if [ -z "$GTK_IM_MODULE" ] && [ "$QT_IM_MODULE" = "fcitx" ]; then
    export GTK_IM_MODULE=fcitx
fi

cd "$DIR"
exec "$DIR/landrop" "$@"
EOF
chmod +x "$DIST/landrop.sh" "$DIST/landrop"

# 不允许依赖比 2.31 更新的 glibc，否则麒麟 V10 SP1 起不来。
if aarch64-linux-gnu-objdump -T "$DIST/landrop" "$DIST"/lib/* | grep -E 'GLIBC_2\.(3[2-9]|[4-9][0-9])' >/dev/null; then
    echo "二进制依赖了高于 2.31 的 glibc" >&2
    exit 1
fi

if command -v qemu-aarch64-static >/dev/null 2>&1; then
    LD_LIBRARY_PATH="$DIST/lib" qemu-aarch64-static -L /opt/arm64-rootfs "$DIST/landrop" --self-check
fi

echo "Linux ARM64 目录: $DIST"
echo "在麒麟上进入该目录后执行: ./landrop.sh"
