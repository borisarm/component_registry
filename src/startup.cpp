#include <component_registry/startup.hpp>

#include <sstream>
#include <string_view>

namespace component_registry {

    std::string_view code_name(ErrorCode code) noexcept {
        switch (code) {
            case ErrorCode::component_not_found:       return "component_not_found";
            case ErrorCode::interface_mismatch:         return "interface_mismatch";
            case ErrorCode::duplicate_registration:     return "duplicate_registration";
            case ErrorCode::server_load_failed:         return "server_load_failed";
            case ErrorCode::server_symbol_missing:      return "server_symbol_missing";
            case ErrorCode::server_abi_incompatible:    return "server_abi_incompatible";
            case ErrorCode::server_registration_failed: return "server_registration_failed";
            case ErrorCode::manifest_unreadable:        return "manifest_unreadable";
            case ErrorCode::unknown_error:              return "unknown_error";
        }
        return "unknown_error";
    }

    std::string StartupResolver::summary() const {
    if (errors_.empty()) {
        return "StartupResolver: todos los componentes obligatorios se resolvieron correctamente.";
    }

    std::ostringstream out;
    out << "StartupResolver: " << errors_.size()
        << " componente(s) obligatorio(s) no se pudieron resolver:\n";
    for (auto const& error : errors_) {
        out << "  - [" << code_name(error.code) << "] " << error.detail << "\n";
    }
    return out.str();
}


}