#pragma once

#include <cstdint>

#include <component_registry/core.hpp>


namespace component_registry
{

    inline constexpr std::uint32_t k_abi_version = 1;
    inline constexpr char k_abi_version_symbol[] = "component_plugin_abi_version";
    inline constexpr char k_register_symbol[] = "component_plugin_register";

}


#if defined(__GNUC__) || defined(__clang__)
#define COMPONENT_PLUGIN_API __attribute__((visibility("default")))
#else
#define COMPONENT_PLUGIN_API
#endif

extern "C" {

    struct ComponentPluginAbiInfo {
        std::uint32_t abi_version;
    };

    using ComponentPluginAbiVersionFn = ComponentPluginAbiInfo (*)();
    using ComponentPluginRegisterFn =     bool (*)(component_registry::ComponentRegistry&) noexcept;

}


// Convenience macro to define a component plugin.
//
//   COMPONENT_REGISTRY_DEFINE_PLUGIN {
//       return registry.register_factory(
//           "postgres.repository.opportunity",
//           [] { return std::make_shared<PostgresOpportunityRepository>(); }
//       ).has_value();
//   }
//
#define COMPONENT_REGISTRY_DEFINE_PLUGIN                                                    \
    extern "C" COMPONENT_PLUGIN_API ComponentPluginAbiInfo component_plugin_abi_version() { \
        return ComponentPluginAbiInfo{component_registry::k_abi_version};                   \
    }                                                                                       \
    extern "C" COMPONENT_PLUGIN_API bool component_plugin_register(                         \
        component_registry::ComponentRegistry& registry) noexcept