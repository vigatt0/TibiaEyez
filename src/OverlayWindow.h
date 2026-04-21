#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include "DwmMirror.h"

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// OverlayWindow
//   A single "mirror" window.  Properties:
//     • WS_EX_LAYERED   — supports per-pixel / constant-alpha blending
//     • WS_EX_TRANSPARENT — mouse events pass through to the window below
//     • WS_EX_TOPMOST    — always on top of other windows
//     • Frameless (no caption/border via WS_POPUP)
//
//   When UNLOCKED the title bar appears so the user can drag/resize the window.
//   When LOCKED the window becomes fully click-through again.
//
//   Glow border: drawn as a layered GDI+ effect around the thumbnail area.
//   Grid overlay: painted on top of the thumbnail to help align crop regions.
// ─────────────────────────────────────────────────────────────────────────────
class OverlayWindow {
public:
    struct Config {
        // Screen position and size of this overlay
        int x      = 100, y      = 100;
        int width  = 200, height = 200;

        // Which region of the Tibia client to mirror
        DwmMirror::SourceRegion srcRegion{ 0, 0, 200, 200 };

        // Visual properties
        BYTE     opacity        = 204;               // ~80 %  (0-255)
        bool     locked         = false;
        bool     visible        = true;
        bool     showGlowBorder = true;
        COLORREF glowColor      = RGB(70, 130, 255); // Default: soft blue
        bool     showGrid       = false;
        int      gridCellPx     = 32;                // Grid cell size in pixels

        std::wstring name = L"Mirror";
    };

    OverlayWindow()  = default;
    ~OverlayWindow() { Destroy(); }

    OverlayWindow(const OverlayWindow&)            = delete;
    OverlayWindow& operator=(const OverlayWindow&) = delete;

    // Create the Win32 window.  Must be called after RegisterWindowClass().
    bool Create(HINSTANCE hInst, const Config& cfg);
    void Destroy();

    // Attach to a Tibia HWND (or nullptr to detach)
    void SetSourceWindow(HWND hwndTibia);

    // Bulk-update all configuration properties
    void ApplyConfig(const Config& cfg);

    // Fine-grained setters (call UpdateMirror() afterwards if needed)
    void SetLocked(bool locked);
    void SetVisible(bool visible);
    void SetOpacity(BYTE opacity);
    void SetGlowColor(COLORREF color);
    void SetShowGrid(bool show);
    void SetGridCellSize(int px);
    void SetSourceRegion(const DwmMirror::SourceRegion& src);
    void MoveWindow(int x, int y, int w = -1, int h = -1);

    // Re-issue DwmUpdateThumbnailProperties with current config
    void UpdateMirror();

    const Config& GetConfig() const { return m_config; }
    HWND          GetHwnd()   const { return m_hwnd;   }
    bool          IsValid()   const { return m_hwnd != nullptr; }

    // Window class management (call once per process)
    static bool         RegisterWindowClass(HINSTANCE hInst);
    static const wchar_t* ClassName() { return L"TibiaEyez_Overlay"; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    // WndProc helpers
    void OnPaint();
    void OnLButtonDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnLButtonUp();
    void OnRButtonUp(int x, int y);   // Show context menu when unlocked

    // Internal helpers
    void ApplyWindowStyle();          // Set / clear WS_EX_TRANSPARENT
    void ApplyOpacityToWindow();      // SetLayeredWindowAttributes
    void RefreshGlowBorder();         // Invalidate so WM_PAINT redraws glow
    void DrawGlowBorder(HDC hdc, const RECT& clientRect);
    void DrawGridOverlay(HDC hdc, const RECT& clientRect);

    HWND       m_hwnd      = nullptr;
    HWND       m_tibiaHwnd = nullptr;
    HINSTANCE  m_hInst     = nullptr;
    DwmMirror  m_mirror;
    Config     m_config;

    // Drag state (used when window is unlocked)
    bool  m_dragging   = false;
    POINT m_dragOffset{};

    static bool s_classRegistered;
};

} // namespace TibiaEyez
