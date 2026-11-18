#include "brain_backend.h"

#include <memory>

#include <esp_log.h>

#include "local_companion.h"

namespace {

constexpr const char* TAG = "agent.brain.local";

/**
 * Local BrainBackend — talks to an off-device companion (Jetson Orin Nano).
 * ESP32-S3 only streams; Whisper / Qwen / Piper run on the companion.
 */
class LocalBrainBackend : public BrainBackend {
public:
    BrainBackendKind kind() const override { return BrainBackendKind::kLocal; }

    bool Init(const BrainBackendConfig& config) override {
        config_ = config;
        companion_.reset(CreateLocalCompanionClient());
        if (!companion_) {
            return false;
        }

        LocalCompanionConfig lc;
        lc.companion_url =
            config_.endpoint_url.empty() ? "ws://192.168.1.50:8080/agent" : config_.endpoint_url;
        lc.asr_model = config_.asr_model.empty() ? "whisper-small" : config_.asr_model;
        lc.llm_model = config_.llm_model.empty() ? "qwen-3b" : config_.llm_model;
        lc.tts_model = config_.tts_model.empty() ? "piper" : config_.tts_model;
        lc.voice_id = config_.voice_id.empty() ? "default" : config_.voice_id;

        if (!companion_->Init(lc)) {
            companion_.reset();
            return false;
        }

        companion_->OnAsr([this](const std::string& text, bool is_final) {
            if (on_asr_) {
                on_asr_(text, is_final);
            }
        });
        companion_->OnLlm([this](const std::string& token, bool is_final) {
            if (on_llm_) {
                on_llm_(token, is_final);
            }
        });
        companion_->OnTts([this](std::unique_ptr<AudioChunk> chunk) {
            if (on_tts_) {
                on_tts_(std::move(chunk));
            }
        });
        companion_->OnError([this](const std::string& message) {
            if (on_error_) {
                on_error_(message);
            }
        });

        ESP_LOGI(TAG, "Local brain backend ready (companion off-device)");
        return true;
    }

    void Deinit() override {
        Disconnect();
        if (companion_) {
            companion_->Deinit();
            companion_.reset();
        }
    }

    bool Connect() override { return companion_ && companion_->Connect(); }

    void Disconnect() override {
        if (companion_) {
            companion_->Disconnect();
        }
    }

    bool IsConnected() const override {
        return companion_ && companion_->IsConnected();
    }

    bool PushAudioChunk(std::unique_ptr<AudioChunk> chunk) override {
        return companion_ && companion_->StreamAudio(std::move(chunk));
    }

    bool EndUtterance() override {
        return companion_ && companion_->EndOfSpeech();
    }

    bool PushTextPrompt(const std::string& text) override {
        return companion_ && companion_->SendText(text);
    }

    void Abort() override {
        if (companion_) {
            companion_->Abort();
        }
    }

    void OnAsrPartial(AsrPartialCallback callback) override { on_asr_ = std::move(callback); }
    void OnLlmToken(LlmTokenCallback callback) override { on_llm_ = std::move(callback); }
    void OnTtsAudio(TtsAudioCallback callback) override { on_tts_ = std::move(callback); }
    void OnError(ErrorCallback callback) override { on_error_ = std::move(callback); }

private:
    BrainBackendConfig config_;
    std::unique_ptr<LocalCompanionClient> companion_;
    AsrPartialCallback on_asr_;
    LlmTokenCallback on_llm_;
    TtsAudioCallback on_tts_;
    ErrorCallback on_error_;
};

}  // namespace

BrainBackend* CreateLocalBrainBackend() {
    return new LocalBrainBackend();
}
