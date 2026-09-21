import { Globe, Laptop, Radio } from 'lucide-react';
import type { PeerNode } from '../types';
import { SettingsGear } from './DeviceAvatar';

interface HeaderProps {
  currentNode: PeerNode;
  backendOk: boolean;
  onOpenSettingsModal: () => void;
  onOpenWebShareModal: () => void;
}

export function Header({
  currentNode,
  backendOk,
  onOpenSettingsModal,
  onOpenWebShareModal,
}: HeaderProps) {
  return (
    <header className="border-b border-slate-200 bg-white/95 backdrop-blur-md sticky top-0 z-30 shadow-xs">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 py-2.5 flex items-center justify-between gap-3">
        <div className="flex items-center gap-3">
          <div className="flex items-center justify-center w-9 h-9 rounded-xl bg-blue-600 text-white shadow-xs">
            <Radio className="w-5 h-5" />
          </div>
          <div>
            <h1 className="text-lg font-bold text-slate-900 tracking-tight">局域快传</h1>
            <div className="flex items-center gap-1.5 text-xs text-slate-500">
              <span className={`w-1.5 h-1.5 rounded-full inline-block ${backendOk ? 'bg-emerald-500' : 'bg-amber-500'}`} />
              <span>{backendOk ? '在线 · 正在发现局域网设备' : '未连上本机服务'}</span>
            </div>
          </div>
        </div>

        <div className="hidden md:flex items-center gap-2">
          <div className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-50 border border-slate-200/80 text-xs text-slate-600">
            <Laptop className="w-3.5 h-3.5 text-blue-600" />
            <span>本机:</span>
            <span className="font-medium text-slate-800">{currentNode.name}</span>
            <span className="text-slate-400 font-mono">({currentNode.ip || '—'})</span>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={onOpenWebShareModal}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-blue-50/90 hover:bg-blue-100 border border-blue-200 text-blue-700 text-xs font-medium transition shadow-2xs"
            title="HTTP 网页共享"
          >
            <Globe className="w-3.5 h-3.5 text-blue-600" />
            <span>网页共享 (HTTP)</span>
          </button>
          <button
            onClick={onOpenSettingsModal}
            className="p-1.5 rounded-md border-none bg-transparent hover:bg-[#F1F5F9] transition"
            title="传输与节点设置"
          >
            <SettingsGear className="w-5 h-5" />
          </button>
        </div>
      </div>
    </header>
  );
}
