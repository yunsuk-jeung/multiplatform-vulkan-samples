#include <vector>

#include "mpvk/instance.hpp"
namespace mpvk {

Instance::Instance(const char* app_name) {
  vk::ApplicationInfo app_info{};
  app_info.pApplicationName = app_name;
  app_info.apiVersion       = VK_API_VERSION_1_4;

  std::vector<const char*> extensions;
  vk::InstanceCreateFlags  flags{};

#ifdef __APPLE__
  // MoltenVK is a non-conformant "portability" driver. Since Vulkan 1.3.216
  // the loader hides such drivers unless we opt in via the portability
  // enumeration extension + flag, otherwise createInstance fails with
  // VK_ERROR_INCOMPATIBLE_DRIVER.
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

  vk::InstanceCreateInfo create_info{};
  create_info.flags            = flags;
  create_info.pApplicationInfo = &app_info;
  create_info.setPEnabledExtensionNames(extensions);

  handle_ = vk::createInstance(create_info);
}

Instance::~Instance() {
  if (handle_) {
    handle_.destroy();
  }
}

std::vector<vk::PhysicalDevice> Instance::physical_devices() const {
  return handle_.enumeratePhysicalDevices();
}

}  // namespace mpvk
