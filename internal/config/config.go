// Package config 读写本机设置。优先用户配置目录，不可用时落到 ./data/settings.json。
package config

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
)

// Settings 是本机可改的运行参数。监听端口与发现端口在进程启动时绑定，改完需重启。
type Settings struct {
	DeviceName   string `json:"deviceName"`
	Port         int    `json:"port"`
	DiscoverPort int    `json:"discoverPort"`
	DownloadDir  string `json:"downloadDir"`
}

// Store 是进程内的设置副本，读写都加锁。
type Store struct {
	mu   sync.RWMutex
	path string
	s    Settings
}

// Defaults 返回首次启动用的缺省值。
func Defaults() Settings {
	name, err := os.Hostname()
	if err != nil || strings.TrimSpace(name) == "" {
		name = "局域快传"
	}
	return Settings{
		DeviceName:   name,
		Port:         8848,
		DiscoverPort: 8850,
		DownloadDir:  "./downloads",
	}
}

// LocalPath 是用户目录不可用时的后备文件。
func LocalPath() string {
	return filepath.Join("data", "settings.json")
}

func userPath() string {
	dir, err := os.UserConfigDir()
	if err != nil || dir == "" {
		return ""
	}
	return filepath.Join(dir, "lan-drop", "settings.json")
}

// ResolvePath 选择已有配置；都没有时优先用户目录（能创建的话）。
func ResolvePath() string {
	if p := userPath(); p != "" {
		if _, err := os.Stat(p); err == nil {
			return p
		}
	}
	local := LocalPath()
	if _, err := os.Stat(local); err == nil {
		return local
	}
	if p := userPath(); p != "" {
		if err := os.MkdirAll(filepath.Dir(p), 0o755); err == nil {
			return p
		}
	}
	return local
}

// Load 读取配置。path 为空时用 ResolvePath。文件不存在则写入默认值。
func Load(path string) (*Store, error) {
	if path != "" {
		return loadAt(path)
	}
	path = ResolvePath()
	st, err := loadAt(path)
	if err != nil && path != LocalPath() {
		if st2, err2 := loadAt(LocalPath()); err2 == nil {
			return st2, nil
		}
	}
	return st, err
}

func loadAt(path string) (*Store, error) {
	st := &Store{path: path, s: Defaults()}
	b, err := os.ReadFile(path)
	if err != nil {
		if !errors.Is(err, os.ErrNotExist) {
			return nil, fmt.Errorf("读取配置失败: %w", err)
		}
		if werr := writeAtomic(path, st.s); werr != nil {
			return nil, fmt.Errorf("写入默认配置失败: %w", werr)
		}
		return st, nil
	}
	var s Settings
	if err := json.Unmarshal(b, &s); err != nil {
		return nil, fmt.Errorf("配置不是合法 JSON: %w", err)
	}
	st.s = normalize(s)
	return st, nil
}

// NewStore 用给定设置建内存副本，不立刻写盘。测试和明确指定路径时用。
func NewStore(path string, s Settings) *Store {
	return &Store{path: path, s: normalize(s)}
}

func normalize(s Settings) Settings {
	d := Defaults()
	s.DeviceName = strings.TrimSpace(s.DeviceName)
	if s.DeviceName == "" {
		s.DeviceName = d.DeviceName
	}
	if s.Port <= 0 || s.Port > 65535 {
		s.Port = d.Port
	}
	if s.DiscoverPort <= 0 || s.DiscoverPort > 65535 {
		s.DiscoverPort = d.DiscoverPort
	}
	s.DownloadDir = strings.TrimSpace(s.DownloadDir)
	if s.DownloadDir == "" {
		s.DownloadDir = d.DownloadDir
	}
	return s
}

// Get 返回当前设置的副本。
func (st *Store) Get() Settings {
	st.mu.RLock()
	defer st.mu.RUnlock()
	return st.s
}

// Path 返回配置文件路径。
func (st *Store) Path() string {
	st.mu.RLock()
	defer st.mu.RUnlock()
	return st.path
}

// Update 先落盘再改内存，避免写失败时界面和磁盘不一致。
func (st *Store) Update(next Settings) (Settings, error) {
	next = normalize(next)
	st.mu.RLock()
	path := st.path
	st.mu.RUnlock()
	if err := writeAtomic(path, next); err != nil {
		return Settings{}, err
	}
	st.mu.Lock()
	st.s = next
	st.mu.Unlock()
	return next, nil
}

func writeAtomic(path string, s Settings) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	b, err := json.MarshalIndent(s, "", "  ")
	if err != nil {
		return err
	}
	b = append(b, '\n')
	tmp := path + ".tmp"
	if err := os.WriteFile(tmp, b, 0o644); err != nil {
		return err
	}
	// Windows 上 Rename 不能覆盖已存在的文件。
	if err := os.Remove(path); err != nil && !errors.Is(err, os.ErrNotExist) {
		os.Remove(tmp)
		return err
	}
	if err := os.Rename(tmp, path); err != nil {
		os.Remove(tmp)
		return err
	}
	return nil
}
