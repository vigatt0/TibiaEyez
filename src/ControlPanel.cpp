#include "ControlPanel.h"
#include "OverlayWindow.h"
#include "HotkeyManager.h"
#include "TimerSystem.h"
#include "Config.h"
#include "TibiaFinder.h"

#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

namespace TibiaEyez {

bool ControlPanel::s_classRegistered = false;

// ── Window class ───────────────────────────────────────────────────────────────

bool ControlPanel::RegisterWindowClass(HINSTANCE hInst)
{
    if (s_classRegistered) return true;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = ClassName();
    wc.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);

    s_classRegistered = (RegisterClassExW(&wc) != 0);
    return s_classRegistered;
}

// ── Create / Destroy ──────────────────────────────────────────────────────────

bool ControlPanel::Create(HINSTANCE hInst, const Deps& deps)
{
    m_hInst = hInst;
    m_deps  = deps;

    m_hwnd = CreateWindowExW(
        0,
        ClassName(),
        L"TibiaEyez — Screen Mirroring Overlay",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 620,
        nullptr, nullptr, hInst, this);

    return m_hwnd != nullptr;
}

void ControlPanel::Destroy()
{
    if (m_brBg)      { DeleteObject(m_brBg);      m_brBg      = nullptr; }
    if (m_brSurface) { DeleteObject(m_brSurface); m_brSurface = nullptr; }
    if (m_brButton)  { DeleteObject(m_brButton);  m_brButton  = nullptr; }
    if (m_fontNorm)  { DeleteObject(m_fontNorm);  m_fontNorm  = nullptr; }
    if (m_fontBold)  { DeleteObject(m_fontBold);  m_fontBold  = nullptr; }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

// ── Visibility ─────────────────────────────────────────────────────────────────

void ControlPanel::Show()
{
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }
}

void ControlPanel::Hide()
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void ControlPanel::Toggle()
{
    IsVisible() ? Hide() : Show();
}

bool ControlPanel::IsVisible() const
{
    return m_hwnd && IsWindowVisible(m_hwnd);
}

// ── Status update ──────────────────────────────────────────────────────────────

void ControlPanel::SetTibiaStatus(bool connected, const wchar_t* windowTitle)
{
    if (!m_hwndStatus) return;

    wchar_t buf[256]{};
    if (connected && windowTitle && *windowTitle) {
        swprintf_s(buf, L"● Connected  —  %s", windowTitle);
    } else if (connected) {
        wcscpy_s(buf, L"● Connected");
    } else {
        wcscpy_s(buf, L"○ Waiting for Tibia…");
    }
    SetWindowTextW(m_hwndStatus, buf);
    InvalidateRect(m_hwndStatus, nullptr, TRUE);
}

// ── RefreshMirrorList ─────────────────────────────────────────────────────────

void ControlPanel::RefreshMirrorList()
{
    if (!m_hwndMirrorList || !m_deps.overlays) return;

    ListBox_ResetContent(m_hwndMirrorList);
    for (auto& ov : *m_deps.overlays) {
        if (!ov) continue;
        const auto& cfg = ov->GetConfig();
        wchar_t buf[128]{};
        swprintf_s(buf, L"%s  [%dx%d @ %d,%d]  %s%s",
            cfg.name.c_str(),
            cfg.width, cfg.height,
            cfg.x, cfg.y,
            cfg.locked  ? L"🔒" : L"",
            cfg.visible ? L"" : L" (hidden)");
        ListBox_AddString(m_hwndMirrorList, buf);
    }
}

// ── OnCreate ──────────────────────────────────────────────────────────────────

