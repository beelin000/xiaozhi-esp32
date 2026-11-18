#ifndef AGENT_PERIPHERALS_PERIPHERAL_H
#define AGENT_PERIPHERALS_PERIPHERAL_H

#include <string>

/**
 * Simple extension interface for adding peripherals (sensors, actuators, cameras).
 * Register instances with PeripheralRegistry.
 */
enum class PeripheralKind {
    kGeneric,
    kSensor,
    kActuator,
    kCamera,
    kMicrophone,
    kDisplay,
    kCustom,
};

class Peripheral {
public:
    virtual ~Peripheral() = default;

    virtual const char* name() const = 0;
    virtual PeripheralKind kind() const = 0;

    virtual bool Init() = 0;
    virtual void Deinit() = 0;
    virtual bool IsReady() const = 0;

    /** Optional JSON-ish status for debugging / MCP exposure. */
    virtual std::string GetStatusJson() const {
        return "{}";
    }
};

#endif  // AGENT_PERIPHERALS_PERIPHERAL_H
