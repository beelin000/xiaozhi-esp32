#ifndef AGENT_SKELETON_H
#define AGENT_SKELETON_H

#include <memory>

#include "cloud/cloud_client.h"
#include "hal/audio_io.h"
#include "hal/camera_hal.h"
#include "hal/microphone.h"
#include "pipeline/audio_stream_pipeline.h"
#include "voice/custom_voice_config.h"
#include "wake/wake_vad.h"

/**
 * Optional composition root for the agent skeleton modules.
 * Does not replace Application; call AgentSkeleton::Start() when
 * CONFIG_AGENT_SKELETON_ENABLE is set (e.g. from a board bring-up hook).
 */
class AgentSkeleton {
public:
    static AgentSkeleton& GetInstance();

    bool Start();
    void Stop();
    bool is_running() const { return running_; }

    AudioIoHal* audio_io() { return audio_io_.get(); }
    MicrophoneHal* microphone() { return microphone_.get(); }
    CameraHal* camera() { return camera_.get(); }
    WakeVad* wake_vad() { return wake_vad_.get(); }
    AudioStreamPipeline* pipeline() { return pipeline_.get(); }
    CloudClient* cloud() { return cloud_.get(); }
    CustomVoiceConfig* voice_config() { return voice_config_.get(); }

private:
    AgentSkeleton() = default;

    bool running_ = false;
    std::unique_ptr<AudioIoHal> audio_io_;
    std::unique_ptr<MicrophoneHal> microphone_;
    std::unique_ptr<CameraHal> camera_;
    std::unique_ptr<WakeVad> wake_vad_;
    std::unique_ptr<AudioStreamPipeline> pipeline_;
    std::unique_ptr<CloudClient> cloud_;
    std::unique_ptr<CustomVoiceConfig> voice_config_;
};

#endif  // AGENT_SKELETON_H
