// ManifestResult: all_loaded() y summary().

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "component_registry/core.hpp"
#include "component_registry/manifest.hpp"

namespace cr = component_registry;
using ::testing::HasSubstr;
using ::testing::Not;

class ManifestSummaryTest : public ::testing::Test {
protected:
    void add_failure(std::string const& path, std::string const& detail) {
        result.failures.push_back(
            cr::ManifestEntryError{path, cr::Error{cr::ErrorCode::server_load_failed, detail}});
    }

    cr::ManifestResult result;
};

TEST_F(ManifestSummaryTest, DefaultResultIsAllLoaded) {
    EXPECT_EQ(result.loaded_count, 0u);
    EXPECT_TRUE(result.all_loaded());
}

TEST_F(ManifestSummaryTest, AnyFailureMeansNotAllLoaded) {
    result.loaded_count = 3;
    add_failure("a.so", "a.so: fallo");

    EXPECT_FALSE(result.all_loaded());
}

TEST_F(ManifestSummaryTest, SuccessSummaryReportsLoadedCount) {
    result.loaded_count = 2;

    EXPECT_EQ(result.summary(), "Manifest: 2 servers correctly loaded.");
}

TEST_F(ManifestSummaryTest, FailureSummaryReportsCountsAndEachDetail) {
    result.loaded_count = 1;
    add_failure("a.so", "a.so: primer fallo");
    add_failure("b.so", "b.so: segundo fallo");

    auto const summary = result.summary();

    EXPECT_THAT(summary, HasSubstr("1 servers correctly loaded"));
    EXPECT_THAT(summary, HasSubstr("2 failures"));
    EXPECT_THAT(summary, HasSubstr(" - a.so: primer fallo\n"));
    EXPECT_THAT(summary, HasSubstr(" - b.so: segundo fallo\n"));
    EXPECT_LT(summary.find("primer fallo"), summary.find("segundo fallo"));
}

TEST_F(ManifestSummaryTest, SuccessSummaryHasNoFailureSection) {
    result.loaded_count = 1;

    EXPECT_THAT(result.summary(), Not(HasSubstr("failures")));
}
