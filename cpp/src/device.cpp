#include <cstring>
#include <vector>

#include "mpvk/device.hpp"
#include "mpvk/physical_device.hpp"

namespace mpvk {
Device::Device(const PhysicalDevice& gpu) {
  constexpr const float priority = 1.0f;

  vk::DeviceQueueCreateInfo queue_info;
  queue_info.setQueueFamilyIndex(gpu.graphics_family());
  queue_info.setQueuePriorities(priority);

  constexpr const char*
    kPortabilitySubset = "VK_KHR_portability_subset";  // for mac os

  std::vector<const char*> extensions;
  auto available = gpu.handle().enumerateDeviceExtensionProperties();
  for (const auto& ext : available) {
    if (std::strcmp(ext.extensionName, kPortabilitySubset) == 0) {
      extensions.push_back(kPortabilitySubset);
      break;
    }
  }

  vk::DeviceCreateInfo device_info;
  device_info.setQueueCreateInfos(queue_info);
  device_info.setPEnabledExtensionNames(extensions);

  handle_          = gpu.handle().createDevice(device_info);
  graphics_family_ = gpu.graphics_family();
  graphics_queue_  = handle_.getQueue(graphics_family_, 0);
}
Device::~Device() {
  if (handle_) {
    handle_.destroy();
  }
  handle_ = nullptr;
}
}  // namespace mpvk