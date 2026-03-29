cmake_minimum_required(VERSION 3.25)
project(edinz VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Generate version.hpp with git hash on every build
add_custom_target(generate_version ALL
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DBINARY_DIR=${CMAKE_BINARY_DIR}
        -DPROJECT_VERSION=${PROJECT_VERSION}
        -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateVersion.cmake
    COMMENT "Generating version header"
)

add_executable(edinz
    src/main.cpp
    src/terminal.cpp
    src/app.cpp
    src/window.cpp
)

add_dependencies(edinz generate_version)
target_include_directories(edinz PRIVATE src ${CMAKE_BINARY_DIR}/generated)

install(TARGETS edinz DESTINATION bin)
