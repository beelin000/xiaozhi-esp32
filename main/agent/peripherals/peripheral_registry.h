#ifndef AGENT_PERIPHERALS_PERIPHERAL_REGISTRY_H
#define AGENT_PERIPHERALS_PERIPHERAL_REGISTRY_H

#include <cstddef>
#include <vector>

#include "peripheral.h"

/**
 * Registry for pluggable peripherals. Boards / features register drivers here
 * without modifying core agent wiring.
 */
class PeripheralRegistry {
public:
    static PeripheralRegistry& GetInstance();

    /** Register a peripheral (does not take ownership). */
    bool Register(Peripheral* peripheral);

    /** Unregister by pointer; returns true if removed. */
    bool Unregister(Peripheral* peripheral);

    Peripheral* FindByName(const char* name) const;
    std::vector<Peripheral*> ListByKind(PeripheralKind kind) const;
    const std::vector<Peripheral*>& all() const { return peripherals_; }
    size_t size() const { return peripherals_.size(); }

    /** Init all registered peripherals; returns count successfully initialized. */
    size_t InitAll();
    void DeinitAll();

private:
    PeripheralRegistry() = default;
    std::vector<Peripheral*> peripherals_;
};

#endif  // AGENT_PERIPHERALS_PERIPHERAL_REGISTRY_H
