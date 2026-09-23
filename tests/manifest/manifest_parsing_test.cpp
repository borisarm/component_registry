// load_servers_from_manifest(): formato del manifiesto (líneas vacías,
// comentarios, espacios y finales de línea).

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::example_server_path;
using component_registry::testing::TempDirectory;

class ManifestParsingTest : public ::testing::Test {
protected:
    TempDirectory dir;
    cr::ComponentRegistry registry;
};

TEST_F(ManifestParsingTest, EmptyManifestLoadsNothing) {
    auto const manifest = dir.write_file("servers.manifest", "");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 0u);
    EXPECT_TRUE(result->all_loaded());
    EXPECT_EQ(registry.loaded_servers_count(), 0u);
}

TEST_F(ManifestParsingTest, BlankAndWhitespaceOnlyLinesAreIgnored) {
    auto const manifest = dir.write_file("servers.manifest", "\n   \n\t\n  \t  \n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 0u);
    EXPECT_TRUE(result->all_loaded());
}

TEST_F(ManifestParsingTest, CommentLinesAreIgnored) {
    auto const manifest = dir.write_file("servers.manifest",
                                         "# comentario\n"
                                         "   # comentario indentado\n"
                                         "#/ruta/que/no/existe.so\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 0u);
    EXPECT_TRUE(result->all_loaded());
}

TEST_F(ManifestParsingTest, SurroundingWhitespaceIsTrimmed) {
    auto const manifest = dir.write_file(
        "servers.manifest", "  \t" + example_server_path().string() + " \t  \n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}

TEST_F(ManifestParsingTest, WindowsLineEndingsAreAccepted) {
    auto const manifest = dir.write_file(
        "servers.manifest", "# comentario\r\n\r\n" + example_server_path().string() + "\r\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_TRUE(result->all_loaded()) << result->summary();
}

TEST_F(ManifestParsingTest, LastLineWithoutTrailingNewlineIsRead) {
    auto const manifest = dir.write_file("servers.manifest", example_server_path().string());

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
}
