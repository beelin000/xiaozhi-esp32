#include "agent_skeleton.h"

#include <sdkconfig.h>
#include <esp_log.h>

#include "peripheral_registry.h"

namespace {
constexpr const char* TAG = "agent.skeleton";
}

AgentSkeleton& AgentSkeleton::GetInstance() {
    static AgentSkeleton instance;
    return instance;
}

bool AgentSkeleton::Start() {
    if (running_) {
        return true;
    }

    ESP_LOGI(TAG, "Starting agent skeleton (ESP32-S3 AI agent stubs)");

    audio_io_.reset(CreateAudioIoHal());
    microphone_.reset(CreateMicrophoneHal());
    camera_.reset(CreateCameraHal());
    wake_vad_.reset(CreateWakeVad());
    pipeline_.reset(CreateAudioStreamPipeline());
    cloud_.reset(CreateCloudClient());
    voice_config_.reset(CreateCustomVoiceConfig());

    if (!audio_io_ || !microphone_ || !camera_ || !wake_vad_ || !pipeline_ || !cloud_ ||
        !voice_config_) {
        ESP_LOGE(TAG, "Failed to allocate skeleton modules");
        Stop();
        return false;
    }

    constexpr int kSampleRate = 16000;
    constexpr int kChannels = 1;
    constexpr int kFrameMs = 60;

    if (!audio_io_->Init(kSampleRate, kChannels) || !microphone_->Init(kSampleRate, kChannels) ||
        !camera_->Init() || !wake_vad_->Init(kSampleRate) ||
        !pipeline_->Init(kSampleRate, kFrameMs)) {
        ESP_LOGE(TAG, "Module Init failed");
        Stop();
        return false;
    }

    voice_config_->Load();

    CloudClientConfig cloud_cfg;
    cloud_cfg.prefer_websocket = true;
    cloud_cfg.websocket_url = CONFIG_AGENT_SKELETON_WS_URL;
    cloud_cfg.http_base_url = CONFIG_AGENT_SKELETON_HTTP_URL;
    if (!cloud_->Init(cloud_cfg)) {
        ESP_LOGE(TAG, "CloudClient Init failed");
        Stop();
        return false;
    }

    // Wire a minimal uplink: pipeline chunks → cloud (stubs no-op network).
    pipeline_->OnUplinkReady([this](std::unique_ptr<AudioChunk> chunk) {
        if (cloud_) {
            cloud_->SendAudio(std::move(chunk));
        }
    });

    PeripheralRegistry::GetInstance().InitAll();

    audio_io_->Start();
    microphone_->Start();
    wake_vad_->Start();
    pipeline_->Start();
    cloud_->Connect();

    running_ = true;
    ESP_LOGI(TAG, "Agent skeleton running (stubs); camera available=%d",
             camera_->IsAvailable() ? 1 : 0);
    return true;
}

void AgentSkeleton::Stop() {
    if (cloud_) {
        cloud_->Disconnect();
        cloud_->Deinit();
    }
    if (pipeline_) {
        pipeline_->Stop();
        pipeline_->Deinit();
    }
    if (wake_vad_) {
        wake_vad_->Stop();
        wake_vad_->Deinit();
    }
    if (microphone_) {
        microphone_->Stop();
        microphone_->Deinit();
    }
    if (audio_io_) {
        audio_io_->Stop();
        audio_io_->Deinit();
    }
    if (camera_) {
        camera_->Deinit();
    }
    if (voice_config_) {
        voice_config_->Save();
    }

    PeripheralRegistry::GetInstance().DeinitAll();

    cloud_.reset();
    pipeline_.reset();
    wake_vad_.reset();
    microphone_.reset();
    audio_io_.reset();
    camera_.reset();
    voice_config_.reset();

    running_ = false;
    ESP_LOGI(TAG, "Agent skeleton stopped");
}
