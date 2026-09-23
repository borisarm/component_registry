// load_servers_from_manifest(): resolución de rutas absolutas y relativas.

#include <filesystem>

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
namespace fs = std::filesystem;
using component_registry::testing::example_server_path;
using component_registry::testing::TempDirectory;

namespace {

// Cambia el directorio de trabajo y lo restaura al salir de ámbito.
class ScopedCurrentPath {
public:
    explicit ScopedCurrentPath(fs::path const& path) : previous_{fs::current_path()} {
        fs::current_path(path);
    }
    ~ScopedCurrentPath() {
        std::error_code ignored;
        fs::current_path(previous_, ignored);
    }
    ScopedCurrentPath(ScopedCurrentPath const&) = delete;
    ScopedCurrentPath& operator=(ScopedCurrentPath const&) = delete;

private:
    fs::path previous_;
};

}  // namespace

class ManifestPathResolutionTest : public ::testing::Test {
protected:
    TempDirectory dir;
    cr::ComponentRegistry registry;
};

TEST_F(ManifestPathResolutionTest, AbsolutePathIsUsedAsIs) {
    auto const manifest =
        dir.write_file("config/servers.manifest", example_server_path().string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}

TEST_F(ManifestPathResolutionTest, RelativePathIsResolvedAgainstManifestDirectory) {
    dir.copy_example_server("config/servers/echo.so");
    auto const manifest = dir.write_file("config/servers.manifest", "servers/echo.so\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}

TEST_F(ManifestPathResolutionTest, RelativePathIsNotResolvedAgainstWorkingDirectory) {
    dir.copy_example_server("servers/echo.so");  // existe respecto al cwd...
    auto const manifest = dir.write_file("config/servers.manifest", "servers/echo.so\n");
    ScopedCurrentPath cwd{dir.path()};

    auto result = cr::load_servers_from_manifest(registry, manifest);

    // ...pero no respecto al manifiesto, que es lo que cuenta.
    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 0u);
    ASSERT_EQ(result->failures.size(), 1u);
    EXPECT_EQ(result->failures[0].server_path, dir.path() / "config" / "servers/echo.so");
}

TEST_F(ManifestPathResolutionTest, BareFileNameIsResolvedAgainstManifestDirectory) {
    dir.copy_example_server("config/echo.so");
    auto const manifest = dir.write_file("config/servers.manifest", "echo.so\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}

TEST_F(ManifestPathResolutionTest, RelativeManifestPathResolvesEntriesAgainstItsDirectory) {
    // Con un manifiesto dado por ruta relativa sin directorio, parent_path()
    // es vacío; una entrada "echo.so" no debe acabar en la búsqueda de
    // bibliotecas de dlopen (LD_LIBRARY_PATH), sino junto al manifiesto.
    dir.copy_example_server("echo.so");
    dir.write_file("servers.manifest", "echo.so\n");
    ScopedCurrentPath cwd{dir.path()};

    auto result = cr::load_servers_from_manifest(registry, "servers.manifest");

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}
