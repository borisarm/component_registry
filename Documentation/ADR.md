# Architecture Decision Records

These are the decisions this project embodies, in the order they were made. Each one exists because an alternative was seriously considered and rejected — the "why not" is as important as the "why."

## ADR-0001 — Identity and interface are separate concerns

A `ComponentId` (a stable, human-readable string) identifies *what to construct*. A C++ abstract base class (an `IComponent`-derived interface) identifies *what contract the caller expects*. This mirrors COM's CLSID/IID split.

`std::type_index` / RTTI was rejected as the identity type: it is not stable across recompilations or compiler/ABI versions, which matters because a `ComponentId` may end up persisted alongside a domain event and resolved months later against a rebuilt binary.

## ADR-0002 — Service Locator scope is deliberately narrow

The registry is the resolution mechanism a composition root uses for dependencies it cannot know at compile time. It is **not** meant to replace explicit constructor injection for the ordinary wiring inside a bounded context, where the composition root already knows which concrete adapter to use. Overusing it as a general-purpose locator would hide the dependency graph and work against the reasons hexagonal architecture favors explicit wiring in the first place.

## ADR-0003 — Load-once, no hot-reload (for now)

Plugins are loaded once at process startup and live until the process exits; there is no `dlclose` of an in-use plugin anywhere in this version. This sidesteps the hardest problem of dynamic loading — coupling an object's reference count to its owning library's lifetime so a `.so` is never unmapped while live objects still point into it — because that problem only exists if unloading is supported. Hot-reload is explicitly out of scope and left for a future iteration; if it's ever added, the lifetime-coupling problem has to be solved first, as a prerequisite, not a follow-up.

## ADR-0004 — No exceptions across the public API

Every operation that can fail (`register_factory`, `create`, `create_as`, `load_plugin`) returns `std::expected<T, Error>` instead of throwing.
This follows directly from the plugin ABI boundary rule below (ADR-0005): if a plugin's entry points must never let a C++ exception escape across the `extern "C"` boundary, it would be inconsistent for the host side to treat exceptions as the normal control-flow mechanism. `Error` carries an `ErrorCode` plus a human-readable `detail` string for diagnostics — the role `what()` used to play, kept as data rather than as a throw/catch mechanism.

The one deliberate exception to "no abrupt termination": `StartupResolver` is the single place in the system where failing loudly (`std::exit`) is correct — a composition root that can't resolve its mandatory dependencies has nothing useful left to do. Once the process is running, error handling goes back to `std::expected` at the point of use.

## ADR-0005 — The plugin ABI boundary is plain C

`plugin_abi.hpp` defines two `extern "C"` entry points (`component_plugin_abi_version`, `component_plugin_register`) that every plugin must export. This is deliberately C, not C++: name mangling isn't standardized across compilers, and while this project currently assumes a single pinned toolchain in the consuming project, keeping the boundary in C avoids baking that assumption into the contract itself. No C++ exception may cross this boundary; a plugin must translate any internal failure into `return false;`.

`component_plugin_abi_version()` exists so the host can reject a plugin built against an incompatible ABI *before* calling anything else that touches its own state — the same principle behind COM refusing an unknown IID before constructing anything.

## ADR-0006 — Interface visibility must survive the `.so` boundary

`dynamic_cast` across a shared-library boundary depends on the dynamic linker unifying `typeinfo` symbols between the host and the plugin. With `-fvisibility=hidden` (used on every plugin, see ADR-0007), that only happens if the interface classes are explicitly marked with default visibility. Every type that derives from `IComponent` and crosses this boundary — `IComponent` itself, and any port interface like the example's `IEchoService` — must carry the `COMPONENT_REGISTRY_INTERFACE` attribute defined in `core.hpp`. Without it, `dynamic_cast` fails silently by returning `nullptr` instead of erroring — the single most surprising failure mode this project has to guard against by convention, since the compiler cannot catch a missing attribute here.

## ADR-0007 — Plugin symbol surface is minimized at two layers

A plugin should expose nothing beyond its two ABI entry points. `-fvisibility=hidden` (plus `-fvisibility-inlines-hidden`) is necessary but not sufficient: several libstdc++ template classes hardcode default visibility in their own headers, which overrides the compiler flag. The definitive fix is a linker version script (`plugin_exports.version`) that explicitly whitelists the two entry point symbols and marks everything else `local`. Verified empirically for this project: compiler flags alone left ~30 template-instantiation symbols exported; the version script reduced that to exactly two.

## ADR-0008 — One plugin per adapter

Packaging granularity is per-adapter, not per-bounded-context. A `.so` per individual adapter (e.g. `postgres_opportunity_repository.so`) can be replaced without touching or recompiling any other component — which is the actual goal (breaking compile-time coupling *between components*, not just between contexts). The tradeoff is more artifacts to deploy and more `dlopen` calls at startup; that cost was accepted deliberately.
