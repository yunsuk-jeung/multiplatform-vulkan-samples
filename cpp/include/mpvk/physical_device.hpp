#pragma once

#include <cstdint>
#include <optional>

#include <vulkan/vulkan.hpp>

#include "mpvk/instance.hpp"

namespace mpvk {
class Instance;
class Surface;

class PhysicalDevice {
 public:
  // surface == nullptr → headless (present family 없음, nullopt)
  explicit PhysicalDevice(const Instance& instance,
                          const Surface*  surface = nullptr);

  vk::PhysicalDevice      handle() const { return handle_; }
  uint32_t                graphics_family() const { return graphics_family_; }
  std::optional<uint32_t> present_family() const { return present_family_; }

 private:
  vk::PhysicalDevice      handle_{nullptr};
  uint32_t                graphics_family_{0};
  std::optional<uint32_t> present_family_{std::nullopt};
};
}  // namespace mpvk
