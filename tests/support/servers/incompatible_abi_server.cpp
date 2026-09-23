// Server defectuoso para tests: exporta el contrato completo pero declara una
// versión de ABI distinta de la del host. load_server() debe rechazarlo con
// server_abi_incompatible sin llegar a llamar a component_server_register.

#include "component_registry/server_abi.hpp"

extern "C" COMPONENT_SERVER_API ComponentServerAbiInfo component_server_abi_version() {
    return ComponentServerAbiInfo{component_registry::k_abi_version + 1};
}

extern "C" COMPONENT_SERVER_API bool component_server_register(
    component_registry::ComponentRegistry&) noexcept {
    return true;
}
