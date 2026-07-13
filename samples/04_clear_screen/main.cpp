#include <vulkan/vulkan.hpp>

#include "mpvk/device.hpp"
#include "mpvk/instance.hpp"
#include "mpvk/logger.hpp"
#include "mpvk/physical_device.hpp"
#include "mpvk/surface.hpp"
#include "mpvk/swapchain.hpp"
#include "mpvk/window.hpp"

int main() {
  mpvk::Window window{800, 600, "04_clear_screen"};

  mpvk::Instance instance("04_clear_screen",
                          mpvk::Window::required_instance_extensions());

  mpvk::Surface        surface{instance, window};
  mpvk::PhysicalDevice gpu{instance, &surface};
  mpvk::Device         device{gpu};
  mpvk::Swapchain      swapchain{gpu, device, window, surface};
  LogI("swapchain ready: {} images", swapchain.image_views().size());

  while (!window.should_close()) {
    window.poll_events();
  }
  return 0;
}
