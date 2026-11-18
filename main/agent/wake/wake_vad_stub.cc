#include "wake_vad.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.wake_vad";

class WakeVadStub : public WakeVad {
public:
    bool Init(int sample_rate_hz) override {
        sample_rate_hz_ = sample_rate_hz;
        state_ = WakeVadState::kListeningForWake;
        ESP_LOGI(TAG, "Init stub @ %d Hz (no SR model loaded)", sample_rate_hz_);
        return true;
    }

    void Deinit() override {
        state_ = WakeVadState::kIdle;
        running_ = false;
    }

    void Start() override {
        running_ = true;
        state_ = WakeVadState::kListeningForWake;
    }

    void Stop() override {
        running_ = false;
        state_ = WakeVadState::kIdle;
    }

    void Feed(const std::vector<int16_t>& pcm) override {
        (void)pcm;
        if (!running_) {
            return;
        }
        // Stub: does not detect wake words or VAD edges.
        // Integrate ESP-SR / AFE wake word here.
    }

    void OnWakeWord(WakeCallback callback) override { on_wake_ = std::move(callback); }
    void OnVadStateChange(VadCallback callback) override { on_vad_ = std::move(callback); }

    WakeVadState state() const override { return state_; }
    const std::string& last_wake_word() const override { return last_wake_word_; }

private:
    int sample_rate_hz_ = 16000;
    bool running_ = false;
    WakeVadState state_ = WakeVadState::kIdle;
    std::string last_wake_word_;
    WakeCallback on_wake_;
    VadCallback on_vad_;
};

}  // namespace

WakeVad* CreateWakeVad() {
    return new WakeVadStub();
}
