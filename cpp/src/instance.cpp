#include "mpvk/instance.hpp"

#include <vector>

namespace mpvk {

Instance::Instance(const char *app_name) {
  vk::ApplicationInfo app_info{};
  app_info.pApplicationName = app_name;
  app_info.apiVersion = VK_API_VERSION_1_4;

  std::vector<const char *> extensions;
  vk::InstanceCreateFlags flags{};

#ifdef __APPLE__
  // MoltenVK
#endif

  vk::InstanceCreateInfo create_info{};
  create_info.flags = flags;
  create_info.pApplicationInfo = &app_info;
  create_info.setPEnabledExtensionNames(extensions);

  instance_ = vk::createInstance(create_info);
}

Instance::~Instance() {
  if (instance_) {
    instance_.destroy();
  }
}

std::vector<vk::PhysicalDevice> Instance::physical_devices() const {
  return instance_.enumeratePhysicalDevices();
}

} // namespace mpvk
