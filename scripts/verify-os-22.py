#!/usr/bin/env python3
# 部署后核对 22 机上报的 os 字段
import json
import os
import time
import urllib.request

import paramiko

HOST = "10.100.20.22"
USER = "qwer"
PWD = os.environ.get("LANDROP_22_PASS", "")


def main():
    if not PWD:
        raise SystemExit("请设置 LANDROP_22_PASS")
    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(HOST, username=USER, password=PWD, timeout=20, allow_agent=False, look_for_keys=False)

    def run(cmd, timeout=30):
        _, o, e = c.exec_command(cmd, timeout=timeout)
        out = o.read().decode("utf-8", "replace")
        err = e.read().decode("utf-8", "replace")
        print(out.strip())
        if err.strip():
            print("ERR", err[:300])
        return out

    run("pgrep -a landrop || echo NO_PROC")
    # 本机 curl 经 SSH
    run("curl -sS http://127.0.0.1:8848/api/info || true")
    c.close()

    # 也可以从本机直接打
    time.sleep(0.5)
    try:
        with urllib.request.urlopen("http://%s:8848/api/info" % HOST, timeout=5) as r:
            data = json.loads(r.read().decode("utf-8"))
            print("remote_info", json.dumps(data, ensure_ascii=False))
            os_tag = data.get("os", "")
            print("os=", os_tag, "OK" if os_tag == "linux" else "NEED_linux")
    except Exception as ex:
        print("fetch_fail", ex)


if __name__ == "__main__":
    main()
