//go:build !windows

package desktop

import "fmt"

// RunWindow 非 Windows 平台暂不提供原生窗口。
func RunWindow(url string) error {
	return fmt.Errorf("当前系统请用浏览器打开界面")
}
