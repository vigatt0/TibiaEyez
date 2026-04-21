#include "DwmMirror.h"

#pragma comment(lib, "dwmapi.lib")

namespace TibiaEyez {

// ── Move semantics ─────────────────────────────────────────────────────────────

DwmMirror::DwmMirror(DwmMirror&& o) noexcept
    : m_thumbnail(o.m_thumbnail)
    , m_hwndSource(o.m_hwndSource)
    , m_hwndDest(o.m_hwndDest)
{
    o.m_thumbnail  = nullptr;
    o.m_hwndSource = nullptr;
    o.m_hwndDest   = nullptr;
}

DwmMirror& DwmMirror::operator=(DwmMirror&& o) noexcept
{
    if (this != &o) {
        Destroy();
        m_thumbnail  = o.m_thumbnail;
        m_hwndSource = o.m_hwndSource;
        m_hwndDest   = o.m_hwndDest;
        o.m_thumbnail  = nullptr;
        o.m_hwndSource = nullptr;
        o.m_hwndDest   = nullptr;
    }
    return *this;
}

// ── Register ───────────────────────────────────────────────────────────────────

bool DwmMirror::Register(HWND hwndSource, HWND hwndDest)
{
    Destroy();

    if (!hwndSource || !hwndDest) return false;
    if (!IsWindow(hwndSource) || !IsWindow(hwndDest)) return false;

    // DwmRegisterThumbnail: the compositor will start feeding frames from
    // hwndSource into hwndDest — entirely within the DWM compositor, zero
    // game-process interaction.
    HRESULT hr = DwmRegisterThumbnail(hwndDest, hwndSource, &m_thumbnail);
    if (FAILED(hr)) {
        m_thumbnail = nullptr;
        return false;
    }

    m_hwndSource = hwndSource;
    m_hwndDest   = hwndDest;
    return true;
}

// ── Update ─────────────────────────────────────────────────────────────────────

bool DwmMirror::Update(const SourceRegion& src, const RECT& destRect, BYTE opacity)
{
    if (!IsValid()) return false;

    DWM_THUMBNAIL_PROPERTIES props{};
    props.dwFlags =
        DWM_TNP_VISIBLE             |
        DWM_TNP_RECTDESTINATION     |
        DWM_TNP_RECTSOURCE          |
        DWM_TNP_OPACITY             |
        DWM_TNP_SOURCECLIENTAREAONLY;

    props.fVisible              = TRUE;
    props.opacity               = opacity;
    props.rcDestination         = destRect;
    // rcSource crops the client area of the source window
    props.rcSource              = { src.x, src.y, src.x + src.w, src.y + src.h };
    props.fSourceClientAreaOnly = TRUE; // Exclude title-bar / frame of Tibia

    HRESULT hr = DwmUpdateThumbnailProperties(m_thumbnail, &props);
    return SUCCEEDED(hr);
}

// ── GetSourceSize ──────────────────────────────────────────────────────────────

bool DwmMirror::GetSourceSize(SIZE& outSize) const
{
    if (!IsValid()) return false;
    HRESULT hr = DwmQueryThumbnailSourceSize(m_thumbnail, &outSize);
    return SUCCEEDED(hr);
}

// ── Destroy ────────────────────────────────────────────────────────────────────

void DwmMirror::Destroy()
{
    if (m_thumbnail) {
        DwmUnregisterThumbnail(m_thumbnail);
        m_thumbnail  = nullptr;
        m_hwndSource = nullptr;
        m_hwndDest   = nullptr;
    }
}

} // namespace TibiaEyez
