import { useEffect, useState } from 'react';
import { X } from 'lucide-react';
import { SettingsGear } from './DeviceAvatar';
import type { AppSettings } from '../types';

interface SettingsModalProps {
  isOpen: boolean;
  onClose: () => void;
  settings: AppSettings;
  onSave: (next: AppSettings) => Promise<void>;
}

export function SettingsModal({ isOpen, onClose, settings, onSave }: SettingsModalProps) {
  const [draft, setDraft] = useState(settings);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    if (isOpen) {
      setDraft(settings);
      setError('');
    }
  }, [isOpen, settings]);

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-slate-900/60 z-50 flex items-center justify-center p-4">
      <div className="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-md w-full overflow-hidden">
        <div className="px-5 py-4 border-b border-slate-200 flex items-center justify-between bg-slate-50">
          <div className="flex items-center gap-2">
            <div className="p-2 rounded-lg bg-slate-800 text-white">
              <SettingsGear className="w-4 h-4" stroke="#ffffff" />
            </div>
            <div>
              <h3 className="text-sm font-bold text-slate-900">局域快传设置</h3>
              <p className="text-[11px] text-slate-500">设备名称、端口与下载目录</p>
            </div>
          </div>
          <button onClick={onClose} className="p-1 rounded-md text-slate-400 hover:text-slate-700 hover:bg-slate-200">
            <X className="w-4 h-4" />
          </button>
        </div>

        <div className="p-5 space-y-4 text-xs">
          <div className="space-y-1">
            <label className="font-semibold text-slate-700">本机设备名称</label>
            <input
              value={draft.deviceName}
              onChange={(e) => setDraft({ ...draft, deviceName: e.target.value })}
              className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg outline-hidden focus:border-blue-500"
            />
          </div>
          <div className="space-y-1">
            <label className="font-semibold text-slate-700">HTTP 监听端口</label>
            <input
              type="number"
              value={draft.port}
              onChange={(e) => setDraft({ ...draft, port: parseInt(e.target.value, 10) || 8848 })}
              className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg font-mono outline-hidden focus:border-blue-500"
            />
            <p className="text-[10px] text-slate-400">改端口后需重启进程，并在防火墙放行 TCP 端口</p>
          </div>
          <div className="space-y-1">
            <label className="font-semibold text-slate-700">下载目录</label>
            <input
              value={draft.downloadDirectory}
              onChange={(e) => setDraft({ ...draft, downloadDirectory: e.target.value })}
              className="w-full px-3 py-2 bg-slate-50 border border-slate-200 rounded-lg font-mono outline-hidden focus:border-blue-500"
            />
          </div>
          <label className="flex items-center justify-between">
            <span className="text-slate-700 font-medium">允许窗口轻颤提醒</span>
            <input
              type="checkbox"
              checked={draft.nudgeEnabled}
              onChange={(e) => setDraft({ ...draft, nudgeEnabled: e.target.checked })}
            />
          </label>
          <label className="flex items-center justify-between">
            <span className="text-slate-700 font-medium">新消息提示音</span>
            <input
              type="checkbox"
              checked={draft.soundNotification}
              onChange={(e) => setDraft({ ...draft, soundNotification: e.target.checked })}
            />
          </label>
          {error && <p className="text-red-600">{error}</p>}
        </div>

        <div className="px-5 py-3 border-t border-slate-200 bg-slate-50 flex justify-end">
          <button
            disabled={saving}
            onClick={() => {
              setSaving(true);
              setError('');
              void onSave(draft)
                .then(() => onClose())
                .catch((err: unknown) => setError(err instanceof Error ? err.message : '保存失败'))
                .finally(() => setSaving(false));
            }}
            className="px-4 py-1.5 bg-blue-600 hover:bg-blue-700 text-white rounded-lg text-xs font-medium disabled:opacity-50"
          >
            {saving ? '保存中…' : '保存并关闭'}
          </button>
        </div>
      </div>
    </div>
  );
}
