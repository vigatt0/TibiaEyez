#include "OverlayWindow.h"
#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "dwmapi.lib")

namespace TibiaEyez {

bool OverlayWindow::s_classRegistered = false;

// ── Window class registration ─────────────────────────────────────────────────

bool OverlayWindow::RegisterWindowClass(HINSTANCE hInst)
{
    if (s_classRegistered) return true;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wc.lpszClassName = ClassName();

    s_classRegistered = (RegisterClassExW(&wc) != 0);
    return s_classRegistered;
}

// ── Create / Destroy ──────────────────────────────────────────────────────────

bool OverlayWindow::Create(HINSTANCE hInst, const Config& cfg)
{
    m_hInst  = hInst;
    m_config = cfg;

    // Extended styles:
    //   WS_EX_LAYERED    – enables alpha / transparency
    //   WS_EX_TRANSPARENT – mouse events fall through to window beneath
    //   WS_EX_TOPMOST    – always on top
    //   WS_EX_TOOLWINDOW – no taskbar button
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    // When unlocked we temporarily remove WS_EX_TRANSPARENT so the user can
    // drag the window; it is restored when locked.

    // Base style: popup (no caption, no system border)
    DWORD style = WS_POPUP;

    m_hwnd = CreateWindowExW(
        exStyle,
        ClassName(),
        cfg.name.c_str(),
        style,
        cfg.x, cfg.y, cfg.width, cfg.height,
        nullptr, nullptr, hInst, this);

    if (!m_hwnd) return false;

    // Set initial opacity / visibility
    ApplyOpacityToWindow();

    if (cfg.visible) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    }

    // DWM: enable blur-behind so painting over transparent areas works
    DWM_BLURBEHIND bb{};
    bb.dwFlags  = DWM_BB_ENABLE;
    bb.fEnable  = FALSE;
    DwmEnableBlurBehindWindow(m_hwnd, &bb);

    return true;
}

void OverlayWindow::Destroy()
{
    m_mirror.Destroy();
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

// ── Source window management ──────────────────────────────────────────────────

void OverlayWindow::SetSourceWindow(HWND hwndTibia)
{
    if (m_tibiaHwnd == hwndTibia) return;
    m_tibiaHwnd = hwndTibia;

    m_mirror.Destroy();

    if (m_hwnd && hwndTibia) {
        if (m_mirror.Register(hwndTibia, m_hwnd)) {
            UpdateMirror();
        }
    }
}

// ── UpdateMirror ──────────────────────────────────────────────────────────────

void OverlayWindow::UpdateMirror()
{
    if (!m_hwnd || !m_mirror.IsValid()) return;

    RECT destRect{ 0, 0, m_config.width, m_config.height };
    m_mirror.Update(m_config.srcRegion, destRect, m_config.opacity);
    RefreshGlowBorder();
}

// ── Config helpers ────────────────────────────────────────────────────────────

void OverlayWindow::ApplyConfig(const Config& cfg)
{
    m_config = cfg;
    if (!m_hwnd) return;

    ::MoveWindow(m_hwnd, cfg.x, cfg.y, cfg.width, cfg.height, TRUE);
    ApplyOpacityToWindow();
    ApplyWindowStyle();
    SetVisible(cfg.visible);
    UpdateMirror();
}

void OverlayWindow::SetLocked(bool locked)
{
    m_config.locked = locked;
    ApplyWindowStyle();
}

void OverlayWindow::SetVisible(bool visible)
{
    m_config.visible = visible;
    if (!m_hwnd) return;
    ShowWindow(m_hwnd, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
}

void OverlayWindow::SetOpacity(BYTE opacity)
{
    m_config.opacity = opacity;
    ApplyOpacityToWindow();
    UpdateMirror(); // DWM thumbnail also respects per-thumbnail opacity
}

void OverlayWindow::SetGlowColor(COLORREF color)
{
    m_config.glowColor = color;
    RefreshGlowBorder();
}

void OverlayWindow::SetShowGrid(bool show)
{
    m_config.showGrid = show;
    RefreshGlowBorder();
}

void OverlayWindow::SetGridCellSize(int px)
{
    m_config.gridCellPx = (px > 4) ? px : 4;
    if (m_config.showGrid) RefreshGlowBorder();
}

void OverlayWindow::SetSourceRegion(const DwmMirror::SourceRegion& src)
{
    m_config.srcRegion = src;
    UpdateMirror();
}

void OverlayWindow::MoveWindow(int x, int y, int w, int h)
{
    m_config.x = x;
    m_config.y = y;
    if (w > 0) m_config.width  = w;
    if (h > 0) m_config.height = h;

    if (m_hwnd) {
        ::MoveWindow(m_hwnd, m_config.x, m_config.y,
                     m_config.width, m_config.height, TRUE);
        UpdateMirror();
    }
}

// ── Internal: window style ────────────────────────────────────────────────────

void OverlayWindow::ApplyWindowStyle()
{
    if (!m_hwnd) return;

    LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);

    if (m_config.locked) {
        // Locked → click-through, no caption
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        // Unlocked → remove click-through so user can drag
        exStyle &= ~WS_EX_TRANSPARENT;
    }

    SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
}

void OverlayWindow::ApplyOpacityToWindow()
{
    if (!m_hwnd) return;
    // Use a constant alpha of 255 — the DWM thumbnail handles per-pixel
    // opacity via DWM_THUMBNAIL_PROPERTIES::opacity.
    // SetLayeredWindowAttributes controls the overall window alpha.
    SetLayeredWindowAttributes(m_hwnd, 0, m_config.opacity, LWA_ALPHA);
}

// ── Internal: painting ────────────────────────────────────────────────────────

void OverlayWindow::RefreshGlowBorder()
{
    if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
}

void OverlayWindow::DrawGlowBorder(HDC hdc, const RECT& clientRect)
{
    if (!m_config.showGlowBorder) return;

    // Draw a 3-layer "glow" border by painting progressively thicker pens
    // with decreasing alpha-like intensity (simulated via colour mixing).
    COLORREF base = m_config.glowColor;
    int r = GetRValue(base), g = GetGValue(base), b = GetBValue(base);

    // Outer glow layers (dim)
    struct Layer { int inset; int alpha255; };
    Layer layers[] = { {0, 60}, {1, 120}, {2, 220} };

    for (auto& layer : layers) {
        int intensity = layer.alpha255;
        COLORREF col = RGB(
            (r * intensity) / 255,
            (g * intensity) / 255,
            (b * intensity) / 255);

        HPEN hPen = CreatePen(PS_SOLID, 1, col);
        HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));
        HBRUSH hBr = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, hBr));

        RECT r2 = {
            clientRect.left   + layer.inset,
            clientRect.top    + layer.inset,
            clientRect.right  - layer.inset,
            clientRect.bottom - layer.inset
        };
        Rectangle(hdc, r2.left, r2.top, r2.right, r2.bottom);

        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOld);
        DeleteObject(hPen);
    }
}

