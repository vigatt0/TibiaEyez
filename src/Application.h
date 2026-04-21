#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <memory>
#include <vector>

// Forward declarations
namespace TibiaEyez {
class TibiaFinder;
class OverlayWindow;
class HotkeyManager;
class TimerSystem;
class Config;
class ControlPanel;
} // namespace TibiaEyez

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// Application
//   Top-level class that owns all subsystems and runs the Win32 message loop.
//
//   Architecture:
//     • A hidden message-only window receives WM_TIMER and WM_HOTKEY messages.
//     • WM_TIMER ID 1 (100 ms)  — polls Tibia window presence via TibiaFinder.
//     • WM_TIMER ID 2 (100 ms)  — drives TimerSystem countdowns.
//     • The ControlPanel is the user-facing window.
//     • OverlayWindows float over everything using DWM thumbnails.
// ─────────────────────────────────────────────────────────────────────────────
class Application {
public:
    Application()  = default;
    ~Application() = default;

    // Initialise all subsystems and create windows.
    bool Init(HINSTANCE hInst);

    // Run the Win32 message loop.  Returns the exit code.
    int  Run();

    // Request graceful exit.
    void Quit(int exitCode = 0);

private:
    // Message-only window for timer / hotkey routing
    static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    bool CreateMessageWindow();

    // Timer callbacks
    void OnTibiaPollingTimer();
    void OnTickTimer();

    // Tibia tracking callbacks
    void OnTibiaFound(HWND hwnd);
    void OnTibiaLost();

    // Default mirrors created on first run (can be adjusted by user)
    void CreateDefaultMirrors();

    HINSTANCE m_hInst = nullptr;
    HWND      m_hwndMsg = nullptr;   // Message-only window

    // Subsystems
    std::unique_ptr<TibiaFinder>  m_tibiaFinder;
    std::unique_ptr<HotkeyManager> m_hotkeyMgr;
    std::unique_ptr<TimerSystem>   m_timerSys;
    std::unique_ptr<Config>        m_config;
    std::unique_ptr<ControlPanel>  m_controlPanel;

    // Active overlays
    std::vector<std::unique_ptr<OverlayWindow>> m_overlays;

    // Current Tibia HWND (nullptr if not running)
    HWND m_tibiaHwnd = nullptr;

    // Timing bookkeeping for TimerSystem::Tick
    DWORD m_lastTickMs = 0;

    int  m_exitCode = 0;
    bool m_running  = false;

    // Timer IDs
    static constexpr UINT TIMER_TIBIA_POLL = 1;
    static constexpr UINT TIMER_TICK       = 2;
    static constexpr UINT TIMER_POLL_MS    = 2000; // 2 s  — low CPU, non-intrusive
    static constexpr UINT TIMER_TICK_MS    = 100;  // 100 ms tick resolution
};

} // namespace TibiaEyez
