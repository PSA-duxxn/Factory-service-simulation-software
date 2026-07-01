cmake_minimum_required(VERSION 3.16)
project(FSSS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Compiler warnings ──────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Release optimisation by default ───────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Source files ───────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/simulation.cpp
    src/machine.cpp
    src/adjuster.cpp
    src/service_manager.cpp
    src/config_loader.cpp
    src/reporter.cpp
)

include_directories(include)

add_executable(fsss ${SOURCES})

# ── Install target ─────────────────────────────────────────────────────
install(TARGETS fsss DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/fsss/config)

# ── Tests (optional, enable with -DFSSS_BUILD_TESTS=ON) ───────────────
option(FSSS_BUILD_TESTS "Build unit tests" OFF)

if(FSSS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
