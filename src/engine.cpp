// Vulkan
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// Standard library
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ECS
#include <entt/entt.hpp>

// Local
#include "camera_system.h"
#include "combat_system.h"
#include "components.h"
#include "depth_buffer.h"
#include "graphics_pipeline.h"
#include "input_system.h"
#include "logical_device.h"
#include "model.h"
#include "order_system.h"
#include "orders.h"
#include "physical_device.h"
#include "render_system.h"
#include "renderer.h"
#include "selection_ring.h"
#include "selection_system.h"
#include "spatial_grid.h"
#include "surface.h"
#include "swap_chain.h"
#include "terrain.h"
#include "uniform_buffer.h"
#include "validation_layers.h"
#include "vulkan_instance.h"
#include "window.h"

class HelloTriangleApplication {
  public:
    void run() {
        initWindow();
        initVulkan();
        enableValidationLayers(true);
        initSurface();
        initPhysicalDevice();
        initLogicalDevice();
        initSwapChain();
        initDepthBuffer();
        initGraphicsPipeline();
        initRenderer();
        initModel();
        initTerrain();
        initUniformBuffer();
        initSelectionRing();
        initScene();
        mainLoop();
    }

  private:
    void enableValidationLayers(bool enable) {
        std::cout << "Enabling validation layers..." << std::endl;
        VulkanHelpers::ValidationLayers validationLayers;
        if (enable) {
            auto requiredLayers = validationLayers.getRequiredLayers();
            auto context        = vulkanInstance->getContext();
            if (!validationLayers.areValidationLayersSupported(requiredLayers, *context))
                throw std::runtime_error("Required layer not supported");
            auto requiredExtensions = window->getRequiredInstanceExtensions();
            if (!validationLayers.areRequiredExtensionsSupported(requiredExtensions, *context))
                throw std::runtime_error("Required extension not supported");
            debugMessenger = validationLayers.createDebugMessenger(*vulkanInstance->getInstance(), &debugCallback);
        }
    }
    void initWindow() {
        std::cout << "creating window..." << std::endl;
        window = std::make_shared<VulkanHelpers::Window>(800, 600, "Catfish Engine");
    }
    void initVulkan() {
        std::cout << "creating vulkan instance..." << std::endl;
        vulkanInstance = std::make_shared<VulkanHelpers::Instance>(window);
    }
    void initSurface() {
        std::cout << "creating window surface..." << std::endl;
        surface = std::make_shared<VulkanHelpers::Surface>(*vulkanInstance->getInstance(), *window);
    }
    void initPhysicalDevice() {
        std::cout << "selecting physical device..." << std::endl;
        physicalDevice = std::make_shared<VulkanHelpers::PhysicalDevice>(*vulkanInstance->getInstance(), *surface->getSurface());
    }
    void initLogicalDevice() {
        std::cout << "creating logical device..." << std::endl;
        logicalDevice = std::make_shared<VulkanHelpers::LogicalDevice>(
            *physicalDevice->getPhysicalDevice(), physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex()
        );
    }
    void initSwapChain() {
        std::cout << "creating swap chain..." << std::endl;
        swapChain = std::make_shared<VulkanHelpers::SwapChain>(
            *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(),
            physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex(), *window
        );
    }
    void initDepthBuffer() {
        std::cout << "creating depth buffer..." << std::endl;
        depthBuffer = std::make_shared<VulkanHelpers::DepthBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent()
        );
    }
    void initGraphicsPipeline() {
        std::cout << "creating graphics pipeline..." << std::endl;
        graphicsPipeline = std::make_shared<VulkanHelpers::GraphicsPipeline>(
            *logicalDevice->getDevice(), swapChain->getFormat(), depthBuffer->getFormat()
        );
    }
    void initRenderer() {
        std::cout << "creating renderer..." << std::endl;
        renderer = std::make_shared<VulkanHelpers::Renderer>(*logicalDevice->getDevice(), physicalDevice->getGraphicsQueueFamilyIndex());
    }
    void initModel() {
        std::cout << "loading model..." << std::endl;
        model = std::make_shared<VulkanHelpers::Model>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(),
            *graphicsPipeline->getTextureLayout(),
            "models/goblin.glb"
        );
    }
    void initTerrain() {
        std::cout << "building terrain..." << std::endl;
        terrain = std::make_shared<VulkanHelpers::Terrain>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(),
            *graphicsPipeline->getTextureLayout()
        );
    }
    void initUniformBuffer() {
        std::cout << "creating uniform buffer..." << std::endl;
        uniformBuffer = std::make_shared<VulkanHelpers::UniformBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            *graphicsPipeline->getUboLayout()
        );
    }
    void initSelectionRing() {
        std::cout << "creating selection ring..." << std::endl;
        selectionRingModel = VulkanHelpers::createSelectionRingModel(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(),
            *graphicsPipeline->getTextureLayout()
        );
    }

    entt::entity spawnUnit(glm::vec3 position, Components::FactionId faction = Components::FactionId::Player) {
        auto e = registry.create();
        registry.emplace<Components::Transform>(e, Components::Transform{
            .position = position,
            .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            .scale    = {1.0f, 1.0f, 1.0f},
        });
        registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{model});
        registry.emplace<Components::Selectable>(e);
        registry.emplace<Components::MovementSpeed>(e);
        registry.emplace<Components::OrderQueue>(e);
        registry.emplace<Components::Faction>(e, Components::Faction{faction});
        registry.emplace<Components::Health>(e);
        registry.emplace<Components::Combat>(e);
        return e;
    }

    void initScene() {
        std::cout << "initialising scene..." << std::endl;

        auto camEntity = registry.create();
        registry.emplace<Components::Camera>(camEntity, Components::Camera{
            .position = {0.0f, -12.0f, 14.0f},
            .target   = {0.0f,  0.0f,  0.0f},
            .fov      = 50.0f,
            .near_    = 0.1f,
            .far_     = 200.0f,
        });

        auto terrainEntity = registry.create();
        registry.emplace<Components::Transform>(terrainEntity);
        registry.emplace<Components::RenderMesh>(terrainEntity,
            Components::RenderMesh{terrain->getModelPtr()});

        // 2×2 player formation
        auto p0 = spawnUnit({-1.5f, -1.5f, 0.0f}, Components::FactionId::Player);
        auto p1 = spawnUnit({ 1.5f, -1.5f, 0.0f}, Components::FactionId::Player);
        spawnUnit({-1.5f,  1.5f, 0.0f}, Components::FactionId::Player);
        spawnUnit({ 1.5f,  1.5f, 0.0f}, Components::FactionId::Player);

        // 2 enemy units approaching from the side
        auto e0 = spawnUnit({6.0f, -0.5f, 0.0f}, Components::FactionId::Enemy);
        auto e1 = spawnUnit({6.0f,  0.5f, 0.0f}, Components::FactionId::Enemy);

        // Enemies immediately attack the nearest player units
        registry.get<Components::OrderQueue>(e0).enqueue(Orders::AttackOrder{p0});
        registry.get<Components::OrderQueue>(e1).enqueue(Orders::AttackOrder{p1});
    }

    void recreateSwapChain() {
        auto [width, height] = window->getFramebufferSize();
        while (width == 0 || height == 0) {
            window->waitEvents();
            std::tie(width, height) = window->getFramebufferSize();
        }
        logicalDevice->getDevice()->waitIdle();
        swapChain->recreate(
            *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(),
            physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex(), *window
        );
        depthBuffer->recreate(*logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent());
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    void mainLoop() {
        auto lastTime = std::chrono::steady_clock::now();

        while (!window->shouldClose()) {
            auto  now       = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime        = now;

            window->pollEvents();
            Systems::updateCameraInput(registry, *window, deltaTime);
            Systems::processCombat(registry, deltaTime);
            Systems::processOrders(registry, deltaTime);
            Systems::updateCamera(registry, *uniformBuffer, swapChain->getExtent());
            Systems::updateSelection(registry, *window, swapChain->getExtent(), spatialGrid);
            spatialGrid.update(registry);
            auto draws = Systems::collectDrawCalls(registry);
            Systems::appendSelectionRings(registry, draws, *selectionRingModel);
            if (renderer->drawFrame(
                    *logicalDevice->getDevice(), *swapChain, *graphicsPipeline,
                    *logicalDevice->getGraphicsQueue(), *logicalDevice->getPresentQueue(),
                    draws, *uniformBuffer, *depthBuffer
                )) {
                recreateSwapChain();
                std::cout << "Recreating swapchain" << std::endl;
            }
        }
        logicalDevice->getDevice()->waitIdle();
    }

    // Vulkan
    std::shared_ptr<VulkanHelpers::Window>          window;
    std::shared_ptr<VulkanHelpers::Instance>        vulkanInstance;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;
    std::shared_ptr<VulkanHelpers::Surface>         surface;
    std::shared_ptr<VulkanHelpers::PhysicalDevice>  physicalDevice;
    std::shared_ptr<VulkanHelpers::LogicalDevice>   logicalDevice;
    std::shared_ptr<VulkanHelpers::SwapChain>       swapChain;
    std::shared_ptr<VulkanHelpers::GraphicsPipeline> graphicsPipeline;
    std::shared_ptr<VulkanHelpers::Renderer>        renderer;
    std::shared_ptr<VulkanHelpers::DepthBuffer>     depthBuffer;
    std::shared_ptr<VulkanHelpers::Model>           model;
    std::shared_ptr<VulkanHelpers::Terrain>         terrain;
    std::shared_ptr<VulkanHelpers::Model>           selectionRingModel;
    std::shared_ptr<VulkanHelpers::UniformBuffer>   uniformBuffer;

    // ECS
    entt::registry              registry;
    VulkanHelpers::SpatialGrid  spatialGrid{2.0f, {-20.0f, -20.0f}, {20.0f, 20.0f}};
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Exiting cleanly" << std::endl;
    return EXIT_SUCCESS;
}
