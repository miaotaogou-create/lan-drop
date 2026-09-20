import { CheckCircle2, Download, FileText, Folder, HardDrive, Trash2 } from 'lucide-react';
import type { PeerNode, TransferFile } from '../types';

interface FileTransferMatrixProps {
  transfers: TransferFile[];
  peers: PeerNode[];
  onClearCompleted: () => void;
  downloadDir: string;
}

function formatBytes(bytes: number) {
  if (!bytes) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.min(units.length - 1, Math.floor(Math.log(bytes) / Math.log(1024)));
  return `${parseFloat((bytes / 1024 ** i).toFixed(1))} ${units[i]}`;
}

export function FileTransferMatrix({ transfers, peers, onClearCompleted, downloadDir }: FileTransferMatrixProps) {
  const peerName = (id: string) => peers.find((p) => p.id === id)?.name || id;
  const hasCompleted = transfers.some((t) => t.status === 'completed');

  return (
    <div className="h-full flex flex-col bg-white">
      <div className="p-4 border-b border-slate-200 flex items-center justify-between">
        <div>
          <h3 className="text-sm font-bold text-slate-900 flex items-center gap-2">
            <HardDrive className="w-4 h-4 text-blue-600" />
            文件传输列表
          </h3>
          <div className="text-xs text-slate-500 mt-0.5">
            保存目录: <code className="font-mono text-slate-700 bg-slate-100 px-1 py-0.5 rounded">{downloadDir}</code>
          </div>
        </div>
        {hasCompleted && (
          <button onClick={onClearCompleted} className="text-xs text-slate-500 hover:text-slate-800 flex items-center gap-1">
            <Trash2 className="w-3.5 h-3.5" /> 清空已完成
          </button>
        )}
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-3">
        {transfers.length === 0 ? (
          <div className="text-center py-12 text-slate-400 text-xs">
            <Download className="w-10 h-10 mx-auto text-slate-300 mb-2" />
            <p>暂无传输任务</p>
            <p className="mt-1">在聊天里发送文件后，记录会出现在这里</p>
          </div>
        ) : (
          transfers.map((item) => {
            const done = item.status === 'completed';
            return (
              <div key={item.id} className="border border-slate-200 rounded-xl p-3.5 bg-slate-50/50 space-y-2">
                <div className="flex items-start justify-between gap-2">
                  <div className="flex items-center gap-2.5 min-w-0">
                    <div className="p-2 rounded-lg bg-blue-50 shrink-0">
                      {item.isDirectory ? <Folder className="w-5 h-5 text-amber-500" /> : <FileText className="w-5 h-5 text-blue-600" />}
                    </div>
                    <div className="min-w-0">
                      <div className="text-xs font-semibold text-slate-900 truncate">{item.name}</div>
                      <div className="text-[11px] text-slate-500 mt-0.5">
                        {formatBytes(item.size)} · 来自 {peerName(item.senderNodeId)}
                      </div>
                    </div>
                  </div>
                  <span className="font-mono text-xs text-slate-700">{done ? '完成' : item.status === 'failed' ? '失败' : '传输中'}</span>
                </div>
                <div className="w-full h-2 bg-slate-200 rounded-full overflow-hidden">
                  <div className={`h-full rounded-full ${done ? 'bg-emerald-500' : 'bg-blue-600'}`} style={{ width: `${item.progress}%` }} />
                </div>
                {done && (
                  <span className="text-[11px] text-emerald-600 inline-flex items-center gap-1">
                    <CheckCircle2 className="w-3 h-3" /> 已落盘
                  </span>
                )}
              </div>
            );
          })
        )}
      </div>
    </div>
  );
}
