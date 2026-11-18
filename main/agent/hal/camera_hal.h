#ifndef AGENT_HAL_CAMERA_HAL_H
#define AGENT_HAL_CAMERA_HAL_H

#include <cstdint>
#include <string>
#include <vector>

/**
 * Optional camera HAL for multimodal agent features.
 * Stub today; adapters can wrap Camera (boards/common/camera.h).
 */
struct CameraFrame {
    int width = 0;
    int height = 0;
    std::string format;  // e.g. "jpeg", "rgb565"
    std::vector<uint8_t> data;
};

class CameraHal {
public:
    virtual ~CameraHal() = default;

    virtual bool Init() = 0;
    virtual void Deinit() = 0;
    virtual bool IsAvailable() const = 0;

    virtual bool Capture(CameraFrame& out_frame) = 0;
    virtual bool SetHorizontalMirror(bool enabled) = 0;
    virtual bool SetVerticalFlip(bool enabled) = 0;
};

CameraHal* CreateCameraHal();

#endif  // AGENT_HAL_CAMERA_HAL_H
