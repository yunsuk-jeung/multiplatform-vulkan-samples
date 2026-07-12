#pragma once

#include <vector>

#include <vulkan/vulkan.hpp>

namespace mpvk {

class Instance {
 public:
  explicit Instance(const char*                     app_name         = "mpvk",
                    const std::vector<const char*>& extra_extensions = {});
  ~Instance();

  Instance(const Instance&)            = delete;
  Instance& operator=(const Instance&) = delete;

  vk::Instance handle() const { return handle_; }

  std::vector<vk::PhysicalDevice> physical_devices() const;

 private:
  vk::Instance handle_;

  vk::DebugUtilsMessengerEXT        debug_messenger_{};
  vk::detail::DispatchLoaderDynamic dispatcher_{};  // for calling EXT functions
};

}  // namespace mpvk
