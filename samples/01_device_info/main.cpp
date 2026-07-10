#include "mpvk/instance.hpp"
#include "mpvk/logger.hpp"

int main() {
  mpvk::Instance instance("01_device_info");
  LogI("Vulkan instance created.");

  auto devices = instance.physical_devices();
  LogI("Physical devices found: {}", devices.size());
  for (const vk::PhysicalDevice& device : devices) {
    vk::PhysicalDeviceProperties props = device.getProperties();
    LogI("  - {} (API {}.{}.{})",
         props.deviceName.data(),
         VK_VERSION_MAJOR(props.apiVersion),
         VK_VERSION_MINOR(props.apiVersion),
         VK_VERSION_PATCH(props.apiVersion));
  }
  return 0;
}
