// Package server 提供本机 HTTP API：节点、会话、文本、文件和设置。
// 局域网内默认同网段互信，本阶段不做鉴权。
package server

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"io/fs"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/miaotaogou-create/lan-drop/internal/config"
	"github.com/miaotaogou-create/lan-drop/internal/discover"
	"github.com/miaotaogou-create/lan-drop/web"
)

// Message 是一条聊天记录。会话只留在内存里。
type Message struct {
	ID        string    `json:"id"`
	PeerID    string    `json:"peerId"`
	FromName  string    `json:"fromName,omitempty"`
	Direction string    `json:"direction"`
	Text      string    `json:"text"`
	Time      time.Time `json:"time"`
}

type infoResp struct {
	ID   string   `json:"id"`
	Name string   `json:"name"`
	Port int      `json:"port"`
	OS   string   `json:"os"`
	IPs  []string `json:"ips"`
}

type inboxBody struct {
	FromID   string `json:"fromId"`
	FromName string `json:"fromName"`
	FromPort int    `json:"fromPort"`
	Text     string `json:"text"`
}

type sendTextBody struct {
	PeerID string `json:"peerId"`
	IP     string `json:"ip"`
	Port   int    `json:"port"`
	Text   string `json:"text"`
}

type addPeerBody struct {
	IP    string `json:"ip"`
	Port  int    `json:"port"`
	Alias string `json:"alias"`
	OS    string `json:"os"`
}

type probeBody struct {
	IP   string `json:"ip"`
	Port int    `json:"port"`
}

// Server 把设置、发现列表和内存会话接到 HTTP。
type Server struct {
	id         string
	listenPort int
	settings   *config.Store
	peers      *discover.Service
	msgs       *msgStore
}

// New 组装 API。listenPort 是进程实际绑定的端口，改设置里的端口要重启才生效。
func New(settings *config.Store, peers *discover.Service, id string, listenPort int) *Server {
	if peers == nil {
		peers = discover.New(id, nil)
	}
	if listenPort == 0 && settings != nil {
		listenPort = settings.Get().Port
	}
	return &Server{
		id:         id,
		listenPort: listenPort,
		settings:   settings,
		peers:      peers,
		msgs:       newMsgStore(),
	}
}

// Handler 是带 CORS 的路由。有 web/dist 时直接托管静态页，否则返回占位页。
func (s *Server) Handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /api/info", s.handleInfo)
	mux.HandleFunc("GET /api/peers", s.handleListPeers)
	mux.HandleFunc("POST /api/peers", s.handleAddPeer)
	mux.HandleFunc("POST /api/peers/probe", s.handleProbe)
	mux.HandleFunc("GET /api/messages", s.handleMessages)
	mux.HandleFunc("POST /api/inbox", s.handleInbox)
	mux.HandleFunc("POST /api/send-text", s.handleSendText)
	mux.HandleFunc("POST /api/send-file", s.handleSendFile)
	mux.HandleFunc("POST /api/upload", s.handleUpload)
	mux.HandleFunc("GET /api/settings", s.handleGetSettings)
	mux.HandleFunc("PUT /api/settings", s.handlePutSettings)
	if sub, err := fs.Sub(web.Dist, "dist"); err == nil {
		mux.Handle("/", http.FileServer(http.FS(sub)))
	} else if dir := distDir(); dir != "" {
		mux.Handle("/", http.FileServer(http.Dir(dir)))
	} else {
		mux.HandleFunc("GET /{$}", s.handlePlaceholder)
	}
	return cors(mux)
}

func cors(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "*")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}

func (s *Server) handleInfo(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, infoResp{
		ID:   s.id,
		Name: s.settings.Get().DeviceName,
		Port: s.listenPort,
		OS:   discover.OSTag(),
		IPs:  discover.LocalIPv4s(),
	})
}

func (s *Server) handleListPeers(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, map[string]any{"peers": s.peers.List()})
}

