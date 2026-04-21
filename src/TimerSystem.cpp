#include "TimerSystem.h"
#include <mmsystem.h>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace TibiaEyez {

// ── Timer management ───────────────────────────────────────────────────────────

void TimerSystem::AddTimer(const TimerEntry& entry)
{
    // Replace existing entry with the same name
    auto it = std::find_if(m_timers.begin(), m_timers.end(),
        [&](const TimerEntry& t) { return t.name == entry.name; });

    TimerEntry copy = entry;
    copy.remainingMs = entry.totalSeconds * 1000;
    copy.running     = false;

    if (it != m_timers.end()) {
        *it = std::move(copy);
    } else {
        m_timers.push_back(std::move(copy));
    }
}

void TimerSystem::RemoveTimer(const std::wstring& name)
{
    m_timers.erase(
        std::remove_if(m_timers.begin(), m_timers.end(),
            [&](const TimerEntry& t) { return t.name == name; }),
        m_timers.end());
}

TimerSystem::TimerEntry* TimerSystem::FindTimer(const std::wstring& name)
{
    auto it = std::find_if(m_timers.begin(), m_timers.end(),
        [&](const TimerEntry& t) { return t.name == name; });
    return (it != m_timers.end()) ? &*it : nullptr;
}

void TimerSystem::Start(const std::wstring& name)
{
    if (auto* t = FindTimer(name)) t->running = true;
}

void TimerSystem::Stop(const std::wstring& name)
{
    if (auto* t = FindTimer(name)) t->running = false;
}

void TimerSystem::Reset(const std::wstring& name)
{
    if (auto* t = FindTimer(name)) {
        t->remainingMs = t->totalSeconds * 1000;
        t->running     = false;
    }
}

void TimerSystem::StartAll()
{
    for (auto& t : m_timers) t.running = true;
}

void TimerSystem::StopAll()
{
    for (auto& t : m_timers) t.running = false;
}

// ── Tick ───────────────────────────────────────────────────────────────────────

void TimerSystem::Tick(int elapsedMs)
{
    for (auto& t : m_timers) {
        if (!t.running) continue;

        t.remainingMs -= elapsedMs;

        if (t.remainingMs <= 0) {
            PlayAlert(t);
            if (t.onExpiry) t.onExpiry(t.name);

            if (t.repeating) {
                t.remainingMs = t.totalSeconds * 1000;
            } else {
                t.running     = false;
                t.remainingMs = 0;
            }
        }
    }
}

// ── PlayAlert ──────────────────────────────────────────────────────────────────

void TimerSystem::PlayAlert(const TimerEntry& entry)
{
    switch (entry.sound) {
    case AlertSound::Beep:
        // Simple synchronous beep — never blocks more than ~200 ms
        MessageBeep(MB_OK);
        break;

    case AlertSound::Asterisk:
        MessageBeep(MB_ICONASTERISK);
        break;

    case AlertSound::Exclamation:
        MessageBeep(MB_ICONEXCLAMATION);
        break;

    case AlertSound::CustomWav:
        if (!entry.customWavPath.empty()) {
            // SND_ASYNC: play without blocking the message loop
            // SND_FILENAME: path is a file name
            // SND_NODEFAULT: don't fall back to default sound if file not found
            PlaySoundW(entry.customWavPath.c_str(), nullptr,
                       SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        } else {
            MessageBeep(MB_OK);
        }
        break;
    }
}

} // namespace TibiaEyez
