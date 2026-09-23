// Server defectuoso para tests: exporta component_server_abi_version pero no
// component_server_register. load_server() debe rechazarlo con
// server_symbol_missing.

#include "component_registry/server_abi.hpp"

extern "C" COMPONENT_SERVER_API ComponentServerAbiInfo component_server_abi_version() {
    return ComponentServerAbiInfo{component_registry::k_abi_version};
}
