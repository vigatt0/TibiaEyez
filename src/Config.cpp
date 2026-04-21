#include "Config.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace TibiaEyez {

// ── Path helpers ───────────────────────────────────────────────────────────────

std::wstring Config::GetConfigDir()
{
    wchar_t appData[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) {
        return std::wstring(appData) + L"\\TibiaEyez\\";
    }
    return L".\\";
}

std::wstring Config::ProfilePath(const std::wstring& profileName)
{
    return GetConfigDir() + profileName + L".ini";
}

// ── Private INI helpers ────────────────────────────────────────────────────────

std::wstring Config::ReadStr(const wchar_t* section, const wchar_t* key,
                              const wchar_t* def,     const wchar_t* path)
{
    wchar_t buf[1024]{};
    GetPrivateProfileStringW(section, key, def, buf, 1024, path);
    return buf;
}

int Config::ReadInt(const wchar_t* section, const wchar_t* key,
                    int def,                const wchar_t* path)
{
    return static_cast<int>(GetPrivateProfileIntW(section, key, def, path));
}

void Config::WriteStr(const wchar_t* section, const wchar_t* key,
                      const wchar_t* value,   const wchar_t* path)
{
    WritePrivateProfileStringW(section, key, value, path);
}

void Config::WriteInt(const wchar_t* section, const wchar_t* key,
                      int value,              const wchar_t* path)
{
    wchar_t buf[32]{};
    swprintf_s(buf, L"%d", value);
    WritePrivateProfileStringW(section, key, buf, path);
}

// ── Load ───────────────────────────────────────────────────────────────────────

bool Config::Load(const std::wstring& profileName, Profile& out)
{
    std::wstring path = ProfilePath(profileName);

    // Check the file exists
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    const wchar_t* p = path.c_str();

    out.name = profileName;
    out.mirrors.clear();
    out.hotkeys.clear();

    // Global section
    int mirrorCount = ReadInt(L"TibiaEyez", L"MirrorCount", 0, p);
    int hotkeyCount = ReadInt(L"TibiaEyez", L"HotkeyCount", 0, p);

    // Read mirrors
    for (int i = 0; i < mirrorCount; ++i) {
        wchar_t section[32]{};
        swprintf_s(section, L"Mirror%d", i);

        OverlayWindow::Config mc{};
        mc.name        = ReadStr(section, L"Name",      L"Mirror", p);
        mc.x           = ReadInt(section, L"X",          100,  p);
        mc.y           = ReadInt(section, L"Y",          100,  p);
        mc.width       = ReadInt(section, L"Width",      200,  p);
        mc.height      = ReadInt(section, L"Height",     200,  p);
        mc.srcRegion.x = ReadInt(section, L"SrcX",       0,    p);
        mc.srcRegion.y = ReadInt(section, L"SrcY",       0,    p);
        mc.srcRegion.w = ReadInt(section, L"SrcW",       200,  p);
        mc.srcRegion.h = ReadInt(section, L"SrcH",       200,  p);
        mc.opacity     = static_cast<BYTE>(ReadInt(section, L"Opacity", 204, p));
        mc.locked      = ReadInt(section, L"Locked",     0,    p) != 0;
        mc.visible     = ReadInt(section, L"Visible",    1,    p) != 0;
        mc.showGlowBorder = ReadInt(section, L"GlowBorder", 1, p) != 0;
        mc.showGrid    = ReadInt(section, L"Grid",       0,    p) != 0;
        mc.gridCellPx  = ReadInt(section, L"GridCell",   32,   p);

        // Glow colour stored as 0xRRGGBB
        int glowRaw    = ReadInt(section, L"GlowColor",
                                 static_cast<int>(RGB(70, 130, 255)), p);
        mc.glowColor   = static_cast<COLORREF>(glowRaw);

        out.mirrors.push_back(mc);
    }

    // Read hotkeys
    for (int i = 0; i < hotkeyCount; ++i) {
        wchar_t section[32]{};
        swprintf_s(section, L"Hotkey%d", i);

        HotkeyEntry hk{};
        hk.name      = ReadStr(section, L"Name",      L"",  p);
        hk.modifiers = static_cast<UINT>(ReadInt(section, L"Modifiers", 0, p));
        hk.vk        = static_cast<UINT>(ReadInt(section, L"VK",        0, p));

        if (!hk.name.empty() && hk.vk != 0) {
            out.hotkeys.push_back(hk);
        }
    }

    return true;
}

