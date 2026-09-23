# Writing a plugin

A plugin is a shared library that implements **one adapter** (ADR-0008) and
exports exactly two C symbols. Use `plugins/example_repository/` as a
template.

## 1. Define (or reuse) the port header

The port is an abstract class shared by the host and the plugin. It must
derive from `IComponent` and carry `COMPONENT_REGISTRY_INTERFACE`:

```cpp
// echo_service_interface.hpp
#pragma once
#include <string>
#include <string_view>
#include "component_registry/core.hpp"

class COMPONENT_REGISTRY_INTERFACE IEchoService : public component_registry::IComponent {
public:
    virtual std::string echo(std::string_view message) const = 0;
};
```

> ⚠️ If `COMPONENT_REGISTRY_INTERFACE` is missing, `create_as<IEchoService>`
> returns `interface_mismatch`, even though the plugin clearly implements
> the interface. The compiler cannot warn you about this (ADR-0006).

Keep port headers free of adapter details. They belong to the bounded
context, not to the plugin.

## 2. Implement the adapter and the entry points

```cpp
// example_repository_plugin.cpp
#include "component_registry/plugin_abi.hpp"
#include "echo_service_interface.hpp"

namespace {
class InMemoryEchoService final : public IEchoService {
public:
    std::string echo(std::string_view message) const override {
        return "echo: " + std::string(message);
    }
};
}  // namespace

COMPONENT_REGISTRY_DEFINE_PLUGIN {
    return registry
        .register_factory("example.echo_service",
                          [] { return std::make_shared<InMemoryEchoService>(); })
        .has_value();
}
```

Rules:

- Put the concrete class in an anonymous namespace. Nothing outside the
  plugin should name it.
- Never let an exception escape the macro body. Wrap risky setup in
  `try { … } catch (...) { return false; }`.
- Don't call back into `registry` from inside a factory lambda. The
  registry lock is held while factories run.
- Choose a `ComponentId` that is unique across **all** plugins the host
  loads. A clash fails the second registration with
  `duplicate_registration`.

## 3. Build it with a minimal symbol surface

```cmake
add_library(my_adapter_plugin SHARED my_adapter_plugin.cpp)
target_link_libraries(my_adapter_plugin PRIVATE component_registry)

set_target_properties(my_adapter_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

target_link_options(my_adapter_plugin PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/plugin_exports.version")
```

`plugin_exports.version`:

```
COMPONENT_PLUGIN_ABI_1 {
    global:
        component_plugin_abi_version;
        component_plugin_register;
    local:
        *;
};
```

Check the result. The command below should list exactly the two entry
points:

```sh
nm -D --defined-only libmy_adapter_plugin.so | grep ' T '
```

Port `typeinfo` symbols (`_ZTI…`) may also appear. That is expected and
required by ADR-0006.

## 4. Load it from the host

```cpp
if (auto r = registry.load_plugin("plugins/libmy_adapter_plugin.so"); !r) {
    std::cerr << r.error().detail << '\n';
    return EXIT_FAILURE;
}
```

The host must use the same compiler and standard library as the plugin (see
*Scope limits* in [Architecture](Architecture.md)).

## Checklist

- [ ] The port derives from `IComponent` and is marked
      `COMPONENT_REGISTRY_INTERFACE`.
- [ ] The adapter is in an anonymous namespace.
- [ ] `COMPONENT_REGISTRY_DEFINE_PLUGIN` is used exactly once, and it
      returns `false` on failure instead of throwing.
- [ ] The `ComponentId` is globally unique and documented.
- [ ] Hidden visibility and the version script are applied, and `nm -D`
      shows only the two entry points.
