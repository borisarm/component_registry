# component_registry documentation

Start here if you want to understand, build, use or extend `component_registry`.
The top-level [README](../README.md) gives the short pitch; these pages go
into detail.

| Document | Read it when you want to… |
|---|---|
| [Architecture](Architecture.md) | understand the moving parts, how a component is resolved end to end, and why the design looks the way it does |
| [API reference](API.md) | look up a type, function, error code or macro in the public headers |
| [Writing a plugin](PluginGuide.md) | create a new adapter `.so` that the registry can load |
| [Building and testing](Building.md) | compile the library, the example plugin and the end-to-end test |
| [Implementation status](Status.md) | know what works today, what is still missing, and the known defects |
| [Architecture Decision Records](ADR.md) | know *why* a decision was made and which alternatives were rejected |

## Glossary

- **Port**: an abstract C++ interface, derived from `IComponent`, that a
  bounded context depends on (e.g. `IOpportunityRepository`).
- **Adapter**: a concrete class implementing a port (e.g.
  `PostgresOpportunityRepository`).
- **Component**: an adapter instance produced by the registry.
- **`ComponentId`**: the stable string that names *which* adapter to build
  (e.g. `"postgres.repository.opportunity"`). See ADR-0001.
- **Factory**: a callable registered under a `ComponentId` that returns a
  fresh `std::shared_ptr<IComponent>`.
- **Plugin**: a shared library (`.so`) that exports the two-function C ABI
  and registers one adapter's factories (ADR-0005, ADR-0008).
- **Composition root**: the startup code of the host application that wires
  ports to adapters. It is the intended caller of the registry (ADR-0002).
