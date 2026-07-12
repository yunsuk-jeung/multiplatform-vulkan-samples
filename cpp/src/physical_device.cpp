#include <optional>
#include <stdexcept>

#include "mpvk/instance.hpp"
#include "mpvk/physical_device.hpp"
#include "mpvk/surface.hpp"

namespace mpvk {
PhysicalDevice::PhysicalDevice(const Instance& instance,
                               const Surface*  surface) {
  for (const auto& physical_device : instance.physical_devices()) {
    auto families = physical_device.getQueueFamilyProperties();

    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    for (uint32_t i = 0; i < families.size(); ++i) {
      if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
        if (!graphics) {
          graphics = i;
        }
      }
      if (surface
          && physical_device.getSurfaceSupportKHR(i, surface->handle())) {
        if (!present) {
          present = i;
        }
      }
    }

    bool present_ok = (surface == nullptr) || present.has_value();
    if (graphics && present_ok) {
      handle_          = physical_device;
      graphics_family_ = *graphics;
      present_family_  = present;
      return;
    }
  }
  throw std::runtime_error(surface ? "no GPU with graphics + present support"
                                   : "no GPU with a graphics queue family");
}
}  // namespace mpvk
