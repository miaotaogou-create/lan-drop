package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"time"

	"github.com/miaotaogou-create/lan-drop/internal/config"
	"github.com/miaotaogou-create/lan-drop/internal/discover"
	"github.com/miaotaogou-create/lan-drop/internal/server"
)

func main() {
	st, err := config.Load("")
	if err != nil {
		log.Fatalf("读取配置失败: %v", err)
	}
	cfg := st.Get()
	if err := os.MkdirAll(cfg.DownloadDir, 0o755); err != nil {
		log.Fatalf("创建下载目录失败: %v", err)
	}

	id := discover.DeviceID()
	// 实际监听端口在启动时定死。设置里改端口后，要重启才重新绑定。
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
	log.Printf("局域快传已启动：%s", cfg.DeviceName)
	log.Printf("HTTP http://127.0.0.1%s  发现 UDP %d  下载目录 %s", addr, cfg.DiscoverPort, cfg.DownloadDir)
	log.Printf("配置文件 %s", st.Path())

	hs := &http.Server{
		Addr:              addr,
		Handler:           srv.Handler(),
		ReadHeaderTimeout: 10 * time.Second,
	}
	if err := hs.ListenAndServe(); err != nil {
		log.Fatalf("HTTP 服务退出: %v", err)
	}
}
