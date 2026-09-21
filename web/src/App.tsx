import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { HardDrive, MessageSquare } from 'lucide-react';
import {
  addPeer,
  getInfo,
  getMessages,
  getPeers,
  getSettings,
  probePeer,
  putSettings,
  sendFile,
  sendText,
} from './api';
import { DeviceAvatar } from './components/DeviceAvatar';
import { DirectDialModal } from './components/DirectDialModal';
import { FileTransferMatrix } from './components/FileTransferMatrix';
import { Header } from './components/Header';
import { PeerSidebar } from './components/PeerSidebar';
import { SettingsModal } from './components/SettingsModal';
import { WebShareModal } from './components/WebShareModal';
import type { AppSettings, ChatMessage, PeerNode, TransferFile } from './types';

const POLL_MS = 2000;

const EMPTY_NODE: PeerNode = {
  id: '',
  name: '本机',
  hostname: '',
  ip: '',
  port: 8848,
  os: 'windows',
  avatarColor: '#3b82f6',
  status: 'offline',
  latencyMs: 0,
  linkSpeed: '',
  isLocal: true,
  lastSeen: 0,
};

const DEFAULT_SETTINGS: AppSettings = {
  port: 8848,
  downloadDirectory: './downloads',
  deviceName: '本机',
  serviceName: 'lan-drop',
  maxParallelStreams: 1,
  autoAcceptFilesUnderMb: 0,
  soundNotification: true,
  nudgeEnabled: true,
};

