#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "vulkan/vulkan.hpp"

namespace mpvk {

class Device;
class CommandPool {
 public:
  explicit CommandPool(const Device& device, uint32_t queue_family);
  ~CommandPool();

  CommandPool(const CommandPool&)           = delete;
  CommandPool& operator=(const CommandPool) = delete;

  vk::CommandPool                handle() const { return handle_; }
  std::vector<vk::CommandBuffer> allocate(uint32_t count) const;

 private:
  vk::Device      vk_device_{nullptr};
  vk::CommandPool handle_{nullptr};
};

}  // namespace mpvk
