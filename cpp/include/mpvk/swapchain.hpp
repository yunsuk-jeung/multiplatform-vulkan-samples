#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace mpvk {

class PhysicalDevice;
class Device;
class Surface;
class Window;
class Swapchain {
 public:
  explicit Swapchain(const PhysicalDevice& gpu,
                     const Device&         device,
                     const Window&         window,
                     const Surface&        surface);
  ~Swapchain();

  Swapchain(const Swapchain&)            = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  vk::SwapchainKHR                  handle() const { return handle_; }
  vk::Format                        format() const { return format_; }
  vk::Extent2D                      extent() const { return extent_; }
  const std::vector<vk::ImageView>& image_views() const { return image_views_; }

 private:
  void create_image_views();  // reused on swapchain recreation (Step 7)

  vk::Device                 vk_device_{nullptr};
  vk::SwapchainKHR           handle_{nullptr};
  vk::Format                 format_{vk::Format::eUndefined};
  vk::Extent2D               extent_{0u, 0u};
  std::vector<vk::Image>     images_{};
  std::vector<vk::ImageView> image_views_{};
};

}  // namespace mpvk
