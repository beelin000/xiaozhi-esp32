#ifndef AGENT_WAKE_VAD_H
#define AGENT_WAKE_VAD_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/**
 * Wake-word + voice-activity detection stub.
 * Production path: audio/wake_word.h + AudioProcessor::OnVadStateChange.
 */
enum class WakeVadState {
    kIdle,
    kListeningForWake,
    kSpeechActive,
    kSpeechEnded,
};

class WakeVad {
public:
    using WakeCallback = std::function<void(const std::string& wake_word)>;
    using VadCallback = std::function<void(bool speaking)>;

    virtual ~WakeVad() = default;

    virtual bool Init(int sample_rate_hz) = 0;
    virtual void Deinit() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;

    /** Feed PCM frames from the microphone HAL. */
    virtual void Feed(const std::vector<int16_t>& pcm) = 0;

    virtual void OnWakeWord(WakeCallback callback) = 0;
    virtual void OnVadStateChange(VadCallback callback) = 0;

    virtual WakeVadState state() const = 0;
    virtual const std::string& last_wake_word() const = 0;
};

WakeVad* CreateWakeVad();

#endif  // AGENT_WAKE_VAD_H
