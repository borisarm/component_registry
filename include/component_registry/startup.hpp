#pragma once

#include <span>
#include <string>
#include <vector>

#include <component_registry/core.hpp>

namespace component_registry {

    class StartupResolver {
        public:
            explicit StartupResolver(ComponentRegistry const& registry) noexcept
                : registry_(registry) {}

            template <typename Interface>
            StartupResolver& require(ComponentId id, std::shared_ptr<Interface>& out)  {
                auto result = registry_.create_as<Interface>(id);
                if (result) {
                    out = std::move(*result);
                } else {
                    errors_.push_back(std::move(result).error());
                }
                return *this;
            }

            [[nodiscard]] bool ok() const noexcept {
                return errors_.empty();
            }


            [[nodiscard]] std::span<Error const> errors() const {
                return errors_;
            }

            [[nodiscard]] std::string summary()  const;



    private:
        ComponentRegistry const& registry_;
        std::vector<Error> errors_;
    };
}