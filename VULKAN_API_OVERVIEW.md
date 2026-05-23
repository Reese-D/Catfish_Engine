# Vulkan API Overview

This document provides an overview of the Vulkan API structure, including key functions, structs, and their parameters. The Vulkan API is designed for low-level graphics and compute operations.

## Core Components

### 1. Instance Creation and Management

**vkCreateInstance**
```c
VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* pInstance);
```

**VkInstanceCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Instance creation flags
- pApplicationInfo: Application information
- enabledLayerCount: Number of layers to enable
- ppEnabledLayerNames: Array of layer names
- enabledExtensionCount: Number of extensions to enable
- ppEnabledExtensionNames: Array of extension names

**vkDestroyInstance**
```c
void vkDestroyInstance(
    VkInstance instance,
    const VkAllocationCallbacks* pAllocator);
```

### 2. Physical Device Enumeration

**vkEnumeratePhysicalDevices**
```c
VkResult vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices);
```

**vkGetPhysicalDeviceProperties**
```c
void vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties);
```

**VkPhysicalDeviceProperties** structure:
- apiVersion: Vulkan API version
- driverVersion: Driver version
- vendorID: Vendor identifier
- deviceID: Device identifier
- deviceType: Type of device
- deviceName: Name of device
- pipelineCacheUUID: Pipeline cache UUID

### 3. Device Creation and Management

**vkCreateDevice**
```c
VkResult vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDevice* pDevice);
```

**VkDeviceCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Device creation flags
- queueCreateInfoCount: Number of queue create info structures
- pQueueCreateInfos: Array of queue create info structures
- enabledLayerCount: Number of layers to enable
- ppEnabledLayerNames: Array of layer names
- enabledExtensionCount: Number of extensions to enable
- ppEnabledExtensionNames: Array of extension names
- pEnabledFeatures: Pointer to physical device features to enable

**vkDestroyDevice**
```c
void vkDestroyDevice(
    VkDevice device,
    const VkAllocationCallbacks* pAllocator);
```

### 4. Queue Management

**vkGetDeviceQueue**
```c
void vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue);
```

### 5. Surface and Swapchain

**vkCreateSwapchainKHR**
```c
VkResult vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain);
```

**VkSwapchainCreateInfoKHR** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR)
- pNext: Pointer to next structure in chain
- flags: Swapchain creation flags
- surface: Surface to create swapchain for
- minImageCount: Minimum number of images in swapchain
- imageFormat: Format of swapchain images
- imageColorSpace: Color space of swapchain images
- imageExtent: Extent of swapchain images
- imageArrayLayers: Number of layers in swapchain images
- imageUsage: Image usage flags
- imageSharingMode: Image sharing mode
- queueFamilyIndexCount: Number of queue families
- pQueueFamilyIndices: Array of queue family indices
- preTransform: Pre-transform operation
- compositeAlpha: Composite alpha mode
- presentMode: Present mode
- clipped: Whether to clip images
- oldSwapchain: Old swapchain to replace

**vkGetSwapchainImagesKHR**
```c
VkResult vkGetSwapchainImagesKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages);
```

### 6. Command Buffers

**vkCreateCommandPool**
```c
VkResult vkCreateCommandPool(
    VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool);
```

**VkCommandPoolCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Command pool creation flags
- queueFamilyIndex: Queue family index

**vkAllocateCommandBuffers**
```c
VkResult vkAllocateCommandBuffers(
    VkDevice device,
    const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers);
```

**VkCommandBufferAllocateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO)
- pNext: Pointer to next structure in chain
- commandPool: Command pool to allocate from
- level: Command buffer level
- commandBufferCount: Number of command buffers to allocate

### 7. Memory Management

**vkAllocateMemory**
```c
VkResult vkAllocateMemory(
    VkDevice device,
    const VkMemoryAllocateInfo* pAllocateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDeviceMemory* pMemory);
```

**VkMemoryAllocateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO)
- pNext: Pointer to next structure in chain
- allocationSize: Size of memory allocation
- memoryTypeIndex: Index of memory type to allocate

**vkBindBufferMemory**
```c
VkResult vkBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory memory,
    VkDeviceSize memoryOffset);
```

### 8. Pipeline Creation

**vkCreateShaderModule**
```c
VkResult vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule);
```

**VkShaderModuleCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Shader module creation flags
- codeSize: Size of shader code
- pCode: Pointer to shader code

**vkCreateGraphicsPipelines**
```c
VkResult vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines);
```

### 9. Render Passes

**vkCreateRenderPass**
```c
VkResult vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkRenderPass* pRenderPass);
```

**VkRenderPassCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Render pass creation flags
- attachmentCount: Number of attachments
- pAttachments: Array of attachment descriptions
- subpassCount: Number of subpasses
- pSubpasses: Array of subpass descriptions
- dependencyCount: Number of dependencies
- pDependencies: Array of subpass dependencies

### 10. Framebuffer Creation

**vkCreateFramebuffer**
```c
VkResult vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFramebuffer* pFramebuffer);
```

**VkFramebufferCreateInfo** structure:
- sType: Structure type (VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO)
- pNext: Pointer to next structure in chain
- flags: Framebuffer creation flags
- renderPass: Render pass the framebuffer is compatible with
- attachmentCount: Number of attachments
- pAttachments: Array of attachment image views
- width: Framebuffer width
- height: Framebuffer height
- layers: Framebuffer layers

### 11. Synchronization Primitives

**vkCreateSemaphore**
```c
VkResult vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSemaphore* pSemaphore);
```

**vkCreateFence**
```c
VkResult vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkFence* pFence);
```

### 12. Drawing Commands

**vkCmdBeginRenderPass**
```c
void vkCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkSubpassContents contents);
```

**vkCmdDraw**
```c
void vkCmdDraw(
    VkCommandBuffer commandBuffer,
    uint32_t vertexCount,
    uint32_t instanceCount,
    uint32_t firstVertex,
    uint32_t firstInstance);
```

**vkCmdEndRenderPass**
```c
void vkCmdEndRenderPass(VkCommandBuffer commandBuffer);
```

### 13. Extension Functions

Many Vulkan functions are provided through extensions. Common ones include:
- VK_KHR_swapchain: For swapchain functionality
- VK_KHR_surface: For surface creation
- VK_EXT_debug_utils: For debugging utilities
- VK_KHR_get_physical_device_properties2: For extended physical device properties

## Key Data Structures

### VkStructureType
Enumeration of structure types used for identifying structure types.

### VkFlags
Bitmask type for various flags.

### VkResult
Enumeration of return codes indicating success or failure.

### VkBool32
Boolean type (VK_TRUE/VK_FALSE).

## Common Patterns

1. **Create/Destroy Pattern**: Most Vulkan objects follow a create/destroy pattern
2. **Info Structures**: Most creation functions take info structures with sType and pNext fields
3. **Allocation Callbacks**: Most functions accept allocation callbacks for custom memory management
4. **Null Handles**: Vulkan uses VK_NULL_HANDLE for null handles instead of NULL

## Important Notes

- All Vulkan functions return VkResult values indicating success or failure
- Memory management is explicit and manual
- Synchronization is critical for correct operation
- Error checking is essential as Vulkan is very strict about correct usage
- Extensions provide additional functionality not part of the core API

This overview covers the main components of the Vulkan API. For complete details, refer to the official Vulkan specification and reference pages.