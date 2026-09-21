import { useEffect, useRef, useState } from 'react';
import {
  ArrowDownToLine,
  Check,
  CheckCheck,
  Copy,
  Download,
  FileArchive,
  FileText,
  FolderOpen,
  FolderPlus,
  Paperclip,
  Send,
  ShieldCheck,
  Zap,
} from 'lucide-react';
import type { ChatMessage, PeerNode } from '../types';
import { DeviceAvatar } from './DeviceAvatar';

interface ChatAreaProps {
  currentPeer: PeerNode;
  localNode: PeerNode;
  messages: ChatMessage[];
  onSendMessage: (text: string) => void;
  onSendFile: (file: File) => void;
  onSendNudge: () => void;
  isNudging: boolean;
  busy: boolean;
}

function formatBytes(bytes: number) {
  if (!bytes) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.min(units.length - 1, Math.floor(Math.log(bytes) / Math.log(1024)));
  return `${parseFloat((bytes / 1024 ** i).toFixed(1))} ${units[i]}`;
}

export function ChatArea({
  currentPeer,
  localNode,
  messages,
  onSendMessage,
  onSendFile,
  onSendNudge,
  isNudging,
  busy,
}: ChatAreaProps) {
  const [inputText, setInputText] = useState('');
  const [isDragging, setIsDragging] = useState(false);
  const [copiedCodeId, setCopiedCodeId] = useState<string | null>(null);
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const folderInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  const submit = () => {
    const text = inputText.trim();
    if (!text || busy) return;
    onSendMessage(text);
    setInputText('');
  };

  const sendFiles = (list: FileList | File[]) => {
    Array.from(list).forEach((file) => onSendFile(file));
  };

  return (
    <div
      className={`flex-1 flex flex-col h-[calc(100vh-130px)] bg-slate-100/50 relative ${isNudging ? 'animate-bounce duration-100' : ''}`}
      onDragOver={(e) => {
        e.preventDefault();
        setIsDragging(true);
      }}
      onDragLeave={(e) => {
        e.preventDefault();
        setIsDragging(false);
      }}
      onDrop={(e) => {
        e.preventDefault();
        setIsDragging(false);
        if (e.dataTransfer.files.length > 0) sendFiles(e.dataTransfer.files);
      }}
    >
      {isDragging && (
        <div className="absolute inset-0 bg-blue-600/90 z-50 flex flex-col items-center justify-center text-white border-4 border-dashed border-white/60 p-8">
          <ArrowDownToLine className="w-16 h-16 animate-bounce mb-3" />
          <h3 className="text-xl font-bold">松开发送文件</h3>
        </div>
      )}

      <div className="flex-1 overflow-y-auto p-4 sm:p-6 space-y-4">
        <div className="flex justify-center">
          <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full bg-white border border-slate-200 text-[11px] text-slate-500">
            <ShieldCheck className="w-3.5 h-3.5 text-emerald-600" />
            <span>
              与 <strong className="text-slate-700">{currentPeer.name}</strong>（{currentPeer.ip}:{currentPeer.port}）的会话
            </span>
          </div>
        </div>

        {messages.length === 0 && (
          <p className="text-center text-xs text-slate-400 pt-8">还没有消息。文字会按约 2 秒刷新。</p>
        )}

        {messages.map((msg) => {
          const isMe = msg.fromId === localNode.id || msg.fromId === 'local';
          if (msg.type === 'nudge') {
            return (
              <div key={msg.id} className="flex justify-center my-2">
                <div className="px-3 py-1 rounded-full bg-amber-50 border border-amber-200 text-amber-800 text-xs flex items-center gap-1.5">
                  <Zap className="w-3.5 h-3.5 text-amber-600" />
                  <span>{isMe ? '你向对方发送了窗口轻颤提示' : `${currentPeer.name} 提醒了你`}</span>
                </div>
              </div>
            );
          }
          return (
            <div key={msg.id} className={`flex gap-3 max-w-3xl ${isMe ? 'ml-auto flex-row-reverse' : 'mr-auto'}`}>
              <DeviceAvatar
                name={isMe ? localNode.name : currentPeer.name}
                color={isMe ? localNode.avatarColor : currentPeer.avatarColor}
                os={isMe ? localNode.os : currentPeer.os}
                size={32}
              />
              <div className={isMe ? 'text-right' : 'text-left'}>
                <div className="flex items-center gap-2 text-[11px] text-slate-400">
                  <span className="font-medium text-slate-600">{isMe ? '我' : currentPeer.name}</span>
                  <span>{new Date(msg.timestamp).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' })}</span>
                  {msg.status === 'failed' && <span className="text-red-500">发送失败</span>}
                </div>
                {msg.type === 'text' && (
                  <div
                    className={`mt-1 p-3 rounded-2xl text-xs sm:text-sm leading-relaxed max-w-prose inline-block ${
                      isMe ? 'bg-blue-600 text-white' : 'bg-white text-slate-800 border border-slate-200/80'
                    }`}
                  >
                    {msg.content}
                  </div>
                )}
                {msg.type === 'code' && (
                  <div className="mt-1 bg-slate-900 rounded-xl overflow-hidden border border-slate-800 text-left max-w-lg">
                    <div className="bg-slate-800/90 px-3 py-1.5 flex items-center justify-between text-[11px] text-slate-300 font-mono">
                      <span>{msg.codeLanguage || 'text'}</span>
                      <button
                        onClick={() => {
                          void navigator.clipboard.writeText(msg.content);
                          setCopiedCodeId(msg.id);
                          setTimeout(() => setCopiedCodeId(null), 2000);
                        }}
                        className="flex items-center gap-1 hover:text-white"
                      >
                        {copiedCodeId === msg.id ? <Check className="w-3 h-3 text-emerald-400" /> : <Copy className="w-3 h-3" />}
                        <span>{copiedCodeId === msg.id ? '已复制' : '复制'}</span>
                      </button>
                    </div>
                    <pre className="p-3 text-xs font-mono text-slate-100 overflow-x-auto">
                      <code>{msg.content}</code>
                    </pre>
                  </div>
                )}
                {(msg.type === 'file' || msg.type === 'folder') && (
                  <div className="mt-1 bg-white rounded-2xl border border-slate-200/90 p-3.5 text-left max-w-md">
                    <div className="flex items-start gap-3">
                      <div className="p-2.5 rounded-xl bg-blue-50 shrink-0">
                        {msg.fileMeta?.isDirectory || msg.type === 'folder' ? (
                          <FolderOpen className="w-6 h-6 text-amber-500" />
                        ) : (
                          <FileText className="w-6 h-6 text-blue-600" />
                        )}
                      </div>
                      <div className="min-w-0 flex-1">
                        <h4 className="text-sm font-semibold text-slate-900 truncate">{msg.fileMeta?.name || msg.content}</h4>
                        <div className="text-[11px] text-slate-500 mt-0.5 flex items-center gap-2">
                          {msg.fileMeta && <span>{formatBytes(msg.fileMeta.size)}</span>}
                          {msg.fileMeta?.name.endsWith('.zip') && <FileArchive className="w-3 h-3" />}
                        </div>
                        {msg.fileMeta && (
                          <div className="mt-2">
                            <div className="w-full h-2 bg-slate-100 rounded-full overflow-hidden">
                              <div className="h-full bg-blue-600 rounded-full" style={{ width: `${msg.fileMeta.progress}%` }} />
                            </div>
                            <div className="mt-1 text-[11px] text-slate-500 flex justify-between">
                              <span>
                                {msg.fileMeta.status === 'completed' ? (
                                  <span className="text-emerald-600 inline-flex items-center gap-1">
                                    <CheckCheck className="w-3.5 h-3.5" /> 已完成
                                  </span>
                                ) : (
                                  `进度 ${msg.fileMeta.progress}%`
                                )}
                              </span>
                            </div>
                          </div>
                        )}
                        {msg.fileMeta?.downloadUrl && (
                          <a
                            href={msg.fileMeta.downloadUrl}
                            className="mt-2 inline-flex items-center gap-1.5 px-3 py-1 rounded-lg bg-blue-50 text-blue-700 text-xs font-medium"
                          >
                            <Download className="w-3.5 h-3.5" />
                            下载
                          </a>
                        )}
                      </div>
                    </div>
                  </div>
                )}
              </div>
            </div>
          );
        })}
        <div ref={messagesEndRef} />
      </div>

      <div className="px-4 py-2 border-t border-slate-200 bg-white/90 flex items-center justify-between gap-2 text-xs">
        <div className="flex items-center gap-1 sm:gap-2">
          <button
            onClick={() => fileInputRef.current?.click()}
            className="flex items-center gap-1 px-2.5 py-1 rounded-lg text-slate-600 hover:text-blue-700 hover:bg-blue-50/80"
          >
            <Paperclip className="w-3.5 h-3.5 text-blue-600" />
            <span>发送文件</span>
          </button>
          <input
            ref={fileInputRef}
            type="file"
            multiple
            className="hidden"
            onChange={(e) => {
              if (e.target.files) sendFiles(e.target.files);
              e.target.value = '';
            }}
          />
          <button
            onClick={() => folderInputRef.current?.click()}
            className="flex items-center gap-1 px-2.5 py-1 rounded-lg text-slate-600 hover:text-amber-700 hover:bg-amber-50/80"
            title="文件夹内文件会逐个发送"
          >
            <FolderPlus className="w-3.5 h-3.5 text-amber-500" />
            <span>发送文件夹</span>
          </button>
          <input
            ref={folderInputRef}
            type="file"
            className="hidden"
            // 浏览器目录选择，逐个走发送文件接口
            {...{ webkitdirectory: '', directory: '' }}
            onChange={(e) => {
              if (e.target.files) sendFiles(e.target.files);
              e.target.value = '';
            }}
          />
          <button
            onClick={onSendNudge}
            className="flex items-center gap-1 px-2.5 py-1 rounded-lg text-slate-600 hover:text-amber-700 hover:bg-amber-50/80"
          >
            <Zap className="w-3.5 h-3.5 text-amber-500" />
            <span>抖动窗口</span>
          </button>
        </div>
        <div className="text-[11px] text-slate-400 hidden md:block">Enter 发送，Shift+Enter 换行</div>
      </div>

      <div className="p-3 sm:p-4 bg-white border-t border-slate-200">
        <div className="flex items-end gap-2 bg-slate-50 border border-slate-200 rounded-xl p-1.5 focus-within:border-blue-500 focus-within:bg-white">
          <textarea
            rows={2}
            value={inputText}
            onChange={(e) => setInputText(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                submit();
              }
            }}
            placeholder={`向 ${currentPeer.name} 发送消息...`}
            className="flex-1 text-xs sm:text-sm bg-transparent border-0 outline-hidden resize-none p-1.5 text-slate-800"
          />
          <button
            onClick={submit}
            disabled={!inputText.trim() || busy}
            className="p-2.5 rounded-lg bg-blue-600 hover:bg-blue-700 disabled:opacity-40 text-white shrink-0"
          >
            <Send className="w-4 h-4" />
          </button>
        </div>
      </div>
    </div>
  );
}
