#include "menu_system.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

namespace VulkanHelpers {
namespace {

void checkVkResult(VkResult result) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Dear ImGui Vulkan backend error: " + std::to_string(result));
    }
}

uint32_t imageCountFor(const SwapChain &swapChain) { return std::max<uint32_t>(2, static_cast<uint32_t>(swapChain.getImages().size())); }

} // namespace

MenuSystem::MenuSystem(
    const Window &window, const vk::raii::Instance &instance, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex,
    const vk::raii::Queue &graphicsQueue, const SwapChain &swapChain, vk::Format depthFormat
) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;

    ImGui_ImplGlfw_InitForVulkan(window.getWindow(), true);

    VkFormat colorFormat = static_cast<VkFormat>(swapChain.getFormat());
    auto pipelineRenderingInfo = VkPipelineRenderingCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = static_cast<VkFormat>(depthFormat),
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_4;
    initInfo.Instance = *instance;
    initInfo.PhysicalDevice = *physicalDevice;
    initInfo.Device = *device;
    initInfo.QueueFamily = graphicsQueueFamilyIndex;
    initInfo.Queue = *graphicsQueue;
    initInfo.DescriptorPoolSize = 32;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = imageCountFor(swapChain);
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    initInfo.UseDynamicRendering = true;
    initInfo.CheckVkResultFn = checkVkResult;
    initInfo.MinAllocationSize = 1024 * 1024;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("Failed to initialize Dear ImGui Vulkan backend");
    }
}

MenuSystem::~MenuSystem() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void MenuSystem::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

MenuAction MenuSystem::drawMainMenu(vk::Extent2D extent) {
    if (gameplayStarted) {
        return MenuAction::None;
    }

    const ImVec2 displaySize{static_cast<float>(extent.width), static_cast<float>(extent.height)};
    ImGui::SetNextWindowPos(ImVec2{0.0f, 0.0f});
    ImGui::SetNextWindowSize(displaySize);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    MenuAction action = MenuAction::None;
    ImGui::Begin("Catfish Engine Main Menu", nullptr, flags);

    const float panelWidth = std::min(360.0f, displaySize.x - 40.0f);
    const float panelHeight = 210.0f;
    ImGui::SetCursorPos(
        ImVec2{
            (displaySize.x - panelWidth) * 0.5f,
            (displaySize.y - panelHeight) * 0.5f,
        }
    );

    ImGui::BeginChild("MainMenuPanel", ImVec2{panelWidth, panelHeight}, ImGuiChildFlags_Borders);
    ImGui::Spacing();
    ImGui::SetCursorPosX((panelWidth - ImGui::CalcTextSize("Catfish Engine").x) * 0.5f);
    ImGui::TextUnformatted("Catfish Engine");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const ImVec2 buttonSize{panelWidth - 48.0f, 42.0f};
    ImGui::SetCursorPosX(24.0f);
    if (ImGui::Button("Play", buttonSize)) {
        gameplayStarted = true;
        action = MenuAction::Play;
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(24.0f);
    if (ImGui::Button("Exit", buttonSize)) {
        action = MenuAction::Exit;
    }

    ImGui::EndChild();
    ImGui::End();

    return action;
}

void MenuSystem::drawOverlay(float deltaTime) {
    if (!gameplayStarted || !showOverlay) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2{10.0f, 10.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin(
        "Overlay", &showOverlay,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
    );
    ImGui::Text("FPS %.1f", deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f);
    ImGui::End();
}

void MenuSystem::render(vk::CommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(commandBuffer));
}

void MenuSystem::onSwapChainRecreated(const SwapChain &swapChain) {
    (void)swapChain;
    ImGui_ImplVulkan_SetMinImageCount(2);
}

bool MenuSystem::wantsMouse() const { return ImGui::GetIO().WantCaptureMouse; }

bool MenuSystem::wantsKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

} // namespace VulkanHelpers
