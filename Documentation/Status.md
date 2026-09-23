# Implementation status

*Last reviewed: 2026-09-23, commit `a47d13b`, with Clang 21.1.8 and CMake 4.2.3.*

## Implemented

- `ComponentRegistry::register_factory`, `create` and `create_as`, including
  the `component_not_found`, `duplicate_registration` and
  `interface_mismatch` error paths.
- `ComponentRegistry::load_plugin`: `dlopen` with `RTLD_NOW | RTLD_LOCAL`,
  `dlsym` of both ABI entry points, the ABI version check and the call to
  `component_plugin_register`. It produces `plugin_load_failed`,
  `plugin_symbol_missing`, `plugin_abi_incompatible` and
  `plugin_registration_failed`. The registry lock is not held while the
  plugin registers, and the handle is kept once registration has run, even
  if it failed (ADR-0003).
- `StartupResolver`: `require` (writes the result into the caller's
  `shared_ptr`), collecting errors, `ok()`, `errors()` and `summary()`.
- The plugin ABI header and the `COMPONENT_REGISTRY_DEFINE_PLUGIN` macro.
- The example plugin, built with hidden visibility and a version script.
  `nm -D` shows exactly the two entry points.

The end-to-end test passes all 18 checks.

## Fixed defects

These were listed as known defects in the previous review (commit `7c8ebcb`).

| # | Where | Problem | Fix |
|---|---|---|---|
| 1 | `plugin_abi.hpp` (last line) | Trailing line-continuation backslash with no final newline broke the build under `-Werror`. | The macro ends cleanly and the file ends with a newline. |
| 2 | `core.cpp`, `load_plugin` | Only called `dlopen`, so no plugin factory was ever registered. | Resolves and calls both entry points, with the three plugin error codes. |
| 3 | `startup.hpp`, `require` | `out` was taken by value, so the result never reached the caller. | `out` is now `std::shared_ptr<Interface>&`. |
| 4 | `core.cpp`, `load_plugin` | The mutex was held for the whole call, so registering would deadlock. | The lock is taken only to store the handle. |
| 5 | `startup.cpp` | `code_name` had external linkage without a header declaration. | Moved into an anonymous namespace. |
| 6 | `core.cpp`, `load_plugin` | A plugin whose register function returned `false` was `dlclose`d, leaving any factories it had already registered pointing at unmapped code. | The handle is kept. Covered by the end-to-end test. |
| 7 | `startup.cpp`, `code_name` | No case for `unknown_error`, so `summary()` printed `[Unknown]`. | Every `ErrorCode` value has a case. |

## Known limitations

- The end-to-end test does not check the exported symbol surface
  (ADR-0007). Check it by hand with `nm -D` (see
  [Writing a plugin](PluginGuide.md)).
- The `plugin_symbol_missing` and `plugin_abi_incompatible` paths are
  implemented but have no test, because they need purpose-built broken
  plugins.
- Error messages from `load_plugin`, `StartupResolver::summary()` and the
  test output are in Spanish, while `register_factory`, `create`,
  `create_as` and the documentation are in English.

## Not yet started

- A vcpkg overlay port (`ports/component-registry/`), plus CMake install
  and export rules for `find_package(ComponentRegistry CONFIG REQUIRED)`.
- Schema and version migration for components whose shape changes over
  time.
- Hot-reload / `dlclose`. This is deliberately deferred (ADR-0003) and
  depends on first solving how library lifetime is tied to object lifetime.
- Non-POSIX platforms (Windows `LoadLibrary`).
