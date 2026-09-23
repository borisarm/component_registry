#include "component_registry/core.hpp"

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


    ReturnValue<void> ComponentRegistry::load_plugin(std::filesystem::path const& path)
    {

        ::dlerror();

        void* handle = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if(!handle)
        {
            const char* error = ::dlerror();
            if(error){
                return std::unexpected(Error { ErrorCode::plugin_load_failure, std::format("Failed to load plugin from path {}: {}", path.string(), error) });
            } else {
                return std::unexpected(Error { ErrorCode::plugin_load_failure, std::format("Failed to load plugin from path {}: unknown error", path.string()) });
            }
        }


        std::lock_guard<std::mutex> lock(mutex_);
        
        return {};
    }

}

