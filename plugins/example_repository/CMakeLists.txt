add_library(example_repository_plugin SHARED
    example_repository_plugin.cpp
)

target_link_libraries(example_repository_plugin PRIVATE component_registry)

target_compile_options(example_repository_plugin PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -fvisibility=hidden
)

set_target_properties(example_repository_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_link_options(example_repository_plugin PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/plugin_exports.version"
)

set_target_properties(example_repository_plugin PROPERTIES
    OUTPUT_NAME "example_repository_plugin"
)