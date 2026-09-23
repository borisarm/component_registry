# Architecture

`component_registry` is an in-process Service Locator. A host application
uses it to bind hexagonal-architecture ports to adapters at startup when it
cannot make that binding at compile time. Adapters live in independently
built shared libraries that the host loads with `dlopen`.

The design keeps two ideas from Windows COM: resolution by identity, and a
small, stable binary contract. It drops everything COM needs only for
cross-process or cross-language work, such as IDL, apartments and
marshaling. Everything happens inside one process, across `.so` boundaries.

## Components

```
            host executable                              plugin .so (one per adapter)
┌──────────────────────────────────────────┐     ┌─────────────────────────────────────┐
│ composition root                         │     │ COMPONENT_REGISTRY_DEFINE_PLUGIN     │
│   └─ StartupResolver ──require<I>(id)──┐ │     │   component_plugin_abi_version()    │
│                                        ▼ │     │   component_plugin_register(reg) ───┼─┐
│ ComponentRegistry                        │     │                                     │ │
│   factories_ : id → Factory  ◄───────────┼─────┼──── register_factory(id, lambda) ◄──┼─┘
│   loaded_plugins_ : dlopen handles       │     │                                     │
│   load_plugin(path) ──dlopen/dlsym──────►┼────►│ class Adapter : public IPort        │
└──────────────────────────────────────────┘     └─────────────────────────────────────┘
                     ▲                                          ▲
                     └───────── shared port header (IPort : IComponent) ─┘
```

| Piece | Where | Role |
|---|---|---|
| `IComponent` | `include/component_registry/core.hpp` | Polymorphic root of every port. It has a virtual destructor, so `dynamic_pointer_cast` works across it. |
| `ComponentId` | `core.hpp` | `std::string` alias that names an adapter. It stays the same across rebuilds (ADR-0001). |
| `ComponentRegistry` | `core.hpp`, `src/core.cpp` | Holds the id → factory map and the handles of the loaded plugins. It is non-copyable and every method takes an internal mutex. |
| Plugin ABI | `include/component_registry/plugin_abi.hpp` | The two `extern "C"` entry points and the `COMPONENT_REGISTRY_DEFINE_PLUGIN` macro that generates them. |
| `StartupResolver` | `include/component_registry/startup.hpp`, `src/startup.cpp` | Resolves a list of mandatory dependencies in one pass. It collects every error, so the composition root can report all of them at once. |
| Example plugin | `plugins/example_repository/` | The `IEchoService` port and its `InMemoryEchoService` adapter, built as `example_repository_plugin.so`. |

## Resolution lifecycle

The flow at process startup. Every step is implemented.

1. **Load.** The host calls `registry.load_plugin("path/to/adapter.so")`.
   The registry `dlopen`s the file with `RTLD_NOW | RTLD_LOCAL`, so an
   unresolved symbol fails at load time instead of on first use.
2. **ABI check.** The registry looks up `component_plugin_abi_version`. It
   compares the result with `k_abi_version` and rejects a mismatch
   (`plugin_abi_incompatible`) before any plugin code touches registry
   state (ADR-0005). If either entry point is missing, loading fails with
   `plugin_symbol_missing`. In both cases the handle is closed, which is
   safe because nothing from the plugin was registered.
3. **Register.** The registry looks up `component_plugin_register` and
   calls it with `*this`. The plugin calls `register_factory` once per
   adapter it provides, then returns `true`, or `false` on any failure.
   The function is `noexcept`, so no exception can cross the boundary.
   If it returns `false`, `load_plugin` reports `plugin_registration_failed`,
   but any factories registered before the failure remain.
4. **Retain.** From this point the registry keeps the `dlopen` handle until
   the process exits, whether or not registration succeeded. Plugins are
   never unloaded (ADR-0003).
5. **Resolve.** The composition root asks for each dependency with
   `create_as<IPort>(id)`, usually through `StartupResolver::require`. The
   registry runs the factory, then `dynamic_pointer_cast`s the
   `shared_ptr<IComponent>` to the requested port.
6. **Fail fast or run.** If `StartupResolver::ok()` is false, the host logs
   `summary()` and exits. Otherwise the resolved `shared_ptr`s are passed
   into the application through ordinary constructor injection.

## Key design properties

### Identity is separate from the interface (ADR-0001)
The `ComponentId` says *what to build*. The template argument of
`create_as` says *what contract the caller expects*. So one id can be
requested through any port the adapter implements, and asking for the wrong
port returns `interface_mismatch` instead of undefined behavior.

### No exceptions in the API (ADR-0004)
Every operation that can fail returns
`component_registry::ReturnValue<T>`, an alias for `std::expected<T, Error>`.
An `Error` holds an `ErrorCode` for programmatic handling and a `detail`
string for humans.

### Type identity has to survive the `.so` boundary (ADR-0006)
`dynamic_cast` between the host and a plugin works only if both sides agree
on the port's `typeinfo`. Plugins are built with hidden visibility, so every
port class needs `COMPONENT_REGISTRY_INTERFACE` to make its `typeinfo`
visible. Without it, the cast fails *silently* and `create_as` reports
`interface_mismatch`.

### Minimal plugin symbol surface (ADR-0007)
Each plugin exports exactly two symbols. Compiler visibility flags are not
enough on their own, because libstdc++ forces default visibility on some
templates. A linker version script (`plugin_exports.version`) enforces the
limit.

### Thread safety
`register_factory`, `create` and `loaded_plugin_count` each take the same
mutex, so concurrent calls are safe. The factory runs while that lock is
held. A factory must therefore **not** call back into the same registry,
because `std::mutex` is not recursive and the call would deadlock.

`load_plugin` does **not** hold the lock while it calls into the plugin:
`component_plugin_register` calls `register_factory`, which takes the lock
itself. `load_plugin` locks only to store the handle.

### Lifetime
Components are handed out as `std::shared_ptr` and are never cached: every
`create` call builds a new instance. Plugins are never unloaded, so an
object's vtable and code stay mapped for as long as the process runs.
Hot-reload would need library lifetime tied to object lifetime, and it is
explicitly out of scope (ADR-0003).

## Scope limits

- Linux / POSIX only (`dlopen`, `dlsym`, ELF version scripts).
- Single toolchain. Host and plugins must use the same compiler and
  standard library, because C++ types such as `std::shared_ptr`,
  `std::function` and `ComponentRegistry&` cross the boundary. Only the
  *entry points* are C.
- This is not a general DI container. Use constructor injection wherever
  the composition root knows the adapter at compile time (ADR-0002).
