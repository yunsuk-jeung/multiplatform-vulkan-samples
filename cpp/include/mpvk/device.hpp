#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan.hpp>

namespace mpvk {
class PhysicalDevice;
class Device {
 public:
  explicit Device(const PhysicalDevice& gpu);
  ~Device();

  Device(const Device&)            = delete;
  Device& operator=(const Device&) = delete;

  vk::Device              handle() const { return handle_; }
  vk::Queue               graphics_queue() const { return graphics_queue_; }
  vk::Queue               present_queue() const { return present_queue_; }
  uint32_t                graphics_family() const { return graphics_family_; }
  std::optional<uint32_t> present_family() const { return present_family_; }

 private:
  vk::Device              handle_{nullptr};
  vk::Queue               graphics_queue_{nullptr};
  vk::Queue               present_queue_{nullptr};
  uint32_t                graphics_family_{0};
  std::optional<uint32_t> present_family_{std::nullopt};
};

}  // namespace mpvk