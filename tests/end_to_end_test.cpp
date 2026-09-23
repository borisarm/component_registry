// Test end-to-end del ciclo completo host -> server:
//   1. El host NO enlaza example_repository_server en tiempo de compilación.
//   2. Carga el .so en tiempo de ejecución vía ComponentRegistry::load_server.
//   3. Resuelve IEchoService por ComponentId, sin conocer InMemoryEchoService.
//   4. Verifica también los casos de error: componente inexistente e
//      interfaz que no coincide con la implementación registrada.

#include <memory>

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/startup.hpp"
#include "echo_service_interface.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::example_server_path;

class EndToEndTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto load_result = registry.load_server(example_server_path());
        ASSERT_TRUE(load_result.has_value()) << load_result.error().detail;
    }

    cr::ComponentRegistry registry;
};

TEST_F(EndToEndTest, RegistryRetainsServerHandle) {
    EXPECT_EQ(registry.loaded_servers_count(), 1u);
}

TEST_F(EndToEndTest, CreateAsResolvesRegisteredComponent) {
    auto echo_service = registry.create_as<IEchoService>("example.echo_service");
    ASSERT_TRUE(echo_service.has_value()) << echo_service.error().detail;
    EXPECT_EQ((*echo_service)->echo("component registry"), "echo: component registry");
}

TEST_F(EndToEndTest, CreateAsFailsForUnknownComponentId) {
    auto missing = registry.create_as<IEchoService>("no.existe");
    ASSERT_FALSE(missing.has_value());
    EXPECT_EQ(missing.error().code, cr::ErrorCode::component_not_found);
}

TEST_F(EndToEndTest, CreateAsFailsForMismatchedInterface) {
    // dynamic_pointer_cast a una interfaz no implementada por la clase
    // concreta debe fallar de forma controlada, no crashear.
    class UnrelatedInterface : public cr::IComponent {
    public:
        virtual void unused() = 0;
    };
    auto mismatched = registry.create_as<UnrelatedInterface>("example.echo_service");
    ASSERT_FALSE(mismatched.has_value());
    EXPECT_EQ(mismatched.error().code, cr::ErrorCode::interface_mismatch);
}

TEST_F(EndToEndTest, StartupResolverResolvesAllRequirements) {
    std::shared_ptr<IEchoService> resolved_echo;
    cr::StartupResolver resolver{registry};
    resolver.require("example.echo_service", resolved_echo);

    EXPECT_TRUE(resolver.ok());
    EXPECT_NE(resolved_echo, nullptr);
}

TEST_F(EndToEndTest, StartupResolverAccumulatesErrors) {
    std::shared_ptr<IEchoService> a;
    std::shared_ptr<IEchoService> b;
    cr::StartupResolver resolver{registry};
    resolver.require("example.echo_service", a).require("no.existe.tampoco", b);

    EXPECT_FALSE(resolver.ok());
    EXPECT_NE(a, nullptr) << "un require() exitoso no se ve afectado por otro que falla";
    EXPECT_EQ(b, nullptr) << "un require() fallido deja 'out' sin modificar";
    EXPECT_EQ(resolver.errors().size(), 1u);
    EXPECT_FALSE(resolver.summary().empty());
}
