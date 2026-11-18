#include "audio_io.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.hal.audio_io";

class AudioIoHalStub : public AudioIoHal {
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

    bool WritePcm(const int16_t* samples, size_t sample_count) override {
        (void)samples;
        if (!running_) {
            return false;
        }
        // Discard PCM in stub; real HAL writes to I2S / codec.
        ESP_LOGD(TAG, "WritePcm stub: %u samples", static_cast<unsigned>(sample_count));
        return true;
    }

    void SetVolume(int volume_percent) override {
        if (volume_percent < 0) {
            volume_percent = 0;
        } else if (volume_percent > 100) {
            volume_percent = 100;
        }
        volume_ = volume_percent;
    }

    int GetVolume() const override { return volume_; }
    int sample_rate_hz() const override { return sample_rate_hz_; }
    int channels() const override { return channels_; }
    bool is_running() const override { return running_; }

private:
    int sample_rate_hz_ = 16000;
    int channels_ = 1;
    int volume_ = 70;
    bool running_ = false;
};

}  // namespace

AudioIoHal* CreateAudioIoHal() {
    return new AudioIoHalStub();
}
