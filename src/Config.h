#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <string>
#include <vector>
#include "OverlayWindow.h"

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// Config
//   Saves and loads application configuration to/from a plain-text INI file
//   stored in %APPDATA%\TibiaEyez\<profileName>.ini
//
//   Format example:
//     [TibiaEyez]
//     Version=1
//
//     [Mirror0]
//     Name=Cooldowns
//     X=100
//     ...
//
//   No external dependencies — the INI is read/written with
//   GetPrivateProfileString / WritePrivateProfileString.
// ─────────────────────────────────────────────────────────────────────────────
class Config {
public:
    // Describes a saved hotkey entry for persistence
    struct HotkeyEntry {
        std::wstring name;
        UINT         modifiers = 0;
        UINT         vk        = 0;
    };

    struct Profile {
        std::wstring                       name    = L"Default";
        std::vector<OverlayWindow::Config> mirrors;
        std::vector<HotkeyEntry>           hotkeys;
    };

    Config()  = default;
    ~Config() = default;

    // Load a named profile.  Returns false if the file does not exist.
    bool Load(const std::wstring& profileName, Profile& out);

    // Save a profile.  Creates the file / directory if necessary.
    bool Save(const Profile& profile);

    // Enumerate profile names (*.ini files in the AppData folder)
    std::vector<std::wstring> ListProfiles();

    // Delete a profile
    bool DeleteProfile(const std::wstring& profileName);

    // Returns %APPDATA%\TibiaEyez\
    static std::wstring GetConfigDir();

private:
    static std::wstring ProfilePath(const std::wstring& profileName);

    static std::wstring ReadStr(const wchar_t* section,
                                const wchar_t* key,
                                const wchar_t* def,
                                const wchar_t* path);

    static int ReadInt(const wchar_t* section,
                       const wchar_t* key,
                       int            def,
                       const wchar_t* path);

    static void WriteStr(const wchar_t* section,
                         const wchar_t* key,
                         const wchar_t* value,
                         const wchar_t* path);

    static void WriteInt(const wchar_t* section,
                         const wchar_t* key,
                         int            value,
                         const wchar_t* path);
};

} // namespace TibiaEyez
