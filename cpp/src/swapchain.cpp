#include <algorithm>
#include <cstdint>
#include <optional>

#include "mpvk/device.hpp"
#include "mpvk/physical_device.hpp"
#include "mpvk/surface.hpp"
#include "mpvk/swapchain.hpp"
#include "mpvk/window.hpp"

namespace mpvk {
namespace {

vk::SurfaceFormatKHR choose_format(
  const std::vector<vk::SurfaceFormatKHR>& formats) {
  for (const auto& f : formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb
        && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return f;
    }
  }
  return formats[0];  // guaranteed non-empty
}

vk::PresentModeKHR choose_present_mode(
  const std::vector<vk::PresentModeKHR>& modes) {
  for (const auto& m : modes) {
    if (m == vk::PresentModeKHR::eMailbox) {
      return m;
    }
  }
  return vk::PresentModeKHR::eFifo;  // always supported
}

vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& caps,
                           const Window&                     window) {
  if (caps.currentExtent.width != UINT32_MAX) {
    return caps.currentExtent;  // window system dictates the size
  }
  vk::Extent2D extent =
    window.framebuffer_size();  // pixels, not logical points
  extent.width  = std::clamp(extent.width,
                            caps.minImageExtent.width,
                            caps.maxImageExtent.width);
  extent.height = std::clamp(extent.height,
                             caps.minImageExtent.height,
                             caps.maxImageExtent.height);
  return extent;
}

uint32_t choose_image_count(const vk::SurfaceCapabilitiesKHR& caps) {
  uint32_t count = caps.minImageCount
                   + 1;  // +1 to avoid stalling on the driver
  if (caps.maxImageCount > 0 && count > caps.maxImageCount) {
    count = caps.maxImageCount;  // maxImageCount == 0 means "no limit"
  }
  return count;
}

}  // namespace

Swapchain::Swapchain(const PhysicalDevice& gpu,
                     const Device&         device,
                     const Window&         window,
                     const Surface&        surface)
  : vk_device_{device.handle()} {
  // 1) query what the surface supports
  auto caps    = gpu.handle().getSurfaceCapabilitiesKHR(surface.handle());
  auto formats = gpu.handle().getSurfaceFormatsKHR(surface.handle());
  auto modes   = gpu.handle().getSurfacePresentModesKHR(surface.handle());

  // 2) choose
  const vk::SurfaceFormatKHR fmt    = choose_format(formats);
  const vk::PresentModeKHR   mode   = choose_present_mode(modes);
  const vk::Extent2D         extent = choose_extent(caps, window);
  const uint32_t             count  = choose_image_count(caps);

  // 3) fill create info
  vk::SwapchainCreateInfoKHR ci;
  ci.surface          = surface.handle();
  ci.minImageCount    = count;
  ci.imageFormat      = fmt.format;
  ci.imageColorSpace  = fmt.colorSpace;
  ci.imageExtent      = extent;
  ci.imageArrayLayers = 1;
  ci.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment
                  | vk::ImageUsageFlagBits::eTransferDst;
  ci.preTransform   = caps.currentTransform;
  ci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  ci.presentMode    = mode;
  ci.clipped        = VK_TRUE;
  ci.oldSwapchain   = nullptr;

  // sharing mode: `families` must outlive createSwapchainKHR below
  const uint32_t gf         = gpu.graphics_family();
  const auto     pf         = gpu.present_family();
  const uint32_t families[] = {gf, pf ? *pf : gf};
  if (pf && *pf != gf) {
    ci.imageSharingMode = vk::SharingMode::eConcurrent;
    ci.setQueueFamilyIndices(families);
  }
  else {
    ci.imageSharingMode = vk::SharingMode::eExclusive;
  }

  // 4) create, retrieve images, build views
  handle_ = vk_device_.createSwapchainKHR(ci);
  format_ = fmt.format;
  extent_ = extent;
  images_ = vk_device_.getSwapchainImagesKHR(handle_);
  create_image_views();
}

void Swapchain::create_image_views() {
  image_views_.reserve(images_.size());
  for (const auto& image : images_) {
    vk::ImageViewCreateInfo vi;
    vi.image    = image;
    vi.viewType = vk::ImageViewType::e2D;
    vi.format   = format_;
    vi.subresourceRange =
      vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    image_views_.push_back(vk_device_.createImageView(vi));
  }
}

Swapchain::~Swapchain() {
  for (auto view : image_views_) {
    vk_device_.destroyImageView(view);
  }
  if (handle_) {
    vk_device_.destroySwapchainKHR(handle_);
  }
}

}  // namespace mpvk
