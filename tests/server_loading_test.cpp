// load_server(): los servers que no cumplen el contrato (símbolos o ABI) se
// rechazan antes de registrar nada y no quedan retenidos por el registro.

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "test_support.hpp"

namespace cr = component_registry;
using component_registry::testing::no_abi_symbol_server_path;
using component_registry::testing::no_register_symbol_server_path;
using component_registry::testing::incompatible_abi_server_path;

class ServerLoadingTest : public ::testing::Test {
protected:
    cr::ComponentRegistry registry;
};

TEST_F(ServerLoadingTest, MissingAbiVersionSymbolIsRejected) {
    auto result = registry.load_server(no_abi_symbol_server_path());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, cr::ErrorCode::server_symbol_missing);
    EXPECT_NE(result.error().detail.find(no_abi_symbol_server_path().string()),
              std::string::npos);
    EXPECT_EQ(registry.loaded_servers_count(), 0u);
}

TEST_F(ServerLoadingTest, MissingRegisterSymbolIsRejected) {
    auto result = registry.load_server(no_register_symbol_server_path());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, cr::ErrorCode::server_symbol_missing);
    EXPECT_EQ(registry.loaded_servers_count(), 0u);
}

TEST_F(ServerLoadingTest, IncompatibleAbiVersionIsRejected) {
    auto result = registry.load_server(incompatible_abi_server_path());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, cr::ErrorCode::server_abi_incompatible);
    EXPECT_NE(result.error().detail.find(incompatible_abi_server_path().string()),
              std::string::npos);
    EXPECT_EQ(registry.loaded_servers_count(), 0u);
}
