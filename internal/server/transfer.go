package server

import (
	"errors"
	"fmt"
	"io"
	"mime"
	"mime/multipart"
	"net"
	"net/http"
	"net/textproto"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/miaotaogou-create/lan-drop/internal/discover"
)

var errBadName = errors.New("无效文件名")

// SaveStream 把上传内容写到下载目录，不把整个文件读进内存。
// 同名文件不覆盖，避免把已经收好的文件冲掉。
func SaveStream(dir, filename string, r io.Reader) (string, int64, error) {
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return "", 0, err
	}
	name, err := safeName(filename)
	if err != nil {
		return "", 0, err
	}
	f, path, err := createUnique(dir, name)
	if err != nil {
		return "", 0, err
	}
	n, copyErr := io.Copy(f, r)
	syncErr := f.Sync()
	closeErr := f.Close()
	if copyErr != nil || syncErr != nil || closeErr != nil {
		os.Remove(path)
		if copyErr != nil {
			return "", n, copyErr
		}
		if syncErr != nil {
			return "", n, syncErr
		}
		return "", n, closeErr
	}
	return path, n, nil
}

func safeName(name string) (string, error) {
	name = strings.ReplaceAll(name, "\\", "/")
	name = filepath.Base(name)
	name = strings.TrimSpace(name)
	if name == "" || name == "." || name == ".." {
		return "", errBadName
	}
	if strings.ContainsAny(name, "/\\:*?\"<>|") {
		return "", errBadName
	}
	if len(name) > 255 {
		return "", errBadName
	}
	return name, nil
}

func createUnique(dir, name string) (*os.File, string, error) {
	ext := filepath.Ext(name)
	base := strings.TrimSuffix(name, ext)
	for i := 0; i < 10000; i++ {
		candidate := name
		if i > 0 {
			candidate = fmt.Sprintf("%s_%d%s", base, i+1, ext)
		}
		path := filepath.Join(dir, candidate)
		f, err := os.OpenFile(path, os.O_CREATE|os.O_EXCL|os.O_WRONLY, 0o644)
		if errors.Is(err, os.ErrExist) {
			continue
		}
		if err != nil {
			return nil, "", err
		}
		return f, path, nil
	}
	return nil, "", fmt.Errorf("无法生成不冲突的文件名")
}

func (s *Server) handleUpload(w http.ResponseWriter, r *http.Request) {
	mr, err := r.MultipartReader()
	if err != nil {
		writeErr(w, http.StatusBadRequest, "无法解析上传数据")
		return
	}
	for {
		part, err := mr.NextPart()
		if err == io.EOF {
			break
		}
		if err != nil {
			writeErr(w, http.StatusBadRequest, "读取上传数据失败")
			return
		}
		if part.FormName() != "file" {
			part.Close()
			continue
		}
		name := part.FileName()
		if name == "" {
			name = "未命名文件"
		}
		path, n, saveErr := SaveStream(s.settings.Get().DownloadDir, name, part)
		part.Close()
		if saveErr != nil {
			if errors.Is(saveErr, errBadName) {
				writeErr(w, http.StatusBadRequest, "文件名无效")
				return
			}
			writeErr(w, http.StatusInternalServerError, "保存文件失败")
			return
		}
		writeJSON(w, http.StatusOK, map[string]any{
			"ok":   true,
			"name": filepath.Base(path),
			"size": n,
			"path": path,
		})
		return
	}
	writeErr(w, http.StatusBadRequest, "缺少文件")
}

