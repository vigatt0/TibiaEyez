#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <dwmapi.h>

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// DwmMirror
//   RAII wrapper around a single DWM Thumbnail registration.
//   Renders a cropped region (SourceRegion) of a source window into an
//   arbitrary RECT of a destination window.
//
//   The DWM Thumbnail approach is 100% passive — it uses the compositor's
//   existing back-buffer; no pixel reading, no hooking, no memory access.
// ─────────────────────────────────────────────────────────────────────────────
class DwmMirror {
public:
    // Pixel crop within the source window's client area
    struct SourceRegion {
        int x = 0, y = 0;
        int w = 200, h = 200;
    };

    DwmMirror() = default;
    ~DwmMirror() { Destroy(); }

    // Non-copyable, movable
    DwmMirror(const DwmMirror&)            = delete;
    DwmMirror& operator=(const DwmMirror&) = delete;
    DwmMirror(DwmMirror&&) noexcept;
    DwmMirror& operator=(DwmMirror&&) noexcept;

    // Register: hwndSource is the Tibia HWND, hwndDest is our overlay HWND.
    // Returns true on success.
    bool Register(HWND hwndSource, HWND hwndDest);

    // Update rendering: maps src crop → destRect, with 0-255 opacity.
    bool Update(const SourceRegion& src, const RECT& destRect, BYTE opacity = 255);

    // Query the natural pixel size of the source window.
    bool GetSourceSize(SIZE& outSize) const;

    void Destroy();

    bool IsValid()    const { return m_thumbnail != nullptr; }
    HWND GetSource()  const { return m_hwndSource; }
    HWND GetDest()    const { return m_hwndDest;   }

private:
    HTHUMBNAIL m_thumbnail  = nullptr;
    HWND       m_hwndSource = nullptr;
    HWND       m_hwndDest   = nullptr;
};

} // namespace TibiaEyez
