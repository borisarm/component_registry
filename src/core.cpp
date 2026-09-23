#include "component_registry/core.hpp"
#include "component_registry/server_abi.hpp"

#include <format>
#include <dlfcn.h>

namespace component_registry {

    
    ReturnValue<void> ComponentRegistry::register_factory(const ComponentId& id, Factory factory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto [it, inserted] = factories_.try_emplace(id, std::move(factory));
        if(!inserted)
            return std::unexpected(Error { ErrorCode::duplicate_registration, std::format("Factory for component id {} is already registered", id) });

        return {};
    }


    ReturnValue<std::shared_ptr<IComponent>> ComponentRegistry::create(const ComponentId& id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = factories_.find(id);
        if(it == factories_.end())
            return std::unexpected(Error { ErrorCode::component_not_found, std::format("No factory registered for component id {}", id) });

        return it->second();
    }


    ReturnValue<void> ComponentRegistry::load_server(std::filesystem::path const& path)
    {
        // No se toma mutex_ aquí: component_plugin_register() llama a
        // register_factory(), que lo toma, y std::mutex no es recursivo.
        ::dlerror();  // limpiar cualquier error residual antes de empezar
        void* handle = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            char const* reason = ::dlerror();
            return std::unexpected(Error{
                ErrorCode::server_load_failed,
                path.string() + ": " + (reason ? reason : "razón desconocida")});
        }

        auto abi_version_fn = reinterpret_cast<ComponentServerAbiVersionFn>(
            ::dlsym(handle, k_abi_version_symbol));
        auto register_fn = reinterpret_cast<ComponentServerRegisterFn>(
            ::dlsym(handle, k_register_symbol));

        if (!abi_version_fn || !register_fn) {
            ::dlclose(handle);  // nunca llegó a registrar nada: sí cerramos aquí
            return std::unexpected(Error{
                ErrorCode::server_symbol_missing,
                path.string() + ": faltan símbolos del contrato del servidor"});
        }

        if (abi_version_fn().abi_version != k_abi_version) {
            ::dlclose(handle);
            return std::unexpected(Error{
                ErrorCode::server_abi_incompatible,
                path.string() + ": ABI del servidor incompatible con la del host "
                "(esperada " + std::to_string(k_abi_version) + ")"});
        }

        bool const registered = register_fn(*this);

       
        {
            std::lock_guard lock(mutex_);
            loaded_servers_.push_back(handle);
        }

        if (!registered) {
            return std::unexpected(Error{
                ErrorCode::server_registration_failed,
                path.string() + ": component_server_register devolvió false"});
        }
        return {};
    }

    [[nodiscard]] std::size_t ComponentRegistry::loaded_servers_count() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return loaded_servers_.size();

    }

}

