# API reference

Every public symbol lives in namespace `component_registry`, except the C
ABI types and the macros. Include paths are relative to `include/`.

- [`core.hpp`](#component_registrycorehpp)
- [`plugin_abi.hpp`](#component_registryplugin_abihpp)
- [`startup.hpp`](#component_registrystartuphpp)

---

## `component_registry/core.hpp`

### `using ComponentId = std::string;`
The stable name of an adapter. Use a dotted, namespaced convention such as
`"<technology>.<kind>.<entity>"`, for example
`"postgres.repository.opportunity"`. Ids may be persisted, so treat them as
part of your public contract.

### `class IComponent`
```cpp
class COMPONENT_REGISTRY_INTERFACE IComponent {
public:
    virtual ~IComponent() = default;
};
```
The base of every port. Derive your port interfaces from it, and mark each
one with `COMPONENT_REGISTRY_INTERFACE` (see ADR-0006).

### `COMPONENT_REGISTRY_INTERFACE`
Expands to `__attribute__((visibility("default")))` on GCC and Clang, and
to nothing elsewhere. Put it on **every** class that is `dynamic_cast` across
a `.so` boundary.

### `enum class ErrorCode`

| Value | Meaning |
|---|---|
| `component_not_found` | No factory is registered under the requested `ComponentId`. |
| `interface_mismatch` | The component exists but does not implement the requested interface, or the port class lacks default visibility. |
| `duplicate_registration` | `register_factory` was called twice with the same id. The first registration stays in place. |
| `plugin_load_failed` | `dlopen` failed. `detail` contains the `dlerror()` text. |
| `plugin_symbol_missing` | The plugin does not export one of the two ABI entry points. |
| `plugin_abi_incompatible` | The plugin reports a different `abi_version` from the host. |
| `plugin_registration_failed` | The plugin's `component_plugin_register` returned `false`. Factories it registered before failing stay registered. |
| `unknown_error` | Catch-all. |

### `struct Error`
```cpp
struct Error {
    ErrorCode   code;
    std::string detail;   // human-readable diagnostic
};
```

### `template<class T> using ReturnValue = std::expected<T, Error>;`
The return type of every fallible operation. Use it with the standard
`std::expected` monadic helpers (`and_then`, `transform`, `or_else`).

### `class ComponentRegistry`
Non-copyable and non-movable (it holds a `std::mutex`). All members are
safe to call concurrently. See *Thread safety* in [Architecture](Architecture.md).

```cpp
using Factory = std::function<std::shared_ptr<IComponent>()>;
```

| Member | Returns | Description |
|---|---|---|
| `register_factory(ComponentId const& id, Factory factory)` | `ReturnValue<void>` | Registers `factory` under `id`. Fails with `duplicate_registration` if `id` is already taken. |
| `create(ComponentId const& id) const` | `ReturnValue<std::shared_ptr<IComponent>>` | Calls the factory for `id` and returns a **new** instance. Fails with `component_not_found`. |
| `create_as<Interface>(ComponentId const& id) const` | `ReturnValue<std::shared_ptr<Interface>>` | Calls `create`, then `std::dynamic_pointer_cast<Interface>`. Fails with `component_not_found`, or with `interface_mismatch` when the cast yields `nullptr`. |
| `load_plugin(std::filesystem::path const& path)` | `ReturnValue<void>` | `dlopen`s `path` (`RTLD_NOW \| RTLD_LOCAL`), checks the ABI version and calls the plugin's `component_plugin_register`. Fails with `plugin_load_failed`, `plugin_symbol_missing`, `plugin_abi_incompatible` or `plugin_registration_failed`. |
| `loaded_plugin_count() const` | `std::size_t` | The number of plugin handles held. |

`load_plugin` keeps the handle for the life of the process once the plugin's
register function has run, **even if it returned `false`**, because factories
registered before the failure point into the plugin's code. On the two
earlier failures (missing symbol, ABI mismatch) no plugin code has touched
the registry, so the handle is closed. `loaded_plugin_count()` therefore
includes plugins whose registration failed.

All of these members are `[[nodiscard]]`.

> **Note:** a factory runs while the registry lock is held, so it must not
> call back into the same registry. `load_plugin` does not hold the lock while
> the plugin registers, so `register_factory` is safe there.

Example:

```cpp
using namespace component_registry;

ComponentRegistry registry;
if (auto r = registry.load_plugin("./example_repository_plugin.so"); !r)
    std::println(stderr, "{}", r.error().detail);

auto echo = registry.create_as<IEchoService>("example.echo_service");
if (echo)
    std::println("{}", (*echo)->echo("hi"));
else if (echo.error().code == ErrorCode::interface_mismatch)
    /* the wrong port was requested, or visibility is missing */;
```

---

## `component_registry/plugin_abi.hpp`

The binary contract between the host and a plugin (ADR-0005).

### Constants
| Name | Value | Purpose |
|---|---|---|
| `k_abi_version` | `1` | Increment it whenever the contract changes incompatibly. |
| `k_abi_version_symbol` | `"component_plugin_abi_version"` | The symbol name to `dlsym`. |
| `k_register_symbol` | `"component_plugin_register"` | The symbol name to `dlsym`. |

### C types
```cpp
extern "C" {
    struct ComponentPluginAbiInfo { std::uint32_t abi_version; };
    using ComponentPluginAbiVersionFn = ComponentPluginAbiInfo (*)();
    using ComponentPluginRegisterFn   = bool (*)(component_registry::ComponentRegistry&) noexcept;
}
```

### `COMPONENT_PLUGIN_API`
Default visibility for the exported entry points.

### `COMPONENT_REGISTRY_DEFINE_PLUGIN`
Generates both entry points. Put a function body right after it. Inside the
body, `registry` is a `ComponentRegistry&`. Return `true` on success.

```cpp
COMPONENT_REGISTRY_DEFINE_PLUGIN {
    return registry.register_factory(
        "example.echo_service",
        [] { return std::make_shared<InMemoryEchoService>(); }).has_value();
}
```

The body is `noexcept`. An exception that escapes it calls
`std::terminate`, so catch internally and return `false`.

---

## `component_registry/startup.hpp`

### `class StartupResolver`
A fluent helper for composition roots. It resolves every mandatory
dependency and collects the failures instead of stopping at the first one.

| Member | Description |
|---|---|
| `explicit StartupResolver(ComponentRegistry const&) noexcept` | Keeps a reference. The registry must outlive the resolver. |
| `template<class I> StartupResolver& require(ComponentId id, std::shared_ptr<I>& out)` | Calls `create_as<I>(id)`. On success, assigns the result to `out`. On failure, appends the error and leaves `out` unchanged. Returns `*this` so calls can be chained. |
| `bool ok() const noexcept` | `true` when no `require` has failed. |
| `std::span<Error const> errors() const` | The collected errors, in the order they occurred. |
| `std::string summary() const` | A multi-line report with one `[error_code] detail` line per failure. The text is in Spanish. |

The ADR says the composition root should exit the process when `ok()` is
false (ADR-0004). `StartupResolver` does not exit by itself; the caller
decides:

```cpp
if (!resolver.ok()) {
    std::cerr << resolver.summary();
    std::exit(EXIT_FAILURE);
}
```