void ControlPanel::OnCreate()
{
    // GDI resources
    m_brBg      = CreateSolidBrush(CLR_BG);
    m_brSurface = CreateSolidBrush(CLR_SURFACE);
    m_brButton  = CreateSolidBrush(CLR_BUTTON);

    LOGFONTW lf{};
    lf.lfHeight  = -13;
    lf.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    m_fontNorm = CreateFontIndirectW(&lf);
    lf.lfWeight = FW_BOLD;
    m_fontBold  = CreateFontIndirectW(&lf);

    // Common controls (slider, list box, combo)
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    RECT cr{};
    GetClientRect(m_hwnd, &cr);
    int cx = cr.right, cy = cr.bottom;

    // ── Status label ──────────────────────────────────────────────────────
    m_hwndStatus = MakeLabel(IDC_STATUS_LABEL,
        L"○ Waiting for Tibia…", 10, 10, cx - 20, 20);
    SendMessageW(m_hwndStatus, WM_SETFONT, (WPARAM)m_fontBold, TRUE);

    // ── Mirror list ───────────────────────────────────────────────────────
    m_hwndMirrorList = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        10, 50, cx - 20, 160,
        m_hwnd, reinterpret_cast<HMENU>(IDC_MIRROR_LIST), m_hInst, nullptr);
    SendMessageW(m_hwndMirrorList, WM_SETFONT, (WPARAM)m_fontNorm, TRUE);

    // Mirror buttons row
    MakeButton(IDC_BTN_ADD_MIRROR,    L"+ Add",    10,  220, 80, 26);
    MakeButton(IDC_BTN_REMOVE_MIRROR, L"✕ Remove", 95,  220, 80, 26);
    MakeButton(IDC_BTN_EDIT_MIRROR,   L"✎ Edit",   180, 220, 80, 26);
    MakeButton(IDC_BTN_LOCK_MIRROR,   L"🔒 Lock",   265, 220, 80, 26);
    MakeButton(IDC_BTN_HIDE_MIRROR,   L"👁 Hide",   350, 220, 80, 26);

    // Opacity slider
    MakeLabel(0, L"Opacity:", 10, 258, 60, 18);
    m_hwndOpacity = CreateWindowExW(
        0, TRACKBAR_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        75, 254, cx - 90, 26,
        m_hwnd, reinterpret_cast<HMENU>(IDC_OPACITY_SLIDER), m_hInst, nullptr);
    SendMessageW(m_hwndOpacity, TBM_SETRANGE, TRUE, MAKELPARAM(20, 100));
    SendMessageW(m_hwndOpacity, TBM_SETPOS,   TRUE, 80);

    // ── Profile section ───────────────────────────────────────────────────
    MakeLabel(0, L"Profile:", 10, 296, 60, 18);
    m_hwndProfileCB = CreateWindowExW(
        0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        75, 292, cx - 175, 200,
        m_hwnd, reinterpret_cast<HMENU>(IDC_PROFILE_COMBO), m_hInst, nullptr);
    SendMessageW(m_hwndProfileCB, WM_SETFONT, (WPARAM)m_fontNorm, TRUE);
    ComboBox_AddString(m_hwndProfileCB, L"Default");
    ComboBox_SetCurSel(m_hwndProfileCB, 0);

    MakeButton(IDC_BTN_SAVE_PROFILE, L"💾 Save",  cx - 95, 292, 85, 26);

    // ── Timer section ─────────────────────────────────────────────────────
    MakeLabel(0, L"Timers:", 10, 334, 60, 18);

    m_hwndTimerList = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
        10, 356, cx - 20, 120,
        m_hwnd, reinterpret_cast<HMENU>(IDC_TIMER_LIST), m_hInst, nullptr);
    SendMessageW(m_hwndTimerList, WM_SETFONT, (WPARAM)m_fontNorm, TRUE);

    MakeButton(IDC_BTN_ADD_TIMER,    L"+ Add",   10,  484, 70, 26);
    MakeButton(IDC_BTN_TIMER_START,  L"▶ Start", 85,  484, 70, 26);
    MakeButton(IDC_BTN_TIMER_STOP,   L"■ Stop",  160, 484, 70, 26);
    MakeButton(IDC_BTN_TIMER_RESET,  L"↺ Reset", 235, 484, 70, 26);

    // Set dark background for the window
    SetClassLongPtrW(m_hwnd, GCLP_HBRBACKGROUND,
                     reinterpret_cast<LONG_PTR>(m_brBg));

    // Apply font to all children
    EnumChildWindows(m_hwnd, [](HWND child, LPARAM lp) -> BOOL {
        SendMessageW(child, WM_SETFONT, lp, TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(m_fontNorm));
}

// ── Painting helpers ──────────────────────────────────────────────────────────

HWND ControlPanel::MakeButton(UINT id, const wchar_t* label,
                               int x, int y, int w, int h)
{
    HWND btn = CreateWindowExW(
        0, L"BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        x, y, w, h,
        m_hwnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        m_hInst, nullptr);
    if (btn) SendMessageW(btn, WM_SETFONT, (WPARAM)m_fontNorm, TRUE);
    return btn;
}

HWND ControlPanel::MakeLabel(UINT id, const wchar_t* text,
                              int x, int y, int w, int h)
{
    HWND lbl = CreateWindowExW(
        0, L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h,
        m_hwnd, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
        m_hInst, nullptr);
    if (lbl) SendMessageW(lbl, WM_SETFONT, (WPARAM)m_fontNorm, TRUE);
    return lbl;
}

// ── OnPaint ───────────────────────────────────────────────────────────────────

void ControlPanel::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT cr{};
    GetClientRect(m_hwnd, &cr);

    // Background
    FillRect(hdc, &cr, m_brBg);

    // Accent top bar
    RECT topBar{ cr.left, cr.top, cr.right, cr.top + 6 };
    HBRUSH hAccent = CreateSolidBrush(CLR_ACCENT);
    FillRect(hdc, &topBar, hAccent);
    DeleteObject(hAccent);

    // Section separators (subtle lines)
    auto DrawLine = [&](int y) {
        HPEN hPen = CreatePen(PS_SOLID, 1, CLR_SURFACE);
        HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, cr.left + 10, y, nullptr);
        LineTo(hdc, cr.right - 10, y);
        SelectObject(hdc, hOld);
        DeleteObject(hPen);
    };

    DrawLine(243);  // Below mirror list + buttons
    DrawLine(283);  // Below opacity slider
    DrawLine(330);  // Below profile section

    EndPaint(m_hwnd, &ps);
}

