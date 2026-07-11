#include <cstring>
#include <vector>

#include "mpvk/instance.hpp"
#include "mpvk/logger.hpp"

namespace mpvk {
namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
               vk::DebugUtilsMessageTypeFlagsEXT /*type*/,
               const vk::DebugUtilsMessengerCallbackDataEXT* data,
               void* /*user*/) {
  const char* id = data->pMessageIdName ? data->pMessageIdName : "";

  if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
    LogE("[vk] {}: {}", id, data->pMessage);
  }
  else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
    LogW("[vk] {}: {}", id, data->pMessage);
  }
  else {
    LogI("[vk] {}: {}", id, data->pMessage);
  }
  return VK_FALSE;
}
}  // namespace

Instance::Instance(const char* app_name) {
  vk::ApplicationInfo app_info{};
  app_info.pApplicationName = app_name;
  app_info.apiVersion       = VK_API_VERSION_1_4;

  std::vector<const char*> extensions;
  vk::InstanceCreateFlags  flags{};
  std::vector<const char*> layers;

  auto supported_extension = vk::enumerateInstanceExtensionProperties();

#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

  auto supported_layers = vk::enumerateInstanceLayerProperties();

#ifndef NDEBUG
  for (auto& e : supported_extension) {
    if (std::strcmp(e.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
      extensions.push_back(e.extensionName);
    }
  }
  constexpr const char* kValidationLayer = "VK_LAYER_KHRONOS_validation";

  for (auto& l : supported_layers) {
    if (std::strcmp(l.layerName, kValidationLayer) == 0) {
      layers.push_back(l.layerName);
    }
  }

#endif

  vk::InstanceCreateInfo create_info{};
  create_info.flags            = flags;
  create_info.pApplicationInfo = &app_info;
  create_info.setPEnabledExtensionNames(extensions);
  create_info.setPEnabledLayerNames(layers);

  handle_ = vk::createInstance(create_info);

  for (auto& e : extensions) {
    if (std::strcmp(e, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
      dispatcher_ = vk::detail::DispatchLoaderDynamic(handle_,
                                                      vkGetInstanceProcAddr);

      vk::DebugUtilsMessengerCreateInfoEXT info{};
      info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                             | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
      info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                         | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                         | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
      info.pfnUserCallback = debug_callback;

      debug_messenger_ =
        handle_.createDebugUtilsMessengerEXT(info, nullptr, dispatcher_);
    }
  }
}

Instance::~Instance() {
  if (debug_messenger_) {
    handle_.destroyDebugUtilsMessengerEXT(debug_messenger_,
                                          nullptr,
                                          dispatcher_);
  }
  if (handle_) {
    handle_.destroy();
  }
  handle_ = nullptr;
}

std::vector<vk::PhysicalDevice> Instance::physical_devices() const {
  return handle_.enumeratePhysicalDevices();
}

}  // namespace mpvk
