#pragma once

#include "mpvk/device.hpp"

namespace mpvk {

class Device;
class ShaderModule {
 public:
  ShaderModule(const Device& device, const std::vector<char>& spv);
  ~ShaderModule();

  ShaderModule(const ShaderModule&)            = delete;
  ShaderModule& operator=(const ShaderModule&) = delete;

 private:
  vk::Device       vk_devic_{nullptr};
  vk::ShaderModule handle_{nullptr};
};

}  // namespace mpvk
