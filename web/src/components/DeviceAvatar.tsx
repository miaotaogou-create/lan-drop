import type { DeviceOS } from '../types';

type BadgeKind = 'terminal' | 'cpu' | 'server' | 'phone';

function badgeKind(os: DeviceOS): BadgeKind {
  switch (os) {
    case 'linux':
      return 'terminal';
    case 'arm-linux':
      return 'cpu';
    case 'ios':
    case 'android':
      return 'phone';
    default:
      return 'server';
  }
}

function BadgeIcon({ kind }: { kind: BadgeKind }) {
  const common = {
    viewBox: '0 0 24 24',
    fill: 'none',
    strokeLinecap: 'round' as const,
    strokeLinejoin: 'round' as const,
    className: 'w-full h-full',
  };
  if (kind === 'terminal') {
    return (
      <svg {...common} stroke="#EA580C" strokeWidth="2.5">
        <polyline points="4 17 10 11 4 5" />
        <line x1="12" x2="20" y1="19" y2="19" />
      </svg>
    );
  }
  if (kind === 'cpu') {
    return (
      <svg {...common} stroke="#059669" strokeWidth="2">
        <rect width="16" height="16" x="4" y="4" rx="2" />
        <rect width="6" height="6" x="9" y="9" rx="1" />
        <path d="M15 2v2" />
        <path d="M15 20v2" />
        <path d="M2 15h2" />
        <path d="M2 9h2" />
        <path d="M20 15h2" />
        <path d="M20 9h2" />
        <path d="M9 2v2" />
        <path d="M9 20v2" />
      </svg>
    );
  }
  if (kind === 'phone') {
    return (
      <svg {...common} stroke="#C026D3" strokeWidth="2">
        <rect width="14" height="20" x="5" y="2" rx="2" ry="2" />
        <path d="M12 18h.01" />
      </svg>
    );
  }
  return (
    <svg {...common} stroke="#2563EB" strokeWidth="2">
      <rect width="20" height="8" x="2" y="14" rx="2" />
      <path d="M6 18h.01" />
      <path d="M10 18h.01" />
      <path d="M4 14V8a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v6" />
    </svg>
  );
}

export function SettingsGear({ className = 'w-5 h-5', stroke = '#64748B' }: { className?: string; stroke?: string }) {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      viewBox="0 0 24 24"
      fill="none"
      stroke={stroke}
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
      className={className}
    >
      <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z" />
      <circle cx="12" cy="12" r="3" />
    </svg>
  );
}

export function DeviceAvatar({
  name,
  color,
  os,
  size = 40,
}: {
  name: string;
  color: string;
  os: DeviceOS;
  size?: number;
}) {
  const badge = Math.round(size * 0.45);
  const radius = Math.round(size * 0.28);
  const letter = (name || '?').slice(0, 1);
  return (
    <div className="relative shrink-0" style={{ width: size, height: size }}>
      <div
        className="flex items-center justify-center font-bold text-white w-full h-full"
        style={{
          backgroundColor: color,
          borderRadius: radius,
          fontSize: Math.round(size * 0.44),
          lineHeight: 1,
        }}
      >
        {letter}
      </div>
      <span
        className="absolute flex items-center justify-center rounded-full bg-white"
        style={{
          width: badge,
          height: badge,
          right: 0,
          bottom: 0,
          boxShadow: '0 0 0 1px rgba(0,0,0,0.08)',
          padding: Math.max(2, Math.round(badge * 0.18)),
        }}
      >
        <BadgeIcon kind={badgeKind(os)} />
      </span>
    </div>
  );
}
