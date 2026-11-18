#ifndef AGENT_CLOUD_TRANSPORT_H
#define AGENT_CLOUD_TRANSPORT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * Low-level cloud transport (WebSocket or HTTP).
 * Higher-level ASR/LLM/TTS APIs sit on CloudClient.
 */
enum class CloudTransportType {
    kWebSocket,
    kHttp,
};

class CloudTransport {
public:
    using BinaryCallback = std::function<void(const uint8_t* data, size_t len)>;
    using TextCallback = std::function<void(const std::string& text)>;
    using StateCallback = std::function<void(bool connected)>;

    virtual ~CloudTransport() = default;

    virtual CloudTransportType type() const = 0;
    virtual bool Connect(const std::string& url) = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual bool SendText(const std::string& text) = 0;
    virtual bool SendBinary(const uint8_t* data, size_t len) = 0;
    virtual bool SendBinary(const std::vector<uint8_t>& data) {
        return SendBinary(data.data(), data.size());
    }

    virtual void OnText(TextCallback callback) = 0;
    virtual void OnBinary(BinaryCallback callback) = 0;
    virtual void OnConnectionState(StateCallback callback) = 0;
};

CloudTransport* CreateWebSocketTransport();
CloudTransport* CreateHttpTransport();

#endif  // AGENT_CLOUD_TRANSPORT_H
