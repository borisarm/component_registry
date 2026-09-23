#pragma once

// Utilidades compartidas por los tests: rutas de los artefactos de build y
// directorios temporales autolimpiables donde escribir manifiestos.

#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string_view>

#include <gtest/gtest.h>

namespace component_registry::testing {

// Ruta absoluta al .so del server de ejemplo, inyectada por CMake.
inline std::filesystem::path example_server_path() {
    return std::filesystem::path{EXAMPLE_SERVER_PATH};
}

// Servers defectuosos de tests/support/servers, también inyectados por CMake.
inline std::filesystem::path no_abi_symbol_server_path() {
    return std::filesystem::path{NO_ABI_SYMBOL_SERVER_PATH};
}

inline std::filesystem::path no_register_symbol_server_path() {
    return std::filesystem::path{NO_REGISTER_SYMBOL_SERVER_PATH};
}

inline std::filesystem::path incompatible_abi_server_path() {
    return std::filesystem::path{INCOMPATIBLE_ABI_SERVER_PATH};
}

// Directorio temporal único que se borra al salir de ámbito.
class TempDirectory {
public:
    TempDirectory() {
        std::random_device random;
        path_ = std::filesystem::temp_directory_path() /
                std::format("component_registry_test_{:08x}{:08x}", random(), random());
        std::filesystem::create_directories(path_);
    }

    ~TempDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    TempDirectory(TempDirectory const&) = delete;
    TempDirectory& operator=(TempDirectory const&) = delete;

    [[nodiscard]] std::filesystem::path const& path() const noexcept { return path_; }

    // Escribe 'content' en 'relative' (creando subdirectorios) y devuelve la
    // ruta absoluta del fichero. Se escribe en binario para que los tests
    // controlen exactamente los finales de línea.
    std::filesystem::path write_file(std::filesystem::path const& relative,
                                     std::string_view content) const {
        auto const full = path_ / relative;
        std::filesystem::create_directories(full.parent_path());
        std::ofstream out(full, std::ios::binary);
        out << content;
        EXPECT_TRUE(out.good()) << "no se pudo escribir " << full;
        return full;
    }

    // Copia el server de ejemplo a 'relative' y devuelve la ruta absoluta.
    std::filesystem::path copy_example_server(std::filesystem::path const& relative) const {
        auto const full = path_ / relative;
        std::filesystem::create_directories(full.parent_path());
        std::filesystem::copy_file(example_server_path(), full,
                                   std::filesystem::copy_options::overwrite_existing);
        return full;
    }

private:
    std::filesystem::path path_;
};

}  // namespace component_registry::testing
