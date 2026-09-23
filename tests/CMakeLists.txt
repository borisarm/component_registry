find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

if(NOT TARGET example_repository_server)
    message(FATAL_ERROR
        "Los tests necesitan el server de ejemplo: activa COMPONENT_REGISTRY_BUILD_EXAMPLE_SERVER")
endif()

# Un único ejecutable; cada fichero contiene una sola clase de test (fixture).
add_executable(component_registry_tests
    end_to_end_test.cpp
    manifest/manifest_unreadable_test.cpp
    manifest/manifest_parsing_test.cpp
    manifest/manifest_path_resolution_test.cpp
    manifest/manifest_loading_test.cpp
    manifest/manifest_failures_test.cpp
    manifest/manifest_summary_test.cpp
)

target_link_libraries(component_registry_tests PRIVATE
    component_registry GTest::gtest_main GTest::gmock
)
target_include_directories(component_registry_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/support
    ${PROJECT_SOURCE_DIR}/servers/example_repository
)
target_compile_options(component_registry_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_definitions(component_registry_tests PRIVATE
    EXAMPLE_SERVER_PATH="$<TARGET_FILE:example_repository_server>"
)
add_dependencies(component_registry_tests example_repository_server)

# Cada TEST_F aparece como entrada propia en CTest con su propio informe JUnit XML.
gtest_discover_tests(component_registry_tests
    XML_OUTPUT_DIR "${CMAKE_BINARY_DIR}/test-reports"
    DISCOVERY_MODE PRE_TEST
)
