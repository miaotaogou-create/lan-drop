# -*- coding: utf-8 -*-
import io
import tarfile
import time
from pathlib import Path

import paramiko

HOST = "10.100.20.22"
USER = "qwer"
PWD = __import__("os").environ.get("LANDROP_22_PASS", "")
LOCAL = Path(r"C:\ZYL\workspace\persion\lan-drop\dist\linux-arm64")
REMOTE = "/home/qwer/landrop"


def main():
    if not PWD:
        raise SystemExit("请设置环境变量 LANDROP_22_PASS")
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz") as tar:
        for p in LOCAL.rglob("*"):
            if p.is_file():
                tar.add(str(p), arcname=p.relative_to(LOCAL).as_posix())
    data = buf.getvalue()
    print("pack_bytes", len(data))

    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(HOST, username=USER, password=PWD, timeout=20, allow_agent=False, look_for_keys=False)

    def run(cmd, timeout=120):
        _, o, e = c.exec_command(cmd, timeout=timeout)
        out = o.read().decode("utf-8", "replace")
        err = e.read().decode("utf-8", "replace")
        code = o.channel.recv_exit_status()
        print("CMD", cmd)
        if out.strip():
            print(out)
        if err.strip():
            print("ERR", err)
        print("exit", code)
        return code, out, err

    run("pkill -f /home/qwer/landrop/landrop || pkill -x landrop || true")
    time.sleep(1)
    run("mkdir -p /tmp/landrop-new %s/downloads" % REMOTE)
    run("rm -rf /tmp/landrop-new/*")

    sftp = c.open_sftp()
    sftp.putfo(io.BytesIO(data), "/tmp/landrop-new.tgz")
    sftp.close()

    run("tar -xzf /tmp/landrop-new.tgz -C /tmp/landrop-new")
    # 覆盖程序，保留 downloads
    run(
        "cd /tmp/landrop-new && tar cf - --exclude downloads . | "
        "(cd %s && tar xf -)" % REMOTE
    )
    run("chmod +x %s/landrop %s/landrop.sh" % (REMOTE, REMOTE))
    run("ls -la %s | head -20" % REMOTE)
    run("who; ls /tmp/.X11-unix 2>/dev/null || true")

    start = (
        "cd %s && nohup env DISPLAY=:0 "
        "DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/bus "
        "./landrop.sh >/tmp/landrop.log 2>&1 & echo STARTED:$!; "
        "sleep 2; ps -ef | grep -v grep | grep landrop || true; "
        "tail -30 /tmp/landrop.log || true" % REMOTE
    )
    run(start)
    c.close()
    print("deploy done")


if __name__ == "__main__":
    main()
