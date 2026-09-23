// StartupResolver: resolución de dependencias obligatorias y el texto de
// summary(), con un registro poblado en proceso (sin cargar servers).

#include <memory>
#include <utility>

#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/startup.hpp"

namespace cr = component_registry;

namespace {

class IGreeter : public cr::IComponent {
public:
    virtual std::string greet() const = 0;
};

class IUnrelated : public cr::IComponent {
public:
    virtual void unused() = 0;
};

class Greeter final : public IGreeter {
public:
    std::string greet() const override { return "hola"; }
};

}  // namespace

class StartupResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto registered = registry.register_factory(
            "greeter", [] { return std::make_shared<Greeter>(); });
        ASSERT_TRUE(registered.has_value()) << registered.error().detail;
    }

    cr::ComponentRegistry registry;
};

TEST_F(StartupResolverTest, SummaryReportsSuccessWhenEverythingResolves) {
    std::shared_ptr<IGreeter> greeter;
    cr::StartupResolver resolver{registry};
    resolver.require("greeter", greeter);

    ASSERT_TRUE(resolver.ok());
    EXPECT_TRUE(resolver.errors().empty());
    EXPECT_EQ(resolver.summary(),
              "StartupResolver: todos los componentes obligatorios se resolvieron correctamente.");
}

TEST_F(StartupResolverTest, SummaryListsEveryFailureWithItsCode) {
    std::shared_ptr<IGreeter> missing;
    std::shared_ptr<IUnrelated> mismatched;
    cr::StartupResolver resolver{registry};
    resolver.require("no.existe", missing).require("greeter", mismatched);

    ASSERT_EQ(resolver.errors().size(), 2u);
    EXPECT_EQ(resolver.errors()[0].code, cr::ErrorCode::component_not_found);
    EXPECT_EQ(resolver.errors()[1].code, cr::ErrorCode::interface_mismatch);

    auto const summary = resolver.summary();
    EXPECT_TRUE(summary.starts_with(
        "StartupResolver: 2 componente(s) obligatorio(s) no se pudieron resolver:\n"))
        << summary;
    EXPECT_NE(summary.find("  - [component_not_found] " + resolver.errors()[0].detail + "\n"),
              std::string::npos) << summary;
    EXPECT_NE(summary.find("  - [interface_mismatch] " + resolver.errors()[1].detail + "\n"),
              std::string::npos) << summary;
}

TEST_F(StartupResolverTest, SummaryReportsAFactoryReturningNullAsInterfaceMismatch) {
    auto registered = registry.register_factory("null", [] { return nullptr; });
    ASSERT_TRUE(registered.has_value()) << registered.error().detail;

    std::shared_ptr<IGreeter> greeter;
    cr::StartupResolver resolver{registry};
    resolver.require("null", greeter);

    EXPECT_FALSE(resolver.ok());
    EXPECT_EQ(greeter, nullptr);
    ASSERT_EQ(resolver.errors().size(), 1u);
    EXPECT_EQ(resolver.errors()[0].code, cr::ErrorCode::interface_mismatch);
    EXPECT_TRUE(resolver.summary().starts_with(
        "StartupResolver: 1 componente(s) obligatorio(s) no se pudieron resolver:\n"));
}

TEST_F(StartupResolverTest, CodeNameCoversEveryErrorCode) {
    std::pair<cr::ErrorCode, std::string_view> const expected[] = {
        {cr::ErrorCode::component_not_found, "component_not_found"},
        {cr::ErrorCode::interface_mismatch, "interface_mismatch"},
        {cr::ErrorCode::duplicate_registration, "duplicate_registration"},
        {cr::ErrorCode::server_load_failed, "server_load_failed"},
        {cr::ErrorCode::server_symbol_missing, "server_symbol_missing"},
        {cr::ErrorCode::server_abi_incompatible, "server_abi_incompatible"},
        {cr::ErrorCode::server_registration_failed, "server_registration_failed"},
        {cr::ErrorCode::manifest_unreadable, "manifest_unreadable"},
        {cr::ErrorCode::unknown_error, "unknown_error"},
    };
    for (auto const& [code, name] : expected) {
        EXPECT_EQ(cr::code_name(code), name);
    }
}

TEST_F(StartupResolverTest, CodeNameFallsBackToUnknownErrorForOutOfRangeValues) {
    EXPECT_EQ(cr::code_name(static_cast<cr::ErrorCode>(-1)), "unknown_error");
}
