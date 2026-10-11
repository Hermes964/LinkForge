cmake_minimum_required(VERSION 3.21)
project(LinkForge
    DESCRIPTION "Custom Android-to-Windows remote desktop and remote-control system"
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    message(STATUS "Platform: Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Platform: Linux")
elseif(APPLE)
    message(STATUS "Platform: macOS")
endif()

# Find GTest for unit tests
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Protocol library
add_library(linkforge_protocol STATIC
    src/linkforge/protocol.cpp
)
target_include_directories(linkforge_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(linkforge_protocol PRIVATE cxx_std_20)

# Add tests
add_executable(linkforge_protocol_tests
    test/protocol_test.cpp
)
target_link_libraries(linkforge_protocol_tests
    linkforge_protocol
    GTest::gtest_main
    Threads::Threads
)
target_compile_features(linkforge_protocol_tests PRIVATE cxx_std_20)

# Add tests to CTest
include(GoogleTest)
gtest_discover_tests(linkforge_protocol_tests)

# Install targets
install(TARGETS linkforge_protocol
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)
