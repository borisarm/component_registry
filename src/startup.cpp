#include <component_registry/startup.hpp>

#include <sstream>
#include <string_view>

namespace component_registry {

namespace {

    std::string_view code_name(ErrorCode code) {
        switch (code) {
            case ErrorCode::component_not_found:       return "component_not_found";
            case ErrorCode::interface_mismatch:         return "interface_mismatch";
            case ErrorCode::duplicate_registration:     return "duplicate_registration";
            case ErrorCode::plugin_load_failed:         return "plugin_load_failed";
            case ErrorCode::plugin_symbol_missing:      return "plugin_symbol_missing";
            case ErrorCode::plugin_abi_incompatible:    return "plugin_abi_incompatible";
            case ErrorCode::plugin_registration_failed: return "plugin_registration_failed";
            case ErrorCode::unknown_error:              return "unknown_error";
        }
        return "unknown_error";
    }

}  // namespace

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