
#include <cstdio>

#include "mpvk/instance.hpp"
#include "vulkan/vulkan.hpp"

int main() {
  mpvk::Instance instance("01_device_info");
  std::printf("Vulkan instance created.\n");

  auto devices = instance.physical_devices();
  std::printf("Physical devices found: %zu\n", devices.size());
  for (const vk::PhysicalDevice& device : devices) {
    vk::PhysicalDeviceProperties props = device.getProperties();
    std::printf("  - %s (API %u.%u.%u)\n",
                props.deviceName.data(),
                VK_VERSION_MAJOR(props.apiVersion),
                VK_VERSION_MINOR(props.apiVersion),
                VK_VERSION_PATCH(props.apiVersion));
  }

  vk::QueueFlags required = vk::QueueFlagBits::eGraphics;
  for (auto& device : devices) {
    auto queue_family_props = device.getQueueFamilyProperties();

    for (auto prop : queue_family_props) {
      if (prop.queueFlags & required) {
        vk::DeviceCreateInfo info;
      }
    }
  }

  return 0;
}
