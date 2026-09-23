// Server defectuoso para tests: exporta component_server_register pero no
// component_server_abi_version. load_server() debe rechazarlo con
// server_symbol_missing.

#include "component_registry/server_abi.hpp"

extern "C" COMPONENT_SERVER_API bool component_server_register(
    component_registry::ComponentRegistry&) noexcept {
    return true;
}
