find_package(Vulkan REQUIRED)

find_package(spdlog REQUIRED CONFIG PATHS "${DEPS_INSTALL_DIR}" NO_DEFAULT_PATH)
find_package(glfw3 REQUIRED CONFIG PATHS "${DEPS_INSTALL_DIR}" NO_DEFAULT_PATH)

add_library(mpvk
  src/instance.cpp
  src/device.cpp
  src/physical_device.cpp
  src/window.cpp
  src/surface.cpp
  src/swapchain.cpp
)
target_include_directories(mpvk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(mpvk PUBLIC Vulkan::Vulkan spdlog::spdlog glfw)

target_compile_definitions(mpvk PUBLIC MPVK_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_library(mpvk::mpvk ALIAS mpvk)

# --- tests (added once there is something to test) --------------------------
# add_subdirectory(tests)
