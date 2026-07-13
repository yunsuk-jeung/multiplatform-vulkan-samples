#include <cstdint>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include "mpvk/window.hpp"

namespace mpvk {

std::vector<const char*> Window::required_instance_extensions() {
  uint32_t     count = 0;
  const char** exts  = glfwGetRequiredInstanceExtensions(&count);
  return std::vector<const char*>(exts, exts + count);
}

Window::Window(int w, int h, const char* title) {
  if (!glfwInit()) {
    glfwTerminate();
    throw std::runtime_error("glfw init failed");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  handle_ = glfwCreateWindow(w, h, title, nullptr, nullptr);

  if (!handle_) {
    glfwTerminate();
    throw std::runtime_error("glfw create window failed");
  }
}

Window::~Window() {
  if (handle_) {
    glfwDestroyWindow(handle_);
  }
  glfwTerminate();
}

bool Window::should_close() const {
  return glfwWindowShouldClose(handle_) != 0;
}

void Window::poll_events() const {
  glfwPollEvents();
}

vk::Extent2D Window::framebuffer_size() const {
  int w = 0, h = 0;
  glfwGetFramebufferSize(handle_, &w, &h);
  return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
}

}  // namespace mpvk
