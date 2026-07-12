#pragma once

#include <vulkan/vulkan.hpp>

namespace mpvk {
class PhysicalDevice;
class Device {
 public:
  explicit Device(const PhysicalDevice& gpu);
  ~Device();

  Device(const Device&)            = delete;
  Device& operator=(const Device&) = delete;

  vk::Device handle() const { return handle_; }
  vk::Queue  graphics_queue() const { return graphics_queue_; }
  uint32_t   graphics_family() const { return graphics_family_; }

 private:
  vk::Device handle_{};
  vk::Queue  graphics_queue_{};
  uint32_t   graphics_family_{0};
};

}  // namespace mpvk