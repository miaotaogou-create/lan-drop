# -*- coding: utf-8 -*-
"""把 AppImage 传到 22 机桌面，便于手测。"""
import os
from pathlib import Path

import paramiko

HOST = "10.100.20.22"
USER = "qwer"
PWD = os.environ.get("LANDROP_22_PASS", "")
LOCAL = Path(r"C:\ZYL\workspace\persion\lan-drop\dist\linux-arm64-portable\landrop-aarch64.AppImage")


def main():
    if not PWD:
        raise SystemExit("请设置环境变量 LANDROP_22_PASS")
    if not LOCAL.is_file():
        raise SystemExit("缺少 %s" % LOCAL)

    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(HOST, username=USER, password=PWD, timeout=20, allow_agent=False, look_for_keys=False)

    def run(cmd, timeout=60):
        _, o, e = c.exec_command(cmd, timeout=timeout)
        out = o.read().decode("utf-8", "replace")
        err = e.read().decode("utf-8", "replace")
        code = o.channel.recv_exit_status()
        print("CMD", cmd)
        if out.strip():
            print(out.rstrip())
        if err.strip():
            print("ERR", err.rstrip())
        print("exit", code)
        return code, out, err

    # 优先 xdg 桌面，其次 ~/桌面、~/Desktop
    _, out, _ = run(
        "d=$(xdg-user-dir DESKTOP 2>/dev/null || true); "
        'if [ -z "$d" ] || [ ! -d "$d" ]; then '
        '  for c in "$HOME/桌面" "$HOME/Desktop"; do '
        '    [ -d "$c" ] && d=$c && break; '
        "  done; "
        "fi; "
        'printf "DESK=%s\\n" "$d"'
    )
    desk = ""
    for line in out.splitlines():
        if line.startswith("DESK="):
            desk = line.split("=", 1)[1].strip()
    if not desk:
        raise SystemExit("找不到桌面目录")

    remote = desk.rstrip("/") + "/landrop-aarch64.AppImage"
    print("upload", LOCAL, "->", remote, "bytes", LOCAL.stat().st_size)
    sftp = c.open_sftp()
    sftp.put(str(LOCAL), remote)
    sftp.chmod(remote, 0o755)
    sftp.close()
    run('ls -lh "%s"' % remote)
    run('file "%s" || true' % remote)
    c.close()
    print("DONE", remote)


if __name__ == "__main__":
    main()
