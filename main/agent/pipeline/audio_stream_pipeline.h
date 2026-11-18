#ifndef AGENT_PIPELINE_AUDIO_STREAM_PIPELINE_H
#define AGENT_PIPELINE_AUDIO_STREAM_PIPELINE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

/**
 * Streaming audio pipeline: mic PCM → (optional encode) → cloud uplink,
 * and cloud downlink → playback.
 * Production counterpart: audio/audio_service.h.
 */
struct AudioChunk {
    int sample_rate_hz = 16000;
    int frame_duration_ms = 60;
    uint32_t timestamp_ms = 0;
    std::vector<uint8_t> payload;  // PCM or Opus, depending on config
};

class AudioStreamPipeline {
public:
    using UplinkCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;
    using DownlinkCallback = std::function<void(std::unique_ptr<AudioChunk> chunk)>;

    virtual ~AudioStreamPipeline() = default;

    virtual bool Init(int sample_rate_hz, int frame_duration_ms) = 0;
    virtual void Deinit() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;

    /** Push captured PCM into the uplink path. */
    virtual void PushCapturePcm(const std::vector<int16_t>& pcm) = 0;

    /** Push a cloud TTS/ASR audio chunk into the downlink path. */
    virtual void PushDownlink(std::unique_ptr<AudioChunk> chunk) = 0;

    virtual void OnUplinkReady(UplinkCallback callback) = 0;
    virtual void OnDownlinkReady(DownlinkCallback callback) = 0;

    virtual bool is_running() const = 0;
};

AudioStreamPipeline* CreateAudioStreamPipeline();

#endif  // AGENT_PIPELINE_AUDIO_STREAM_PIPELINE_H
