#include "audio_stream_pipeline.h"

#include <cstring>

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.pipeline";

class AudioStreamPipelineStub : public AudioStreamPipeline {
public:
    bool Init(int sample_rate_hz, int frame_duration_ms) override {
        sample_rate_hz_ = sample_rate_hz;
        frame_duration_ms_ = frame_duration_ms;
        ESP_LOGI(TAG, "Init stub: %d Hz, %d ms frames", sample_rate_hz_, frame_duration_ms_);
        return true;
    }

    void Deinit() override {
        running_ = false;
        on_uplink_ = nullptr;
        on_downlink_ = nullptr;
    }

    bool Start() override {
        running_ = true;
        return true;
    }

    void Stop() override { running_ = false; }

    void PushCapturePcm(const std::vector<int16_t>& pcm) override {
        if (!running_ || !on_uplink_ || pcm.empty()) {
            return;
        }
        auto chunk = std::make_unique<AudioChunk>();
        chunk->sample_rate_hz = sample_rate_hz_;
        chunk->frame_duration_ms = frame_duration_ms_;
        // Stub packs raw PCM bytes; real pipeline would Opus-encode.
        chunk->payload.resize(pcm.size() * sizeof(int16_t));
        std::memcpy(chunk->payload.data(), pcm.data(), chunk->payload.size());
        on_uplink_(std::move(chunk));
    }

    void PushDownlink(std::unique_ptr<AudioChunk> chunk) override {
        if (!running_ || !on_downlink_ || !chunk) {
            return;
        }
        on_downlink_(std::move(chunk));
    }

    void OnUplinkReady(UplinkCallback callback) override { on_uplink_ = std::move(callback); }
    void OnDownlinkReady(DownlinkCallback callback) override {
        on_downlink_ = std::move(callback);
    }

    bool is_running() const override { return running_; }

private:
    int sample_rate_hz_ = 16000;
    int frame_duration_ms_ = 60;
    bool running_ = false;
    UplinkCallback on_uplink_;
    DownlinkCallback on_downlink_;
};

}  // namespace

AudioStreamPipeline* CreateAudioStreamPipeline() {
    return new AudioStreamPipelineStub();
}
