import { useState } from 'react';
import { Globe, Plus, X, Zap } from 'lucide-react';

interface DirectDialModalProps {
  isOpen: boolean;
  onClose: () => void;
  onAddPeer: (input: { ip: string; port: number; name: string }) => Promise<void>;
  onProbe: (ip: string, port: number) => Promise<{ ok: boolean; latencyMs?: number; error?: string }>;
}

export function DirectDialModal({ isOpen, onClose, onAddPeer, onProbe }: DirectDialModalProps) {
  const [ip, setIp] = useState('');
  const [port, setPort] = useState('8848');
  const [name, setName] = useState('');
  const [isPinging, setIsPinging] = useState(false);
  const [probeText, setProbeText] = useState('');
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState('');

  if (!isOpen) return null;

  const portNum = parseInt(port, 10) || 8848;

  return (
    <div className="fixed inset-0 bg-slate-900/60 z-50 flex items-center justify-center p-4">
      <div className="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-md w-full overflow-hidden">
        <div className="px-5 py-4 border-b border-slate-200 flex items-center justify-between bg-slate-50">
          <div className="flex items-center gap-2">
            <div className="p-2 rounded-lg bg-blue-600 text-white">
              <Globe className="w-4 h-4" />
            </div>
            <div>
              <h3 className="text-sm font-bold text-slate-900">手动添加节点</h3>
              <p className="text-[11px] text-slate-500">跨网段、或发现被关掉时，直接填对端 IP</p>
            </div>
          </div>
          <button onClick={onClose} className="p-1 rounded-md text-slate-400 hover:text-slate-700 hover:bg-slate-200">
            <X className="w-4 h-4" />
          </button>
        </div>

        <form
          onSubmit={(e) => {
            e.preventDefault();
            if (!ip.trim() || saving) return;
            setSaving(true);
            setError('');
            void onAddPeer({ ip: ip.trim(), port: portNum, name: name.trim() })
              .then(() => onClose())
              .catch((err: unknown) => setError(err instanceof Error ? err.message : '添加失败'))
              .finally(() => setSaving(false));
          }}
          className="p-5 space-y-4 text-xs"
        >
          <div className="grid grid-cols-3 gap-2">
            <div className="col-span-2 space-y-1">
              <label className="font-semibold text-slate-700">目标 IP 地址</label>
              <input
                required
                value={ip}
                onChange={(e) => setIp(e.target.value)}
                placeholder="192.168.1.10"
                className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg font-mono focus:bg-white focus:border-blue-500 outline-hidden"
              />
            </div>
            <div className="space-y-1">
              <label className="font-semibold text-slate-700">端口</label>
              <input
                type="number"
                value={port}
                onChange={(e) => setPort(e.target.value)}
                className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg font-mono focus:bg-white focus:border-blue-500 outline-hidden"
              />
            </div>
          </div>
          <div className="space-y-1">
            <label className="font-semibold text-slate-700">设备别名（可选）</label>
            <input
              value={name}
              onChange={(e) => setName(e.target.value)}
              placeholder="例如：机房测试机"
              className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg focus:bg-white focus:border-blue-500 outline-hidden"
            />
          </div>
          <div className="flex items-center justify-between gap-2">
            <button
              type="button"
              disabled={isPinging || !ip.trim()}
              onClick={() => {
                setIsPinging(true);
                setProbeText('');
                void onProbe(ip.trim(), portNum)
                  .then((result) => {
                    setProbeText(
                      result.ok
                        ? `连通${result.latencyMs != null ? `（${result.latencyMs} ms）` : ''}`
                        : result.error || '不通',
                    );
                  })
                  .catch((err: unknown) => setProbeText(err instanceof Error ? err.message : '探测失败'))
                  .finally(() => setIsPinging(false));
              }}
              className="text-xs text-blue-600 font-medium flex items-center gap-1 disabled:opacity-50"
            >
              <Zap className="w-3.5 h-3.5" />
              {isPinging ? '正在探测…' : '测试连通（/api/info）'}
            </button>
            {probeText && <span className="text-[11px] text-slate-600">{probeText}</span>}
          </div>
          {error && <p className="text-red-600">{error}</p>}
          <div className="pt-3 border-t border-slate-200 flex justify-end gap-2">
            <button type="button" onClick={onClose} className="px-3.5 py-1.5 rounded-lg border border-slate-200 text-slate-600">
              取消
            </button>
            <button
              type="submit"
              disabled={saving}
              className="px-4 py-1.5 rounded-lg bg-blue-600 text-white font-medium flex items-center gap-1 disabled:opacity-50"
            >
              <Plus className="w-3.5 h-3.5" />
              {saving ? '添加中…' : '添加并连接'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
