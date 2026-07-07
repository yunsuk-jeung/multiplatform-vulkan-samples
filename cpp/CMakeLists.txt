# Vulkan SDK must be installed per-OS. Locally, scripts/setup-vulkan-sdk.sh +
# direnv (.envrc) put it on CMAKE_PREFIX_PATH so this find_package succeeds.
find_package(Vulkan REQUIRED)

# --- mpvk: the shared framework library every sample links against ----------
add_library(mpvk
  src/instance.cpp)
target_include_directories(mpvk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mpvk PUBLIC Vulkan::Vulkan)
add_library(mpvk::mpvk ALIAS mpvk)

# --- tests (added once there is something to test) --------------------------
# add_subdirectory(tests)
