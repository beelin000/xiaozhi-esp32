#include "microphone.h"

#include <cstring>

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.hal.mic";

class MicrophoneHalStub : public MicrophoneHal {
public:
    bool Init(int sample_rate_hz, int channels) override {
        sample_rate_hz_ = sample_rate_hz;
        channels_ = channels;
        ESP_LOGI(TAG, "Init stub: %d Hz, %d ch", sample_rate_hz_, channels_);
        return true;
    }

    void Deinit() override {
        running_ = false;
        ESP_LOGI(TAG, "Deinit stub");
    }

    bool Start() override {
        running_ = true;
        return true;
    }

    void Stop() override { running_ = false; }

    size_t ReadPcm(int16_t* dest, size_t max_samples) override {
        if (!running_ || dest == nullptr || max_samples == 0) {
            return 0;
        }
        // Silence frame: no hardware present.
        std::memset(dest, 0, max_samples * sizeof(int16_t));
        return max_samples;
    }

    void SetGain(float gain) override { gain_ = gain; }
    float GetGain() const override { return gain_; }
    int sample_rate_hz() const override { return sample_rate_hz_; }
    int channels() const override { return channels_; }
    bool is_running() const override { return running_; }

private:
    int sample_rate_hz_ = 16000;
    int channels_ = 1;
    float gain_ = 1.0f;
    bool running_ = false;
};

}  // namespace

MicrophoneHal* CreateMicrophoneHal() {
    return new MicrophoneHalStub();
}
