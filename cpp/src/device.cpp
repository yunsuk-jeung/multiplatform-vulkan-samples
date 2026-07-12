#include <cstring>
#include <vector>

#include "mpvk/device.hpp"
#include "mpvk/physical_device.hpp"

namespace mpvk {
Device::Device(const PhysicalDevice& gpu) {
  constexpr const float priority = 1.0f;

  present_family_ = gpu.present_family();

  std::vector<uint32_t> families{gpu.graphics_family()};
  if (present_family_ && present_family_ != gpu.graphics_family()) {
    families.push_back(*present_family_);
  }

  std::vector<vk::DeviceQueueCreateInfo> queue_infos;
  for (const auto& f : families) {
    vk::DeviceQueueCreateInfo queue_info;
    queue_info.setQueueFamilyIndex(f);
    queue_info.setQueuePriorities(priority);
    queue_infos.push_back(queue_info);
  }

  // for macOS
  constexpr const char* kPortabilitySubset = "VK_KHR_portability_subset";

  std::vector<const char*> extensions;
  auto available = gpu.handle().enumerateDeviceExtensionProperties();
  for (const auto& ext : available) {
    if (std::strcmp(ext.extensionName, kPortabilitySubset) == 0) {
      extensions.push_back(kPortabilitySubset);
      break;
    }
  }

  vk::DeviceCreateInfo device_info;
  device_info.setQueueCreateInfos(queue_infos);
  device_info.setPEnabledExtensionNames(extensions);

  handle_          = gpu.handle().createDevice(device_info);
  graphics_family_ = gpu.graphics_family();
  graphics_queue_  = handle_.getQueue(graphics_family_, 0);
  if (present_family_) {
    present_queue_ = handle_.getQueue(*present_family_, 0);
  }
}
Device::~Device() {
  if (handle_) {
    handle_.destroy();
  }
  handle_ = nullptr;
}
}  // namespace mpvk