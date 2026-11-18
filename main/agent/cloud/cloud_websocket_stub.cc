#include "cloud_transport.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.cloud.ws";

class WebSocketTransportStub : public CloudTransport {
public:
    CloudTransportType type() const override { return CloudTransportType::kWebSocket; }

    bool Connect(const std::string& url) override {
        url_ = url;
        connected_ = true;
        ESP_LOGI(TAG, "Connect stub → %s", url_.c_str());
        if (on_state_) {
            on_state_(true);
        }
        return true;
    }

    void Disconnect() override {
        connected_ = false;
        ESP_LOGI(TAG, "Disconnect stub");
        if (on_state_) {
            on_state_(false);
        }
    }

    bool IsConnected() const override { return connected_; }

    bool SendText(const std::string& text) override {
        if (!connected_) {
            return false;
        }
        ESP_LOGD(TAG, "SendText stub (%u bytes)", static_cast<unsigned>(text.size()));
        return true;
    }

    bool SendBinary(const uint8_t* data, size_t len) override {
        (void)data;
        if (!connected_) {
            return false;
        }
        ESP_LOGD(TAG, "SendBinary stub (%u bytes)", static_cast<unsigned>(len));
        return true;
    }

    void OnText(TextCallback callback) override { on_text_ = std::move(callback); }
    void OnBinary(BinaryCallback callback) override { on_binary_ = std::move(callback); }
    void OnConnectionState(StateCallback callback) override { on_state_ = std::move(callback); }

private:
    std::string url_;
    bool connected_ = false;
    TextCallback on_text_;
    BinaryCallback on_binary_;
    StateCallback on_state_;
};

}  // namespace

CloudTransport* CreateWebSocketTransport() {
    return new WebSocketTransportStub();
}
