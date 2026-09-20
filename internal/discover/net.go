package discover

import "net"

// 名称里带这些词的接口不当发现网卡用。规格点名的虚拟网卡，外加常见隧道/容器网卡。
var virtualHints = []string{
	"vmware", "virtualbox", "vbox", "hyper-v", "vethernet", "wsl",
	"loopback", "docker", "tap", "tun", "virtual", "虚拟",
}

func virtualName(name string) bool {
	n := lowerASCII(name)
	for _, h := range virtualHints {
		if containsFold(n, h) {
			return true
		}
	}
	return false
}

func skipIface(iface net.Interface) bool {
	if iface.Flags&net.FlagUp == 0 {
		return true
	}
	if iface.Flags&net.FlagLoopback != 0 {
		return true
	}
	return virtualName(iface.Name)
}

func lowerASCII(s string) string {
	b := []byte(s)
	for i, c := range b {
		if c >= 'A' && c <= 'Z' {
			b[i] = c + ('a' - 'A')
		}
	}
	return string(b)
}

func containsFold(lowered, hint string) bool {
	h := lowerASCII(hint)
	return len(h) > 0 && indexFold(lowered, h) >= 0
}

func indexFold(s, sub string) int {
	if len(sub) == 0 || len(s) < len(sub) {
		return -1
	}
	for i := 0; i+len(sub) <= len(s); i++ {
		if s[i:i+len(sub)] == sub {
			return i
		}
	}
	return -1
}

func ipv4Nets(iface net.Interface) []*net.IPNet {
	addrs, err := iface.Addrs()
	if err != nil {
		return nil
	}
	var out []*net.IPNet
	for _, a := range addrs {
		ipnet, ok := a.(*net.IPNet)
		if !ok || ipnet.IP.To4() == nil || ipnet.IP.IsUnspecified() {
			continue
		}
		out = append(out, &net.IPNet{IP: ipnet.IP.To4(), Mask: ipnet.Mask})
	}
	return out
}

// LocalIPv4s 返回可用于展示的本机 IPv4，已跳过回环和虚拟网卡。
func LocalIPv4s() []string {
	ifaces, err := net.Interfaces()
	if err != nil {
		return []string{}
	}
	var ips []string
	seen := map[string]bool{}
	for _, iface := range ifaces {
		if skipIface(iface) {
			continue
		}
		for _, n := range ipv4Nets(iface) {
			s := n.IP.String()
			if seen[s] {
				continue
			}
			seen[s] = true
			ips = append(ips, s)
		}
	}
	if ips == nil {
		ips = []string{}
	}
	return ips
}

func broadcastTargets() []net.IP {
	seen := map[string]net.IP{"255.255.255.255": net.IPv4bcast}
	ifaces, err := net.Interfaces()
	if err != nil {
		return []net.IP{net.IPv4bcast}
	}
	for _, iface := range ifaces {
		if skipIface(iface) {
			continue
		}
		for _, n := range ipv4Nets(iface) {
			ones, bits := n.Mask.Size()
			if bits != 32 || ones >= 31 {
				continue
			}
			bcast := broadcastIP(n)
			if bcast == nil {
				continue
			}
			seen[bcast.String()] = bcast
		}
	}
	out := make([]net.IP, 0, len(seen))
	for _, ip := range seen {
		out = append(out, ip)
	}
	return out
}

func broadcastIP(n *net.IPNet) net.IP {
	ip := n.IP.To4()
	if ip == nil {
		return nil
	}
	mask := n.Mask
	if len(mask) == 16 {
		mask = mask[12:]
	}
	if len(mask) != 4 {
		return nil
	}
	out := make(net.IP, 4)
	for i := 0; i < 4; i++ {
		out[i] = ip[i] | ^mask[i]
	}
	return out
}
