# component_registry

A runtime component registry for C++23 modular monolith / hexagonal
architectures in another project. It resolves the "port → adapter"
indirection at process startup, without requiring the implementation of a
port to be known at compile time by the code that consumes it.

The design deliberately borrows the *useful* half of the Windows COM
model — identity-based resolution, a stable interface contract, in-process
loading — while discarding everything that existed only to support
cross-process or cross-language marshaling (IDL, apartments, distributed
reference counting). Nothing here crosses a process boundary; everything
crosses a `.so` boundary within the same process, which is a much easier
problem.

## Why this exists

In a hexagonal architecture, a bounded context defines ports (interfaces)
and depends on adapters (implementations) that satisfy them. The usual way
to wire a port to an adapter is constructor injection from an explicit
composition root — and that remains the right tool whenever the
composition root already knows, at compile time, which adapter to use.

`component_registry` exists for the cases where it *doesn't* know that at
compile time:

- Selecting an adapter by external configuration without recompiling
  (`InMemoryRepository` in tests vs. `PostgresRepository` in production).
- Loading an implementation whose translation unit didn't even exist when
  the host binary was built — the actual goal of this project: breaking
  the compile-time dependency between components, not just hiding it
  behind a closed `std::variant` or a static factory table.
- Dispatching by a runtime-known type id (e.g. a domain event id read back
  from a message queue or from event-sourcing storage).

This is intentionally a **Service Locator**, not a replacement for
dependency injection. It is the tool a composition root reaches for when
it cannot express a dependency at compile time — not a substitute for
explicit wiring everywhere else. See the architecture decisions in
[Documentation/ADR.md](Documentation/ADR.md).

## How it works, in one paragraph

Each adapter lives in its own shared library (`.so`), compiled
independently and never linked into the host binary at compile time. The
host loads it dynamically (`dlopen`) at startup, resolves two `extern "C"`
entry points that make up a small, versioned ABI contract, and lets the
plugin register its factories against a `ComponentRegistry` instance
passed in by reference. From then on, the host asks the registry for a
`ComponentId` and the interface it expects, and gets back a
`std::shared_ptr<Interface>` — or a `std::expected` error it can act on,
never an exception.

## Documentation

Full project documentation lives in [Documentation/](Documentation/README.md):

- [Architecture](Documentation/Architecture.md): components, resolution lifecycle, thread safety and lifetime
- [API reference](Documentation/API.md): `core.hpp`, `plugin_abi.hpp`, `startup.hpp`
- [Writing a plugin](Documentation/PluginGuide.md): step-by-step guide and checklist
- [Building and testing](Documentation/Building.md): requirements, CMake options, the end-to-end test
- [Implementation status](Documentation/Status.md): what works, known defects, roadmap
- [Architecture Decision Records](Documentation/ADR.md): why the design is the way it is


## Directory layout

```
component_registry/
├── Documentation/        # architecture, API, plugin guide, building, status, ADRs
├── include/component_registry/
│   ├── core.hpp          # ComponentId, IComponent, Error, ComponentRegistry
│   ├── plugin_abi.hpp    # extern "C" plugin contract + COMPONENT_REGISTRY_DEFINE_PLUGIN macro
│   └── startup.hpp       # StartupResolver
├── src/
│   ├── core.cpp          # register_factory / create / load_plugin (dlopen/dlsym)
│   └── startup.cpp       # StartupResolver::summary()
├── plugins/example_repository/
│   ├── echo_service_interface.hpp   # example port shared between host and plugin
│   ├── example_repository_plugin.cpp
│   ├── plugin_exports.version       # linker version script (ADR-0007)
│   └── CMakeLists.txt
├── tests/
│   ├── end_to_end_test.cpp  # loads the real .so, exercises success and error paths
│   └── CMakeLists.txt
└── CMakeLists.txt
```

## Building

Requires a C++23 compiler with `<expected>` support (verified against
Clang 21.1.8 with libstdc++) and CMake ≥ 4.2. See
[Documentation/Building.md](Documentation/Building.md) for options and details.

```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
ctest --output-on-failure
```

This builds the static `component_registry` library, the
`example_repository_plugin.so` used for the end-to-end test, and the test
itself, then runs it via `ctest`.

## Minimal usage example

```cpp
component_registry::ComponentRegistry registry;

if (auto loaded = registry.load_plugin("./plugins/postgres_repository.so"); !loaded) {
    log_fatal(loaded.error().detail);
    std::exit(EXIT_FAILURE);
}

std::shared_ptr<IOpportunityRepository> repo;
std::shared_ptr<IEventPublisher> publisher;

component_registry::StartupResolver resolver{registry};
resolver.require("postgres.repository.opportunity", repo)
        .require("rabbitmq.publisher.domain_events", publisher);

if (!resolver.ok()) {
    log_fatal(resolver.summary());
    std::exit(EXIT_FAILURE);
}
```

A plugin implementing a port looks like this:

```cpp
// my_plugin.cpp
#include "component_registry/plugin_abi.hpp"
#include "i_opportunity_repository.hpp"  // the shared port header

namespace {
class PostgresOpportunityRepository final : public IOpportunityRepository {
    // ...
};
}

COMPONENT_REGISTRY_DEFINE_PLUGIN {
    return registry
        .register_factory(
            "postgres.repository.opportunity",
            [] { return std::make_shared<PostgresOpportunityRepository>(); })
        .has_value();
}
```

## Status

The core resolution cycle (register → create → `create_as` → `load_plugin` →
`StartupResolver`) is implemented and covered by an end-to-end test that
exercises a real dynamically loaded `.so`, including the error paths
(missing component, interface mismatch, failed plugin registration) and
cross-`.so` `dynamic_cast` (ADR-0006). The plugin symbol surface
(ADR-0007) is verified by hand with `nm -D`, not by the test. See
[Documentation/Status.md](Documentation/Status.md) for details.

Not yet done:
- vcpkg overlay port (`ports/component-registry/`) so another project can
  use `find_package(ComponentRegistry CONFIG REQUIRED)`.
- Schema/version migration for components whose shape changes over time
  (originally the motivating use case, before the scope widened to a
  general port/adapter resolution mechanism — see conversation history).
- Hot-reload (explicitly deferred, ADR-0003).

## License

This project is licensed under the [MIT License](LICENSE).