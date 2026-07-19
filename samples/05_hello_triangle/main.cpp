#include <fstream>
#include <string>
#include <vector>

#include "mpvk/logger.hpp"

namespace {
// Reads a whole file into bytes; empty vector on failure.
std::vector<char> read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return {};
  }
  const std::streamsize size = file.tellg();
  file.seekg(0);
  std::vector<char> buffer(static_cast<size_t>(size));
  file.read(buffer.data(), size);
  return buffer;
}
}  // namespace

int main() {
  // Verify the glslc build step + SHADER_DIR path (temporary; replace with the
  // real pipeline setup — see docs/samples/05_hello_triangle.md).
  const std::string dir  = SHADER_DIR;
  const auto        vert = read_file(dir + "/triangle.vert.spv");
  const auto        frag = read_file(dir + "/triangle.frag.spv");
  LogI("shaders loaded: vert={} bytes, frag={} bytes",
       vert.size(),
       frag.size());

  // TODO Step 2: mpvk::ShaderModule from the .spv bytes
  // TODO Step 3: mpvk::RenderPass
  // TODO Step 4: mpvk::Framebuffer (per swapchain image view)
  // TODO Step 5: mpvk::GraphicsPipeline
  // TODO Step 6: record beginRenderPass -> bindPipeline -> draw(3) ->
  // endRenderPass
  //              (reuse the 04 window/instance/device/swapchain/sync setup)
  return 0;
}
