#include "TibiaFinder.h"
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace TibiaEyez {

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool WStrContainsI(const wchar_t* haystack, const wchar_t* needle)
{
    if (!haystack || !needle || *needle == L'\0') return false;

    // Lower-case copies for case-insensitive comparison
    wchar_t h[512]{}, n[512]{};
    wcsncpy_s(h, haystack, _TRUNCATE);
    wcsncpy_s(n, needle,   _TRUNCATE);
    _wcslwr_s(h);
    _wcslwr_s(n);
    return wcsstr(h, n) != nullptr;
}

// ── TibiaFinder implementation ────────────────────────────────────────────────

bool TibiaFinder::ProcessNameMatches(DWORD pid, wchar_t* exeNameOut, DWORD exeNameLen)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    PROCESSENTRY32W pe{ sizeof(pe) };

    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (pe.th32ProcessID != pid) continue;

            for (int i = 0; TIBIA_PROCESS_NAMES[i]; ++i) {
                if (_wcsicmp(pe.szExeFile, TIBIA_PROCESS_NAMES[i]) == 0) {
                    found = true;
                    if (exeNameOut && exeNameLen)
                        wcsncpy_s(exeNameOut, exeNameLen, pe.szExeFile, _TRUNCATE);
                    break;
                }
            }
            break; // PID found, no need to continue
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return found;
}

bool TibiaFinder::IsLikelyTibia(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd)) return false;

    // Skip tool windows / tiny helper windows
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return false;

    // Match by process name — most reliable
    if (ProcessNameMatches(pid)) return true;

    // Fallback: match by window title substring
    wchar_t title[256]{};
    GetWindowTextW(hwnd, title, 255);
    for (int i = 0; TIBIA_TITLE_SUBSTRINGS[i]; ++i) {
        if (WStrContainsI(title, TIBIA_TITLE_SUBSTRINGS[i])) {
            // Require a minimum window size to avoid matching splash screens
            RECT rc{};
            GetWindowRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w >= 640 && h >= 480) return true;
        }
    }

    return false;
}

BOOL CALLBACK TibiaFinder::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<EnumData*>(lParam);

    if (!IsLikelyTibia(hwnd)) return TRUE;

    if (data->findFirst) {
        data->found = hwnd;
        return FALSE; // Stop enumeration immediately
    }

    // Populate full info for EnumerateAll()
    WindowInfo info{};
    info.hwnd = hwnd;
    GetWindowThreadProcessId(hwnd, &info.processId);
    GetWindowTextW(hwnd,  info.title,     255);
    GetClassNameW(hwnd,   info.className, 255);
    ProcessNameMatches(info.processId, info.exeName, MAX_PATH);
    data->results->push_back(info);

    return TRUE;
}

HWND TibiaFinder::Find()
{
    EnumData data{};
    data.findFirst = true;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.found;
}

bool TibiaFinder::IsValid(HWND hwnd) const
{
    if (!hwnd || !IsWindow(hwnd)) return false;
    return IsLikelyTibia(hwnd);
}

std::vector<TibiaFinder::WindowInfo> TibiaFinder::EnumerateAll()
{
    std::vector<WindowInfo> results;
    EnumData data{};
    data.results    = &results;
    data.findFirst  = false;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return results;
}

void TibiaFinder::SetCallbacks(WindowFoundCallback onFound, WindowLostCallback onLost)
{
    m_onFound = std::move(onFound);
    m_onLost  = std::move(onLost);
}

void TibiaFinder::Poll()
{
    bool wasValid = (m_currentHwnd != nullptr);
    bool isStillValid = IsValid(m_currentHwnd);

    if (isStillValid) return; // Nothing changed

    // Either we lost the window, or never had one — attempt re-discovery
    HWND found = Find();

    if (found) {
        bool isNew = (found != m_currentHwnd);
        m_currentHwnd = found;
        if (isNew && m_onFound) m_onFound(m_currentHwnd);
    } else {
        if (wasValid && m_onLost) m_onLost();
        m_currentHwnd = nullptr;
    }
}

} // namespace TibiaEyez
