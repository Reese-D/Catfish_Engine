#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>

namespace VulkanHelpers {
class Window {
  public:
    Window(uint32_t width, uint32_t height, const char *title);
    ~Window();

    Window(const Window &) = delete;            // Copy constructor
    Window(Window &&) = delete;                 // Move Constructor
    Window &operator=(const Window &) = delete; // Copy Assignment
    Window &operator=(Window &&) = delete;      // Move Assignment

    GLFWwindow *getWindow() const { return window; }
    bool shouldClose() const;
    void pollEvents() const;
    void waitEvents() const;

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }

    std::vector<const char *> getRequiredInstanceExtensions() const;

  private:
    GLFWwindow *window;
    uint32_t width;
    uint32_t height;
};
} // namespace VulkanHelpers
#endif // WINDOW_H
