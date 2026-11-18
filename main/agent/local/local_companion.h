#ifndef AGENT_LOCAL_COMPANION_H
#define AGENT_LOCAL_COMPANION_H

#include <functional>
#include <memory>
#include <string>

#include "audio_stream_pipeline.h"

/**
 * Off-device local brain companion (e.g. Jetson Orin Nano 8GB).
 *
 * Heavy models do NOT run on the ESP32-S3. The S3 streams audio/text to this
 * companion over LAN (WebSocket/HTTP). Suggested companion stack for ~1s e2e:
 *   - ASR: Whisper small (streaming)
 *   - LLM: ~3B Qwen (token streaming)
 *   - TTS: Piper (chunked synthesis)
 *
 * Stages overlap: ASR partials/finals → LLM tokens → TTS audio while speaking.
 */
struct LocalCompanionConfig {
    std::string companion_url = "ws://192.168.1.50:8080/agent";
    std::string asr_model = "whisper-small";
    std::string llm_model = "qwen-3b";
    std::string tts_model = "piper";
    std::string voice_id = "default";
};

class LocalCompanionClient {
public:
    using AsrCallback = std::function<void(const std::string& text, bool is_final)>;
    using LlmCallback = std::function<void(const std::string& token, bool is_final)>;
    using TtsCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;
    using ErrorCallback = std::function<void(const std::string& message)>;

    virtual ~LocalCompanionClient() = default;

    virtual bool Init(const LocalCompanionConfig& config) = 0;
    virtual void Deinit() = 0;
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    virtual bool StreamAudio(std::unique_ptr<AudioChunk> chunk) = 0;
    virtual bool EndOfSpeech() = 0;
    virtual bool SendText(const std::string& text) = 0;
    virtual void Abort() = 0;

    virtual void OnAsr(AsrCallback callback) = 0;
    virtual void OnLlm(LlmCallback callback) = 0;
    virtual void OnTts(TtsCallback callback) = 0;
    virtual void OnError(ErrorCallback callback) = 0;
};

LocalCompanionClient* CreateLocalCompanionClient();

#endif  // AGENT_LOCAL_COMPANION_H
