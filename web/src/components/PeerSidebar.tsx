import { useState } from 'react';
import { Plus, Radio, Search, Zap } from 'lucide-react';
import type { PeerNode } from '../types';
import { DeviceAvatar } from './DeviceAvatar';

interface PeerSidebarProps {
  peers: PeerNode[];
  selectedPeerId: string;
  onSelectPeer: (peerId: string) => void;
  onOpenDirectDialModal: () => void;
}

function osBadge(os: PeerNode['os']) {
  switch (os) {
    case 'macos':
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-slate-100 text-slate-600">macOS</span>;
    case 'windows':
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-blue-50 text-blue-600">Windows</span>;
    case 'linux':
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-amber-50 text-amber-700">Linux</span>;
    case 'arm-linux':
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-emerald-50 text-emerald-700 font-medium">ARM64</span>;
    case 'ios':
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-purple-50 text-purple-600">iOS</span>;
    default:
      return <span className="text-[10px] px-1.5 py-0.5 rounded bg-green-50 text-green-700">Android</span>;
  }
}

export function PeerSidebar({ peers, selectedPeerId, onSelectPeer, onOpenDirectDialModal }: PeerSidebarProps) {
  const [searchTerm, setSearchTerm] = useState('');
  const filteredPeers = peers.filter((peer) => {
    if (peer.isLocal) return false;
    const q = searchTerm.toLowerCase();
    return (
      peer.name.toLowerCase().includes(q) ||
      peer.ip.includes(searchTerm) ||
      peer.hostname.toLowerCase().includes(q)
    );
  });

  return (
    <aside className="w-full md:w-80 lg:w-88 border-r border-slate-200 bg-slate-50/70 flex flex-col shrink-0 h-[calc(100vh-65px)]">
      <div className="p-3 border-b border-slate-200 bg-white">
        <div className="flex items-center justify-between mb-2">
          <div className="flex items-center gap-2 text-xs font-semibold text-slate-800">
            <div className="relative flex items-center justify-center">
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-500" />
              <span className="animate-ping absolute w-4 h-4 rounded-full bg-emerald-400 opacity-60" />
            </div>
            <span>附近在线设备</span>
            <span className="text-[11px] px-1.5 py-0.2 rounded-full bg-emerald-50 text-emerald-700 font-medium border border-emerald-200/50">
              {filteredPeers.length}
            </span>
          </div>
          <button
            onClick={onOpenDirectDialModal}
            className="p-1 rounded-md text-slate-400 hover:text-slate-700 hover:bg-slate-100 transition flex items-center gap-1 text-[11px]"
            title="手动添加节点"
          >
            <Plus className="w-3.5 h-3.5" />
            <span className="text-slate-500">加 IP</span>
          </button>
        </div>
        <div className="relative">
          <Search className="w-3.5 h-3.5 absolute left-2.5 top-2.5 text-slate-400" />
          <input
            type="text"
            placeholder="搜索设备名称或 IP..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full pl-8 pr-3 py-1.5 text-xs bg-slate-100/80 focus:bg-white border border-transparent focus:border-blue-400 rounded-lg outline-hidden text-slate-800 transition"
          />
        </div>
      </div>

      <div className="flex-1 overflow-y-auto p-2 space-y-1.5">
        {filteredPeers.length === 0 ? (
          <div className="p-6 text-center text-xs text-slate-400">
            <Radio className="w-8 h-8 mx-auto mb-2 text-slate-300 animate-pulse" />
            <p>还没有发现其他设备</p>
            <p className="text-[11px] text-slate-400 mt-1">同一网段会自动出现；跨网段请手动加 IP</p>
            <button
              onClick={onOpenDirectDialModal}
              className="mt-3 inline-flex items-center gap-1 text-blue-600 hover:underline font-medium"
            >
              <Plus className="w-3.5 h-3.5" /> 手动输入 IP 连接
            </button>
          </div>
        ) : (
          filteredPeers.map((peer) => {
            const isSelected = peer.id === selectedPeerId;
            return (
              <button
                key={peer.id}
                onClick={() => onSelectPeer(peer.id)}
                className={`w-full text-left p-2.5 rounded-xl border transition group flex items-start gap-3 relative ${
                  isSelected
                    ? 'bg-white border-blue-500/50 shadow-sm ring-1 ring-blue-500/20'
                    : 'bg-white/70 hover:bg-white border-slate-200/80 hover:border-slate-300 hover:shadow-xs'
                }`}
              >
                <DeviceAvatar name={peer.name} color={peer.avatarColor} os={peer.os} size={44} />
                <div className="flex-1 min-w-0">
                  <div className="flex items-center justify-between gap-1">
                    <span className="font-semibold text-xs text-slate-900 truncate">{peer.name}</span>
                    {osBadge(peer.os)}
                  </div>
                  <div className="text-[11px] text-slate-500 font-mono truncate mt-0.5">
                    {peer.ip}:{peer.port}
                  </div>
                  <div className="flex items-center justify-between mt-1 text-[10px] text-slate-400">
                    <span className="truncate max-w-[130px] font-mono text-slate-500">{peer.hostname || peer.status}</span>
                    {peer.latencyMs > 0 && (
                      <span className="flex items-center gap-1 font-mono text-emerald-600 font-medium">
                        <Zap className="w-2.5 h-2.5" /> {peer.latencyMs}ms
                      </span>
                    )}
                  </div>
                  {peer.department && (
                    <div className="mt-1 text-[10px] text-slate-400 truncate">
                      {peer.department}
                      {peer.linkSpeed ? ` · ${peer.linkSpeed}` : ''}
                    </div>
                  )}
                </div>
              </button>
            );
          })
        )}
      </div>
    </aside>
  );
}
