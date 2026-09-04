cmake_minimum_required(VERSION 3.20)

project(mkwii_server VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mkwii-server
    src/main.cpp
    src/config.cpp
    src/gamespy_qr.cpp
    src/gamespy_profile.cpp
    src/nas_http.cpp
    src/server.cpp
)

target_include_directories(mkwii-server PRIVATE include)

target_compile_options(mkwii-server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(mkwii-server PRIVATE -fsanitize=address,undefined)
    target_link_options(mkwii-server PRIVATE -fsanitize=address,undefined)
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(config-test
        tests/config_test.cpp
        src/config.cpp
    )
    target_include_directories(config-test PRIVATE include)
    add_test(NAME config-test COMMAND config-test)

    add_executable(gamespy-qr-test
        tests/gamespy_qr_test.cpp
        src/gamespy_qr.cpp
    )
    target_include_directories(gamespy-qr-test PRIVATE include)
    add_test(NAME gamespy-qr-test COMMAND gamespy-qr-test)

    add_executable(gamespy-profile-test
        tests/gamespy_profile_test.cpp
        src/gamespy_profile.cpp
    )
    target_include_directories(gamespy-profile-test PRIVATE include)
    add_test(NAME gamespy-profile-test COMMAND gamespy-profile-test)

    add_executable(nas-http-test
        tests/nas_http_test.cpp
        src/nas_http.cpp
    )
    target_include_directories(nas-http-test PRIVATE include)
    add_test(NAME nas-http-test COMMAND nas-http-test)
endif()
