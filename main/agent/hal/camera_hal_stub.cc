#include "camera_hal.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "agent.hal.camera";

class CameraHalStub : public CameraHal {
public:
    bool Init() override {
        ESP_LOGI(TAG, "Init stub (no camera hardware)");
        initialized_ = true;
        return true;
    }

    void Deinit() override {
        initialized_ = false;
        ESP_LOGI(TAG, "Deinit stub");
    }

    bool IsAvailable() const override {
        // Skeleton reports unavailable until a real driver is wired.
        return false;
    }

    bool Capture(CameraFrame& out_frame) override {
        (void)out_frame;
        if (!initialized_) {
            return false;
        }
        ESP_LOGD(TAG, "Capture stub: no frame");
        return false;
    }

    bool SetHorizontalMirror(bool enabled) override {
        h_mirror_ = enabled;
        return true;
    }

    bool SetVerticalFlip(bool enabled) override {
        v_flip_ = enabled;
        return true;
    }

private:
    bool initialized_ = false;
    bool h_mirror_ = false;
    bool v_flip_ = false;
};

}  // namespace

CameraHal* CreateCameraHal() {
    return new CameraHalStub();
}
