package config

import (
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestSettingsJSONRoundTrip(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "settings.json")
	st, err := Load(path)
	if err != nil {
		t.Fatal(err)
	}
	want := Settings{
		DeviceName:   "工控机",
		Port:         9000,
		DiscoverPort: 9001,
		DownloadDir:  filepath.Join(dir, "收件"),
	}
	got, err := st.Update(want)
	if err != nil {
		t.Fatal(err)
	}
	if got != want {
		t.Fatalf("更新后 %#v", got)
	}

	again, err := Load(path)
	if err != nil {
		t.Fatal(err)
	}
	if again.Get() != want {
		t.Fatalf("读回 %#v", again.Get())
	}

	raw, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(string(raw), "工控机") {
		t.Fatalf("JSON 未保留中文: %s", raw)
	}

	// 非法端口回落到默认，避免写出无法绑定的配置。
	fixed, err := st.Update(Settings{
		DeviceName:   "工控机",
		Port:         0,
		DiscoverPort: 0,
		DownloadDir:  want.DownloadDir,
	})
	if err != nil {
		t.Fatal(err)
	}
	if fixed.Port != 8848 || fixed.DiscoverPort != 8850 {
		t.Fatalf("端口未回落: %+v", fixed)
	}
}

func TestSettingsRejectsBrokenJSON(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "settings.json")
	if err := os.WriteFile(path, []byte("{"), 0o644); err != nil {
		t.Fatal(err)
	}
	if _, err := Load(path); err == nil {
		t.Fatal("损坏的 JSON 应报错")
	}
}
