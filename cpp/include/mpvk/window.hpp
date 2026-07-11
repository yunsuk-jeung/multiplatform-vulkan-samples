#pragma once

#include <memory>

struct GLFWwindow;

namespace mpvk {
class Window {
public:
  explicit Window(int w, int h, const char* title);
  ~Window();

  Window(const Window&)            = delete;
  Window& operator=(const Window&) = delete;

  bool should_close() const;
  void poll_events() const;

  GLFWwindow* handle() const { return handle_; }

private:
  GLFWwindow* handle_{nullptr};
};

}  // namespace mpvk
