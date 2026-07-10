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

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  message(STATUS "No build type specified; defaulting to Debug")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(DEPS_OS "ubuntu")
elseif(APPLE)
  set(DEPS_OS "mac")
else()
  message(FATAL_ERROR "Unsupported target OS: ${CMAKE_SYSTEM_NAME}")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
if(_build_type_lower STREQUAL "debug")
  set(DEPS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/libs/${DEPS_OS}/debug")
else()
  set(DEPS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/libs/${DEPS_OS}/release")
endif()

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release "")

if(NOT EXISTS "${DEPS_INSTALL_DIR}")
  message(FATAL_ERROR
    "Dependency prefix not found: ${DEPS_INSTALL_DIR}\n"
    "  Build it first: scripts/install_dependencies_${DEPS_OS}.sh ${_build_type_lower}")
endif()

add_subdirectory(cpp)
add_subdirectory(samples)
