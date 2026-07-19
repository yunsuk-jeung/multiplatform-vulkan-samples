#include <cstdint>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "mpvk/command_pool.hpp"
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

  mpvk::CommandPool pool{device, device.graphics_family()};

  constexpr uint32_t kFramesInFlight = 2;
  auto               cmd_buffers     = pool.allocate(kFramesInFlight);
  LogI("command buffers: {}", cmd_buffers.size());

  std::vector<vk::Semaphore> image_available, render_finished;
  std::vector<vk::Fence>     in_flight;

  for (uint32_t i = 0; i < kFramesInFlight; i++) {
    image_available.push_back(device.handle().createSemaphore({}));
    in_flight.push_back(
      device.handle().createFence({vk::FenceCreateFlagBits::eSignaled}));
  }

  for (size_t i = 0; i < swapchain.image_views().size(); ++i) {
    render_finished.push_back(device.handle().createSemaphore({}));
  }

  uint32_t f = 0;

  while (!window.should_close()) {
    vk::ImageSubresourceRange range{vk::ImageAspectFlagBits::eColor,
                                    0,
                                    1,
                                    0,
                                    1};
    vk::ClearColorValue       color{
      std::array<float, 4>{0.0f, 0.0f, 0.2f, 1.0f}
    };
    auto vk_device = device.handle();
    (void)vk_device.waitForFences(in_flight[f], vk::True, UINT64_MAX);

    uint32_t img_idx;
    try {
      img_idx = vk_device
                  .acquireNextImageKHR(swapchain.handle(),
                                       UINT64_MAX,
                                       image_available[f],
                                       nullptr)
                  .value;
    } catch (const vk::OutOfDateKHRError&) {
      swapchain.recreate(gpu, window, surface);
      continue;
    }

    vk_device.resetFences(in_flight[f]);

    auto& cb = cmd_buffers[f];
    cb.reset();
    cb.begin(vk::CommandBufferBeginInfo{});

    vk::ImageMemoryBarrier to_dst{};
    to_dst.oldLayout           = vk::ImageLayout::eUndefined;
    to_dst.newLayout           = vk::ImageLayout::eTransferDstOptimal;
    to_dst.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    to_dst.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    to_dst.image               = swapchain.images()[img_idx];
    to_dst.subresourceRange    = range;
    to_dst.dstAccessMask       = vk::AccessFlagBits::eTransferWrite;

    cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                       vk::PipelineStageFlagBits::eTransfer,
                       {},
                       {},
                       {},
                       to_dst);
    cb.clearColorImage(swapchain.images()[img_idx],
                       vk::ImageLayout::eTransferDstOptimal,
                       color,
                       range);
    vk::ImageMemoryBarrier to_present{};
    to_present.oldLayout           = vk::ImageLayout::eTransferDstOptimal;
    to_present.newLayout           = vk::ImageLayout::ePresentSrcKHR;
    to_present.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    to_present.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    to_present.image               = swapchain.images()[img_idx];
    to_present.subresourceRange    = range;
    to_present.srcAccessMask       = vk::AccessFlagBits::eTransferWrite;
    cb.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                       vk::PipelineStageFlagBits::eBottomOfPipe,
                       {},
                       {},
                       {},
                       to_present);
    cb.end();

    vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo         submit{};
    submit.setWaitSemaphores(image_available[f]);
    submit.setWaitDstStageMask(wait_stage);
    submit.setCommandBuffers(cb);
    submit.setSignalSemaphores(render_finished[img_idx]);
    device.graphics_queue().submit(submit, in_flight[f]);

    auto               sc = swapchain.handle();
    vk::PresentInfoKHR present{};
    present.setWaitSemaphores(render_finished[img_idx]);
    present.setSwapchains(sc);
    present.setImageIndices(img_idx);

    vk::Result pr = vk::Result::eSuccess;
    try {
      pr = device.present_queue().presentKHR(present);
    } catch (const vk::OutOfDateKHRError&) {
      pr = vk::Result::eErrorOutOfDateKHR;
    }

    if (pr == vk::Result::eErrorOutOfDateKHR || pr == vk::Result::eSuboptimalKHR
        || window.was_resized()) {
      window.reset_resized();
      swapchain.recreate(gpu, window, surface);
    }

    window.poll_events();

    f = (f + 1) % kFramesInFlight;
  }
  device.handle().waitIdle();
  for (auto s : image_available) device.handle().destroySemaphore(s);
  for (auto s : render_finished) device.handle().destroySemaphore(s);
  for (auto fence : in_flight) device.handle().destroyFence(fence);

  return 0;
}
