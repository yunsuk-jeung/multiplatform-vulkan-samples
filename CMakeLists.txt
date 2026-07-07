cmake_minimum_required(VERSION 3.21)

project(multiplatform_vulkan_samples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    RESULT _cc_link SYMBOLIC)
  if(NOT _cc_link EQUAL 0)
    message(STATUS "compile_commands.json symlink skipped: ${_cc_link}")
  endif()
endif()

add_subdirectory(cpp)
add_subdirectory(samples)
