#include <component_registry/manifest.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>


namespace component_registry {
    namespace {
        std::string trim(std::string s) {
            auto not_space = [](unsigned char c) { return !std::isspace(c); };
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
            s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
            return s;
        }
    }


    std::string ManifestResult::summary() const {
        std::ostringstream out;
        out << "Manifest: " << loaded_count << " servers correctly loaded";
        if(failures.empty())
        {
            out << ".";
            return out.str();
        }
        out << ", " << failures.size() << " failures.\n";
        for(auto const& failure : failures)
            out << " - " << failure.error.detail << "\n";

        return out.str();
    }

    std::expected<ManifestResult, Error> load_servers_from_manifest(ComponentRegistry& registry, std::filesystem::path const& manifest_path)
    {
        // ifstream abre directorios sin error en Linux: hay que descartarlos antes.
        std::ifstream file;
        std::error_code ec;
        if(std::filesystem::is_regular_file(manifest_path, ec))
            file.open(manifest_path);
        if(!file.is_open())
            return std::unexpected(Error{ErrorCode::manifest_unreadable, std::format("Failed to open manifest file {}", manifest_path.string())});

        // Base absoluta: con un manifiesto relativo sin directorio, parent_path()
        // sería vacío y dlopen buscaría la entrada en LD_LIBRARY_PATH.
        auto const manifest_dir = std::filesystem::absolute(manifest_path).parent_path();

        ManifestResult result;

        std::string line;
        while(std::getline(file, line))
        {
            auto trimmed = trim(line);
            if(trimmed.empty()|| trimmed.front() == '#')
            {
                continue;
            }

            std::filesystem::path server_path = trimmed;
            if(server_path.is_relative()) 
            {
                server_path = manifest_dir / server_path;
            }

            auto load_result = registry.load_server(server_path);
            if(load_result)
                ++result.loaded_count;
            else
            {
                result.failures.push_back(
                    ManifestEntryError{server_path, std::move(load_result).error()});
            }
          
        }

        return result;
    }
    
}