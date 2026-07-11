#include <stdexcept>

#include <GLFW/glfw3.h>

#include "mpvk/logger.hpp"
#include "mpvk/window.hpp"

namespace mpvk {
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

}  // namespace mpvk
