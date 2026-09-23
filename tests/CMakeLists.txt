add_executable(end_to_end_test end_to_end_test.cpp)

target_link_libraries(end_to_end_test PRIVATE component_registry)
target_include_directories(end_to_end_test PRIVATE
    ${CMAKE_SOURCE_DIR}/plugins/example_repository
)
target_compile_options(end_to_end_test PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(COMPONENT_REGISTRY_BUILD_EXAMPLE_PLUGIN)
    add_test(
        NAME end_to_end
        COMMAND end_to_end_test $<TARGET_FILE:example_repository_plugin>
    )
    add_dependencies(end_to_end_test example_repository_plugin)
endif()