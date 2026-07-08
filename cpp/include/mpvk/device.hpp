#pragma once

#include <vulkan/vulkan.hpp>

namespace mpvk {
class Device {
public:
  Device();
  ~Device();

  vk::Device handle() const { return handle_; }

private:
  vk::Device handle_;
};
}  // namespace mpvk