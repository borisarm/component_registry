// load_servers_from_manifest(): los fallos por entrada se acumulan en
// ManifestResult::failures sin abortar la carga del resto.

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"
#include "echo_service_interface.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::example_server_path;
using component_registry::testing::TempDirectory;

class ManifestFailuresTest : public ::testing::Test {
protected:
    TempDirectory dir;
    cr::ComponentRegistry registry;
};

TEST_F(ManifestFailuresTest, MissingServerIsReportedAsLoadFailure) {
    auto const missing = dir.path() / "missing.so";
    auto const manifest = dir.write_file("servers.manifest", missing.string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_FALSE(result->all_loaded());
    EXPECT_EQ(result->loaded_count, 0u);
    ASSERT_EQ(result->failures.size(), 1u);
    EXPECT_EQ(result->failures[0].server_path, missing);
    EXPECT_EQ(result->failures[0].error.code, cr::ErrorCode::server_load_failed);
}

TEST_F(ManifestFailuresTest, FileThatIsNotASharedLibraryIsReportedAsLoadFailure) {
    auto const bogus = dir.write_file("bogus.so", "esto no es un ELF");
    auto const manifest = dir.write_file("servers.manifest", bogus.string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    ASSERT_EQ(result->failures.size(), 1u);
    EXPECT_EQ(result->failures[0].server_path, bogus);
    EXPECT_EQ(result->failures[0].error.code, cr::ErrorCode::server_load_failed);
}

TEST_F(ManifestFailuresTest, DuplicateServerIsReportedAsRegistrationFailure) {
    auto const server = example_server_path().string();
    auto const manifest = dir.write_file("servers.manifest", server + "\n" + server + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    ASSERT_EQ(result->failures.size(), 1u);
    EXPECT_EQ(result->failures[0].error.code, cr::ErrorCode::server_registration_failed);
}

TEST_F(ManifestFailuresTest, FailingEntryDoesNotStopFollowingEntries) {
    auto const manifest = dir.write_file("servers.manifest",
                                         (dir.path() / "missing.so").string() + "\n" +
                                             example_server_path().string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    EXPECT_EQ(result->loaded_count, 1u);
    EXPECT_EQ(result->failures.size(), 1u);
    EXPECT_TRUE(registry.create_as<IEchoService>("example.echo_service").has_value());
}

TEST_F(ManifestFailuresTest, FailuresAreReportedInManifestOrder) {
    auto const first = dir.path() / "first_missing.so";
    auto const second = dir.path() / "second_missing.so";
    auto const third = dir.path() / "third_missing.so";
    auto const manifest = dir.write_file(
        "servers.manifest",
        first.string() + "\n# comentario\n" + second.string() + "\n\n" + third.string() + "\n");

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_TRUE(result.has_value()) << result.error().detail;
    ASSERT_EQ(result->failures.size(), 3u);
    EXPECT_EQ(result->failures[0].server_path, first);
    EXPECT_EQ(result->failures[1].server_path, second);
    EXPECT_EQ(result->failures[2].server_path, third);
}
