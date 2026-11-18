#ifndef AGENT_PERIPHERALS_EXAMPLE_PERIPHERAL_H
#define AGENT_PERIPHERALS_EXAMPLE_PERIPHERAL_H

#include "peripheral.h"
#include "peripheral_registry.h"

/**
 * Minimal example of implementing and registering a peripheral.
 * Copy this pattern for sensors, actuators, extra mics/cameras, etc.
 *
 * Usage (e.g. from a board bring-up file):
 *   static ExamplePeripheral example;
 *   PeripheralRegistry::GetInstance().Register(&example);
 */
class ExamplePeripheral : public Peripheral {
public:
    const char* name() const override { return "example"; }
    PeripheralKind kind() const override { return PeripheralKind::kCustom; }

    bool Init() override {
        ready_ = true;
        return true;
    }

    void Deinit() override { ready_ = false; }
    bool IsReady() const override { return ready_; }

    std::string GetStatusJson() const override {
        return ready_ ? "{\"ready\":true}" : "{\"ready\":false}";
    }

private:
    bool ready_ = false;
};

#endif  // AGENT_PERIPHERALS_EXAMPLE_PERIPHERAL_H