// ── WM_CTLCOLORSTATIC / WM_CTLCOLOREDIT ──────────────────────────────────────

void ControlPanel::OnCtlColorStatic(HDC hdc, HWND /*hCtrl*/)
{
    SetTextColor(hdc, CLR_TEXT);
    SetBkColor(hdc, CLR_BG);
}

// ── Command handlers ──────────────────────────────────────────────────────────

void ControlPanel::ActionAddMirror()
{
    if (!m_deps.overlays) return;

    OverlayWindow::Config cfg{};
    cfg.name    = L"Mirror " + std::to_wstring(m_deps.overlays->size() + 1);
    cfg.x       = 100 + static_cast<int>(m_deps.overlays->size()) * 20;
    cfg.y       = 100 + static_cast<int>(m_deps.overlays->size()) * 20;
    cfg.locked  = false;
    cfg.visible = true;

    auto ov = std::make_unique<OverlayWindow>();
    if (ov->Create(m_hInst, cfg)) {
        if (m_deps.tibiaHwnd && *m_deps.tibiaHwnd) {
            ov->SetSourceWindow(*m_deps.tibiaHwnd);
            ov->UpdateMirror();
        }
        m_deps.overlays->push_back(std::move(ov));
        RefreshMirrorList();
    }
}

void ControlPanel::ActionRemoveMirror()
{
    if (!m_deps.overlays) return;
    int sel = ListBox_GetCurSel(m_hwndMirrorList);
    if (sel < 0 || sel >= static_cast<int>(m_deps.overlays->size())) return;

    m_deps.overlays->erase(m_deps.overlays->begin() + sel);
    RefreshMirrorList();
}

void ControlPanel::ActionEditMirror()
{
    // Placeholder: open a dedicated edit dialog for the selected overlay
    MessageBoxW(m_hwnd,
        L"Mirror editing dialog coming soon.\n\n"
        L"Use position/size values in the source code configuration for now.",
        L"Edit Mirror", MB_OK | MB_ICONINFORMATION);
}

