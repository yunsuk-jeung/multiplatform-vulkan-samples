#include <cstdio>
#include <exception>

#include <vulkan/vulkan.hpp>

#include "mpvk/device.hpp"
#include "mpvk/instance.hpp"
#include "mpvk/physical_device.hpp"

int main() {
  mpvk::Instance instance("02_logical_device");
  std::printf("Vulkan instance created.\n");

  mpvk::PhysicalDevice physical_device{instance};  // 참조로 되돌린 경우
  mpvk::Device         device{physical_device};

  auto props = physical_device.handle().getProperties();
  std::printf("Selected GPU: %s (graphics family = %u)\n",
              props.deviceName.data(),
              physical_device.graphics_family());
  std::printf("Logical device created. Graphics queue: %s\n",
              device.graphics_queue() ? "OK" : "NULL");

  return 0;
}
