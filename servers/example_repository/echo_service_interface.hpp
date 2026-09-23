#pragma once

#include <string>
#include <string_view>

#include "component_registry/core.hpp"

class COMPONENT_REGISTRY_INTERFACE IEchoService : public component_registry::IComponent {
public:
    virtual std::string echo(std::string_view message) const = 0;
};