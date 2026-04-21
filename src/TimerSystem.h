#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace TibiaEyez {

// ─────────────────────────────────────────────────────────────────────────────
// TimerSystem
//   Manages a collection of named countdown timers.
//   When a timer expires it plays an audio alert via PlaySound / MessageBeep.
//   Timers are polled by calling Tick() from the application's WM_TIMER handler.
// ─────────────────────────────────────────────────────────────────────────────
class TimerSystem {
public:
    enum class AlertSound {
        Beep,       // Simple beep (no file needed)
        Asterisk,   // SystemAsterisk
        Exclamation,// SystemExclamation
        CustomWav   // Custom .wav file path
    };

    struct TimerEntry {
        std::wstring name;
        int          totalSeconds   = 60;
        int          remainingMs    = 0;   // milliseconds remaining
        bool         running        = false;
        bool         repeating      = false;
        AlertSound   sound          = AlertSound::Beep;
        std::wstring customWavPath;        // Only used when sound == CustomWav

        // Optional callback invoked on expiry (in addition to the sound)
        std::function<void(const std::wstring& name)> onExpiry;
    };

    TimerSystem()  = default;
    ~TimerSystem() = default;

    // Add a timer (or replace one with the same name)
    void AddTimer(const TimerEntry& entry);
    void RemoveTimer(const std::wstring& name);

    // Control
    void Start(const std::wstring& name);
    void Stop(const std::wstring& name);
    void Reset(const std::wstring& name);
    void StartAll();
    void StopAll();

    // Call once per WM_TIMER tick (pass elapsed ms since last call)
    void Tick(int elapsedMs);

    const std::vector<TimerEntry>& GetTimers() const { return m_timers; }
    TimerEntry* FindTimer(const std::wstring& name);

private:
    void PlayAlert(const TimerEntry& entry);

    std::vector<TimerEntry> m_timers;
};

} // namespace TibiaEyez
