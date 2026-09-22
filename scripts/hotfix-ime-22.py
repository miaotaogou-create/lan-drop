#!/usr/bin/env python3
# 热修 landrop.sh 并重启（启动命令火后即忘，避免 SSH 挂死）
import os
import time
from pathlib import Path

import paramiko

HOST = "10.100.20.22"
USER = "qwer"
PWD = os.environ.get("LANDROP_22_PASS", "")
REMOTE = "/home/qwer/landrop"
LOCAL_SH = Path(r"C:\ZYL\workspace\persion\lan-drop\dist\linux-arm64\landrop.sh")


def main():
    if not PWD:
        raise SystemExit("请设置环境变量 LANDROP_22_PASS")
    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(HOST, username=USER, password=PWD, timeout=20, allow_agent=False, look_for_keys=False)

    def run(cmd, timeout=30):
        print("CMD", cmd[:140])
        _, o, e = c.exec_command(cmd, timeout=timeout)
        out = o.read().decode("utf-8", "replace")
        err = e.read().decode("utf-8", "replace")
        print(out)
        if err.strip():
            print("ERR", err[:400])
        return out

    def fire(cmd):
        print("FIRE", cmd[:140])
        chan = c.get_transport().open_session()
        chan.exec_command(cmd)
        time.sleep(0.5)
        chan.close()

    data = LOCAL_SH.read_bytes()
    sftp = c.open_sftp()
    with sftp.file(REMOTE + "/landrop.sh", "wb") as f:
        f.write(data)
    sftp.chmod(REMOTE + "/landrop.sh", 0o755)

    starter = """#!/bin/sh
pkill -x landrop 2>/dev/null || true
sleep 1
cd /home/qwer/landrop || exit 1
export DISPLAY=:0
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/bus
exec ./landrop.sh > /tmp/landrop.log 2>&1
"""
    with sftp.file("/tmp/start-landrop-ime.sh", "w") as f:
        f.write(starter)
    sftp.chmod("/tmp/start-landrop-ime.sh", 0o755)
    sftp.close()

    fire("setsid /tmp/start-landrop-ime.sh </dev/null >/dev/null 2>&1 &")
    time.sleep(3)

    run("pgrep -a landrop || echo NO_PROC")
    run("ls -la %s/plugins/platforminputcontexts/" % REMOTE)
    run(
        "PID=$(pgrep -n landrop); echo PID=$PID; "
        "if [ -n \"$PID\" ]; then cat /proc/$PID/environ | tr '\\0' '\\n' | "
        "grep -E 'IM_MODULE|XMODIFIERS|QT_|DISPLAY|DBUS'; fi"
    )
    run("tail -15 /tmp/landrop.log")
    c.close()
    print("hotfix done")


if __name__ == "__main__":
    main()
