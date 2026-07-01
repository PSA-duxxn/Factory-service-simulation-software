cmake_minimum_required(VERSION 3.16)

# ── Compile simulation library (without main) for tests ───────────────
set(LIB_SOURCES
    ${CMAKE_SOURCE_DIR}/src/simulation.cpp
    ${CMAKE_SOURCE_DIR}/src/machine.cpp
    ${CMAKE_SOURCE_DIR}/src/adjuster.cpp
    ${CMAKE_SOURCE_DIR}/src/service_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/config_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/reporter.cpp
)

add_library(fsss_lib STATIC ${LIB_SOURCES})
target_include_directories(fsss_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ── Test executables ───────────────────────────────────────────────────
function(add_fsss_test name src)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE fsss_lib)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_fsss_test(test_basic  test_basic.cpp)
add_fsss_test(test_config test_config.cpp)
