#include <mpvk/instance.hpp>

#include <cstdio>

int main() {
  mpvk::Instance instance("01_device_info");
  std::printf("Vulkan instance created.\n");

  auto devices = instance.physical_devices();
  std::printf("Physical devices found: %zu\n", devices.size());
  for (const vk::PhysicalDevice &device : devices) {
    vk::PhysicalDeviceProperties props = device.getProperties();
    std::printf("  - %s (API %u.%u.%u)\n", props.deviceName.data(),
                VK_VERSION_MAJOR(props.apiVersion),
                VK_VERSION_MINOR(props.apiVersion),
                VK_VERSION_PATCH(props.apiVersion));
  }
  return 0;
}
