#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <objbase.h>   // CoInitialize

#include "Application.h"

// ─────────────────────────────────────────────────────────────────────────────
// WinMain — application entry point
// ─────────────────────────────────────────────────────────────────────────────
int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_     LPWSTR    /*lpCmdLine*/,
    _In_     int       /*nCmdShow*/)
{
    // COM initialisation required for shell APIs (SHGetFolderPath, etc.)
    CoInitialize(nullptr);

    // Enable DPI awareness — prevents blurry overlays on high-DPI displays
    SetProcessDPIAware();

    TibiaEyez::Application app;
    if (!app.Init(hInstance)) {
        MessageBoxW(nullptr,
            L"Failed to initialise TibiaEyez.\n"
            L"Please ensure you are running Windows 10 or later with DWM enabled.",
            L"TibiaEyez — Fatal Error",
            MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    int exitCode = app.Run();

    CoUninitialize();
    return exitCode;
}
