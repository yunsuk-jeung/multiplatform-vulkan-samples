#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace mpvk {

class Instance {
public:
  explicit Instance(const char *app_name = "mpvk");
  ~Instance();

  Instance(const Instance &) = delete;
  Instance &operator=(const Instance &) = delete;

  vk::Instance handle() const { return instance_; }

  std::vector<vk::PhysicalDevice> physical_devices() const;

private:
  vk::Instance instance_;
};

} // namespace mpvk
