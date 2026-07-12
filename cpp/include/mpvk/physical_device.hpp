#pragma once

#include <cstdint>

#include <vulkan/vulkan.hpp>

#include "mpvk/instance.hpp"

namespace mpvk {
class Instance;
class PhysicalDevice {
 public:
  explicit PhysicalDevice(const Instance& instance);

  vk::PhysicalDevice handle() const { return handle_; }
  uint32_t           graphics_family() const { return graphics_family_; }

 private:
  vk::PhysicalDevice handle_{nullptr};
  uint32_t           graphics_family_{0};
};
}  // namespace mpvk