# -*- coding: utf-8 -*-
"""杀掉旧 AppImage，更新桌面文件并重新启动。"""
import os
import time
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

    run("pkill -f landrop-aarch64.AppImage || true")
    run("pkill -f '/tmp/.mount_landro' || true")
    run("pkill -x landrop || true")
    time.sleep(2)

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
    # 先写临时再 mv，避免占用
    tmp = "/tmp/landrop-aarch64.AppImage.new"
    print("upload", LOCAL, "->", remote)
    sftp = c.open_sftp()
    sftp.put(str(LOCAL), tmp)
    sftp.close()
    run('mv -f "%s" "%s" && chmod +x "%s"' % (tmp, remote, remote))
    run('ls -lh "%s"' % remote)

    start = (
        'APP="%s"; '
        "setsid env DISPLAY=:0 "
        "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/bus "
        '"$APP" >/tmp/landrop-appimage.log 2>&1 </dev/null &'
    ) % remote
    print("FIRE", start)
    chan = c.get_transport().open_session()
    chan.exec_command(start)
    time.sleep(0.5)
    chan.close()
    time.sleep(3)
    run("pgrep -a landrop || echo NO_PROC")
    run("tail -40 /tmp/landrop-appimage.log 2>/dev/null || true")
    run("ls -ld ~/landrop/downloads")
    c.close()
    print("DONE")


if __name__ == "__main__":
    main()
