#include "component_registry/server_abi.hpp"
#include "echo_service_interface.hpp"

namespace {

class InMemoryEchoService final : public IEchoService {
public:
    std::string echo(std::string_view message) const override {
        return "echo: " + std::string(message);
    }
};

}  // namespace
COMPONENT_REGISTRY_DEFINE_SERVER {
    return registry
        .register_factory(
            "example.echo_service",
            [] { return std::make_shared<InMemoryEchoService>(); })
        .has_value();
}