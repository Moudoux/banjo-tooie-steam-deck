#pragma once
// F4 audio-domain retrospective ring (issue_evidence.cpp); records only in BASELINE_PLAYER.
// Hooks copy argument bits only. No guest state or PCM is retained or changed.
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace baseline::sound_trace {
enum class Kind { FlipInit, FlipSubstate, Landing, SoundId, StartRequest, Submit, Stop,
                  AudioChunk, AudioClock, AudioDrop, AudioFrequency };
struct Event {
    uint64_t sequence, time_us;
    Kind kind;
    uint64_t a, b, aux;
};
constexpr size_t capacity = 8192;
constexpr uint64_t window_us = 10000000;
inline uint64_t now_us() noexcept {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct Snapshot {
    uint64_t end_us, overwritten;
    bool truncated;
    uint64_t frequency, clock_rate, clock_revision;
    std::vector<Event> events;
};
class Ring {
    std::mutex mutex;
    std::array<Event, capacity> events{};
    uint64_t written = 0;
    uint64_t last_overwritten_us = 0, frequency = 0, clock_rate = 1, clock_revision = 0;
public:
    void record(Kind kind, uint64_t a, uint64_t b, uint64_t aux) noexcept {
        std::lock_guard guard(mutex);
        if (written >= capacity) last_overwritten_us = events[written % capacity].time_us;
        if (kind == Kind::AudioFrequency) frequency = a;
        if (kind == Kind::AudioClock) { clock_rate = a; clock_revision = b; }
        events[written % capacity] = {written + 1, now_us(), kind, a, b, aux};
        ++written;
    }
    Snapshot snapshot() {
        Snapshot result{};
        result.events.reserve(capacity); // Allocate outside the producer lock.
        std::lock_guard guard(mutex);
        result.end_us = now_us();
        result.overwritten = written > capacity ? written - capacity : 0;
        result.truncated = written > capacity && result.end_us - last_overwritten_us <= window_us;
        result.frequency = frequency;
        result.clock_rate = clock_rate;
        result.clock_revision = clock_revision;
        for (uint64_t i = result.overwritten; i < written; ++i) {
            const auto& event = events[i % capacity];
            if (result.end_us - event.time_us <= window_us) result.events.push_back(event);
        }
        return result;
    }
};
inline Ring ring;
inline void record(Kind kind, uint64_t a = 0, uint64_t b = 0, uint64_t aux = 0) noexcept {
#if BASELINE_PLAYER
    ring.record(kind, a, b, aux);
#endif
}
inline const char* name(Kind kind) noexcept {
    switch (kind) {
    case Kind::FlipInit: return "flip_init";
    case Kind::FlipSubstate: return "flip_substate_request";
    case Kind::Landing: return "landing";
    case Kind::SoundId: return "sound_id_request";
    case Kind::StartRequest: return "sound_start_request";
    case Kind::Submit: return "sound_submit_stage";
    case Kind::Stop: return "sound_stop_request";
    case Kind::AudioChunk: return "audio_chunk";
    case Kind::AudioClock: return "audio_clock_clear";
    case Kind::AudioDrop: return "audio_accelerated_drop";
    case Kind::AudioFrequency: return "audio_frequency";
    }
    return "unknown";
}
inline std::string format(const Snapshot& captured, unsigned marker) {
    std::string text;
    text.reserve(1024 + captured.events.size() * 200);
    char line[512];
    std::snprintf(line, sizeof(line),
        "sound-trace begin marker=%u window_us=%llu end_us=%llu events=%zu overwritten_total=%llu capacity=%zu schema=1 enabled=%d truncated=%d hz=%llu clock_rate=%llu clock_revision=%llu\n",
        marker, static_cast<unsigned long long>(window_us),
        static_cast<unsigned long long>(captured.end_us), captured.events.size(),
        static_cast<unsigned long long>(captured.overwritten), capacity,
#if BASELINE_PLAYER
        1,
#else
        0,
#endif
        int(captured.truncated), static_cast<unsigned long long>(captured.frequency),
        static_cast<unsigned long long>(captured.clock_rate), static_cast<unsigned long long>(captured.clock_revision));
    text += line;
    text += "sound-trace fields: guest a=self/handle b=substate/sound_id aux=raw_r31_not_caller; audio_chunk a=int16_sample_count b=pcm_byte_fingerprint aux=queued_bytes; audio_clock a=rate b=revision\n";
    // File I/O holds no producer lock. All fields are numerical, not raw memory.
    for (const auto& event : captured.events) {
        std::snprintf(line, sizeof(line),
            "sound-trace marker=%u seq=%llu age_us=%llu event=%s a=0x%llx b=0x%llx aux=0x%llx\n",
            marker, static_cast<unsigned long long>(event.sequence),
            static_cast<unsigned long long>(captured.end_us - event.time_us), name(event.kind),
            static_cast<unsigned long long>(event.a), static_cast<unsigned long long>(event.b),
            static_cast<unsigned long long>(event.aux));
        text += line;
    }
    std::snprintf(line, sizeof(line), "sound-trace end marker=%u\n", marker);
    text += line;
    return text;
}
inline bool dump(FILE* file, unsigned marker, const Snapshot& captured) {
    const auto text = format(captured, marker);
    // One buffered write avoids thousands of unbuffered stderr calls on F4.
    const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
    return std::fflush(file) == 0 && written;
}
inline bool dump(FILE* file, unsigned marker) {
    return dump(file, marker, ring.snapshot());
}
} // namespace baseline::sound_trace