function playChime() {
  try {
    const Ctx = window.AudioContext || (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
    if (!Ctx) return;
    const ctx = new Ctx();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.frequency.setValueAtTime(587.33, ctx.currentTime);
    gain.gain.setValueAtTime(0.06, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.15);
    osc.start();
    osc.stop(ctx.currentTime + 0.15);
  } catch {
    // 浏览器可能拦截自动播放
  }
}

export default function App() {
  const [peers, setPeers] = useState<PeerNode[]>([]);
  const [currentNode, setCurrentNode] = useState<PeerNode>(EMPTY_NODE);
  const [selectedPeerId, setSelectedPeerId] = useState('');
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [hiddenTransferIds, setHiddenTransferIds] = useState<string[]>([]);
  const [settings, setSettings] = useState<AppSettings>(DEFAULT_SETTINGS);
  const [backendOk, setBackendOk] = useState(false);
  const [notice, setNotice] = useState('');
  const [busy, setBusy] = useState(false);
  const [centerTab, setCenterTab] = useState<'chat' | 'transfers'>('chat');
  const [dialOpen, setDialOpen] = useState(false);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [shareOpen, setShareOpen] = useState(false);
  const [isNudging, setIsNudging] = useState(false);
  const seenIds = useRef<Set<string>>(new Set());
  const primed = useRef(false);

  const refreshPeers = useCallback(async () => {
    const [info, list] = await Promise.all([getInfo(), getPeers()]);
    setCurrentNode(info);
    setPeers(list.filter((p) => p.id !== info.id));
    setBackendOk(true);
  }, []);

  useEffect(() => {
    let stop = false;
    const tick = () => {
      refreshPeers().catch(() => {
        if (!stop) setBackendOk(false);
      });
    };
    tick();
    const timer = window.setInterval(tick, POLL_MS);
    return () => {
      stop = true;
      window.clearInterval(timer);
    };
  }, [refreshPeers]);

  useEffect(() => {
    getSettings()
      .then(setSettings)
      .catch(() => setBackendOk(false));
  }, []);

  useEffect(() => {
    if (selectedPeerId && peers.some((p) => p.id === selectedPeerId)) return;
    const first = peers.find((p) => !p.isLocal);
    setSelectedPeerId(first?.id ?? '');
  }, [peers, selectedPeerId]);

  useEffect(() => {
    if (!selectedPeerId) {
      setMessages([]);
      return;
    }
    let stop = false;
    primed.current = false;
    seenIds.current = new Set();
    const tick = () => {
      getMessages(selectedPeerId)
        .then((list) => {
          if (stop) return;
          if (primed.current && settings.soundNotification) {
            for (const msg of list) {
              if (!seenIds.current.has(msg.id) && msg.fromId !== currentNode.id) playChime();
            }
          }
          list.forEach((msg) => seenIds.current.add(msg.id));
          primed.current = true;
          setMessages(list);
        })
        .catch(() => {
          if (!stop) setBackendOk(false);
        });
    };
    tick();
    const timer = window.setInterval(tick, POLL_MS);
    return () => {
      stop = true;
      window.clearInterval(timer);
    };
  }, [selectedPeerId, settings.soundNotification, currentNode.id]);

  const selectedPeer = useMemo(
    () => peers.find((p) => p.id === selectedPeerId),
    [peers, selectedPeerId],
  );

  const transfers = useMemo(() => {
    const hidden = new Set(hiddenTransferIds);
    const list: TransferFile[] = [];
    for (const msg of messages) {
      if (msg.fileMeta && !hidden.has(msg.fileMeta.id)) list.push(msg.fileMeta);
    }
    return list;
  }, [messages, hiddenTransferIds]);

  const handleSendMessage = useCallback(
    (text: string) => {
      if (!selectedPeer) return;
      setBusy(true);
      setNotice('');
      sendText(selectedPeer.id, text)
        .then(() => getMessages(selectedPeer.id))
        .then(setMessages)
        .catch((err: unknown) => setNotice(err instanceof Error ? err.message : '发送失败'))
        .finally(() => setBusy(false));
    },
    [selectedPeer],
  );

  const handleSendFile = useCallback(
    (file: File) => {
      if (!selectedPeer) return;
      setBusy(true);
      setNotice('');
      sendFile(selectedPeer.id, file)
        .then(() => getMessages(selectedPeer.id))
        .then(setMessages)
        .catch((err: unknown) => setNotice(err instanceof Error ? err.message : '发送文件失败'))
        .finally(() => setBusy(false));
    },
    [selectedPeer],
  );

  const handleNudge = useCallback(() => {
    if (!settings.nudgeEnabled || !selectedPeer) return;
    setIsNudging(true);
    window.setTimeout(() => setIsNudging(false), 400);
    handleSendMessage('轻击了对方窗口');
  }, [handleSendMessage, selectedPeer, settings.nudgeEnabled]);

  return (
    <div className="min-h-screen bg-slate-100 flex flex-col font-sans text-slate-900 antialiased">
      <Header
        currentNode={currentNode}
        backendOk={backendOk}
        onOpenSettingsModal={() => setSettingsOpen(true)}
        onOpenWebShareModal={() => setShareOpen(true)}
      />

      <main className="flex-1 flex flex-col md:flex-row overflow-hidden max-w-7xl w-full mx-auto border-x border-slate-200/70 bg-white">
        <PeerSidebar
          peers={peers}
          selectedPeerId={selectedPeerId}
          onSelectPeer={setSelectedPeerId}
          onOpenDirectDialModal={() => setDialOpen(true)}
        />

        <div className="flex-1 flex flex-col min-w-0 bg-white">
          {selectedPeer ? (
            <>
              <div className="border-b border-slate-200 px-4 sm:px-6 py-2.5 bg-white flex flex-wrap items-center justify-between gap-3 shrink-0">
                <div className="flex items-center gap-3">
                  <DeviceAvatar
                    name={selectedPeer.name}
                    color={selectedPeer.avatarColor}
                    os={selectedPeer.os}
                    size={40}
                  />
                  <div>
                    <div className="flex items-center gap-2">
                      <h2 className="text-sm font-bold text-slate-900">{selectedPeer.name}</h2>
                      <span className="text-[11px] font-mono text-slate-500 bg-slate-100 px-1.5 py-0.5 rounded">
                        {selectedPeer.ip}:{selectedPeer.port}
                      </span>
                    </div>
                    <div className="text-xs text-slate-500">{selectedPeer.hostname || selectedPeer.status}</div>
                  </div>
                </div>
                <div className="flex items-center bg-slate-100 p-1 rounded-xl border border-slate-200/80 text-xs font-medium">
                  <button
                    onClick={() => setCenterTab('chat')}
                    className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg ${centerTab === 'chat' ? 'bg-white text-blue-700 font-semibold' : 'text-slate-600'}`}
                  >
                    <MessageSquare className="w-3.5 h-3.5" />
                    即时聊天
                  </button>
                  <button
                    onClick={() => setCenterTab('transfers')}
                    className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg ${centerTab === 'transfers' ? 'bg-white text-blue-700 font-semibold' : 'text-slate-600'}`}
                  >
                    <HardDrive className="w-3.5 h-3.5 text-blue-600" />
                    文件传输 ({transfers.length})
                  </button>
                </div>
              </div>
              {notice && <div className="px-4 py-1.5 text-xs text-red-700 bg-red-50 border-b border-red-100">{notice}</div>}
              <div className="flex-1 flex flex-col min-h-0">
                {centerTab === 'chat' ? (
                  <ChatArea
                    currentPeer={selectedPeer}
                    localNode={currentNode}
                    messages={messages}
                    onSendMessage={handleSendMessage}
                    onSendFile={handleSendFile}
                    onSendNudge={handleNudge}
                    isNudging={isNudging}
                    busy={busy}
                  />
                ) : (
                  <FileTransferMatrix
                    transfers={transfers}
                    peers={[currentNode, ...peers]}
                    downloadDir={settings.downloadDirectory}
                    onClearCompleted={() =>
                      setHiddenTransferIds((prev) => [
                        ...prev,
                        ...transfers.filter((t) => t.status === 'completed').map((t) => t.id),
                      ])
                    }
                  />
                )}
              </div>
            </>
          ) : (
            <div className="flex-1 flex items-center justify-center text-sm text-slate-500 p-8 text-center">
              {backendOk ? '还没有对端。同一网段等待发现，或点左侧「加 IP」。' : '连不上本机服务。请先启动局域快传（默认 http://127.0.0.1:8848）。'}
            </div>
          )}
        </div>
      </main>

      <DirectDialModal
        isOpen={dialOpen}
        onClose={() => setDialOpen(false)}
        onProbe={(ip, port) => probePeer({ ip, port })}
        onAddPeer={async (input) => {
          const peer = await addPeer(input);
          await refreshPeers();
          if (peer.id) setSelectedPeerId(peer.id);
        }}
      />
      <SettingsModal
        isOpen={settingsOpen}
        onClose={() => setSettingsOpen(false)}
        settings={settings}
        onSave={async (next) => {
          const saved = await putSettings(next);
          setSettings(saved);
        }}
      />
      <WebShareModal isOpen={shareOpen} onClose={() => setShareOpen(false)} />
    </div>
  );
}
