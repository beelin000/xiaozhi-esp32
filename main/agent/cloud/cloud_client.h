#ifndef AGENT_CLOUD_CLIENT_H
#define AGENT_CLOUD_CLIENT_H

#include <functional>
#include <memory>
#include <string>

#include "audio_stream_pipeline.h"
#include "cloud_transport.h"

/**
 * Cloud communication façade for ASR / LLM / TTS and data aggregation.
 * Production counterpart: protocols/protocol.h (+ websocket / mqtt).
 */
enum class CloudServiceKind {
    kAsr,
    kLlm,
    kTts,
    kSearch,
    kAggregate,
};

struct CloudClientConfig {
    std::string websocket_url;
    std::string http_base_url;
    std::string auth_token;
    std::string session_id;
    bool prefer_websocket = true;
};

class CloudClient {
public:
    using AsrResultCallback = std::function<void(const std::string& text, bool is_final)>;
    using LlmTokenCallback = std::function<void(const std::string& token, bool is_final)>;
    using TtsAudioCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;
    using ErrorCallback = std::function<void(const std::string& message)>;

    virtual ~CloudClient() = default;

    virtual bool Init(const CloudClientConfig& config) = 0;
    virtual void Deinit() = 0;
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    /** Stream uplink audio toward cloud ASR. */
    virtual bool SendAudio(std::unique_ptr<AudioChunk> chunk) = 0;

    /** Request LLM completion (text prompt; multimodal hooks later). */
    virtual bool SendLlmPrompt(const std::string& prompt) = 0;

    /** Request TTS for text (uses custom voice config when set). */
    virtual bool RequestTts(const std::string& text, const std::string& voice_id) = 0;

    /** Optional search / aggregation helpers for the "cloud brain". */
    virtual bool RequestSearch(const std::string& query) = 0;

    virtual void OnAsrResult(AsrResultCallback callback) = 0;
    virtual void OnLlmToken(LlmTokenCallback callback) = 0;
    virtual void OnTtsAudio(TtsAudioCallback callback) = 0;
    virtual void OnError(ErrorCallback callback) = 0;
};

CloudClient* CreateCloudClient();

#endif  // AGENT_CLOUD_CLIENT_H
