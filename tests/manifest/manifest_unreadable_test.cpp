// load_servers_from_manifest(): el manifiesto no se puede leer.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::TempDirectory;
using ::testing::HasSubstr;

class ManifestUnreadableTest : public ::testing::Test {
protected:
    TempDirectory dir;
    cr::ComponentRegistry registry;
};

TEST_F(ManifestUnreadableTest, MissingFileReturnsManifestUnreadable) {
    auto const manifest = dir.path() / "does_not_exist.manifest";

    auto result = cr::load_servers_from_manifest(registry, manifest);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, cr::ErrorCode::manifest_unreadable);
    EXPECT_THAT(result.error().detail, HasSubstr(manifest.string()));
}

TEST_F(ManifestUnreadableTest, DirectoryIsNotAValidManifest) {

    auto result = cr::load_servers_from_manifest(registry, dir.path());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, cr::ErrorCode::manifest_unreadable);
}

TEST_F(ManifestUnreadableTest, FailureDoesNotLoadAnyServer) {

    auto result = cr::load_servers_from_manifest(registry, dir.path() / "missing.manifest");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(registry.loaded_servers_count(), 0u);
}
