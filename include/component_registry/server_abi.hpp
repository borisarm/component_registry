#pragma once

#include <cstdint>

#include "component_registry/core.hpp"



namespace component_registry {

inline constexpr std::uint32_t k_abi_version = 1;

inline constexpr char k_abi_version_symbol[] = "component_server_abi_version";
inline constexpr char k_register_symbol[] = "component_server_register";

} 
#if defined(__GNUC__) || defined(__clang__)
#define COMPONENT_SERVER_API __attribute__((visibility("default")))
#else
#define COMPONENT_SERVER_API
#endif

extern "C" {

struct ComponentServerAbiInfo {
    std::uint32_t abi_version;
};

using ComponentServerAbiVersionFn = ComponentServerAbiInfo (*)();

using ComponentServerRegisterFn =
    bool (*)(component_registry::ComponentRegistry&) noexcept;

}  // extern "C"

#define COMPONENT_REGISTRY_DEFINE_SERVER                                                 \
    extern "C" COMPONENT_SERVER_API ComponentServerAbiInfo component_server_abi_version() { \
        return ComponentServerAbiInfo{component_registry::k_abi_version};                \
    }                                                                                    \
    extern "C" COMPONENT_SERVER_API bool component_server_register(                      \
        component_registry::ComponentRegistry& registry) noexcept
