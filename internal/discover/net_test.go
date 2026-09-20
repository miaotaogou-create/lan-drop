package discover

import "testing"

func TestVirtualNames(t *testing.T) {
	for _, name := range []string{
		"VMware Network Adapter VMnet8",
		"VirtualBox Host-Only",
		"vEthernet (WSL)",
		"Hyper-V Virtual Ethernet Adapter",
		"Loopback Pseudo-Interface 1",
		"docker0",
	} {
		if !virtualName(name) {
			t.Fatalf("应过滤 %s", name)
		}
	}
	for _, name := range []string{"以太网", "WLAN", "eth0"} {
		if virtualName(name) {
			t.Fatalf("不应过滤 %s", name)
		}
	}
}
