# Implementation status

*Last reviewed: 2026-09-23, commit `7c8ebcb`, with GCC 15.2 and CMake 4.2.3.*

## Implemented

- `ComponentRegistry::register_factory`, `create` and `create_as`, including
  the `component_not_found`, `duplicate_registration` and
  `interface_mismatch` error paths.
- `ComponentRegistry::load_plugin`: `dlopen` with `RTLD_NOW | RTLD_LOCAL`,
  `dlerror` reporting, and keeping the handle.
- `StartupResolver`: collecting errors, `ok()`, `errors()` and
  `summary()`.
- The plugin ABI header and the `COMPONENT_REGISTRY_DEFINE_PLUGIN` macro.
- The example plugin, built with hidden visibility and a version script.

## Known defects

These defects make the end-to-end test fail on the current `master`.

| # | Where | Problem | Effect |
|---|---|---|---|
| 1 | `include/component_registry/plugin_abi.hpp` (last line) | The file ends with a line-continuation backslash and no trailing newline. | GCC reports `backslash-newline at end of file`. With `-Werror`, the plugin, and therefore the test, fails to build. |
| 2 | `src/core.cpp`, `load_plugin` | Only calls `dlopen`. It never `dlsym`s `component_plugin_abi_version` or `component_plugin_register`, and never calls either one. | No plugin factory is ever registered, so `create_as` on a plugin-provided id returns `component_not_found`. The `plugin_symbol_missing`, `plugin_abi_incompatible` and `plugin_registration_failed` error codes are never produced. |
| 3 | `include/component_registry/startup.hpp`, `require` | `out` is a by-value `std::shared_ptr<Interface>`. | A resolved dependency is assigned to a local copy and discarded, so the caller's pointer stays `nullptr`. It should be `std::shared_ptr<Interface>& out`. |
| 4 | `src/core.cpp`, `load_plugin` (once #2 is fixed) | The registry mutex is held for the whole call. | If `component_plugin_register` runs under that lock, its call to `register_factory` would deadlock. Release the lock before calling into the plugin, or register through an unlocked internal path. |
| 5 | `src/startup.cpp` | `code_name` has external linkage but is not declared in any header. | It pollutes the library's symbol namespace. Make it `static` or put it in an anonymous namespace. |

Test output after fixing #1 only, in a scratch copy: 8 of 13 checks pass.
Every check that depends on the plugin registering its factory fails (#2),
as do the `StartupResolver` output checks (#3).

## Documentation drift

- ADR-0004 says `StartupResolver` is "the single place where failing loudly
  (`std::exit`) is correct". In practice `StartupResolver` never exits; the
  composition root does, based on `ok()`. That is the reasonable design,
  but the ADR wording should be clarified.
- `StartupResolver::summary()` and the test output are in Spanish, while the
  rest of the project is in English.

## Not yet started

- A vcpkg overlay port (`ports/component-registry/`), plus CMake install
  and export rules for `find_package(ComponentRegistry CONFIG REQUIRED)`.
- Schema and version migration for components whose shape changes over
  time.
- Hot-reload / `dlclose`. This is deliberately deferred (ADR-0003) and
  depends on first solving how library lifetime is tied to object lifetime.
- Non-POSIX platforms (Windows `LoadLibrary`).
