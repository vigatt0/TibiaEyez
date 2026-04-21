#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>

#include <string>
#include <vector>
#include <functional>

namespace TibiaEyez {

// Known Tibia executable names (OT servers may use different names)
static const wchar_t* const TIBIA_PROCESS_NAMES[] = {
    L"Tibia.exe",
    L"TibiaClient.exe",
    L"client.exe",
    nullptr
};

// Substrings found in the Tibia window title
static const wchar_t* const TIBIA_TITLE_SUBSTRINGS[] = {
    L"Tibia",
    nullptr
};

// ─────────────────────────────────────────────────────────────────────────────
// TibiaFinder
//   Locates the Tibia client window (HWND) by enumerating top-level windows
//   and matching against known process/title names.
//   Provides polling support so the application can react when Tibia is
//   opened or closed at runtime.
// ─────────────────────────────────────────────────────────────────────────────
class TibiaFinder {
public:
    struct WindowInfo {
        HWND    hwnd      = nullptr;
        DWORD   processId = 0;
        wchar_t title[256]{};
        wchar_t className[256]{};
        wchar_t exeName[MAX_PATH]{};
    };

    using WindowFoundCallback = std::function<void(HWND hwnd)>;
    using WindowLostCallback  = std::function<void()>;

    TibiaFinder() = default;
    ~TibiaFinder() = default;

    // Synchronous one-shot search.  Returns nullptr if not found.
    HWND Find();

    // Check whether a previously obtained HWND is still alive and belongs to
    // the Tibia process.
    bool IsValid(HWND hwnd) const;

    // Enumerate all candidate windows (useful for a "pick window" dialog).
    std::vector<WindowInfo> EnumerateAll();

    // Register callbacks invoked by Poll().
    void SetCallbacks(WindowFoundCallback onFound, WindowLostCallback onLost);

    // Call periodically (e.g. from a WM_TIMER handler) to detect window
    // appearing / disappearing.
    void Poll();

    HWND GetCurrentHwnd() const { return m_currentHwnd; }

private:
    struct EnumData {
        std::vector<WindowInfo>* results = nullptr;
        bool   findFirst = false;
        HWND   found     = nullptr;
    };

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    static bool          IsLikelyTibia(HWND hwnd);
    static bool          ProcessNameMatches(DWORD pid, wchar_t* exeNameOut = nullptr, DWORD exeNameLen = 0);

    HWND                m_currentHwnd = nullptr;
    WindowFoundCallback m_onFound;
    WindowLostCallback  m_onLost;
};

} // namespace TibiaEyez