func (s *Server) handleAddPeer(w http.ResponseWriter, r *http.Request) {
	var body addPeerBody
	if err := decodeJSON(r, &body); err != nil {
		writeErr(w, http.StatusBadRequest, "请求不是合法 JSON")
		return
	}
	ip := strings.TrimSpace(body.IP)
	if !validHost(ip) {
		writeErr(w, http.StatusBadRequest, "IP 无效")
		return
	}
	port := body.Port
	if port == 0 {
		port = 8848
	}
	if port < 1 || port > 65535 {
		writeErr(w, http.StatusBadRequest, "端口无效")
		return
	}
	writeJSON(w, http.StatusOK, s.peers.AddManual(ip, strings.TrimSpace(body.Alias), strings.TrimSpace(body.OS), port))
}

func (s *Server) handleProbe(w http.ResponseWriter, r *http.Request) {
	var body probeBody
	if err := decodeJSON(r, &body); err != nil {
		writeErr(w, http.StatusBadRequest, "请求不是合法 JSON")
		return
	}
	ip := strings.TrimSpace(body.IP)
	if !validHost(ip) {
		writeErr(w, http.StatusBadRequest, "IP 无效")
		return
	}
	port := body.Port
	if port == 0 {
		port = 8848
	}
	if port < 1 || port > 65535 {
		writeErr(w, http.StatusBadRequest, "端口无效")
		return
	}
	info, err := fetchInfo(ip, port)
	if err != nil {
		writeJSON(w, http.StatusOK, map[string]any{"ok": false, "error": err.Error()})
		return
	}
	s.peers.Touch(ip, port, info.ID, info.Name, info.OS)
	writeJSON(w, http.StatusOK, map[string]any{"ok": true, "info": info})
}

func (s *Server) handleMessages(w http.ResponseWriter, r *http.Request) {
	peerID := strings.TrimSpace(r.URL.Query().Get("peerId"))
	if peerID == "" {
		writeErr(w, http.StatusBadRequest, "缺少 peerId")
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{"messages": s.msgs.list(s.messageKey(peerID))})
}

func (s *Server) handleInbox(w http.ResponseWriter, r *http.Request) {
	var body inboxBody
	if err := decodeJSON(r, &body); err != nil {
		writeErr(w, http.StatusBadRequest, "请求不是合法 JSON")
		return
	}
	if strings.TrimSpace(body.Text) == "" {
		writeErr(w, http.StatusBadRequest, "文本不能为空")
		return
	}
	host := remoteIP(r.RemoteAddr)
	if body.FromID == "" {
		body.FromID = host
	}
	if body.FromID == "" {
		writeErr(w, http.StatusBadRequest, "缺少发送方")
		return
	}
	key := body.FromID
	if host != "" && body.FromPort > 0 && body.FromPort <= 65535 {
		key = net.JoinHostPort(host, strconv.Itoa(body.FromPort))
		s.peers.Touch(host, body.FromPort, body.FromID, body.FromName, "")
	}
	msg := s.msgs.add(key, Message{
		FromName:  body.FromName,
		Direction: "in",
		Text:      body.Text,
	})
	writeJSON(w, http.StatusOK, map[string]any{"ok": true, "message": msg})
}

func (s *Server) handleSendText(w http.ResponseWriter, r *http.Request) {
	var body sendTextBody
	if err := decodeJSON(r, &body); err != nil {
		writeErr(w, http.StatusBadRequest, "请求不是合法 JSON")
		return
	}
	if strings.TrimSpace(body.Text) == "" {
		writeErr(w, http.StatusBadRequest, "文本不能为空")
		return
	}
	peer, ok := s.peers.Resolve(strings.TrimSpace(body.PeerID), strings.TrimSpace(body.IP), body.Port)
	if !ok {
		writeErr(w, http.StatusNotFound, "找不到该节点")
		return
	}
	if err := s.postInbox(r.Context(), peer, body.Text); err != nil {
		writeErr(w, http.StatusBadGateway, "发送失败，对端无响应")
		return
	}
	msg := s.msgs.add(net.JoinHostPort(peer.IP, strconv.Itoa(peer.Port)), Message{
		FromName:  s.settings.Get().DeviceName,
		Direction: "out",
		Text:      body.Text,
	})
	writeJSON(w, http.StatusOK, map[string]any{"ok": true, "message": msg})
}

