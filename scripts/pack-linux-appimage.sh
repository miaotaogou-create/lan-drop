#!/bin/sh
# 在 WSL Ubuntu 20.04 里，把已有 dist/linux-arm64 打成 aarch64 AppImage。
# 先跑 scripts/build-linux-arm64.sh；本脚本不重新编译。
set -eu
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist/linux-arm64"
OUT_DIR="$ROOT/dist/linux-arm64-portable"
APPDIR="$OUT_DIR/LanDrop.AppDir"
TOOL_DIR="$ROOT/build/appimage-tools"
APPIMAGETOOL="$TOOL_DIR/appimagetool-x86_64.AppImage"

if [ ! -x "$DIST/landrop" ] || [ ! -f "$DIST/landrop.sh" ]; then
    echo "缺少 $DIST（请先 sh scripts/build-linux-arm64.sh）" >&2
    exit 1
fi

mkdir -p "$TOOL_DIR" "$OUT_DIR"
if [ ! -x "$APPIMAGETOOL" ]; then
    echo "下载 appimagetool ..."
    wget -q -O "$APPIMAGETOOL" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "$APPIMAGETOOL"
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR"

# 目录布局与现网目录包一致，AppRun 复用 landrop.sh 逻辑（含 fcitx）
cp -a "$DIST/." "$APPDIR/"
rm -rf "$APPDIR/downloads" 2>/dev/null || true

# AppRun：优先用 APPDIR（AppImage 挂载点）
# 注意：AppImage 挂载只读，不能把 fcitx 插件 ln/cp 进 APPDIR（目录包 landrop.sh 那套会静默失败）
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/sh
DIR=$(dirname "$(readlink -f "$0")")
export APPDIR="${APPDIR:-$DIR}"
export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# 可写插件层：挂系统 fcitx，并保留包内 ibus/compose
IME_ROOT="${XDG_CACHE_HOME:-$HOME/.cache}/landrop-ime"
IME_PIC="$IME_ROOT/platforminputcontexts"
mkdir -p "$IME_PIC"
for f in "$APPDIR/plugins/platforminputcontexts/"*; do
    [ -e "$f" ] || continue
    base=$(basename "$f")
    [ -e "$IME_PIC/$base" ] || ln -sf "$f" "$IME_PIC/$base" 2>/dev/null || true
done

FCITX_DST="$IME_PIC/libfcitxplatforminputcontextplugin.so"
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

# 先搜可写 IME 层，再搜包内 platforms/imageformats 等
export QT_PLUGIN_PATH="$IME_ROOT:$APPDIR/plugins"

if [ -z "${QT_IM_MODULE:-}" ]; then
    if [ -e "$FCITX_DST" ]; then
        export QT_IM_MODULE=fcitx
    elif [ -f "$IME_PIC/libibusplatforminputcontextplugin.so" ]; then
        export QT_IM_MODULE=ibus
    fi
fi
if [ -z "${XMODIFIERS:-}" ] && [ "${QT_IM_MODULE:-}" = "fcitx" ]; then
    export XMODIFIERS=@im=fcitx
fi
if [ -z "${GTK_IM_MODULE:-}" ] && [ "${QT_IM_MODULE:-}" = "fcitx" ]; then
    export GTK_IM_MODULE=fcitx
fi

cd "${HOME:-/tmp}"
exec "$APPDIR/landrop" "$@"
EOF
chmod +x "$APPDIR/AppRun" "$APPDIR/landrop"

# 桌面项 + 图标（从 SVG 栅格化；失败则用纯色占位）
cat > "$APPDIR/landrop.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=局域快传
Name[en]=LanDrop
Comment=LAN file and text transfer
Exec=landrop
Icon=landrop
Categories=Network;Utility;
Terminal=false
EOF

ICON_SVG="$ROOT/icons/landrop_app_icon.svg"
ICON_PNG="$APPDIR/landrop.png"
REPO_PNG="$ROOT/icons/landrop.png"
if [ -f "$REPO_PNG" ]; then
    cp -f "$REPO_PNG" "$ICON_PNG"
elif command -v python3 >/dev/null 2>&1 && [ -f "$ICON_SVG" ]; then
    python3 - << PY || true
import sys
from pathlib import Path
svg = Path(r"$ICON_SVG")
png = Path(r"$ICON_PNG")
try:
    from PyQt5.QtWidgets import QApplication
    from PyQt5.QtGui import QImage, QPainter
    from PyQt5.QtSvg import QSvgRenderer
    from PyQt5.QtCore import Qt, QRectF, QByteArray
except Exception:
    sys.exit(1)
app = QApplication([])
r = QSvgRenderer(QByteArray(svg.read_bytes()))
if not r.isValid():
    sys.exit(1)
img = QImage(256, 256, QImage.Format_ARGB32_Premultiplied)
img.fill(Qt.transparent)
p = QPainter(img)
p.setRenderHint(QPainter.Antialiasing, True)
r.render(p, QRectF(0, 0, 256, 256))
p.end()
img.save(str(png))
print("icon", png)
PY
fi
if [ ! -f "$ICON_PNG" ]; then
    if command -v convert >/dev/null 2>&1 && [ -f "$ICON_SVG" ]; then
        convert -background none "$ICON_SVG" -resize 256x256 "$ICON_PNG" || true
    fi
fi
if [ ! -f "$ICON_PNG" ]; then
    printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xb4\x00\x00\x00\x00IEND\xaeB`\x82' > "$ICON_PNG"
fi
cp -f "$ICON_PNG" "$APPDIR/.DirIcon"

# 在 x86_64 主机为 aarch64 打 AppImage（嵌入 aarch64 runtime）
rm -f "$OUT_DIR"/局域快传-*.AppImage "$OUT_DIR"/LanDrop-*.AppImage "$OUT_DIR"/landrop-*.AppImage 2>/dev/null || true
cd "$OUT_DIR"
export ARCH=aarch64
export VERSION="${VERSION:-$(date +%Y%m%d)}"
# 部分环境 FUSE 不可用，用提取运行
if "$APPIMAGETOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "landrop-${VERSION}-aarch64.AppImage"; then
    :
else
    # 旧参数兼容
    "$APPIMAGETOOL" --appimage-extract-and-run "$APPDIR" "landrop-${VERSION}-aarch64.AppImage"
fi

OUT=$(ls -1 "$OUT_DIR"/landrop-*-aarch64.AppImage 2>/dev/null | head -n1)
if [ -z "$OUT" ]; then
    echo "未生成 AppImage" >&2
    exit 1
fi
# 稳定文件名便于发布
STABLE="$OUT_DIR/landrop-aarch64.AppImage"
cp -f "$OUT" "$STABLE"
chmod +x "$STABLE" "$OUT"
echo "Linux ARM64 AppImage: $STABLE"
ls -lh "$STABLE"
