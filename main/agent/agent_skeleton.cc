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

BrainBackendConfig AgentSkeleton::BuildBrainConfig() const {
    BrainBackendConfig cfg;
#if defined(CONFIG_AGENT_SKELETON_BRAIN_LOCAL)
    cfg.kind = BrainBackendKind::kLocal;
    cfg.endpoint_url = CONFIG_AGENT_SKELETON_LOCAL_COMPANION_URL;
    cfg.asr_model = CONFIG_AGENT_SKELETON_LOCAL_ASR_MODEL;
    cfg.llm_model = CONFIG_AGENT_SKELETON_LOCAL_LLM_MODEL;
    cfg.tts_model = CONFIG_AGENT_SKELETON_LOCAL_TTS_MODEL;
#else
    cfg.kind = BrainBackendKind::kCloud;
    cfg.endpoint_url = CONFIG_AGENT_SKELETON_WS_URL;
#endif
    if (voice_config_) {
        cfg.voice_id = voice_config_->GetProfile().voice_id;
    }
    return cfg;
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
    voice_session_.reset(new StreamingVoiceSession());

    if (!audio_io_ || !microphone_ || !camera_ || !wake_vad_ || !pipeline_ || !cloud_ ||
        !voice_config_ || !voice_session_) {
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

    // Keep CloudClient available for direct use / aggregation helpers.
    CloudClientConfig cloud_cfg;
    cloud_cfg.prefer_websocket = true;
    cloud_cfg.websocket_url = CONFIG_AGENT_SKELETON_WS_URL;
    cloud_cfg.http_base_url = CONFIG_AGENT_SKELETON_HTTP_URL;
    if (!cloud_->Init(cloud_cfg)) {
        ESP_LOGE(TAG, "CloudClient Init failed");
        Stop();
        return false;
    }

    const BrainBackendConfig brain_cfg = BuildBrainConfig();
    if (!voice_session_->Init(brain_cfg, pipeline_.get(), audio_io_.get())) {
        ESP_LOGE(TAG, "StreamingVoiceSession Init failed");
        Stop();
        return false;
    }

    // VAD end → close ASR window so LLM/TTS can overlap on the backend.
    wake_vad_->OnVadStateChange([this](bool speaking) {
        if (!speaking && voice_session_) {
            voice_session_->NotifyUtteranceEnd();
        }
    });

    PeripheralRegistry::GetInstance().InitAll();

    audio_io_->Start();
    microphone_->Start();
    wake_vad_->Start();
    pipeline_->Start();

    if (!voice_session_->Start()) {
        ESP_LOGE(TAG, "StreamingVoiceSession Start failed");
        Stop();
        return false;
    }

    // CloudClient connect only when cloud brain is selected (session already connected).
    if (brain_cfg.kind == BrainBackendKind::kCloud) {
        cloud_->Connect();
    }

    running_ = true;
    ESP_LOGI(TAG, "Agent skeleton running; brain=%s camera=%d",
             brain_cfg.kind == BrainBackendKind::kLocal ? "local" : "cloud",
             camera_->IsAvailable() ? 1 : 0);
    return true;
}

void AgentSkeleton::Stop() {
    if (voice_session_) {
        voice_session_->Deinit();
    }
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

    voice_session_.reset();
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