func (s *Server) postInbox(ctx context.Context, peer discover.Peer, text string) error {
	payload, err := json.Marshal(inboxBody{
		FromID:   s.id,
		FromName: s.settings.Get().DeviceName,
		FromPort: s.listenPort,
		Text:     text,
	})
	if err != nil {
		return err
	}
	u := fmt.Sprintf("http://%s/api/inbox", net.JoinHostPort(peer.IP, strconv.Itoa(peer.Port)))
	ctx, cancel := context.WithTimeout(ctx, 8*time.Second)
	defer cancel()
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, u, strings.NewReader(string(payload)))
	if err != nil {
		return err
	}
	req.Header.Set("Content-Type", "application/json; charset=utf-8")
	resp, err := textHTTP.Do(req)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	io.Copy(io.Discard, io.LimitReader(resp.Body, 1<<20))
	if resp.StatusCode != http.StatusOK {
		return fmt.Errorf("对端返回 %s", resp.Status)
	}
	return nil
}

func (s *Server) handleGetSettings(w http.ResponseWriter, r *http.Request) {
	writeJSON(w, http.StatusOK, s.settings.Get())
}

func (s *Server) handlePutSettings(w http.ResponseWriter, r *http.Request) {
	var next config.Settings
	if err := decodeJSON(r, &next); err != nil {
		writeErr(w, http.StatusBadRequest, "请求不是合法 JSON")
		return
	}
	saved, err := s.settings.Update(next)
	if err != nil {
		writeErr(w, http.StatusInternalServerError, "保存设置失败")
		return
	}
	if err := os.MkdirAll(saved.DownloadDir, 0o755); err != nil {
		writeErr(w, http.StatusInternalServerError, "创建下载目录失败")
		return
	}
	writeJSON(w, http.StatusOK, saved)
}

func (s *Server) handlePlaceholder(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	io.WriteString(w, placeholderHTML)
}

// messageKey 把 peerId 或 ip:port 收成「对方 IP:端口」。
// 这样手动节点后来被发现换成真实 ID 时，会话仍能对上。
func (s *Server) messageKey(peerID string) string {
	peerID = strings.TrimSpace(peerID)
	if p, ok := s.peers.Resolve(peerID, "", 0); ok {
		return net.JoinHostPort(p.IP, strconv.Itoa(p.Port))
	}
	host, portStr, err := net.SplitHostPort(peerID)
	if err == nil && validHost(host) {
		if port, conv := strconv.Atoi(portStr); conv == nil && port > 0 && port <= 65535 {
			return net.JoinHostPort(host, portStr)
		}
	}
	return peerID
}

func distDir() string {
	candidates := []string{filepath.Join("web", "dist")}
	if exe, err := os.Executable(); err == nil {
		candidates = append(candidates, filepath.Join(filepath.Dir(exe), "web", "dist"))
	}
	for _, c := range candidates {
		st, err := os.Stat(c)
		if err == nil && st.IsDir() {
			return c
		}
	}
	return ""
}

func remoteIP(remoteAddr string) string {
	host, _, err := net.SplitHostPort(remoteAddr)
	if err != nil {
		return strings.TrimSpace(remoteAddr)
	}
	ip := net.ParseIP(host)
	if ip == nil {
		return host
	}
	if v4 := ip.To4(); v4 != nil {
		return v4.String()
	}
	return ip.String()
}

func validHost(s string) bool {
	if s == "" || len(s) > 253 || strings.ContainsAny(s, " \t\r\n/\\") {
		return false
	}
	return true
}

func decodeJSON(r *http.Request, dst any) error {
	dec := json.NewDecoder(io.LimitReader(r.Body, 1<<20))
	return dec.Decode(dst)
}

func writeErr(w http.ResponseWriter, code int, msg string) {
	writeJSON(w, code, map[string]string{"error": msg})
}

