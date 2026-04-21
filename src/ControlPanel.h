#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <commctrl.h>

#include <memory>
#include <vector>
#include <string>

// Forward declarations
namespace TibiaEyez {
class OverlayWindow;
class HotkeyManager;
class TimerSystem;
class Config;
class TibiaFinder;
} // namespace TibiaEyez

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// ControlPanel
//   The main management window.  Dark-themed, standard Win32 with owner-draw
//   controls.  Provides:
//     • List of active mirror overlays (add / remove / configure each)
//     • Tibia connection status indicator
//     • Profile picker (load / save)
//     • Timer list with start/stop controls
//     • Hotkey editor
// ─────────────────────────────────────────────────────────────────────────────
class ControlPanel {
public:
    struct Deps {
        std::vector<std::unique_ptr<OverlayWindow>>* overlays = nullptr;
        HotkeyManager*  hotkeyManager  = nullptr;
        TimerSystem*    timerSystem    = nullptr;
        Config*         config         = nullptr;
        TibiaFinder*    tibiaFinder    = nullptr;
        HWND*           tibiaHwnd      = nullptr;
    };

    ControlPanel()  = default;
    ~ControlPanel() { Destroy(); }

    bool Create(HINSTANCE hInst, const Deps& deps);
    void Destroy();

    void Show();
    void Hide();
    void Toggle();

    HWND GetHwnd() const { return m_hwnd; }
    bool IsVisible() const;

    // Update status text shown in the panel
    void SetTibiaStatus(bool connected, const wchar_t* windowTitle = nullptr);

    // Refresh the mirror list (call after adding/removing mirrors)
    void RefreshMirrorList();

    static const wchar_t* ClassName() { return L"TibiaEyez_ControlPanel"; }
    static bool RegisterWindowClass(HINSTANCE hInst);

private:
    // ── Colour palette (dark gamer theme) ──────────────────────────────────
    static constexpr COLORREF CLR_BG        = RGB(18,  18,  30);   // #12121e
    static constexpr COLORREF CLR_SURFACE   = RGB(26,  26,  46);   // #1a1a2e
    static constexpr COLORREF CLR_ACCENT    = RGB(70, 130, 255);   // #4682FF
    static constexpr COLORREF CLR_ACCENT2   = RGB(233, 69,  96);   // #E94560
    static constexpr COLORREF CLR_TEXT      = RGB(168, 178, 216);  // #a8b2d8
    static constexpr COLORREF CLR_TEXT_DIM  = RGB(90,  100, 130);
    static constexpr COLORREF CLR_BUTTON    = RGB(35,  35,  65);
    static constexpr COLORREF CLR_BUTTON_HV = RGB(50,  50,  90);

    // ── Control IDs ────────────────────────────────────────────────────────
    enum CtrlId : UINT {
        IDC_STATUS_LABEL     = 100,
        IDC_MIRROR_LIST      = 101,
        IDC_BTN_ADD_MIRROR   = 102,
        IDC_BTN_REMOVE_MIRROR= 103,
        IDC_BTN_EDIT_MIRROR  = 104,
        IDC_BTN_LOCK_MIRROR  = 105,
        IDC_BTN_HIDE_MIRROR  = 106,
        IDC_OPACITY_SLIDER   = 107,
        IDC_PROFILE_COMBO    = 108,
        IDC_BTN_SAVE_PROFILE = 109,
        IDC_BTN_LOAD_PROFILE = 110,
        IDC_TIMER_LIST       = 111,
        IDC_BTN_TIMER_START  = 112,
        IDC_BTN_TIMER_STOP   = 113,
        IDC_BTN_TIMER_RESET  = 114,
        IDC_BTN_ADD_TIMER    = 115,
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    // Message handlers
    void OnCreate();
    void OnPaint();
    void OnDrawItem(DRAWITEMSTRUCT* dis);
    void OnCommand(UINT id, UINT notif, HWND hCtrl);
    void OnHScroll(HWND hCtrl);
    void OnSize(int cx, int cy);
    void OnCtlColorStatic(HDC hdc, HWND hCtrl);

    // Actions
    void ActionAddMirror();
    void ActionRemoveMirror();
    void ActionEditMirror();
    void ActionToggleLock();
    void ActionToggleVisible();
    void ActionSaveProfile();
    void ActionLoadProfile();
    void ActionStartTimer();
    void ActionStopTimer();
    void ActionResetTimer();
    void ActionAddTimer();

    // Layout helpers
    void LayoutControls(int cx, int cy);
    void PaintSectionHeader(HDC hdc, const wchar_t* text, RECT& r);
    HWND MakeButton(UINT id, const wchar_t* label, int x, int y, int w, int h);
    HWND MakeLabel(UINT id, const wchar_t* text, int x, int y, int w, int h);

    // Brushes / fonts (created in OnCreate, deleted in Destroy)
    HBRUSH m_brBg      = nullptr;
    HBRUSH m_brSurface = nullptr;
    HBRUSH m_brButton  = nullptr;
    HFONT  m_fontNorm  = nullptr;
    HFONT  m_fontBold  = nullptr;

    HWND   m_hwnd          = nullptr;
    HWND   m_hwndStatus    = nullptr;
    HWND   m_hwndMirrorList= nullptr;
    HWND   m_hwndOpacity   = nullptr;
    HWND   m_hwndProfileCB = nullptr;
    HWND   m_hwndTimerList = nullptr;

    HINSTANCE m_hInst = nullptr;
    Deps      m_deps;

    static bool s_classRegistered;
};

} // namespace TibiaEyez
