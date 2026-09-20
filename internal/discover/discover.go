// Package discover 用 UDP 广播维护局域网节点，并保存手动添加的节点。
package discover

import (
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"net"
	"os"
	"runtime"
	"strconv"
	"sync"
	"time"
)

const (
	heartbeatEvery = 3 * time.Second
	onlineTTL      = 12 * time.Second
	// 自动发现的节点超过此时长没再出现就从列表拿掉；手动添加的保留。
	forgetAfter = 60 * time.Second
)

// Service 维护 peer 列表，并可选地收发心跳。
type Service struct {
	mu       sync.Mutex
	id       string
	local    func() (name string, httpPort int)
	peers    []Peer
	conn     *net.UDPConn
	discPort int
	stop     chan struct{}
	logOnce  sync.Once
}

// New 创建发现服务。local 在每次心跳时读取当前设备名和实际监听端口。
func New(id string, local func() (name string, httpPort int)) *Service {
	if id == "" {
		id = DeviceID()
	}
	if local == nil {
		local = func() (string, int) { return "局域快传", 8848 }
	}
	return &Service{id: id, local: local, peers: []Peer{}}
}

// ID 返回本机节点标识。
func (s *Service) ID() string { return s.id }

// DeviceID 尽量稳定：主机名加第一块非虚拟网卡的 MAC。
func DeviceID() string {
	host, err := os.Hostname()
	if err != nil || host == "" {
		host = "landrop"
	}
	ifaces, err := net.Interfaces()
	if err != nil {
		return host + "-local"
	}
	for _, iface := range ifaces {
		if skipIface(iface) || len(iface.HardwareAddr) == 0 {
			continue
		}
		return host + "-" + iface.HardwareAddr.String()
	}
	return host + "-local"
}

// Start 在 port 上监听并周期性广播。绑定失败时调用方仍可手动添加节点。
func (s *Service) Start(port int) error {
	if port <= 0 || port > 65535 {
		return fmt.Errorf("发现端口无效")
	}
	conn, err := net.ListenUDP("udp4", &net.UDPAddr{IP: net.IPv4zero, Port: port})
	if err != nil {
		return fmt.Errorf("监听发现端口 %d 失败: %w", port, err)
	}
	s.mu.Lock()
	if s.conn != nil {
		s.mu.Unlock()
		conn.Close()
		return fmt.Errorf("发现服务已启动")
	}
	s.conn = conn
	s.discPort = port
	s.stop = make(chan struct{})
	s.mu.Unlock()
	go s.readLoop(conn)
	go s.broadcastLoop(conn, port)
	return nil
}

// Stop 停止收发。未启动时调用无效果。
func (s *Service) Stop() {
	s.mu.Lock()
	ch := s.stop
	conn := s.conn
	s.conn = nil
	s.mu.Unlock()
	if ch != nil {
		select {
		case <-ch:
		default:
			close(ch)
		}
	}
	if conn != nil {
		conn.Close()
	}
}

func (s *Service) readLoop(conn *net.UDPConn) {
	buf := make([]byte, 4096)
	for {
		_ = conn.SetReadDeadline(time.Now().Add(time.Second))
		n, addr, err := conn.ReadFromUDP(buf)
		if err != nil {
			if errors.Is(err, net.ErrClosed) {
				return
			}
			var ne net.Error
			if errors.As(err, &ne) && ne.Timeout() {
				select {
				case <-s.stop:
					return
				default:
					continue
				}
			}
			select {
			case <-s.stop:
				return
			default:
				log.Printf("读取发现报文失败: %v", err)
				return
			}
		}
		s.onPacket(buf[:n], addr)
	}
}

func (s *Service) broadcastLoop(conn *net.UDPConn, port int) {
	tick := time.NewTicker(heartbeatEvery)
	defer tick.Stop()
	s.sendAnnouncement(conn, port)
	for {
		select {
		case <-s.stop:
			return
		case <-tick.C:
			s.sendAnnouncement(conn, port)
		}
	}
}

