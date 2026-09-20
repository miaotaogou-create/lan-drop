package discover

import (
	"encoding/json"
	"testing"
	"time"
)

func TestAnnouncementJSON(t *testing.T) {
	in := Announcement{ID: "pc-1", Name: "工控机", Port: 8848, OS: "windows"}
	b, err := json.Marshal(in)
	if err != nil {
		t.Fatal(err)
	}
	var got Announcement
	if err := json.Unmarshal(b, &got); err != nil {
		t.Fatal(err)
	}
	if got != in {
		t.Fatalf("编解码不一致: %+v", got)
	}
}

func TestMergePeers(t *testing.T) {
	t.Run("新增", func(t *testing.T) {
		list := Merge(nil, Peer{ID: "a", Name: "甲", IP: "10.0.0.2", Port: 8848})
		if len(list) != 1 || list[0].Name != "甲" {
			t.Fatalf("%+v", list)
		}
	})

	t.Run("相同ID更新名称", func(t *testing.T) {
		list := Merge(nil, Peer{ID: "a", Name: "甲", IP: "10.0.0.2", Port: 8848, Alias: "旧"})
		list = Merge(list, Peer{ID: "a", Name: "甲改", IP: "10.0.0.3", Port: 8848})
		if len(list) != 1 {
			t.Fatalf("应合并为一条: %+v", list)
		}
		if list[0].Name != "甲改" || list[0].IP != "10.0.0.3" || list[0].Alias != "旧" {
			t.Fatalf("合并结果不对: %+v", list[0])
		}
	})

	t.Run("手动节点与广播合并", func(t *testing.T) {
		manual := Peer{IP: "10.0.0.8", Port: 8848, Alias: "现场机", Manual: true}
		list := Merge(nil, manual)
		if list[0].ID != "10.0.0.8:8848" {
			t.Fatalf("占位 ID: %s", list[0].ID)
		}
		seen := time.Unix(1_700_000_000, 0).UTC()
		list = Merge(list, Peer{
			ID: "host-aa", Name: "麒麟机", IP: "10.0.0.8", Port: 8848, OS: "linux", LastSeen: seen,
		})
		if len(list) != 1 {
			t.Fatalf("应仍是一条: %+v", list)
		}
		p := list[0]
		if !p.Manual || p.Alias != "现场机" || p.ID != "host-aa" || p.Name != "麒麟机" || p.OS != "linux" {
			t.Fatalf("合并丢了手动信息: %+v", p)
		}
		if !p.LastSeen.Equal(seen) {
			t.Fatalf("LastSeen %s", p.LastSeen)
		}
	})

	t.Run("不同地址保持两条", func(t *testing.T) {
		list := Merge(nil, Peer{ID: "a", IP: "10.0.0.2", Port: 8848})
		list = Merge(list, Peer{ID: "b", IP: "10.0.0.3", Port: 8848})
		if len(list) != 2 {
			t.Fatalf("len=%d", len(list))
		}
	})
}
