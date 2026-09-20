package server

import (
	"bytes"
	"mime/multipart"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/miaotaogou-create/lan-drop/internal/config"
	"github.com/miaotaogou-create/lan-drop/internal/discover"
)

func TestUploadSavesBytes(t *testing.T) {
	dir := t.TempDir()
	payload := "你好"
	path, n, err := SaveStream(dir, "说明.txt", strings.NewReader(payload))
	if err != nil {
		t.Fatal(err)
	}
	if n != int64(len(payload)) {
		t.Fatalf("字节数 %d", n)
	}
	got, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != payload {
		t.Fatalf("内容 %q", got)
	}
	rel, err := filepath.Rel(dir, path)
	if err != nil || strings.HasPrefix(rel, "..") {
		t.Fatalf("文件跑出下载目录: %s", path)
	}

	second, _, err := SaveStream(dir, "说明.txt", strings.NewReader("第二份"))
	if err != nil {
		t.Fatal(err)
	}
	if second == path {
		t.Fatal("同名文件被覆盖")
	}
	got, err = os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != payload {
		t.Fatal("原文件内容变了")
	}

	if _, _, err := SaveStream(dir, "..", strings.NewReader("no")); err == nil {
		t.Fatal("应拒绝无效文件名")
	}
	escaped, _, err := SaveStream(dir, `..\..\outside.txt`, strings.NewReader("stay"))
	if err != nil {
		t.Fatal(err)
	}
	rel, err = filepath.Rel(dir, escaped)
	if err != nil || strings.HasPrefix(rel, "..") || filepath.Base(escaped) != "outside.txt" {
		t.Fatalf("路径未收敛到下载目录: %s", escaped)
	}
}

func TestUploadHTTP(t *testing.T) {
	dir := t.TempDir()
	st := config.NewStore(filepath.Join(dir, "settings.json"), config.Settings{
		DeviceName:   "工控机",
		Port:         8848,
		DiscoverPort: 8850,
		DownloadDir:  filepath.Join(dir, "dl"),
	})
	srv := New(st, discover.New("dev-1", nil), "dev-1", 8848)

	var body bytes.Buffer
	mw := multipart.NewWriter(&body)
	part, err := createFilePart(mw, "说明.txt")
	if err != nil {
		t.Fatal(err)
	}
	if _, err := part.Write([]byte("abc")); err != nil {
		t.Fatal(err)
	}
	if err := mw.Close(); err != nil {
		t.Fatal(err)
	}
	req := httptest.NewRequest(http.MethodPost, "/api/upload", &body)
	req.Header.Set("Content-Type", mw.FormDataContentType())
	rr := httptest.NewRecorder()
	srv.Handler().ServeHTTP(rr, req)
	if rr.Code != http.StatusOK {
		t.Fatalf("status %d body %s", rr.Code, rr.Body.String())
	}
	saved := filepath.Join(dir, "dl", "说明.txt")
	got, err := os.ReadFile(saved)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != "abc" {
		t.Fatalf("落盘内容 %q", got)
	}

	infoReq := httptest.NewRequest(http.MethodGet, "/api/info", nil)
	infoRR := httptest.NewRecorder()
	srv.Handler().ServeHTTP(infoRR, infoReq)
	if infoRR.Code != http.StatusOK || !strings.Contains(infoRR.Body.String(), "工控机") {
		t.Fatalf("info %d %s", infoRR.Code, infoRR.Body.String())
	}

	pageReq := httptest.NewRequest(http.MethodGet, "/", nil)
	pageRR := httptest.NewRecorder()
	srv.Handler().ServeHTTP(pageRR, pageReq)
	if pageRR.Code != http.StatusOK || !strings.Contains(pageRR.Body.String(), "HTTP API 已启动") {
		t.Fatalf("占位页 %d %s", pageRR.Code, pageRR.Body.String())
	}
}
