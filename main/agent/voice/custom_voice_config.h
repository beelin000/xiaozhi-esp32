#ifndef AGENT_VOICE_CUSTOM_VOICE_CONFIG_H
#define AGENT_VOICE_CUSTOM_VOICE_CONFIG_H

#include <functional>
#include <string>

/**
 * Hook for custom TTS / persona voice settings.
 * Persist locally and/or push to the cloud TTS endpoint.
 */
struct CustomVoiceProfile {
    std::string voice_id;       // Cloud TTS voice identifier
    std::string display_name;   // Human-readable label
    float speaking_rate = 1.0f; // 1.0 = normal
    float pitch = 1.0f;         // 1.0 = normal
    std::string language;       // e.g. "zh-CN", "en-US"
    std::string style;          // Optional style / emotion tag
};

class CustomVoiceConfig {
public:
    using ChangeCallback = std::function<void(const CustomVoiceProfile& profile)>;

    virtual ~CustomVoiceConfig() = default;

    virtual bool Load() = 0;
    virtual bool Save() = 0;

    virtual void SetProfile(const CustomVoiceProfile& profile) = 0;
    virtual const CustomVoiceProfile& GetProfile() const = 0;

    virtual void OnProfileChanged(ChangeCallback callback) = 0;
};

CustomVoiceConfig* CreateCustomVoiceConfig();

#endif  // AGENT_VOICE_CUSTOM_VOICE_CONFIG_H
