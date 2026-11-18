#include "peripheral_registry.h"

#include <cstring>

#include <esp_log.h>

namespace {
constexpr const char* TAG = "agent.periph";
}

PeripheralRegistry& PeripheralRegistry::GetInstance() {
    static PeripheralRegistry instance;
    return instance;
}

bool PeripheralRegistry::Register(Peripheral* peripheral) {
    if (peripheral == nullptr || peripheral->name() == nullptr) {
        return false;
    }
    if (FindByName(peripheral->name()) != nullptr) {
        ESP_LOGW(TAG, "Register: duplicate name '%s'", peripheral->name());
        return false;
    }
    peripherals_.push_back(peripheral);
    ESP_LOGI(TAG, "Registered peripheral '%s'", peripheral->name());
    return true;
}

bool PeripheralRegistry::Unregister(Peripheral* peripheral) {
    for (auto it = peripherals_.begin(); it != peripherals_.end(); ++it) {
        if (*it == peripheral) {
            peripherals_.erase(it);
            return true;
        }
    }
    return false;
}

Peripheral* PeripheralRegistry::FindByName(const char* name) const {
    if (name == nullptr) {
        return nullptr;
    }
    for (Peripheral* p : peripherals_) {
        if (p && p->name() && std::strcmp(p->name(), name) == 0) {
            return p;
        }
    }
    return nullptr;
}

std::vector<Peripheral*> PeripheralRegistry::ListByKind(PeripheralKind kind) const {
    std::vector<Peripheral*> out;
    for (Peripheral* p : peripherals_) {
        if (p && p->kind() == kind) {
            out.push_back(p);
        }
    }
    return out;
}

size_t PeripheralRegistry::InitAll() {
    size_t ok = 0;
    for (Peripheral* p : peripherals_) {
        if (p && p->Init()) {
            ++ok;
        }
    }
    ESP_LOGI(TAG, "InitAll: %u / %u ready", static_cast<unsigned>(ok),
             static_cast<unsigned>(peripherals_.size()));
    return ok;
}

void PeripheralRegistry::DeinitAll() {
    for (auto it = peripherals_.rbegin(); it != peripherals_.rend(); ++it) {
        if (*it) {
            (*it)->Deinit();
        }
    }
}
