#ifndef AGENT_HAL_AUDIO_IO_H
#define AGENT_HAL_AUDIO_IO_H

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * Hardware abstraction for speaker / PCM audio output.
 * Stub today; adapters can wrap AudioCodec (main/audio/audio_codec.h).
 */
class AudioIoHal {
public:
    virtual ~AudioIoHal() = default;

    virtual bool Init(int sample_rate_hz, int channels) = 0;
    virtual void Deinit() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;

    virtual bool WritePcm(const int16_t* samples, size_t sample_count) = 0;
    virtual bool WritePcm(const std::vector<int16_t>& samples) {
        return WritePcm(samples.data(), samples.size());
    }

    virtual void SetVolume(int volume_percent) = 0;
    virtual int GetVolume() const = 0;
    virtual int sample_rate_hz() const = 0;
    virtual int channels() const = 0;
    virtual bool is_running() const = 0;
};

/** Factory for the default stub (or future board-backed) implementation. */
AudioIoHal* CreateAudioIoHal();

#endif  // AGENT_HAL_AUDIO_IO_H
