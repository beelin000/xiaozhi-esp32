#include "streaming_voice_session.h"

#include <esp_log.h>

namespace {
constexpr const char* TAG = "agent.brain.session";
}

bool StreamingVoiceSession::Init(const BrainBackendConfig& config,
                                 AudioStreamPipeline* device_pipeline,
                                 AudioIoHal* audio_out) {
    device_pipeline_ = device_pipeline;
    audio_out_ = audio_out;

    backend_.reset(CreateBrainBackend(config.kind));
    if (!backend_) {
        ESP_LOGE(TAG, "CreateBrainBackend failed");
        return false;
    }
    if (!backend_->Init(config)) {
        ESP_LOGE(TAG, "BrainBackend Init failed (kind=%d)", static_cast<int>(config.kind));
        backend_.reset();
        return false;
    }

    WireCallbacks();
    ESP_LOGI(TAG, "Init ok; backend=%s",
             config.kind == BrainBackendKind::kLocal ? "local" : "cloud");
    return true;
}

void StreamingVoiceSession::Deinit() {
    Stop();
    if (backend_) {
        backend_->Deinit();
        backend_.reset();
    }
    device_pipeline_ = nullptr;
    audio_out_ = nullptr;
}

bool StreamingVoiceSession::Start() {
    if (!backend_ || running_) {
        return running_;
    }
    if (!backend_->Connect()) {
        ESP_LOGE(TAG, "BrainBackend Connect failed");
        return false;
    }

    if (device_pipeline_) {
        device_pipeline_->OnUplinkReady([this](std::unique_ptr<AudioChunk> chunk) {
            FeedAudio(std::move(chunk));
        });
    }

    running_ = true;
    if (on_state_) {
        on_state_("listening");
    }
    ESP_LOGI(TAG, "Streaming voice session started (overlapping ASR/LLM/TTS)");
    return true;
}

void StreamingVoiceSession::Stop() {
    if (!running_) {
        return;
    }
    if (backend_) {
        backend_->Abort();
        backend_->Disconnect();
    }
    running_ = false;
    if (on_state_) {
        on_state_("stopped");
    }
}

void StreamingVoiceSession::FeedAudio(std::unique_ptr<AudioChunk> chunk) {
    if (!running_ || !backend_ || !chunk) {
        return;
    }
    backend_->PushAudioChunk(std::move(chunk));
}

void StreamingVoiceSession::NotifyUtteranceEnd() {
    if (!running_ || !backend_) {
        return;
    }
    backend_->EndUtterance();
    if (on_state_) {
        on_state_("thinking");
    }
}

void StreamingVoiceSession::OnState(StateCallback callback) {
    on_state_ = std::move(callback);
}

void StreamingVoiceSession::WireCallbacks() {
    if (!backend_) {
        return;
    }

    // Stage 1 → 2 handoff is inside the backend; we only observe / play.
    backend_->OnAsrPartial([this](const std::string& text, bool is_final) {
        ESP_LOGD(TAG, "ASR %s: %s", is_final ? "final" : "partial", text.c_str());
        if (is_final && on_state_) {
            on_state_("asr_final");
        }
    });

    backend_->OnLlmToken([this](const std::string& token, bool is_final) {
        (void)token;
        if (on_state_) {
            on_state_(is_final ? "llm_done" : "speaking_gen");
        }
    });

    backend_->OnTtsAudio([this](std::unique_ptr<AudioChunk> chunk) {
        if (!chunk || !audio_out_ || !audio_out_->is_running()) {
            return;
        }
        // Stub path: payload may be PCM bytes; real path decodes Opus first.
        if (chunk->payload.size() >= sizeof(int16_t)) {
            const auto* samples =
                reinterpret_cast<const int16_t*>(chunk->payload.data());
            const size_t count = chunk->payload.size() / sizeof(int16_t);
            audio_out_->WritePcm(samples, count);
        }
        if (on_state_) {
            on_state_("playing");
        }
    });

    backend_->OnError([this](const std::string& message) {
        ESP_LOGW(TAG, "Brain error: %s", message.c_str());
        if (on_state_) {
            on_state_("error");
        }
    });
}
