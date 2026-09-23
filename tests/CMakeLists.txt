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

# Informe de cobertura: `cmake --build <dir> --target coverage`.
# Ejecuta CTest con perfiles por proceso, los fusiona y genera el resumen
# en consola y el informe HTML en <dir>/coverage/html.
if(COMPONENT_REGISTRY_ENABLE_COVERAGE)
    string(REGEX MATCH "^[0-9]+" _clang_major "${CMAKE_CXX_COMPILER_VERSION}")
    find_program(LLVM_PROFDATA NAMES llvm-profdata-${_clang_major} llvm-profdata REQUIRED)
    find_program(LLVM_COV NAMES llvm-cov-${_clang_major} llvm-cov REQUIRED)

    set(_cov_dir "${CMAKE_BINARY_DIR}/coverage")
    set(_cov_objects
        "$<TARGET_FILE:component_registry_tests>"
        -object "$<TARGET_FILE:example_repository_server>"
    )
    set(_cov_filters
        -instr-profile=${_cov_dir}/coverage.profdata
        "-ignore-filename-regex=(vcpkg_installed|/tests/)"
    )

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${_cov_dir}
        COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${_cov_dir}/raw/%p.profraw
                ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
        COMMAND ${LLVM_PROFDATA} merge -sparse ${_cov_dir}/raw -o ${_cov_dir}/coverage.profdata
        COMMAND ${LLVM_COV} report ${_cov_objects} ${_cov_filters}
        COMMAND ${LLVM_COV} show ${_cov_objects} ${_cov_filters}
                -format=html -show-branches=count -output-dir=${_cov_dir}/html
        COMMAND ${CMAKE_COMMAND} -E echo "Informe HTML: ${_cov_dir}/html/index.html"
        DEPENDS component_registry_tests example_repository_server
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generando informe de cobertura"
        VERBATIM
    )
endif()
