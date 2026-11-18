#include "brain_backend.h"

#include <esp_log.h>

namespace {
constexpr const char* TAG = "agent.brain.factory";
}

// Defined in cloud_brain_backend.cc / local_brain_backend.cc
BrainBackend* CreateCloudBrainBackend();
BrainBackend* CreateLocalBrainBackend();

BrainBackend* CreateBrainBackend(BrainBackendKind kind) {
    switch (kind) {
        case BrainBackendKind::kLocal:
            ESP_LOGI(TAG, "Selecting local (Jetson companion) brain backend");
            return CreateLocalBrainBackend();
        case BrainBackendKind::kCloud:
        default:
            ESP_LOGI(TAG, "Selecting cloud brain backend (default)");
            return CreateCloudBrainBackend();
    }
}
