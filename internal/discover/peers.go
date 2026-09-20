package discover

import (
	"net"
	"strconv"
	"time"
)

// Peer 是一台对端。在线与否由最近一次见到的时间推算，不单独持久化。
type Peer struct {
	ID       string    `json:"id"`
	Name     string    `json:"name"`
	IP       string    `json:"ip"`
	Port     int       `json:"port"`
	OS       string    `json:"os,omitempty"`
	Alias    string    `json:"alias,omitempty"`
	Manual   bool      `json:"manual"`
	LastSeen time.Time `json:"lastSeen,omitempty"`
	Online   bool      `json:"online"`
}

// Announcement 是 UDP 心跳报文。
type Announcement struct {
	ID   string `json:"id"`
	Name string `json:"name"`
	Port int    `json:"port"`
	OS   string `json:"os"`
}

func placeholderID(p Peer) bool {
	if p.IP == "" || p.Port == 0 || p.ID == "" {
		return false
	}
	return p.ID == net.JoinHostPort(p.IP, strconv.Itoa(p.Port))
}

// Merge 把 incoming 并进列表：相同 ID，或相同 IP+端口，视为同一台。
// 手动标记和别名不会被空字段清掉；占位 ID（ip:port）会在拿到真实 ID 后换成真实 ID。
func Merge(list []Peer, in Peer) []Peer {
	if in.ID == "" && in.IP != "" && in.Port != 0 {
		in.ID = net.JoinHostPort(in.IP, strconv.Itoa(in.Port))
	}
	for i, old := range list {
		byID := old.ID != "" && in.ID != "" && old.ID == in.ID
		byAddr := old.IP != "" && in.IP != "" && old.Port != 0 && old.IP == in.IP && old.Port == in.Port
		if !byID && !byAddr {
			continue
		}
		list[i] = mergeOne(old, in, byAddr && !byID)
		return list
	}
	if in.Name == "" {
		in.Name = in.Alias
	}
	return append(list, in)
}

func mergeOne(old, in Peer, matchedOnlyByAddr bool) Peer {
	if in.ID != "" && (old.ID == "" || placeholderID(old) || matchedOnlyByAddr) {
		old.ID = in.ID
	}
	if in.Name != "" {
		old.Name = in.Name
	} else if old.Name == "" && in.Alias != "" {
		old.Name = in.Alias
	}
	if in.IP != "" {
		old.IP = in.IP
	}
	if in.Port != 0 {
		old.Port = in.Port
	}
	if in.OS != "" {
		old.OS = in.OS
	}
	if in.Alias != "" {
		old.Alias = in.Alias
	}
	if in.Manual {
		old.Manual = true
	}
	if !in.LastSeen.IsZero() {
		old.LastSeen = in.LastSeen
	}
	return old
}
