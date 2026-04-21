#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <functional>
#include <unordered_map>
#include <string>

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// HotkeyManager
//   Registers / unregisters global hotkeys via RegisterHotKey() and dispatches
//   WM_HOTKEY messages to named callbacks.
//
//   RegisterHotKey() is approved by BattlEye because it uses the Windows
//   shell message mechanism — it does NOT hook or intercept any process.
// ─────────────────────────────────────────────────────────────────────────────
class HotkeyManager {
public:
    using HotkeyCallback = std::function<void()>;

    explicit HotkeyManager(HWND messageWindow);
    ~HotkeyManager();

    // Register a named hotkey.  modifiers: MOD_ALT | MOD_CONTROL | MOD_SHIFT
    // Returns false if the hotkey is already registered by another app.
    bool Register(const std::wstring& name, UINT modifiers, UINT vk);

    // Unregister a named hotkey
    void Unregister(const std::wstring& name);

    // Set (or replace) the callback for a named hotkey
    void SetCallback(const std::wstring& name, HotkeyCallback cb);

    // Call from the message loop when WM_HOTKEY is received
    void Dispatch(WPARAM hotkeyId);

    // Iterate registered hotkeys (name → id, vk, modifiers)
    struct HotkeyEntry {
        std::wstring name;
        int          id;
        UINT         modifiers;
        UINT         vk;
        HotkeyCallback callback;
    };

    const std::vector<HotkeyEntry>& GetEntries() const { return m_entries; }

    void UnregisterAll();

private:
    int NextId();

    HWND                    m_hwnd;
    int                     m_nextId = 1000;
    std::vector<HotkeyEntry> m_entries;
};

} // namespace TibiaEyez
