# Building and testing

## Requirements

- Linux or another POSIX system with `dlopen` and an ELF linker that
  supports `--version-script`.
- A C++23 compiler with `<expected>` and `<format>`, such as GCC 13 or
  newer or a recent Clang with libstdc++. The Dev Container uses Clang
  21.1.8.
- CMake **4.2** or newer. `CMakeLists.txt` declares
  `cmake_minimum_required(VERSION 4.2)`.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Targets:

| Target | Type | Notes |
|---|---|---|
| `component_registry` (alias `ComponentRegistry::component_registry`) | static library | Built as PIC so plugins can link it. Links `${CMAKE_DL_LIBS}`. Compiled with `-Wall -Wextra -Wpedantic -Werror`. |
| `example_repository_plugin` | shared library | The sample adapter. Uses hidden visibility and the version script. |
| `end_to_end_test` | executable | Takes the plugin path as `argv[1]`. |

## CMake options

| Option | Default | Effect |
|---|---|---|
| `COMPONENT_REGISTRY_BUILD_EXAMPLE_PLUGIN` | `ON` | Builds `plugins/example_repository`. The `end_to_end` test is registered only when this is `ON`. |
| `COMPONENT_REGISTRY_BUILD_TESTS` | `ON` | Builds `tests/` and enables CTest. |
| `COMPONENT_REGISTRY_ENABLE_COVERAGE` | `OFF` | Instruments every target for Clang source-based coverage and adds the `coverage` target. Requires Clang. |

## Code coverage

Use a separate build directory, because instrumentation slows the build and
the tests:

```sh
cmake -S . -B build-cov -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug \
  -DCOMPONENT_REGISTRY_ENABLE_COVERAGE=ON
cmake --build build-cov --target coverage
```

The `coverage` target builds the tests, runs CTest, merges the profiles with
`llvm-profdata` and prints a per-file summary with `llvm-cov report`. It
also writes an HTML report with per-line and per-branch detail to
`build-cov/coverage/html/index.html`. The report covers the library,
the public headers and the example server. It leaves out test sources and
vcpkg dependencies.

CMake looks for `llvm-profdata` and `llvm-cov` that match the Clang major
version, such as `llvm-cov-21`, and falls back to the unversioned names.

## The end-to-end test

`tests/end_to_end_test.cpp` loads the real plugin `.so` and checks:

1. `load_plugin` succeeds and `loaded_plugin_count() == 1`.
2. `create_as<IEchoService>("example.echo_service")` resolves, and the
   result's `echo()` works.
3. An unknown id returns `component_not_found`.
4. Requesting an interface the adapter doesn't implement returns
   `interface_mismatch`.
5. `StartupResolver` succeeds when every dependency resolves, and when one
   fails it collects exactly that error and leaves the other outputs alone.
6. Loading the same plugin a second time makes its registration fail
   (`duplicate_registration` inside the plugin). `load_plugin` returns
   `plugin_registration_failed`, keeps the handle, and the factories already
   registered keep working.

It does not check the exported symbol surface (ADR-0007). Check that by hand
with `nm -D`, as described in [Writing a plugin](PluginGuide.md).

To run it by hand:

```sh
./build/tests/end_to_end_test ./build/plugins/example_repository/libexample_repository_plugin.so
```

It prints one `[ OK ]` or `[FAIL]` line per check (18 in total) and returns
a non-zero exit code if any check fails. The messages are in Spanish.

## Consuming from another project

A vcpkg overlay port and `find_package(ComponentRegistry CONFIG)` support
are not implemented yet. For now, add the project with
`add_subdirectory()` or `FetchContent` and link
`ComponentRegistry::component_registry`, with
`COMPONENT_REGISTRY_BUILD_TESTS=OFF` and
`COMPONENT_REGISTRY_BUILD_EXAMPLE_PLUGIN=OFF`.
