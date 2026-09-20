import type { AppSettings, ChatMessage, DeviceOS, PeerNode, TransferFile } from './types';

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

function apiBase(): string {
  const fromEnv = import.meta.env.VITE_API_BASE;
  if (typeof fromEnv === 'string' && fromEnv.trim() !== '') {
    return fromEnv.trim().replace(/\/$/, '');
  }
  return '';
}

function url(path: string): string {
  return `${apiBase()}${path}`;
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const headers = new Headers(init?.headers);
  if (init?.body && !(init.body instanceof FormData) && !headers.has('Content-Type')) {
    headers.set('Content-Type', 'application/json');
  }
  const res = await fetch(url(path), { ...init, headers });
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || `请求失败（${res.status}）`);
  }
  if (res.status === 204) return undefined as T;
  const contentType = res.headers.get('content-type') || '';
  if (!contentType.includes('json')) return undefined as T;
  return (await res.json()) as T;
}

const PALETTE = ['#3b82f6', '#0ea5e9', '#10b981', '#f97316', '#8b5cf6', '#e11d48'];

function colorFromKey(key: string): string {
  let n = 0;
  for (let i = 0; i < key.length; i++) n = (n + key.charCodeAt(i)) % PALETTE.length;
  return PALETTE[n];
}

const OS_SET = new Set<DeviceOS>(['macos', 'windows', 'linux', 'arm-linux', 'ios', 'android']);

function asOs(value: unknown): DeviceOS {
  return typeof value === 'string' && OS_SET.has(value as DeviceOS) ? (value as DeviceOS) : 'linux';
}

export function normalizePeer(raw: Partial<PeerNode> | null | undefined): PeerNode {
  const ip = raw?.ip || '';
  const id = raw?.id || (ip ? `${ip}:${raw?.port || 8848}` : '');
  return {
    id,
    name: raw?.name || ip || '未命名设备',
    hostname: raw?.hostname || '',
    ip,
    port: raw?.port || 8848,
    os: asOs(raw?.os),
    avatarColor: raw?.avatarColor || colorFromKey(id || ip || 'local'),
    status: raw?.status || 'online',
    latencyMs: raw?.latencyMs ?? 0,
    linkSpeed: raw?.linkSpeed || '',
    isLocal: raw?.isLocal,
    department: raw?.department,
    lastSeen: raw?.lastSeen || Date.now(),
  };
}

function asList<T>(data: T[] | { peers?: T[]; messages?: T[] } | null | undefined, key: 'peers' | 'messages'): T[] {
  if (Array.isArray(data)) return data;
  if (data && Array.isArray(data[key])) return data[key] as T[];
  return [];
}

export function getInfo(): Promise<PeerNode> {
  return request<Partial<PeerNode>>('/api/info').then((raw) => normalizePeer({ ...raw, isLocal: true }));
}

export function getPeers(): Promise<PeerNode[]> {
  return request<PeerNode[] | { peers: PeerNode[] }>('/api/peers').then((data) =>
    asList(data, 'peers').map((item) => normalizePeer(item)),
  );
}

export function addPeer(input: { ip: string; port: number; name?: string }): Promise<PeerNode> {
  return request<Partial<PeerNode>>('/api/peers', {
    method: 'POST',
    body: JSON.stringify({ ip: input.ip, port: input.port, name: input.name || '' }),
  }).then((raw) => normalizePeer(raw));
}

export interface ProbeResult {
  ok: boolean;
  latencyMs?: number;
  error?: string;
  peer?: PeerNode;
}

export function probePeer(input: { ip: string; port: number }): Promise<ProbeResult> {
  return request<ProbeResult>('/api/peers/probe', {
    method: 'POST',
    body: JSON.stringify({ ip: input.ip, port: input.port }),
  }).then((raw) => ({
    ok: !!raw?.ok,
    latencyMs: raw?.latencyMs,
    error: raw?.error,
    peer: raw?.peer ? normalizePeer(raw.peer) : undefined,
  }));
}

function normalizeMessage(raw: Partial<ChatMessage>): ChatMessage {
  return {
    id: raw.id || `${raw.timestamp || Date.now()}`,
    fromId: raw.fromId || '',
    toId: raw.toId || '',
    timestamp: raw.timestamp || Date.now(),
    type: raw.type || 'text',
    content: raw.content || '',
    codeLanguage: raw.codeLanguage,
    fileMeta: raw.fileMeta ? normalizeTransfer(raw.fileMeta) : undefined,
    ackLatencyMs: raw.ackLatencyMs,
    status: raw.status || 'delivered',
  };
}

function normalizeTransfer(raw: Partial<TransferFile>): TransferFile {
  return {
    id: raw.id || '',
    name: raw.name || '文件',
    size: raw.size || 0,
    type: raw.type || 'application/octet-stream',
    status: raw.status || 'completed',
    progress: raw.progress ?? (raw.status === 'completed' ? 100 : 0),
    speedMBs: raw.speedMBs || 0,
    chunksTotal: raw.chunksTotal || 0,
    chunksTransferred: raw.chunksTransferred || 0,
    sha256: raw.sha256,
    isDirectory: raw.isDirectory,
    fileCount: raw.fileCount,
    downloadUrl: raw.downloadUrl,
    senderNodeId: raw.senderNodeId || '',
    receiverNodeId: raw.receiverNodeId || '',
    startedAt: raw.startedAt || Date.now(),
    completedAt: raw.completedAt,
  };
}

export function getMessages(peerId: string): Promise<ChatMessage[]> {
  const q = new URLSearchParams({ peerId });
  return request<ChatMessage[] | { messages: ChatMessage[] }>(`/api/messages?${q}`).then((data) =>
    asList(data, 'messages').map((item) => normalizeMessage(item)),
  );
}

export function sendText(peerId: string, text: string): Promise<void> {
  return request<void>('/api/send-text', {
    method: 'POST',
    body: JSON.stringify({ peerId, text }),
  });
}

export function sendFile(peerId: string, file: File): Promise<void> {
  const body = new FormData();
  body.append('peerId', peerId);
  body.append('file', file, file.name);
  return request<void>('/api/send-file', { method: 'POST', body });
}

export function getSettings(): Promise<AppSettings> {
  return request<Partial<AppSettings>>('/api/settings').then((raw) => ({
    ...DEFAULT_SETTINGS,
    ...raw,
    port: raw?.port || DEFAULT_SETTINGS.port,
    deviceName: raw?.deviceName || DEFAULT_SETTINGS.deviceName,
    downloadDirectory: raw?.downloadDirectory || DEFAULT_SETTINGS.downloadDirectory,
    serviceName: raw?.serviceName || DEFAULT_SETTINGS.serviceName,
  }));
}

export function putSettings(settings: AppSettings): Promise<AppSettings> {
  return request<Partial<AppSettings>>('/api/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
  }).then((raw) => ({
    ...settings,
    ...raw,
  }));
}
