#include <string>

#include <vulkan/vulkan.hpp>

#include "mpvk/device.hpp"
#include "mpvk/instance.hpp"
#include "mpvk/logger.hpp"
#include "mpvk/physical_device.hpp"
#include "mpvk/surface.hpp"
#include "mpvk/window.hpp"

int main() {
  mpvk::Window window{640, 640, "03_window_surface"};

  auto required_instance_extensions =
    mpvk::Window::required_instance_extensions();

  mpvk::Instance instance("03_window_surface", required_instance_extensions);
  LogI("Vulkan instance created.");

  mpvk::Surface surface{instance, window};

  mpvk::PhysicalDevice physical_device{instance, &surface};

  auto props = physical_device.handle().getProperties();
  LogI("Selected GPU: {} (graphics family = {})",
       props.deviceName.data(),
       physical_device.graphics_family());
  auto present = physical_device.present_family();
  LogI("present family = {}", present ? std::to_string(*present) : "none");

  mpvk::Device device{physical_device};
  LogI("Logical device created. Graphics queue: {}",
       device.graphics_queue() ? "OK" : "NULL");

  LogI("present queue: {}", device.present_queue() ? "OK" : "NULL");

  while (!window.should_close()) {
    window.poll_events();
  }
  return 0;
}