// ── Save ───────────────────────────────────────────────────────────────────────

bool Config::Save(const Profile& profile)
{
    // Ensure the config directory exists
    std::wstring dir = GetConfigDir();
    CreateDirectoryW(dir.c_str(), nullptr);

    std::wstring path = ProfilePath(profile.name);
    const wchar_t* p  = path.c_str();

    WriteInt(L"TibiaEyez", L"Version",     1,                               p);
    WriteInt(L"TibiaEyez", L"MirrorCount", static_cast<int>(profile.mirrors.size()),  p);
    WriteInt(L"TibiaEyez", L"HotkeyCount", static_cast<int>(profile.hotkeys.size()),  p);

    for (int i = 0; i < static_cast<int>(profile.mirrors.size()); ++i) {
        const auto& mc = profile.mirrors[i];
        wchar_t section[32]{};
        swprintf_s(section, L"Mirror%d", i);

        WriteStr(section, L"Name",       mc.name.c_str(),                          p);
        WriteInt(section, L"X",          mc.x,                                     p);
        WriteInt(section, L"Y",          mc.y,                                     p);
        WriteInt(section, L"Width",      mc.width,                                 p);
        WriteInt(section, L"Height",     mc.height,                                p);
        WriteInt(section, L"SrcX",       mc.srcRegion.x,                           p);
        WriteInt(section, L"SrcY",       mc.srcRegion.y,                           p);
        WriteInt(section, L"SrcW",       mc.srcRegion.w,                           p);
        WriteInt(section, L"SrcH",       mc.srcRegion.h,                           p);
        WriteInt(section, L"Opacity",    mc.opacity,                               p);
        WriteInt(section, L"Locked",     mc.locked  ? 1 : 0,                       p);
        WriteInt(section, L"Visible",    mc.visible ? 1 : 0,                       p);
        WriteInt(section, L"GlowBorder", mc.showGlowBorder ? 1 : 0,               p);
        WriteInt(section, L"Grid",       mc.showGrid ? 1 : 0,                      p);
        WriteInt(section, L"GridCell",   mc.gridCellPx,                            p);
        WriteInt(section, L"GlowColor",  static_cast<int>(mc.glowColor),           p);
    }

    for (int i = 0; i < static_cast<int>(profile.hotkeys.size()); ++i) {
        const auto& hk = profile.hotkeys[i];
        wchar_t section[32]{};
        swprintf_s(section, L"Hotkey%d", i);

        WriteStr(section, L"Name",      hk.name.c_str(),                p);
        WriteInt(section, L"Modifiers", static_cast<int>(hk.modifiers), p);
        WriteInt(section, L"VK",        static_cast<int>(hk.vk),        p);
    }

    return true;
}

// ── ListProfiles ───────────────────────────────────────────────────────────────

std::vector<std::wstring> Config::ListProfiles()
{
    std::vector<std::wstring> names;
    std::wstring pattern = GetConfigDir() + L"*.ini";

    WIN32_FIND_DATAW fd{};
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return names;

    do {
        std::wstring fname = fd.cFileName;
        // Strip .ini extension
        auto dot = fname.rfind(L'.');
        if (dot != std::wstring::npos) fname = fname.substr(0, dot);
        names.push_back(fname);
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    return names;
}

// ── DeleteProfile ─────────────────────────────────────────────────────────────

bool Config::DeleteProfile(const std::wstring& profileName)
{
    std::wstring path = ProfilePath(profileName);
    return DeleteFileW(path.c_str()) != FALSE;
}

} // namespace TibiaEyez
