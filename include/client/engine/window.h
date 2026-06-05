#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <cstdint>
#include <utility>
#include <vector>

namespace VulkanHelpers {
class Window {
  public:
    Window(uint32_t width, uint32_t height, const char *title);
    ~Window();

    Window(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(const Window &) = delete;
    Window &operator=(Window &&) = delete;

    GLFWwindow *getWindow() const { return m_window; }
    bool shouldClose() const;
    void requestClose() const;
    void pollEvents() const;
    void waitEvents() const;

    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    std::pair<int, int> getFramebufferSize() const;

    std::vector<const char *> getRequiredInstanceExtensions() const;

    // Input queries
    bool isKeyPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    std::pair<double, double> getMousePosition() const;
    float consumeScrollDelta(); // returns accumulated scroll since last call and resets it

  private:
    static void scrollCallback(GLFWwindow *win, double xoffset, double yoffset);
    static void windowSizeCallback(GLFWwindow *win, int width, int height);

    GLFWwindow *m_window;
    uint32_t m_width;
    uint32_t m_height;
    float m_scrollDelta{0.0f};
};
} // namespace VulkanHelpers
#endif // WINDOW_H
