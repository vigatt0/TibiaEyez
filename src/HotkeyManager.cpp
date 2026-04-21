#include "HotkeyManager.h"
#include <algorithm>

namespace TibiaEyez {

HotkeyManager::HotkeyManager(HWND messageWindow)
    : m_hwnd(messageWindow)
{}

HotkeyManager::~HotkeyManager()
{
    UnregisterAll();
}

int HotkeyManager::NextId()
{
    return m_nextId++;
}

bool HotkeyManager::Register(const std::wstring& name, UINT modifiers, UINT vk)
{
    // Remove any existing registration with the same name
    Unregister(name);

    int id = NextId();
    if (!::RegisterHotKey(m_hwnd, id, modifiers, vk)) {
        return false; // Another app already owns this hotkey
    }

    HotkeyEntry entry{};
    entry.name      = name;
    entry.id        = id;
    entry.modifiers = modifiers;
    entry.vk        = vk;
    m_entries.push_back(std::move(entry));
    return true;
}

void HotkeyManager::Unregister(const std::wstring& name)
{
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [&](const HotkeyEntry& e) { return e.name == name; });

    if (it != m_entries.end()) {
        ::UnregisterHotKey(m_hwnd, it->id);
        m_entries.erase(it);
    }
}

void HotkeyManager::SetCallback(const std::wstring& name, HotkeyCallback cb)
{
    auto it = std::find_if(m_entries.begin(), m_entries.end(),
        [&](const HotkeyEntry& e) { return e.name == name; });

    if (it != m_entries.end()) {
        it->callback = std::move(cb);
    }
}

void HotkeyManager::Dispatch(WPARAM hotkeyId)
{
    for (auto& entry : m_entries) {
        if (entry.id == static_cast<int>(hotkeyId)) {
            if (entry.callback) entry.callback();
            return;
        }
    }
}

void HotkeyManager::UnregisterAll()
{
    for (auto& entry : m_entries) {
        ::UnregisterHotKey(m_hwnd, entry.id);
    }
    m_entries.clear();
}

} // namespace TibiaEyez
