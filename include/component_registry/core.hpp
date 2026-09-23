#pragma once

#include <string>
#include <memory>
#include <functional>
#include <expected>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <vector>


#if defined(__GNUC__) || defined(__clang__)
#define COMPONENT_REGISTRY_INTERFACE __attribute__((visibility("default")))
#else
#define COMPONENT_REGISTRY_INTERFACE
#endif

namespace component_registry {


    using ComponentId = std::string;


    class COMPONENT_REGISTRY_INTERFACE IComponent {
    public:
        virtual ~IComponent() = default;
    };  


    enum class ErrorCode {
        component_not_found,
        interface_mismatch,
        duplicate_registration,
        server_load_failed,
        server_symbol_missing,
        server_abi_incompatible,
        server_registration_failed,
        manifest_unreadable,
        unknown_error
    };


    struct Error {
        ErrorCode code;
        std::string detail;
    };

    template<typename successType>
    using ReturnValue = std::expected<successType, Error>;

    

    class ComponentRegistry {
        public:
        using Factory = std::function<std::shared_ptr<IComponent>()>;

        ComponentRegistry() = default;

        ComponentRegistry(const ComponentRegistry&) = delete;
        ComponentRegistry& operator=(const ComponentRegistry&) = delete;

        [[nodiscard]] ReturnValue<void> register_factory(const ComponentId& id, Factory factory);

        [[nodiscard]] ReturnValue<std::shared_ptr<IComponent>>  create(ComponentId const& id) const;

        template<typename Interface>
        [[nodiscard]] ReturnValue<std::shared_ptr<Interface>>  create_as(ComponentId const& id) const{
            return create(id).and_then(
                [](std::shared_ptr<IComponent> comp) -> ReturnValue<std::shared_ptr<Interface>> {
                auto casted = std::dynamic_pointer_cast<Interface>(comp);
                if(!casted) {
                    return std::unexpected(Error{ErrorCode::interface_mismatch, "Failed to cast component to requested interface"});
                }
                return casted;
            });
        }

        [[nodiscard]] ReturnValue<void> load_server(std::filesystem::path const& path);


        [[nodiscard]] std::size_t loaded_servers_count() const;

        private:

        mutable std::mutex mutex_;
        std::unordered_map<ComponentId, Factory> factories_;
        std::vector<void *> loaded_servers_; 

    };

}
