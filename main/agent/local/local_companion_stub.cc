#include "local_companion.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.local.companion";

class LocalCompanionClientStub : public LocalCompanionClient {
public:
    bool Init(const LocalCompanionConfig& config) override {
        config_ = config;
        ESP_LOGI(TAG,
                 "Init stub companion=%s asr=%s llm=%s tts=%s "
                 "(models run on Jetson companion, not ESP32-S3)",
                 config_.companion_url.c_str(), config_.asr_model.c_str(),
                 config_.llm_model.c_str(), config_.tts_model.c_str());
        return true;
    }

    void Deinit() override {
        Disconnect();
        ESP_LOGI(TAG, "Deinit stub");
    }

    bool Connect() override {
        connected_ = true;
        ESP_LOGI(TAG, "Connect stub → %s", config_.companion_url.c_str());
        return true;
    }

    void Disconnect() override {
        connected_ = false;
        ESP_LOGI(TAG, "Disconnect stub");
    }

    bool IsConnected() const override { return connected_; }

    bool StreamAudio(std::unique_ptr<AudioChunk> chunk) override {
        if (!connected_ || !chunk) {
            return false;
        }
        // Would stream PCM/Opus to companion ASR (Whisper small) over WS.
        ESP_LOGD(TAG, "StreamAudio stub: %u bytes → ASR stage",
                 static_cast<unsigned>(chunk->payload.size()));
        return true;
    }

    bool EndOfSpeech() override {
        if (!connected_) {
            return false;
        }
        // Companion should finalize ASR, kick LLM, then stream TTS.
        ESP_LOGI(TAG, "EndOfSpeech stub → finalize ASR → LLM → TTS (overlapped)");
        return true;
    }

    bool SendText(const std::string& text) override {
        if (!connected_) {
            return false;
        }
        ESP_LOGI(TAG, "SendText stub (%u chars) → LLM stage",
                 static_cast<unsigned>(text.size()));
        return true;
    }

    void Abort() override {
        ESP_LOGD(TAG, "Abort stub (barge-in)");
    }

    void OnAsr(AsrCallback callback) override { on_asr_ = std::move(callback); }
    void OnLlm(LlmCallback callback) override { on_llm_ = std::move(callback); }
    void OnTts(TtsCallback callback) override { on_tts_ = std::move(callback); }
    void OnError(ErrorCallback callback) override { on_error_ = std::move(callback); }

private:
    LocalCompanionConfig config_;
    bool connected_ = false;
    AsrCallback on_asr_;
    LlmCallback on_llm_;
    TtsCallback on_tts_;
    ErrorCallback on_error_;
};

}  // namespace

LocalCompanionClient* CreateLocalCompanionClient() {
    return new LocalCompanionClientStub();
}
