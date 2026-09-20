package main

import (
	"context"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"runtime"
	"time"

	"github.com/miaotaogou-create/lan-drop/internal/config"
	"github.com/miaotaogou-create/lan-drop/internal/desktop"
	"github.com/miaotaogou-create/lan-drop/internal/discover"
	"github.com/miaotaogou-create/lan-drop/internal/server"
)

func main() {
	browserOnly := flag.Bool("browser", false, "强制用系统浏览器打开界面（不弹原生窗口）")
	flag.Parse()

	st, err := config.Load("")
	if err != nil {
		log.Fatalf("读取配置失败: %v", err)
	}
	cfg := st.Get()
	if err := os.MkdirAll(cfg.DownloadDir, 0o755); err != nil {
		log.Fatalf("创建下载目录失败: %v", err)
	}

	id := discover.DeviceID()
	listenPort := cfg.Port
	disc := discover.New(id, func() (string, int) {
		return st.Get().DeviceName, listenPort
	})
	if err := disc.Start(cfg.DiscoverPort); err != nil {
		log.Printf("发现服务未启动，仍可手动添加节点: %v", err)
	} else {
		defer disc.Stop()
	}

	srv := server.New(st, disc, id, listenPort)
	addr := fmt.Sprintf(":%d", listenPort)
	uiURL := fmt.Sprintf("http://127.0.0.1:%d/", listenPort)
	log.Printf("局域快传已启动：%s", cfg.DeviceName)
	log.Printf("HTTP %s  发现 UDP %d  下载目录 %s", uiURL, cfg.DiscoverPort, cfg.DownloadDir)
	log.Printf("配置文件 %s", st.Path())

	hs := &http.Server{
		Addr:              addr,
		Handler:           srv.Handler(),
		ReadHeaderTimeout: 10 * time.Second,
	}
	errCh := make(chan error, 1)
	go func() {
		errCh <- hs.ListenAndServe()
	}()

	if err := waitReady(uiURL+"api/info", 5*time.Second); err != nil {
		log.Fatalf("服务未就绪: %v", err)
	}

	useWindow := runtime.GOOS == "windows" && !*browserOnly
	if useWindow {
		if err := desktop.RunWindow(uiURL); err != nil {
			log.Printf("原生窗口不可用，改用浏览器: %v", err)
			if err := desktop.OpenBrowser(uiURL); err != nil {
				log.Printf("%v；请手动打开 %s", err, uiURL)
			}
			if err := <-errCh; err != nil && err != http.ErrServerClosed {
				log.Fatalf("HTTP 服务退出: %v", err)
			}
			return
		}
		ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
		defer cancel()
		_ = hs.Shutdown(ctx)
		return
	}

	if err := desktop.OpenBrowser(uiURL); err != nil {
		log.Printf("%v；请手动打开 %s", err, uiURL)
	}
	if err := <-errCh; err != nil && err != http.ErrServerClosed {
		log.Fatalf("HTTP 服务退出: %v", err)
	}
}

func waitReady(url string, timeout time.Duration) error {
	deadline := time.Now().Add(timeout)
	client := &http.Client{Timeout: 500 * time.Millisecond}
	for time.Now().Before(deadline) {
		resp, err := client.Get(url)
		if err == nil {
			resp.Body.Close()
			if resp.StatusCode >= 200 && resp.StatusCode < 500 {
				return nil
			}
		}
		time.Sleep(100 * time.Millisecond)
	}
	return fmt.Errorf("超时等待 %s", url)
}
