
#pragma once

#include <vulkan/vulkan.hpp>

namespace mpvk {
class PhysicalDevice {
public:
  PhysicalDevice() {}
  vk::PhysicalDevice handle() const { return handle_; }

private:
  vk::PhysicalDevice handle_;
};
}  // namespace mpvk