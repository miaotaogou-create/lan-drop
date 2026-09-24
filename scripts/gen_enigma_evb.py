#!/usr/bin/env python3
# 生成 Enigma Virtual Box 工程文件（UTF-16 LE），供 enigmavbconsole 装箱。
# 格式参考公开的 generate-evb 模板（Type=2 文件，Type=3 目录）。

from __future__ import annotations

import argparse
import os
from pathlib import Path


def esc(s: str) -> str:
    return (
        s.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def file_xml(name: str, abs_path: str) -> str:
    return (
        "<File>"
        "<Type>2</Type>"
        f"<Name>{esc(name)}</Name>"
        f"<File>{esc(abs_path)}</File>"
        "<ActiveX>false</ActiveX>"
        "<ActiveXInstall>false</ActiveXInstall>"
        "<Action>0</Action>"
        "<OverwriteDateTime>false</OverwriteDateTime>"
        "<OverwriteAttributes>false</OverwriteAttributes>"
        "<PassCommandLine>false</PassCommandLine>"
        "</File>"
    )


def dir_xml(name: str, children: str) -> str:
    return (
        "<File>"
        "<Type>3</Type>"
        f"<Name>{esc(name)}</Name>"
        "<Action>0</Action>"
        "<OverwriteDateTime>false</OverwriteDateTime>"
        "<OverwriteAttributes>false</OverwriteAttributes>"
        f"<Files>{children}</Files>"
        "</File>"
    )


def walk_tree(root: Path, skip_names: set[str]) -> str:
    parts: list[str] = []
    try:
        entries = sorted(root.iterdir(), key=lambda p: (not p.is_dir(), p.name.lower()))
    except OSError:
        return ""
    for p in entries:
        if p.name in skip_names or p.name.startswith("."):
            continue
        if p.is_dir():
            # 运行时下载目录不必打进便携包
            if p.name.lower() == "downloads":
                continue
            parts.append(dir_xml(p.name, walk_tree(p, skip_names)))
        elif p.is_file():
            parts.append(file_xml(p.name, str(p.resolve())))
    return "".join(parts)


def build_evb(input_exe: Path, output_exe: Path, pack_dir: Path, skip_names: set[str]) -> str:
    files = walk_tree(pack_dir, skip_names)
    return (
        '<?xml encoding="utf-16"?>'
        "<>"
        f"<InputFile>{esc(str(input_exe.resolve()))}</InputFile>"
        f"<OutputFile>{esc(str(output_exe.resolve()))}</OutputFile>"
        "<Files>"
        "<Enabled>true</Enabled>"
        "<DeleteExtractedOnExit>true</DeleteExtractedOnExit>"
        "<CompressFiles>true</CompressFiles>"
        "<Files>"
        "<File>"
        "<Type>3</Type>"
        "<Name>%DEFAULT FOLDER%</Name>"
        "<Action>0</Action>"
        "<OverwriteDateTime>false</OverwriteDateTime>"
        "<OverwriteAttributes>false</OverwriteAttributes>"
        f"<Files>{files}</Files>"
        "</File>"
        "</Files>"
        "</Files>"
        "<Registries>"
        "<Enabled>false</Enabled>"
        "<Registries>"
        "<Registry><Type>1</Type><Virtual>true</Virtual><Name>Classes</Name>"
        "<ValueType>0</ValueType><Value/><Registries/></Registry>"
        "<Registry><Type>1</Type><Virtual>true</Virtual><Name>User</Name>"
        "<ValueType>0</ValueType><Value/><Registries/></Registry>"
        "<Registry><Type>1</Type><Virtual>true</Virtual><Name>Machine</Name>"
        "<ValueType>0</ValueType><Value/><Registries/></Registry>"
        "<Registry><Type>1</Type><Virtual>true</Virtual><Name>Users</Name>"
        "<ValueType>0</ValueType><Value/><Registries/></Registry>"
        "<Registry><Type>1</Type><Virtual>true</Virtual><Name>Config</Name>"
        "<ValueType>0</ValueType><Value/><Registries/></Registry>"
        "</Registries>"
        "</Registries>"
        "<Packaging><Enabled>false</Enabled></Packaging>"
        "<Options>"
        "<ShareVirtualSystem>false</ShareVirtualSystem>"
        "<MapExecutableWithTemporaryFile>true</MapExecutableWithTemporaryFile>"
        "<AllowRunningOfVirtualExeFiles>true</AllowRunningOfVirtualExeFiles>"
        "</Options>"
        "</>"
    )


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate Enigma Virtual Box .evb project")
    ap.add_argument("--input", required=True, help="input landrop.exe")
    ap.add_argument("--output", required=True, help="boxed output exe path")
    ap.add_argument("--pack-dir", required=True, help="directory whose files to virtualize")
    ap.add_argument("--evb", required=True, help="output .evb path")
    ap.add_argument(
        "--skip",
        default="landrop.exe",
        help="comma-separated file names to skip at any level (default: landrop.exe)",
    )
    args = ap.parse_args()

    input_exe = Path(args.input)
    output_exe = Path(args.output)
    pack_dir = Path(args.pack_dir)
    evb_path = Path(args.evb)
    skip = {s.strip() for s in args.skip.split(",") if s.strip()}

    if not input_exe.is_file():
        raise SystemExit(f"missing input exe: {input_exe}")
    if not pack_dir.is_dir():
        raise SystemExit(f"missing pack dir: {pack_dir}")

    xml = build_evb(input_exe, output_exe, pack_dir, skip)
    evb_path.parent.mkdir(parents=True, exist_ok=True)
    # Enigma 期望 UCS-2/UTF-16 LE；带 BOM 更稳
    evb_path.write_bytes(b"\xff\xfe" + xml.encode("utf-16-le"))
    print(f"wrote {evb_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
