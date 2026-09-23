CMAKE_MINIMUM_REQUIRED(VERSION 4.2)

# Dependencias gestionadas por vcpkg en modo manifiesto (vcpkg.json).
# Debe fijarse antes de project() para que el toolchain se aplique.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "vcpkg toolchain file")
endif()

project(component_registry
    VERSION 1.0
    LANGUAGES CXX
    DESCRIPTION "Component Registry Library"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(COMPONENT_REGISTRY_BUILD_TESTS "Compile component registry tests" ON)
option(COMPONENT_REGISTRY_BUILD_EXAMPLE_SERVER "Compile sample servers" ON)

add_library(component_registry STATIC
    src/core.cpp
    src/startup.cpp
    src/manifest.cpp
)

add_library(ComponentRegistry::component_registry ALIAS component_registry)

target_include_directories(component_registry
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


target_compile_features(component_registry PUBLIC cxx_std_23)

target_link_libraries(component_registry PUBLIC ${CMAKE_DL_LIBS})

target_compile_options(component_registry PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)


set_target_properties(component_registry PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)


if(COMPONENT_REGISTRY_BUILD_EXAMPLE_SERVER)
    add_subdirectory(servers/example_repository)
endif()


if(COMPONENT_REGISTRY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