func (s *Server) handleSendFile(w http.ResponseWriter, r *http.Request) {
	mr, err := r.MultipartReader()
	if err != nil {
		writeErr(w, http.StatusBadRequest, "无法解析表单")
		return
	}
	var peerID, ip, filename string
	var port int
	var tmp *os.File
	var size int64
	defer func() {
		if tmp == nil {
			return
		}
		name := tmp.Name()
		tmp.Close()
		os.Remove(name)
	}()

	for {
		part, err := mr.NextPart()
		if err == io.EOF {
			break
		}
		if err != nil {
			writeErr(w, http.StatusBadRequest, "读取表单失败")
			return
		}
		switch part.FormName() {
		case "peerId":
			peerID = readField(part)
		case "ip":
			ip = readField(part)
		case "port":
			port, _ = strconv.Atoi(readField(part))
		case "file":
			if tmp != nil {
				part.Close()
				continue
			}
			filename = part.FileName()
			f, err := os.CreateTemp("", "landrop-send-*")
			if err != nil {
				part.Close()
				writeErr(w, http.StatusInternalServerError, "无法暂存文件")
				return
			}
			n, err := io.Copy(f, part)
			part.Close()
			if err != nil {
				f.Close()
				os.Remove(f.Name())
				writeErr(w, http.StatusBadRequest, "读取文件失败")
				return
			}
			if _, err := f.Seek(0, io.SeekStart); err != nil {
				f.Close()
				os.Remove(f.Name())
				writeErr(w, http.StatusInternalServerError, "无法暂存文件")
				return
			}
			tmp = f
			size = n
			continue
		default:
			io.Copy(io.Discard, part)
		}
		part.Close()
	}

	if tmp == nil {
		writeErr(w, http.StatusBadRequest, "缺少文件")
		return
	}
	if filename == "" {
		filename = "未命名文件"
	}
	filename = filepath.Base(strings.ReplaceAll(filename, "\\", "/"))
	peer, ok := s.peers.Resolve(peerID, ip, port)
	if !ok {
		writeErr(w, http.StatusNotFound, "找不到该节点")
		return
	}
	// ponytail: 先落临时文件再转发，避免表单字段顺序不定时把整个文件读进内存。
	// 上限是多占一份磁盘；字段若总在文件之前，可改为 io.Pipe 边读边传。
	if err := forwardFile(peer, filename, tmp); err != nil {
		writeErr(w, http.StatusBadGateway, "发送失败，对端未保存")
		return
	}
	writeJSON(w, http.StatusOK, map[string]any{
		"ok":   true,
		"name": filename,
		"size": size,
	})
}

func readField(p *multipart.Part) string {
	b, _ := io.ReadAll(io.LimitReader(p, 4096))
	io.Copy(io.Discard, p)
	return strings.TrimSpace(string(b))
}

func forwardFile(peer discover.Peer, filename string, src io.Reader) error {
	pr, pw := io.Pipe()
	mw := multipart.NewWriter(pw)
	errc := make(chan error, 1)
	go func() {
		part, err := createFilePart(mw, filename)
		if err != nil {
			pw.CloseWithError(err)
			errc <- err
			return
		}
		if _, err = io.Copy(part, src); err != nil {
			pw.CloseWithError(err)
			errc <- err
			return
		}
		if err = mw.Close(); err != nil {
			pw.CloseWithError(err)
			errc <- err
			return
		}
		pw.Close()
		errc <- nil
	}()

	rawURL := fmt.Sprintf("http://%s/api/upload", net.JoinHostPort(peer.IP, strconv.Itoa(peer.Port)))
	req, err := http.NewRequest(http.MethodPost, rawURL, pr)
	if err != nil {
		pr.CloseWithError(err)
		<-errc
		return err
	}
	req.Header.Set("Content-Type", mw.FormDataContentType())
	resp, err := fileHTTP.Do(req)
	if err != nil {
		pr.CloseWithError(err)
		<-errc
		return fmt.Errorf("连接对端失败")
	}
	defer resp.Body.Close()
	copyErr := <-errc
	body, _ := io.ReadAll(io.LimitReader(resp.Body, 4096))
	if copyErr != nil {
		return copyErr
	}
	if resp.StatusCode != http.StatusOK {
		msg := strings.TrimSpace(string(body))
		if msg == "" {
			msg = resp.Status
		}
		return fmt.Errorf("对端保存失败: %s", msg)
	}
	return nil
}

func createFilePart(w *multipart.Writer, filename string) (io.Writer, error) {
	h := make(textproto.MIMEHeader)
	// FormatMediaType 会按 RFC 处理中文文件名，避免只靠裸引号把名字写坏。
	disp := mime.FormatMediaType("form-data", map[string]string{
		"name":     "file",
		"filename": filename,
	})
	if disp == "" {
		return w.CreateFormFile("file", filename)
	}
	h.Set("Content-Disposition", disp)
	h.Set("Content-Type", "application/octet-stream")
	return w.CreatePart(h)
}
