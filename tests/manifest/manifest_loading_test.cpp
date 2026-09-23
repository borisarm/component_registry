// load_servers_from_manifest(): los servers listados quedan cargados y sus
// componentes disponibles en el registro.

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"
#include "echo_service_interface.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::example_server_path;
using component_registry::testing::TempDirectory;

class ManifestLoadingTest : public ::testing::Test {
protected:
    TempDirectory dir;
    cr::ComponentRegistry registry;
};

TEST_F(ManifestLoadingTest, LoadedServerIsRetainedByRegistry) {
    auto const manifest = dir.write_file("servers.manifest", example_server_path().string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_EQ(registry.loaded_servers_count(), 1u);
}

TEST_F(ManifestLoadingTest, ComponentsOfLoadedServerCanBeCreated) {
    auto const manifest = dir.write_file("servers.manifest", example_server_path().string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);
    ASSERT_TRUE(result.has_value()) << result.error().detail;

    auto echo = registry.create_as<IEchoService>("example.echo_service");
    ASSERT_TRUE(echo.has_value()) << echo.error().detail;
    EXPECT_EQ((*echo)->echo("manifest"), "echo: manifest");
}

TEST_F(ManifestLoadingTest, SeparateRegistriesLoadTheSameManifestIndependently) {
    auto const manifest = dir.write_file("servers.manifest", example_server_path().string() + "\n");
    cr::ComponentRegistry first;
    cr::ComponentRegistry second;

    auto first_result = cr::load_servers_from_manifest(first, manifest);
    auto second_result = cr::load_servers_from_manifest(second, manifest);

    ASSERT_TRUE(first_result.has_value()) << first_result.error().detail;
    ASSERT_TRUE(second_result.has_value()) << second_result.error().detail;
    EXPECT_TRUE(first_result->all_loaded()) << first_result->summary();
    EXPECT_TRUE(second_result->all_loaded()) << second_result->summary();
    EXPECT_TRUE(second.create_as<IEchoService>("example.echo_service").has_value());
}
