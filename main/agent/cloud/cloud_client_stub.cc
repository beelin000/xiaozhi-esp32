#include "cloud_client.h"

#include <memory>

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.cloud.client";

class CloudClientStub : public CloudClient {
public:
    bool Init(const CloudClientConfig& config) override {
        config_ = config;
        if (config_.prefer_websocket) {
            transport_.reset(CreateWebSocketTransport());
        } else {
            transport_.reset(CreateHttpTransport());
        }
        ESP_LOGI(TAG, "Init stub (prefer_ws=%d)", config_.prefer_websocket ? 1 : 0);
        return transport_ != nullptr;
    }

    void Deinit() override {
        Disconnect();
        transport_.reset();
    }

    bool Connect() override {
        if (!transport_) {
            return false;
        }
        const std::string& url =
            config_.prefer_websocket ? config_.websocket_url : config_.http_base_url;
        return transport_->Connect(url.empty() ? "wss://example.invalid/agent" : url);
    }

    void Disconnect() override {
        if (transport_) {
            transport_->Disconnect();
        }
    }

    bool IsConnected() const override {
        return transport_ && transport_->IsConnected();
    }

    bool SendAudio(std::unique_ptr<AudioChunk> chunk) override {
        if (!IsConnected() || !chunk) {
            return false;
        }
        ESP_LOGD(TAG, "SendAudio stub: %u bytes", static_cast<unsigned>(chunk->payload.size()));
        return transport_->SendBinary(chunk->payload);
    }

    bool SendLlmPrompt(const std::string& prompt) override {
        if (!IsConnected()) {
            return false;
        }
        ESP_LOGI(TAG, "SendLlmPrompt stub (%u chars)", static_cast<unsigned>(prompt.size()));
        return transport_->SendText(prompt);
    }

    bool RequestTts(const std::string& text, const std::string& voice_id) override {
        if (!IsConnected()) {
            return false;
        }
        ESP_LOGI(TAG, "RequestTts stub voice=%s text_len=%u", voice_id.c_str(),
                 static_cast<unsigned>(text.size()));
        return transport_->SendText(text);
    }

    bool RequestSearch(const std::string& query) override {
        if (!IsConnected()) {
            return false;
        }
        ESP_LOGI(TAG, "RequestSearch stub: %s", query.c_str());
        return transport_->SendText(query);
    }

    void OnAsrResult(AsrResultCallback callback) override { on_asr_ = std::move(callback); }
    void OnLlmToken(LlmTokenCallback callback) override { on_llm_ = std::move(callback); }
    void OnTtsAudio(TtsAudioCallback callback) override { on_tts_ = std::move(callback); }
    void OnError(ErrorCallback callback) override { on_error_ = std::move(callback); }

private:
    CloudClientConfig config_;
    std::unique_ptr<CloudTransport> transport_;
    AsrResultCallback on_asr_;
    LlmTokenCallback on_llm_;
    TtsAudioCallback on_tts_;
    ErrorCallback on_error_;
};

}  // namespace

CloudClient* CreateCloudClient() {
    return new CloudClientStub();
}
