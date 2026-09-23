#ifndef AUTOSTART_H
#define AUTOSTART_H

// 登录时自动启动本程序（Windows 写当前用户 Run；其它平台写 XDG autostart，若失败仅返回 false）
namespace Autostart {
bool isEnabled();
bool setEnabled(bool on);
}

#endif
