#ifndef AGENT_BRAIN_BACKEND_H
#define AGENT_BRAIN_BACKEND_H

#include <functional>
#include <memory>
#include <string>

#include "audio_stream_pipeline.h"

/**
 * Single swappable interface for the "brain" that runs ASR / LLM / TTS.
 *
 * Backends:
 *   - Cloud (default): remote ASR/LLM/TTS via CloudClient
 *   - Local: off-device companion (e.g. Jetson Orin Nano) running Whisper /
 *     Qwen / Piper — NOT on the ESP32-S3 itself
 *
 * Streaming contract (overlapping stages):
 *   1. PushAudioChunk while the user speaks → partial/final ASR text
 *   2. ASR finals (and optionally partials) feed LLM → streaming tokens
 *   3. LLM tokens feed TTS → audio chunks for playback as they arrive
 */
enum class BrainBackendKind {
    kCloud,
    kLocal,
};

struct BrainBackendConfig {
    BrainBackendKind kind = BrainBackendKind::kCloud;
    std::string endpoint_url;   // cloud WS/HTTP or local companion URL
    std::string auth_token;
    std::string session_id;
    std::string voice_id;       // TTS voice / Piper voice name
    std::string asr_model;      // e.g. "whisper-small" (local hint)
    std::string llm_model;      // e.g. "qwen-3b" (local hint)
    std::string tts_model;      // e.g. "piper" (local hint)
    bool stream_partial_asr_to_llm = false;  // usually wait for final
};

class BrainBackend {
public:
    using AsrPartialCallback = std::function<void(const std::string& text, bool is_final)>;
    using LlmTokenCallback = std::function<void(const std::string& token, bool is_final)>;
    using TtsAudioCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;
    using ErrorCallback = std::function<void(const std::string& message)>;

    virtual ~BrainBackend() = default;

    virtual BrainBackendKind kind() const = 0;

    virtual bool Init(const BrainBackendConfig& config) = 0;
    virtual void Deinit() = 0;
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    /** Stage 1: stream mic audio toward ASR (while user speaks). */
    virtual bool PushAudioChunk(std::unique_ptr<AudioChunk> chunk) = 0;

    /** Signal end of user utterance (VAD end / button release). */
    virtual bool EndUtterance() = 0;

    /** Optional explicit LLM kick with a text prompt (bypasses ASR). */
    virtual bool PushTextPrompt(const std::string& text) = 0;

    /** Abort in-flight ASR/LLM/TTS for barge-in. */
    virtual void Abort() = 0;

    virtual void OnAsrPartial(AsrPartialCallback callback) = 0;
    virtual void OnLlmToken(LlmTokenCallback callback) = 0;
    virtual void OnTtsAudio(TtsAudioCallback callback) = 0;
    virtual void OnError(ErrorCallback callback) = 0;
};

/** Factory: selects cloud (default) or local companion from config.kind. */
BrainBackend* CreateBrainBackend(BrainBackendKind kind);

#endif  // AGENT_BRAIN_BACKEND_H
