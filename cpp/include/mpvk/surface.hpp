#pragma once

#include <vulkan/vulkan.hpp>

namespace mpvk {
class Instance;
class Window;
class Surface {
 public:
  explicit Surface(const Instance& instance, const Window& window);
  ~Surface();

  Surface(const Surface&)            = delete;
  Surface& operator=(const Surface&) = delete;

  vk::SurfaceKHR handle() const { return handle_; }

 private:
  vk::SurfaceKHR handle_{nullptr};
  vk::Instance   vk_instance_{nullptr};
};

}  // namespace mpvk