func writeJSON(w http.ResponseWriter, code int, v any) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.Header().Set("Cache-Control", "no-store")
	w.WriteHeader(code)
	enc := json.NewEncoder(w)
	enc.SetEscapeHTML(false)
	_ = enc.Encode(v)
}

func fetchInfo(ip string, port int) (infoResp, error) {
	u := fmt.Sprintf("http://%s/api/info", net.JoinHostPort(ip, strconv.Itoa(port)))
	req, err := http.NewRequest(http.MethodGet, u, nil)
	if err != nil {
		return infoResp{}, err
	}
	resp, err := probeHTTP.Do(req)
	if err != nil {
		return infoResp{}, fmt.Errorf("无法连接 %s", net.JoinHostPort(ip, strconv.Itoa(port)))
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(io.LimitReader(resp.Body, 1<<20))
	if err != nil {
		return infoResp{}, fmt.Errorf("读取对端信息失败")
	}
	if resp.StatusCode != http.StatusOK {
		return infoResp{}, fmt.Errorf("对端返回 %s", resp.Status)
	}
	var info infoResp
	if err := json.Unmarshal(body, &info); err != nil {
		return infoResp{}, fmt.Errorf("对端信息无法解析")
	}
	return info, nil
}

var (
	textHTTP = &http.Client{
		Timeout:       10 * time.Second,
		CheckRedirect: refuseRedirect,
		Transport:     lanTransport(5 * time.Second),
	}
	probeHTTP = &http.Client{
		Timeout:       3 * time.Second,
		CheckRedirect: refuseRedirect,
		Transport:     lanTransport(3 * time.Second),
	}
	// 传文件不设总超时，避免大文件传到一半被掐断。只限制拨号。
	fileHTTP = &http.Client{
		CheckRedirect: refuseRedirect,
		Transport:     lanTransport(5 * time.Second),
	}
)

func refuseRedirect(req *http.Request, via []*http.Request) error {
	return http.ErrUseLastResponse
}

func lanTransport(dial time.Duration) *http.Transport {
	return &http.Transport{
		Proxy:               nil, // 局域网直连，不走系统代理
		DialContext:         (&net.Dialer{Timeout: dial}).DialContext,
		DisableCompression:  true,
		MaxIdleConnsPerHost: 4,
	}
}

type msgStore struct {
	mu sync.Mutex
	by map[string][]Message
}

func newMsgStore() *msgStore {
	return &msgStore{by: map[string][]Message{}}
}

func (m *msgStore) add(peerKey string, msg Message) Message {
	msg.ID = newID()
	msg.PeerID = peerKey
	msg.Time = time.Now()
	m.mu.Lock()
	defer m.mu.Unlock()
	list := append(m.by[peerKey], msg)
	// ponytail: 每个会话只留最近 500 条在内存里，进程退出即丢。升级路径是按节点落盘。
	if len(list) > 500 {
		list = append([]Message(nil), list[len(list)-500:]...)
	}
	m.by[peerKey] = list
	return msg
}

func (m *msgStore) list(peerKey string) []Message {
	m.mu.Lock()
	defer m.mu.Unlock()
	src := m.by[peerKey]
	out := make([]Message, len(src))
	copy(out, src)
	return out
}

func newID() string {
	var b [8]byte
	if _, err := rand.Read(b[:]); err != nil {
		return strconv.FormatInt(time.Now().UnixNano(), 36)
	}
	return hex.EncodeToString(b[:])
}

const placeholderHTML = `<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>局域快传</title>
</head>
<body>
<h1>局域快传</h1>
<p>HTTP API 已启动。当前没有 <code>web/dist</code>，这是占位页。</p>
<ul>
<li>GET /api/info</li>
<li>GET /api/peers</li>
<li>POST /api/peers</li>
<li>POST /api/peers/probe</li>
<li>GET /api/messages?peerId=</li>
<li>POST /api/send-text</li>
<li>POST /api/inbox</li>
<li>POST /api/send-file</li>
<li>POST /api/upload</li>
<li>GET /api/settings</li>
<li>PUT /api/settings</li>
</ul>
</body>
</html>
`
