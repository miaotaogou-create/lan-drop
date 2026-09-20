export type DeviceOS = 'macos' | 'windows' | 'linux' | 'arm-linux' | 'ios' | 'android';

export interface PeerNode {
  id: string;
  name: string;
  hostname: string;
  ip: string;
  port: number;
  os: DeviceOS;
  avatarColor: string;
  status: 'online' | 'busy' | 'away' | 'offline';
  latencyMs: number;
  linkSpeed: string;
  isLocal?: boolean;
  department?: string;
  lastSeen: number;
}

export interface NetworkInterface {
  name: string;
  ip: string;
  mac: string;
  type: 'Ethernet' | 'Wi-Fi' | 'Bridge';
  speed: string;
}

export interface TransferFile {
  id: string;
  name: string;
  size: number;
  type: string;
  status: 'pending' | 'transferring' | 'completed' | 'paused' | 'failed';
  progress: number;
  speedMBs: number;
  chunksTotal: number;
  chunksTransferred: number;
  sha256?: string;
  isDirectory?: boolean;
  fileCount?: number;
  downloadUrl?: string;
  senderNodeId: string;
  receiverNodeId: string;
  startedAt: number;
  completedAt?: number;
}

export type MessageType = 'text' | 'file' | 'folder' | 'nudge' | 'code';

export interface ChatMessage {
  id: string;
  fromId: string;
  toId: string;
  timestamp: number;
  type: MessageType;
  content: string;
  codeLanguage?: string;
  fileMeta?: TransferFile;
  ackLatencyMs?: number;
  status: 'sending' | 'delivered' | 'failed';
}

export interface AppSettings {
  port: number;
  downloadDirectory: string;
  deviceName: string;
  serviceName: string;
  maxParallelStreams: number;
  autoAcceptFilesUnderMb: number;
  soundNotification: boolean;
  nudgeEnabled: boolean;
}
