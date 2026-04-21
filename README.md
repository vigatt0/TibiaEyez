# TibiaEyez

> **Screen Mirroring Overlay for Tibia** — powered by the Windows DWM Thumbnail Service.

TibiaEyez lets you clone any region of the Tibia client (cooldown bars, action bars, character status, minimap, …) into one or more always-on-top, click-through overlay windows that sit on top of any secondary monitor or free screen space — **without** reading memory, injecting code or touching the game process in any way.

---

## ✅ Safety & Anti-cheat Compliance

| Technique | Status |
|---|---|
| ReadProcessMemory / WriteProcessMemory | ❌ Never used |
| DLL injection | ❌ Never used |
| API hooking (SetWindowsHookEx, detours) | ❌ Never used |
| Packet sniffing | ❌ Never used |
| Pixel-level screen capture (BitBlt) | ❌ Never used |
| **DWM Thumbnail Service** (compositor back-buffer clone) | ✅ Used exclusively |

The DWM Thumbnail API (`DwmRegisterThumbnail` / `DwmUpdateThumbnailProperties`) is the same technology used by the Windows **Task View** and **Aero Peek** previews.  It is fully passive and equivalent to using the Windows Magnifier tool.  There is **zero interaction** with the Tibia game process.

---

## Features

* **Multiple independent mirror windows** — each mirrors a user-defined crop region of the Tibia client.
* **Always-on-top, frameless, click-through** — overlays float above everything; mouse input passes through to Tibia.
* **Lock / Unlock** — when unlocked, drag a mirror to reposition it; lock it again to restore click-through.
* **Opacity control** (20 – 100%) per mirror.
* **Glow border** — customisable colour glow around each mirror for quick identification.
* **Grid overlay** — toggleable dot grid for precise crop alignment.
* **Global hotkeys** (`RegisterHotKey`) — default bindings:
  * `Ctrl+Alt+T` — show / hide the control panel.
  * `Ctrl+Alt+H` — show / hide all overlays.
* **Audio countdown timers** — named timers with configurable durations and sound alerts (`PlaySound` / `MessageBeep`), useful for spell cooldowns and respawn timers.
* **Profile persistence** — layout, positions, source regions and hotkeys saved to `%APPDATA%\TibiaEyez\<profile>.ini`.  Auto-saved on exit; auto-loaded on start.
* **Dark gamer UI** — custom-painted Win32 control panel with a `#12121e` background and blue/red accent colours.
* **Standalone executable** — statically linked CRT, no .NET or Visual C++ Redistributable required.

---

## Architecture

```
Application (WinMain)
├── TibiaFinder      — polls every 2 s for Tibia's HWND via EnumWindows
├── OverlayWindow[]  — one per mirror; WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST
│   └── DwmMirror   — DwmRegisterThumbnail wrapper; updates source crop via DwmUpdateThumbnailProperties
├── HotkeyManager    — RegisterHotKey-based global hotkeys
├── TimerSystem      — countdown timers ticked from WM_TIMER
├── Config           — INI read/write via GetPrivateProfileString / WritePrivateProfileString
└── ControlPanel     — dark-themed Win32 management window
```

### Key classes

| Class | File | Purpose |
|---|---|---|
| `TibiaFinder` | `src/TibiaFinder.h/.cpp` | Find Tibia HWND by process name; poll for window open/close |
| `DwmMirror` | `src/DwmMirror.h/.cpp` | RAII wrapper around `HTHUMBNAIL`; crops source via `rcSource` |
| `OverlayWindow` | `src/OverlayWindow.h/.cpp` | Per-mirror Win32 window with glow border and grid overlay |
| `HotkeyManager` | `src/HotkeyManager.h/.cpp` | `RegisterHotKey` dispatch table |
| `TimerSystem` | `src/TimerSystem.h/.cpp` | Named countdown timers with audio alert |
| `Config` | `src/Config.h/.cpp` | INI-based profile save/load |
| `ControlPanel` | `src/ControlPanel.h/.cpp` | Dark-themed management UI |
| `Application` | `src/Application.h/.cpp` | Orchestrates all subsystems; Win32 message loop |

---

## Build Requirements

| Requirement | Version |
|---|---|
| Windows SDK | 10.0.19041+ |
| MSVC (Visual Studio) | 2019 or 2022 |
| CMake | 3.16+ |

**Linux / macOS**: not supported (the application requires Win32 / DWM APIs).

### Build steps

```powershell
# In a Visual Studio Developer PowerShell:
git clone https://github.com/vigatt0/TibiaEyez
cd TibiaEyez
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# Output: build\Release\TibiaEyez.exe
```

The resulting `.exe` is fully standalone — it links the CRT statically and requires no additional runtime.

---

## Configuration

Profiles are stored in `%APPDATA%\TibiaEyez\<ProfileName>.ini`.

### INI format

```ini
[TibiaEyez]
Version=1
MirrorCount=2
HotkeyCount=2

[Mirror0]
Name=Action Bar
X=50
Y=50
Width=300
Height=80
SrcX=0
SrcY=0
SrcW=300
SrcH=80
Opacity=230
Locked=1
Visible=1
GlowBorder=1
GlowColor=4671615
Grid=0
GridCell=32

[Hotkey0]
Name=TogglePanel
Modifiers=3
VK=84
```

---

## Extending

* **Add a new mirror programmatically**:
  ```cpp
  OverlayWindow::Config cfg;
  cfg.name      = L"Minimap";
  cfg.srcRegion = { 620, 0, 160, 160 };  // top-right corner of Tibia
  cfg.x = 1800; cfg.y = 20;
  cfg.width = 160; cfg.height = 160;
  auto ov = std::make_unique<OverlayWindow>();
  ov->Create(hInst, cfg);
  ov->SetSourceWindow(tibiaHwnd);
  ov->UpdateMirror();
  ```

* **Add a hotkey**:
  ```cpp
  hotkeyMgr.Register(L"Timer1", MOD_ALT, VK_F5);
  hotkeyMgr.SetCallback(L"Timer1", [&]() { timerSys.Start(L"Respawn"); });
  ```

* **Add an audio timer**:
  ```cpp
  TimerSystem::TimerEntry t;
  t.name         = L"Respawn";
  t.totalSeconds = 120;
  t.sound        = TimerSystem::AlertSound::Exclamation;
  t.repeating    = false;
  timerSys.AddTimer(t);
  timerSys.Start(L"Respawn");
  ```

---

## License

MIT
