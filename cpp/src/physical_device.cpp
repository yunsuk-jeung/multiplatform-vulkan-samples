#include <stdexcept>

#include "mpvk/instance.hpp"
#include "mpvk/physical_device.hpp"

namespace mpvk {
PhysicalDevice::PhysicalDevice(const Instance& instance) {
  auto physical_devices = instance.physical_devices();

  for (auto& physical_device : physical_devices) {
    auto queue_family_props = physical_device.getQueueFamilyProperties();

    for (size_t i = 0; i < queue_family_props.size(); i++) {
      auto& prop = queue_family_props[i];

      if (prop.queueFlags & vk::QueueFlagBits::eGraphics) {
        handle_          = physical_device;
        graphics_family_ = i;
        return;
      }
    }
  }
  throw std::runtime_error("no gpu with a graphics queue family");
}
}  // namespace mpvk