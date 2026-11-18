#ifndef AGENT_HAL_MICROPHONE_H
#define AGENT_HAL_MICROPHONE_H

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * Hardware abstraction for microphone capture.
 * Stub today; adapters can wrap AudioCodec::InputData / Read.
 */
class MicrophoneHal {
public:
    virtual ~MicrophoneHal() = default;

    virtual bool Init(int sample_rate_hz, int channels) = 0;
    virtual void Deinit() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;

    /** Read up to max_samples into dest; returns samples actually read. */
    virtual size_t ReadPcm(int16_t* dest, size_t max_samples) = 0;
    virtual size_t ReadPcm(std::vector<int16_t>& dest) {
        if (dest.empty()) {
            return 0;
        }
        return ReadPcm(dest.data(), dest.size());
    }

    virtual void SetGain(float gain) = 0;
    virtual float GetGain() const = 0;
    virtual int sample_rate_hz() const = 0;
    virtual int channels() const = 0;
    virtual bool is_running() const = 0;
};

MicrophoneHal* CreateMicrophoneHal();

#endif  // AGENT_HAL_MICROPHONE_H
