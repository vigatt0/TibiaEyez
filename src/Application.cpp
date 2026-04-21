#include "Application.h"
#include "TibiaFinder.h"
#include "OverlayWindow.h"
#include "DwmMirror.h"
#include "HotkeyManager.h"
#include "TimerSystem.h"
#include "Config.h"
#include "ControlPanel.h"

#include <dwmapi.h>

namespace TibiaEyez {

// ── Init ──────────────────────────────────────────────────────────────────────

bool Application::Init(HINSTANCE hInst)
{
    m_hInst = hInst;

    // Register window classes
    if (!OverlayWindow::RegisterWindowClass(hInst))  return false;
    if (!ControlPanel::RegisterWindowClass(hInst))   return false;

    // Create the hidden message-only window first (needed before hotkeys)
    if (!CreateMessageWindow()) return false;

    // Subsystems
    m_tibiaFinder = std::make_unique<TibiaFinder>();
    m_hotkeyMgr   = std::make_unique<HotkeyManager>(m_hwndMsg);
    m_timerSys    = std::make_unique<TimerSystem>();
    m_config      = std::make_unique<Config>();

    // Wire up TibiaFinder callbacks
    m_tibiaFinder->SetCallbacks(
        [this](HWND hwnd) { OnTibiaFound(hwnd); },
        [this]()          { OnTibiaLost();       });

    // Control panel
    m_controlPanel = std::make_unique<ControlPanel>();
    ControlPanel::Deps deps{};
    deps.overlays      = &m_overlays;
    deps.hotkeyManager = m_hotkeyMgr.get();
    deps.timerSystem   = m_timerSys.get();
    deps.config        = m_config.get();
    deps.tibiaFinder   = m_tibiaFinder.get();
    deps.tibiaHwnd     = &m_tibiaHwnd;

    if (!m_controlPanel->Create(hInst, deps)) return false;
    m_controlPanel->Show();

    // ── Default hotkeys ────────────────────────────────────────────────────
    // Ctrl+Alt+T — Toggle control panel
    m_hotkeyMgr->Register(L"TogglePanel", MOD_CONTROL | MOD_ALT, 'T');
    m_hotkeyMgr->SetCallback(L"TogglePanel", [this]() {
        m_controlPanel->Toggle();
    });

    // Ctrl+Alt+H — Hide / show all overlays
    m_hotkeyMgr->Register(L"ToggleOverlays", MOD_CONTROL | MOD_ALT, 'H');
    m_hotkeyMgr->SetCallback(L"ToggleOverlays", [this]() {
        static bool hidden = false;
        hidden = !hidden;
        for (auto& ov : m_overlays) {
            if (ov) ov->SetVisible(!hidden);
        }
    });

    // ── Try to load Default profile ───────────────────────────────────────
    Config::Profile profile{};
    if (m_config->Load(L"Default", profile)) {
        for (auto& mc : profile.mirrors) {
            auto ov = std::make_unique<OverlayWindow>();
            if (ov->Create(hInst, mc)) {
                m_overlays.push_back(std::move(ov));
            }
        }
        m_controlPanel->RefreshMirrorList();
    }

    // ── Timers ────────────────────────────────────────────────────────────
    SetTimer(m_hwndMsg, TIMER_TIBIA_POLL, TIMER_POLL_MS, nullptr);
    SetTimer(m_hwndMsg, TIMER_TICK,       TIMER_TICK_MS, nullptr);
    m_lastTickMs = GetTickCount();

    // Initial poll so we connect immediately if Tibia is already open
    m_tibiaFinder->Poll();

    return true;
}

// ── Run ───────────────────────────────────────────────────────────────────────

int Application::Run()
{
    m_running = true;

    MSG msg{};
    while (m_running) {
        // Use GetMessage which blocks until a message arrives, keeping CPU at 0%
        // when idle (WM_TIMER wakes it up periodically).
        BOOL ret = GetMessageW(&msg, nullptr, 0, 0);
        if (ret == 0) {
            // WM_QUIT received
            m_exitCode = static_cast<int>(msg.wParam);
            break;
        }
        if (ret < 0) break; // Error

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Persist the current layout on exit
    if (m_config && !m_overlays.empty()) {
        Config::Profile profile{};
        profile.name = L"Default";
        for (auto& ov : m_overlays) {
            if (ov) profile.mirrors.push_back(ov->GetConfig());
        }
        m_config->Save(profile);
    }

    return m_exitCode;
}

void Application::Quit(int exitCode)
{
    m_exitCode = exitCode;
    m_running  = false;
    PostQuitMessage(exitCode);
}

// ── Message-only window ───────────────────────────────────────────────────────

bool Application::CreateMessageWindow()
{
    // A HWND_MESSAGE window is never shown and has no overhead.
    m_hwndMsg = CreateWindowExW(
        0, L"STATIC", L"TibiaEyezMsg",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, m_hInst, nullptr);

    if (!m_hwndMsg) return false;

    // Subclass the static control to intercept timer and hotkey messages
    SetWindowLongPtrW(m_hwndMsg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    SetWindowLongPtrW(m_hwndMsg, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(MsgWndProc));
    return true;
}

LRESULT CALLBACK Application::MsgWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!app) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_TIMER:
        if (wp == TIMER_TIBIA_POLL) app->OnTibiaPollingTimer();
        if (wp == TIMER_TICK)       app->OnTickTimer();
        return 0;

    case WM_HOTKEY:
        app->m_hotkeyMgr->Dispatch(wp);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Timer callbacks ───────────────────────────────────────────────────────────

void Application::OnTibiaPollingTimer()
{
    m_tibiaFinder->Poll();

    // Update status in control panel
    if (m_controlPanel) {
        bool connected = (m_tibiaHwnd != nullptr);
        wchar_t title[256]{};
        if (connected) GetWindowTextW(m_tibiaHwnd, title, 255);
        m_controlPanel->SetTibiaStatus(connected, title);
    }
}

void Application::OnTickTimer()
{
    DWORD now = GetTickCount();
    int   elapsed = static_cast<int>(now - m_lastTickMs);
    m_lastTickMs  = now;

    m_timerSys->Tick(elapsed);
}

// ── Tibia window found / lost ─────────────────────────────────────────────────

void Application::OnTibiaFound(HWND hwnd)
{
    m_tibiaHwnd = hwnd;

    // Register the DWM thumbnail for every overlay
    for (auto& ov : m_overlays) {
        if (ov) {
            ov->SetSourceWindow(hwnd);
            ov->UpdateMirror();
        }
    }

    // If no overlays were loaded from a profile, create defaults
    if (m_overlays.empty()) {
        CreateDefaultMirrors();
    }

    if (m_controlPanel) {
        wchar_t title[256]{};
        GetWindowTextW(hwnd, title, 255);
        m_controlPanel->SetTibiaStatus(true, title);
        m_controlPanel->RefreshMirrorList();
    }
}

void Application::OnTibiaLost()
{
    m_tibiaHwnd = nullptr;

    // Detach thumbnails — they become invalid when the source window closes
    for (auto& ov : m_overlays) {
        if (ov) ov->SetSourceWindow(nullptr);
    }

    if (m_controlPanel) {
        m_controlPanel->SetTibiaStatus(false);
    }
}

// ── Default mirror creation ────────────────────────────────────────────────────

void Application::CreateDefaultMirrors()
{
    if (!m_tibiaHwnd) return;

    // Example: mirror the top-left 300×80 pixels (typical action-bar area)
    OverlayWindow::Config barCfg{};
    barCfg.name         = L"Action Bar";
    barCfg.x            = 50;
    barCfg.y            = 50;
    barCfg.width        = 300;
    barCfg.height       = 80;
    barCfg.srcRegion    = { 0, 0, 300, 80 };
    barCfg.opacity      = 230;
    barCfg.locked       = false;
    barCfg.visible      = true;
    barCfg.showGlowBorder = true;

    auto bar = std::make_unique<OverlayWindow>();
    if (bar->Create(m_hInst, barCfg)) {
        bar->SetSourceWindow(m_tibiaHwnd);
        bar->UpdateMirror();
        m_overlays.push_back(std::move(bar));
    }

    // Example: mirror a 150×150 status area
    OverlayWindow::Config statusCfg{};
    statusCfg.name         = L"Status";
    statusCfg.x            = 50;
    statusCfg.y            = 150;
    statusCfg.width        = 150;
    statusCfg.height       = 150;
    statusCfg.srcRegion    = { 0, 0, 150, 150 };
    statusCfg.opacity      = 200;
    statusCfg.locked       = false;
    statusCfg.visible      = true;
    statusCfg.showGlowBorder = true;
    statusCfg.glowColor    = RGB(233, 69, 96); // Red accent

    auto status = std::make_unique<OverlayWindow>();
    if (status->Create(m_hInst, statusCfg)) {
        status->SetSourceWindow(m_tibiaHwnd);
        status->UpdateMirror();
        m_overlays.push_back(std::move(status));
    }

    if (m_controlPanel) m_controlPanel->RefreshMirrorList();
}

} // namespace TibiaEyez
