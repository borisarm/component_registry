#pragma once

#include <cstdint>

#include "component_registry/core.hpp"

// Contrato ABI que todo plugin (.so) debe implementar para ser cargado por
// ComponentRegistry::load_plugin(). Es deliberadamente C puro: el name
// mangling de C++ no está estandarizado entre compiladores, y aunque en este
// ecosistema todo se compila con el mismo Clang fijado en el Dev Container,
// mantener el borde en C evita sorpresas si eso cambia.
//
// Reglas del contrato (documentadas aquí, no verificables por el compilador):
//   1. component_plugin_register() NUNCA debe dejar escapar una excepción de
//      C++. Cualquier error debe traducirse a `return false;`.
//   2. component_plugin_register() no debe bloquear ni hacer I/O costoso:
//      solo debe llamar a registry.register_factory(...) por cada componente
//      que el plugin ofrece.
//   3. Las fábricas registradas SÍ pueden lanzar excepciones internamente
//      (viven en el mismo espacio de direcciones, dentro del proceso host),
//      pero por convención del resto del sistema, se prefiere que devuelvan
//      errores a través de las interfaces de puerto, no por excepción.

namespace component_registry {

// Versión del contrato ABI en sí (no de los componentes que registra). Se
// incrementa solo cuando cambia la firma de PluginAbiInfo o de las funciones
// de entrada — no cuando cambian los componentes que un plugin registra.
inline constexpr std::uint32_t k_abi_version = 1;

inline constexpr char k_abi_version_symbol[] = "component_plugin_abi_version";
inline constexpr char k_register_symbol[] = "component_plugin_register";

}  // namespace component_registry

// Los dos puntos de entrada del contrato tienen que quedar visibles en la
// tabla de símbolos dinámicos del .so incluso cuando el plugin se compila
// con -fvisibility=hidden por defecto (recomendado, ver CMakeLists.txt del
// plugin de ejemplo): sin esto, dlsym() no los encuentra y load_plugin()
// falla con plugin_symbol_missing.
#if defined(__GNUC__) || defined(__clang__)
#define COMPONENT_PLUGIN_API __attribute__((visibility("default")))
#else
#define COMPONENT_PLUGIN_API
#endif

extern "C" {

struct ComponentPluginAbiInfo {
    std::uint32_t abi_version;
};

// Firma de component_plugin_abi_version(): el host la resuelve por dlsym
// usando component_registry::k_abi_version_symbol.
using ComponentPluginAbiVersionFn = ComponentPluginAbiInfo (*)();

// Firma de component_plugin_register(): el host la resuelve por dlsym usando
// component_registry::k_register_symbol. Debe devolver true si el registro
// de todos los componentes del plugin fue exitoso.
using ComponentPluginRegisterFn =
    bool (*)(component_registry::ComponentRegistry&) noexcept;

}  // extern "C"

// Macro de conveniencia para que cada plugin defina sus dos puntos de
// entrada con el nombre y la firma correctos, sin repetir extern "C" a mano.
// Uso en el .cpp del plugin:
//
//   COMPONENT_REGISTRY_DEFINE_PLUGIN {
//       return registry.register_factory(
//           "postgres.repository.opportunity",
//           [] { return std::make_shared<PostgresOpportunityRepository>(); }
//       ).has_value();
//   }
//
#define COMPONENT_REGISTRY_DEFINE_PLUGIN                                                 \
    extern "C" COMPONENT_PLUGIN_API ComponentPluginAbiInfo component_plugin_abi_version() { \
        return ComponentPluginAbiInfo{component_registry::k_abi_version};                \
    }                                                                                    \
    extern "C" COMPONENT_PLUGIN_API bool component_plugin_register(                      \
        component_registry::ComponentRegistry& registry) noexcept
