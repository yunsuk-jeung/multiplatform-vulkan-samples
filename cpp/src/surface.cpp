#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "mpvk/instance.hpp"
#include "mpvk/surface.hpp"
#include "mpvk/window.hpp"

namespace mpvk {
Surface::Surface(const Instance& instance, const Window& window)
  : vk_instance_(instance.handle()) {
  VkSurfaceKHR raw = VK_NULL_HANDLE;

  VkResult res =
    glfwCreateWindowSurface(vk_instance_, window.handle(), nullptr, &raw);

  if (res != VK_SUCCESS) {
    throw std::runtime_error("vk surface create failed");
  }

  handle_ = raw;
}

Surface::~Surface() {
  if (handle_) {
    vk_instance_.destroySurfaceKHR(handle_);
  }
}

}  // namespace mpvk
