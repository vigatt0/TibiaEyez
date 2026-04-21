cmake_minimum_required(VERSION 3.16)
project(TibiaEyez VERSION 1.0.0 LANGUAGES CXX)

# ── Language standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Platform guard ─────────────────────────────────────────────────────────────
if(NOT WIN32)
    message(FATAL_ERROR "TibiaEyez is a Windows-only application (requires DWM / Win32).")
endif()

# ── Sources ────────────────────────────────────────────────────────────────────
set(TIBIA_SOURCES
    src/main.cpp
    src/Application.cpp
    src/TibiaFinder.cpp
    src/DwmMirror.cpp
    src/OverlayWindow.cpp
    src/ControlPanel.cpp
    src/HotkeyManager.cpp
    src/TimerSystem.cpp
    src/Config.cpp
)

set(TIBIA_HEADERS
    src/Application.h
    src/TibiaFinder.h
    src/DwmMirror.h
    src/OverlayWindow.h
    src/ControlPanel.h
    src/HotkeyManager.h
    src/TimerSystem.h
    src/Config.h
)

set(TIBIA_RESOURCES
    resources/TibiaEyez.rc
)

# ── Executable ─────────────────────────────────────────────────────────────────
add_executable(TibiaEyez WIN32
    ${TIBIA_SOURCES}
    ${TIBIA_HEADERS}
    ${TIBIA_RESOURCES}
)

# ── Windows system libraries ───────────────────────────────────────────────────
target_link_libraries(TibiaEyez PRIVATE
    user32      # Window management
    gdi32       # GDI painting
    dwmapi      # DwmRegisterThumbnail / DwmUpdateThumbnailProperties
    winmm       # PlaySound / timeSetEvent
    comctl32    # Common controls (progress bar, list view, etc.)
    shell32     # SHGetKnownFolderPath for AppData path
    ole32       # CoInitialize
    uuid        # GUID constants
    psapi       # GetModuleFileNameExW (process name lookup)
)

# ── Compile definitions ────────────────────────────────────────────────────────
target_compile_definitions(TibiaEyez PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00   # Target Windows 10+
    WINVER=0x0A00
)

# ── Include directory ──────────────────────────────────────────────────────────
target_include_directories(TibiaEyez PRIVATE src resources)

# ── MSVC-specific settings ─────────────────────────────────────────────────────
if(MSVC)
    # Warning level 4, treat as errors (except C4100 unreferenced formal parameter)
    target_compile_options(TibiaEyez PRIVATE /W4 /wd4100 /permissive-)

    # Static CRT — no runtime DLL required (standalone deployment)
    set_property(TARGET TibiaEyez PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # High-resolution executable
    set_property(TARGET TibiaEyez PROPERTY
        VS_DPI_AWARE "PerMonitor")

    # LTCG in Release
    target_compile_options(TibiaEyez PRIVATE $<$<CONFIG:Release>:/GL>)
    target_link_options(TibiaEyez PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS TibiaEyez DESTINATION .)
