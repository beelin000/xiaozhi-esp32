#include "custom_voice_config.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.voice";

class CustomVoiceConfigStub : public CustomVoiceConfig {
public:
    bool Load() override {
        // Stub default profile; real impl would read NVS / assets.
        profile_.voice_id = "default";
        profile_.display_name = "Default Voice";
        profile_.speaking_rate = 1.0f;
        profile_.pitch = 1.0f;
        profile_.language = "zh-CN";
        profile_.style = "neutral";
        ESP_LOGI(TAG, "Load stub profile voice_id=%s", profile_.voice_id.c_str());
        return true;
    }

    bool Save() override {
        ESP_LOGI(TAG, "Save stub profile voice_id=%s", profile_.voice_id.c_str());
        return true;
    }

    void SetProfile(const CustomVoiceProfile& profile) override {
        profile_ = profile;
        ESP_LOGI(TAG, "SetProfile: %s (%s)", profile_.display_name.c_str(),
                 profile_.voice_id.c_str());
        if (on_change_) {
            on_change_(profile_);
        }
    }

    const CustomVoiceProfile& GetProfile() const override { return profile_; }

    void OnProfileChanged(ChangeCallback callback) override {
        on_change_ = std::move(callback);
    }

private:
    CustomVoiceProfile profile_;
    ChangeCallback on_change_;
};

}  // namespace

CustomVoiceConfig* CreateCustomVoiceConfig() {
    return new CustomVoiceConfigStub();
}
