import { Globe, X } from 'lucide-react';

interface WebShareModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export function WebShareModal({ isOpen, onClose }: WebShareModalProps) {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-slate-900/60 z-50 flex items-center justify-center p-4">
      <div className="bg-white rounded-2xl shadow-2xl border border-slate-200 max-w-md w-full overflow-hidden">
        <div className="px-5 py-4 border-b border-slate-200 flex items-center justify-between bg-slate-50">
          <div className="flex items-center gap-2">
            <div className="p-2 rounded-lg bg-blue-600 text-white">
              <Globe className="w-4 h-4" />
            </div>
            <h3 className="text-sm font-bold text-slate-900">HTTP 网页共享</h3>
          </div>
          <button onClick={onClose} className="p-1 rounded-md text-slate-400 hover:text-slate-700 hover:bg-slate-200">
            <X className="w-4 h-4" />
          </button>
        </div>
        <div className="p-5 text-sm text-slate-600 space-y-2">
          <p>后续版本会支持用浏览器打开本机页面，拉取共享文件（对方没有客户端时的兜底）。</p>
          <p className="text-xs text-slate-400">当前版本请用局域快传客户端互传文字和文件。</p>
        </div>
        <div className="px-5 py-3 border-t border-slate-200 bg-slate-50 flex justify-end">
          <button onClick={onClose} className="px-4 py-1.5 rounded-lg bg-slate-800 text-white text-xs">
            关闭
          </button>
        </div>
      </div>
    </div>
  );
}
