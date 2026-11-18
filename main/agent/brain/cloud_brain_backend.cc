#include "brain_backend.h"

#include <memory>

#include <esp_log.h>

#include "cloud_client.h"

namespace {

constexpr const char* TAG = "agent.brain.cloud";

/**
 * Cloud BrainBackend adapter — default path.
 * Wraps CloudClient; overlapping ASR→LLM→TTS is expected from the server.
 */
class CloudBrainBackend : public BrainBackend {
public:
    BrainBackendKind kind() const override { return BrainBackendKind::kCloud; }

    bool Init(const BrainBackendConfig& config) override {
        config_ = config;
        client_.reset(CreateCloudClient());
        if (!client_) {
            return false;
        }

        CloudClientConfig cc;
        cc.prefer_websocket = true;
        cc.websocket_url = config_.endpoint_url;
        cc.http_base_url = config_.endpoint_url;
        cc.auth_token = config_.auth_token;
        cc.session_id = config_.session_id;

        if (!client_->Init(cc)) {
            client_.reset();
            return false;
        }

        client_->OnAsrResult([this](const std::string& text, bool is_final) {
            if (on_asr_) {
                on_asr_(text, is_final);
            }
        });
        client_->OnLlmToken([this](const std::string& token, bool is_final) {
            if (on_llm_) {
                on_llm_(token, is_final);
            }
        });
        client_->OnTtsAudio([this](std::unique_ptr<AudioChunk> chunk) {
            if (on_tts_) {
                on_tts_(std::move(chunk));
            }
        });
        client_->OnError([this](const std::string& message) {
            if (on_error_) {
                on_error_(message);
            }
        });

        ESP_LOGI(TAG, "Cloud brain backend ready");
        return true;
    }

    void Deinit() override {
        Disconnect();
        if (client_) {
            client_->Deinit();
            client_.reset();
        }
    }

    bool Connect() override { return client_ && client_->Connect(); }

    void Disconnect() override {
        if (client_) {
            client_->Disconnect();
        }
    }

    bool IsConnected() const override { return client_ && client_->IsConnected(); }

    bool PushAudioChunk(std::unique_ptr<AudioChunk> chunk) override {
        return client_ && client_->SendAudio(std::move(chunk));
    }

    bool EndUtterance() override {
        // Cloud protocol typically uses listen stop / VAD; stub no-ops.
        ESP_LOGD(TAG, "EndUtterance stub");
        return IsConnected();
    }

    bool PushTextPrompt(const std::string& text) override {
        return client_ && client_->SendLlmPrompt(text);
    }

    void Abort() override {
        ESP_LOGD(TAG, "Abort stub");
    }

    void OnAsrPartial(AsrPartialCallback callback) override { on_asr_ = std::move(callback); }
    void OnLlmToken(LlmTokenCallback callback) override { on_llm_ = std::move(callback); }
    void OnTtsAudio(TtsAudioCallback callback) override { on_tts_ = std::move(callback); }
    void OnError(ErrorCallback callback) override { on_error_ = std::move(callback); }

private:
    BrainBackendConfig config_;
    std::unique_ptr<CloudClient> client_;
    AsrPartialCallback on_asr_;
    LlmTokenCallback on_llm_;
    TtsAudioCallback on_tts_;
    ErrorCallback on_error_;
};

}  // namespace

BrainBackend* CreateCloudBrainBackend() {
    return new CloudBrainBackend();
}