void ControlPanel::ActionToggleLock()
{
    if (!m_deps.overlays) return;
    int sel = ListBox_GetCurSel(m_hwndMirrorList);
    if (sel < 0 || sel >= static_cast<int>(m_deps.overlays->size())) return;

    auto& ov  = (*m_deps.overlays)[sel];
    bool  now = !ov->GetConfig().locked;
    ov->SetLocked(now);
    RefreshMirrorList();
}

void ControlPanel::ActionToggleVisible()
{
    if (!m_deps.overlays) return;
    int sel = ListBox_GetCurSel(m_hwndMirrorList);
    if (sel < 0 || sel >= static_cast<int>(m_deps.overlays->size())) return;

    auto& ov  = (*m_deps.overlays)[sel];
    bool  now = !ov->GetConfig().visible;
    ov->SetVisible(now);
    RefreshMirrorList();
}

void ControlPanel::ActionSaveProfile()
{
    if (!m_deps.config || !m_deps.overlays) return;

    // Get selected profile name from combo box
    wchar_t buf[128]{};
    int sel = ComboBox_GetCurSel(m_hwndProfileCB);
    if (sel >= 0) {
        ComboBox_GetLBText(m_hwndProfileCB, sel, buf);
    }
    if (*buf == L'\0') wcscpy_s(buf, L"Default");

    Config::Profile profile{};
    profile.name = buf;
    for (auto& ov : *m_deps.overlays) {
        if (ov) profile.mirrors.push_back(ov->GetConfig());
    }

    if (m_deps.config->Save(profile)) {
        MessageBoxW(m_hwnd, L"Profile saved.", L"TibiaEyez", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(m_hwnd, L"Failed to save profile.", L"TibiaEyez", MB_OK | MB_ICONERROR);
    }
}

void ControlPanel::ActionLoadProfile()
{
    if (!m_deps.config || !m_deps.overlays) return;

    wchar_t buf[128]{};
    int sel = ComboBox_GetCurSel(m_hwndProfileCB);
    if (sel >= 0) {
        ComboBox_GetLBText(m_hwndProfileCB, sel, buf);
    }
    if (*buf == L'\0') return;

    Config::Profile profile{};
    if (!m_deps.config->Load(buf, profile)) {
        MessageBoxW(m_hwnd, L"Profile not found.", L"TibiaEyez", MB_OK | MB_ICONWARNING);
        return;
    }

    // Recreate overlays from profile
    m_deps.overlays->clear();
    for (auto& mc : profile.mirrors) {
        auto ov = std::make_unique<OverlayWindow>();
        if (ov->Create(m_hInst, mc)) {
            if (m_deps.tibiaHwnd && *m_deps.tibiaHwnd) {
                ov->SetSourceWindow(*m_deps.tibiaHwnd);
                ov->UpdateMirror();
            }
            m_deps.overlays->push_back(std::move(ov));
        }
    }
    RefreshMirrorList();
}

void ControlPanel::ActionStartTimer()
{
    if (!m_deps.timerSystem) return;
    int sel = ListBox_GetCurSel(m_hwndTimerList);
    if (sel < 0) return;

    const auto& timers = m_deps.timerSystem->GetTimers();
    if (sel < static_cast<int>(timers.size())) {
        m_deps.timerSystem->Start(timers[sel].name);
    }
}

void ControlPanel::ActionStopTimer()
{
    if (!m_deps.timerSystem) return;
    int sel = ListBox_GetCurSel(m_hwndTimerList);
    if (sel < 0) return;

    const auto& timers = m_deps.timerSystem->GetTimers();
    if (sel < static_cast<int>(timers.size())) {
        m_deps.timerSystem->Stop(timers[sel].name);
    }
}

void ControlPanel::ActionResetTimer()
{
    if (!m_deps.timerSystem) return;
    int sel = ListBox_GetCurSel(m_hwndTimerList);
    if (sel < 0) return;

    const auto& timers = m_deps.timerSystem->GetTimers();
    if (sel < static_cast<int>(timers.size())) {
        m_deps.timerSystem->Reset(timers[sel].name);
    }
}

void ControlPanel::ActionAddTimer()
{
    if (!m_deps.timerSystem) return;

    // Simple dialog: ask for timer name and duration
    // For PoC, add a hardcoded example timer
    TimerSystem::TimerEntry entry{};
    entry.name         = L"Spell " + std::to_wstring(m_deps.timerSystem->GetTimers().size() + 1);
    entry.totalSeconds = 30;
    entry.sound        = TimerSystem::AlertSound::Beep;
    entry.repeating    = false;

    m_deps.timerSystem->AddTimer(entry);

    // Refresh timer list
    ListBox_ResetContent(m_hwndTimerList);
    for (auto& t : m_deps.timerSystem->GetTimers()) {
        wchar_t buf[128]{};
        swprintf_s(buf, L"%s  — %ds  %s",
            t.name.c_str(),
            t.totalSeconds,
            t.running ? L"▶" : L"■");
        ListBox_AddString(m_hwndTimerList, buf);
    }
}

// ── OnCommand ─────────────────────────────────────────────────────────────────

void ControlPanel::OnCommand(UINT id, UINT /*notif*/, HWND /*hCtrl*/)
{
    switch (id) {
    case IDC_BTN_ADD_MIRROR:    ActionAddMirror();    break;
    case IDC_BTN_REMOVE_MIRROR: ActionRemoveMirror(); break;
    case IDC_BTN_EDIT_MIRROR:   ActionEditMirror();   break;
    case IDC_BTN_LOCK_MIRROR:   ActionToggleLock();   break;
    case IDC_BTN_HIDE_MIRROR:   ActionToggleVisible();break;
    case IDC_BTN_SAVE_PROFILE:  ActionSaveProfile();  break;
    case IDC_BTN_LOAD_PROFILE:  ActionLoadProfile();  break;
    case IDC_BTN_ADD_TIMER:     ActionAddTimer();     break;
    case IDC_BTN_TIMER_START:   ActionStartTimer();   break;
    case IDC_BTN_TIMER_STOP:    ActionStopTimer();    break;
    case IDC_BTN_TIMER_RESET:   ActionResetTimer();   break;
    }
}

void ControlPanel::OnHScroll(HWND hCtrl)
{
    if (hCtrl != m_hwndOpacity || !m_deps.overlays) return;

    int val = static_cast<int>(SendMessageW(m_hwndOpacity, TBM_GETPOS, 0, 0));
    BYTE opacity = static_cast<BYTE>((val * 255) / 100);

    int sel = ListBox_GetCurSel(m_hwndMirrorList);
    if (sel >= 0 && sel < static_cast<int>(m_deps.overlays->size())) {
        (*m_deps.overlays)[sel]->SetOpacity(opacity);
        (*m_deps.overlays)[sel]->UpdateMirror();
    }
}

void ControlPanel::OnSize(int /*cx*/, int /*cy*/)
{
    // Future: re-layout controls on resize
}

// ── WndProc ───────────────────────────────────────────────────────────────────

LRESULT CALLBACK ControlPanel::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    ControlPanel* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<ControlPanel*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<ControlPanel*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_CREATE:
        self->OnCreate();
        return 0;

    case WM_PAINT:
        self->OnPaint();
        return 0;

    case WM_COMMAND:
        self->OnCommand(LOWORD(wp), HIWORD(wp), reinterpret_cast<HWND>(lp));
        return 0;

    case WM_HSCROLL:
        self->OnHScroll(reinterpret_cast<HWND>(lp));
        return 0;

    case WM_SIZE:
        self->OnSize(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN: {
        HDC  hdc  = reinterpret_cast<HDC>(wp);
        SetTextColor(hdc, CLR_TEXT);
        SetBkColor(hdc, CLR_SURFACE);
        return reinterpret_cast<LRESULT>(self->m_brSurface);
    }

    case WM_ERASEBKGND: {
        HDC   hdc  = reinterpret_cast<HDC>(wp);
        RECT  cr{};
        GetClientRect(hwnd, &cr);
        FillRect(hdc, &cr, self->m_brBg);
        return 1;
    }

    case WM_CLOSE:
        // Hide instead of destroying so the user can reopen via tray
        self->Hide();
        return 0;

    case WM_DESTROY:
        self->m_hwnd = nullptr;
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace TibiaEyez
