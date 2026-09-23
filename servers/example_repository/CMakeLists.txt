add_library(example_repository_server SHARED
    example_repository_server.cpp
)
    
target_link_libraries(example_repository_server PRIVATE component_registry)

target_compile_options(example_repository_server PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -fvisibility=hidden
)

set_target_properties(example_repository_server PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_link_options(example_repository_server PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/server_exports.version"
)

set_target_properties(example_repository_server PROPERTIES
    OUTPUT_NAME "example_repository_server"
)