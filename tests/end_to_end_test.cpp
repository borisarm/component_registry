#include <cstdlib>
#include <iostream>

#include "component_registry/core.hpp"
#include "component_registry/startup.hpp"
#include "echo_service_interface.hpp"

namespace {

int failures = 0;

void check(bool condition, std::string_view description) {
    if (condition) {
        std::cout << "[ OK ] " << description << "\n";
    } else {
        std::cout << "[FAIL] " << description << "\n";
        ++failures;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <ruta al .so del plugin de ejemplo>\n";
        return EXIT_FAILURE;
    }
    std::filesystem::path plugin_path = argv[1];

    component_registry::ComponentRegistry registry;

    // --- Carga del plugin -------------------------------------------------
    auto load_result = registry.load_plugin(plugin_path);
    check(load_result.has_value(), "load_plugin() carga el .so sin error");
    check(registry.loaded_plugin_count() == 1, "el registro retiene un handle de plugin");

    // --- Resolución exitosa por interfaz -----------------------------------
    auto echo_service = registry.create_as<IEchoService>("example.echo_service");
    check(echo_service.has_value(), "create_as<IEchoService> resuelve el componente registrado");
    if (echo_service) {
        auto message = (*echo_service)->echo("component registry");
        check(message == "echo: component registry",
              "el componente cargado dinámicamente responde correctamente");
    }

    // --- Caso de error: ComponentId inexistente ----------------------------
    auto missing = registry.create_as<IEchoService>("no.existe");
    check(!missing.has_value(), "create_as() devuelve error para un ComponentId inexistente");
    check(missing.has_value() ||
              missing.error().code == component_registry::ErrorCode::component_not_found,
          "el error reporta component_not_found");

    // --- Caso de error: interfaz que no coincide ---------------------------
    // IComponent no es lo que registramos como IEchoService, pero
    // dynamic_pointer_cast a una interfaz no implementada por la clase
    // concreta debe fallar de forma controlada, no crashear.
    class UnrelatedInterface : public component_registry::IComponent {
    public:
        virtual void unused() = 0;
    };
    auto mismatched = registry.create_as<UnrelatedInterface>("example.echo_service");
    check(!mismatched.has_value(),
          "create_as() devuelve error cuando la interfaz no coincide con la implementación");
    check(mismatched.has_value() ||
              mismatched.error().code == component_registry::ErrorCode::interface_mismatch,
          "el error reporta interface_mismatch");

    // --- StartupResolver: caso exitoso y caso con errores acumulados -------
    {
        std::shared_ptr<IEchoService> resolved_echo;
        component_registry::StartupResolver resolver{registry};
        resolver.require("example.echo_service", resolved_echo);
        check(resolver.ok(), "StartupResolver.ok() es true cuando todo se resuelve");
        check(resolved_echo != nullptr, "StartupResolver deja el shared_ptr resuelto en 'out'");
    }
    {
        std::shared_ptr<IEchoService> a;
        std::shared_ptr<IEchoService> b;
        component_registry::StartupResolver resolver{registry};
        resolver.require("example.echo_service", a).require("no.existe.tampoco", b);
        check(!resolver.ok(), "StartupResolver.ok() es false si algún require() falla");
        check(a != nullptr, "un require() exitoso no se ve afectado por otro que falla");
        check(b == nullptr, "un require() fallido deja 'out' sin modificar");
        check(resolver.errors().size() == 1, "StartupResolver acumula exactamente los errores ocurridos");
        std::cout << "--- summary() de ejemplo ---\n" << resolver.summary();
    }

    // --- Caso de error: el plugin devuelve false al registrarse ------------
    // Cargar el mismo .so otra vez hace que su register_factory() choque con
    // duplicate_registration, así que component_plugin_register devuelve false.
    {
        auto reload = registry.load_plugin(plugin_path);
        check(!reload.has_value(), "load_plugin() devuelve error si el registro del plugin falla");
        check(reload.has_value() ||
                  reload.error().code == component_registry::ErrorCode::plugin_registration_failed,
              "el error reporta plugin_registration_failed");
        check(registry.loaded_plugin_count() == 2,
              "el handle se retiene aunque el registro haya fallado");
        auto still_there = registry.create_as<IEchoService>("example.echo_service");
        check(still_there.has_value() && (*still_there)->echo("x") == "echo: x",
              "las fábricas ya registradas siguen funcionando tras el fallo");
    }

    std::cout << "\n" << (failures == 0 ? "TODOS LOS TESTS PASARON" : "HUBO FALLOS") << "\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}