#include <vulkan/vulkan.hpp>

#include "mpvk/device.hpp"
#include "mpvk/instance.hpp"
#include "mpvk/logger.hpp"
#include "mpvk/physical_device.hpp"

int main() {
  mpvk::Instance instance("02_logical_device");
  LogI("Vulkan instance created.");

  mpvk::PhysicalDevice physical_device{instance};  // 참조로 되돌린 경우
  mpvk::Device         device{physical_device};

  auto props = physical_device.handle().getProperties();
  LogI("Selected GPU: {} (graphics family = {})",
       props.deviceName.data(),
       physical_device.graphics_family());
  LogI("Logical device created. Graphics queue: {}",
       device.graphics_queue() ? "OK" : "NULL");

  return 0;
}
