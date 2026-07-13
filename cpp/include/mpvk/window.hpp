#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

namespace mpvk {
class Window {
 public:
  static std::vector<const char*> required_instance_extensions();

  explicit Window(int w, int h, const char* title);
  ~Window();

  Window(const Window&)            = delete;
  Window& operator=(const Window&) = delete;

  bool should_close() const;
  void poll_events() const;

  vk::Extent2D framebuffer_size() const;

  GLFWwindow* handle() const { return handle_; }

 private:
  GLFWwindow* handle_{nullptr};
};

}  // namespace mpvk
