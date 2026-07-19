#include <cstdint>

#include "mpvk/command_pool.hpp"
#include "mpvk/device.hpp"

namespace mpvk {

CommandPool::CommandPool(const Device& device, uint32_t queue_family)
  : vk_device_{device.handle()} {
  vk::CommandPoolCreateInfo ci;
  ci.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  ci.queueFamilyIndex = queue_family;

  handle_ = vk_device_.createCommandPool(ci);
}

CommandPool::~CommandPool() {
  if (handle_) {
    vk_device_.destroyCommandPool(handle_);
  }
}

std::vector<vk::CommandBuffer> CommandPool::allocate(uint32_t count) const {
  vk::CommandBufferAllocateInfo ai{handle_,
                                   vk::CommandBufferLevel::ePrimary,
                                   count};
  return vk_device_.allocateCommandBuffers(ai);
}

}  // namespace mpvk
