#pragma once

#include <filesystem>
#include <vector>

#include "component_registry/core.hpp"

namespace component_registry {

    struct ManifestEntryError {
        std::filesystem::path server_path;
        Error error;
    };

    struct ManifestResult {
        std::size_t loaded_count{0};
        std::vector<ManifestEntryError> failures;

        [[nodiscard]] bool all_loaded() const noexcept { return failures.empty(); }
        [[nodiscard]] std::string summary() const;

    };

    [[nodiscard]] std::expected<ManifestResult, Error> load_servers_from_manifest(ComponentRegistry& registry, std::filesystem::path const& manifest_path);

}