func (s *Service) sendAnnouncement(conn *net.UDPConn, port int) {
	name, httpPort := s.local()
	body, err := json.Marshal(Announcement{
		ID:   s.id,
		Name: name,
		Port: httpPort,
		OS:   runtime.GOOS,
	})
	if err != nil {
		return
	}
	var first error
	for _, ip := range broadcastTargets() {
		dst := &net.UDPAddr{IP: ip, Port: port}
		if _, err := conn.WriteToUDP(body, dst); err != nil && first == nil {
			first = err
		}
	}
	if first != nil {
		s.logOnce.Do(func() {
			log.Printf("发送发现广播失败: %v", first)
		})
	}
}

func (s *Service) onPacket(b []byte, addr *net.UDPAddr) {
	if addr == nil || addr.IP == nil {
		return
	}
	var a Announcement
	if err := json.Unmarshal(b, &a); err != nil {
		return
	}
	if a.ID == "" || a.ID == s.id || a.Port <= 0 || a.Port > 65535 {
		return
	}
	ip := addr.IP.String()
	if v4 := addr.IP.To4(); v4 != nil {
		ip = v4.String()
	}
	created := s.upsert(Peer{
		ID:       a.ID,
		Name:     a.Name,
		IP:       ip,
		Port:     a.Port,
		OS:       a.OS,
		LastSeen: time.Now(),
	})
	if created {
		log.Printf("发现节点 %s（%s:%d）", a.Name, ip, a.Port)
	}
}

// AddManual 记入一台手动节点。已存在则保留发现到的名称，并补上别名。
func (s *Service) AddManual(ip, alias, osName string, port int) Peer {
	if port == 0 {
		port = 8848
	}
	p := Peer{
		IP:     ip,
		Port:   port,
		Alias:  alias,
		OS:     osName,
		Manual: true,
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.peers = Merge(s.peers, p)
	for _, item := range s.peers {
		if item.IP == ip && item.Port == port {
			return item
		}
	}
	return p
}

// Touch 用探测或收件得到的信息刷新节点。
func (s *Service) Touch(ip string, port int, id, name, osName string) {
	if port == 0 {
		port = 8848
	}
	p := Peer{
		ID:       id,
		Name:     name,
		IP:       ip,
		Port:     port,
		OS:       osName,
		LastSeen: time.Now(),
	}
	s.upsert(p)
}

// Resolve 按 peerId 或 ip+port 找节点。只有地址时，即使不在列表里也返回一个临时节点。
func (s *Service) Resolve(id, ip string, port int) (Peer, bool) {
	if port == 0 {
		port = 8848
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	if id != "" {
		for _, p := range s.peers {
			if p.ID == id && p.IP != "" && p.Port != 0 {
				return p, true
			}
		}
		if ip == "" {
			return Peer{}, false
		}
	}
	if ip == "" {
		return Peer{}, false
	}
	for _, p := range s.peers {
		if p.IP == ip && p.Port == port {
			return p, true
		}
	}
	return Peer{
		ID:   net.JoinHostPort(ip, strconv.Itoa(port)),
		IP:   ip,
		Port: port,
	}, true
}

// List 返回当前列表，并按最近出现时间填写 online。
func (s *Service) List() []Peer {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.prune(time.Now())
	now := time.Now()
	out := make([]Peer, len(s.peers))
	for i, p := range s.peers {
		p.Online = !p.LastSeen.IsZero() && now.Sub(p.LastSeen) <= onlineTTL
		out[i] = p
	}
	return out
}

func (s *Service) upsert(p Peer) bool {
	s.mu.Lock()
	defer s.mu.Unlock()
	before := len(s.peers)
	s.peers = Merge(s.peers, p)
	s.prune(time.Now())
	return len(s.peers) > before
}

func (s *Service) prune(now time.Time) {
	kept := make([]Peer, 0, len(s.peers))
	for _, p := range s.peers {
		if p.Manual || p.LastSeen.IsZero() || now.Sub(p.LastSeen) <= forgetAfter {
			kept = append(kept, p)
		}
	}
	s.peers = kept
}
