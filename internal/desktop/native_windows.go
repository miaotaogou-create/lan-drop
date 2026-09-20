//go:build windows

package desktop

import (
	"fmt"

	"github.com/jchv/go-webview2"
)

// RunWindow 打开原生窗口并阻塞到用户关闭。
func RunWindow(url string) error {
	w := webview2.NewWithOptions(webview2.WebViewOptions{
		Debug:     false,
		AutoFocus: true,
		WindowOptions: webview2.WindowOptions{
			Title:  "局域快传",
			Width:  1280,
			Height: 800,
			Center: true,
		},
	})
	if w == nil {
		return fmt.Errorf("本机没有可用的 WebView2 运行时")
	}
	defer w.Destroy()
	w.SetSize(1280, 800, webview2.HintNone)
	w.Navigate(url)
	w.Run()
	return nil
}