void OverlayWindow::DrawGridOverlay(HDC hdc, const RECT& cr)
{
    if (!m_config.showGrid) return;

    int cell = m_config.gridCellPx;
    if (cell <= 0) return;

    // Semi-transparent white dashed grid lines
    HPEN hPen = CreatePen(PS_DOT, 1, RGB(255, 255, 255));
    HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));

    // Vertical lines
    for (int x = cr.left; x < cr.right; x += cell) {
        MoveToEx(hdc, x, cr.top, nullptr);
        LineTo(hdc, x, cr.bottom);
    }
    // Horizontal lines
    for (int y = cr.top; y < cr.bottom; y += cell) {
        MoveToEx(hdc, cr.left, y, nullptr);
        LineTo(hdc, cr.right, y);
    }

    SelectObject(hdc, hOld);
    DeleteObject(hPen);
}

void OverlayWindow::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT cr{};
    GetClientRect(m_hwnd, &cr);

    // Fill with pure black which becomes transparent via SetLayeredWindowAttributes
    // (key colour 0x000000 is NOT used here; we use constant alpha instead).
    // Paint the background transparent.
    HBRUSH hBg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &cr, hBg);
    DeleteObject(hBg);

    DrawGlowBorder(hdc, cr);
    DrawGridOverlay(hdc, cr);

    // When unlocked draw a subtle drag-handle indicator at top
    if (!m_config.locked) {
        SetTextColor(hdc, RGB(200, 200, 200));
        SetBkMode(hdc, TRANSPARENT);
        RECT labelRect = { cr.left + 4, cr.top + 2, cr.right - 4, cr.top + 18 };
        DrawTextW(hdc, m_config.name.c_str(), -1, &labelRect,
                  DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP);
    }

    EndPaint(m_hwnd, &ps);
}

// ── Drag support ──────────────────────────────────────────────────────────────

void OverlayWindow::OnLButtonDown(int x, int y)
{
    if (m_config.locked) return;
    m_dragging    = true;
    m_dragOffset  = { x, y };
    SetCapture(m_hwnd);
}

void OverlayWindow::OnMouseMove(int x, int y)
{
    if (!m_dragging) return;

    POINT screen{ x, y };
    ClientToScreen(m_hwnd, &screen);

    int newX = screen.x - m_dragOffset.x;
    int newY = screen.y - m_dragOffset.y;
    m_config.x = newX;
    m_config.y = newY;

    SetWindowPos(m_hwnd, nullptr, newX, newY, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void OverlayWindow::OnLButtonUp()
{
    if (m_dragging) {
        m_dragging = false;
        ReleaseCapture();
    }
}

void OverlayWindow::OnRButtonUp(int /*x*/, int /*y*/)
{
    // Placeholder: a future context menu can go here
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    OverlayWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<OverlayWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!self) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
    case WM_PAINT:
        self->OnPaint();
        return 0;

    case WM_LBUTTONDOWN:
        self->OnLButtonDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;

    case WM_MOUSEMOVE:
        self->OnMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;

    case WM_LBUTTONUP:
        self->OnLButtonUp();
        return 0;

    case WM_RBUTTONUP:
        self->OnRButtonUp(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        return 0;

    case WM_DESTROY:
        self->m_mirror.Destroy();
        self->m_hwnd = nullptr;
        return 0;

    // Keep window always on top after any Z-order change
    case WM_WINDOWPOSCHANGING: {
        auto* wp2 = reinterpret_cast<WINDOWPOS*>(lp);
        wp2->hwndInsertAfter = HWND_TOPMOST;
        wp2->flags &= ~SWP_NOZORDER;
        break;
    }
    }

    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace TibiaEyez
