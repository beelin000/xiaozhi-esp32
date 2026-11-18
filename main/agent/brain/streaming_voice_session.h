#ifndef AGENT_BRAIN_STREAMING_VOICE_SESSION_H
#define AGENT_BRAIN_STREAMING_VOICE_SESSION_H

#include <functional>
#include <memory>

#include "audio_io.h"
#include "brain_backend.h"
#include "audio_stream_pipeline.h"

/**
 * Device-side orchestrator for overlapping ASR → LLM → TTS.
 *
 * Owns the BrainBackend selection and wires:
 *   AudioStreamPipeline uplink → BrainBackend::PushAudioChunk
 *   BrainBackend TTS downlink  → AudioIoHal playback
 *
 * Latency target (documented, not enforced in stubs): sub-second to ~1s
 * e2e when the local backend runs Whisper-small + ~3B Qwen + Piper on a
 * Jetson Orin Nano 8GB-class companion.
 */
class StreamingVoiceSession {
public:
    using StateCallback = std::function<void(const char* state)>;

    StreamingVoiceSession() = default;
    ~StreamingVoiceSession() = default;

    bool Init(const BrainBackendConfig& config,
              AudioStreamPipeline* device_pipeline,
              AudioIoHal* audio_out);
    void Deinit();

    bool Start();
    void Stop();
    bool is_running() const { return running_; }

    BrainBackend* backend() { return backend_.get(); }
    BrainBackendKind backend_kind() const {
        return backend_ ? backend_->kind() : BrainBackendKind::kCloud;
    }

    /** Forward a captured chunk (also used if pipeline callback is bypassed). */
    void FeedAudio(std::unique_ptr<AudioChunk> chunk);

    /** Call on VAD end / push-to-talk release to close the ASR window. */
    void NotifyUtteranceEnd();

    void OnState(StateCallback callback);

private:
    void WireCallbacks();

    bool running_ = false;
    AudioStreamPipeline* device_pipeline_ = nullptr;  // not owned
    AudioIoHal* audio_out_ = nullptr;                 // not owned
    std::unique_ptr<BrainBackend> backend_;
    StateCallback on_state_;
};

#endif  // AGENT_BRAIN_STREAMING_VOICE_SESSION_H
