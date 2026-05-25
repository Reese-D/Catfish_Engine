#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#include "graphics_pipeline.h"

namespace VulkanHelpers {

GraphicsPipeline::GraphicsPipeline(const vk::raii::Device &device, vk::Format swapChainFormat) {
    auto vertCode = readShaderFile("shaders/vert.spv");
    auto fragCode = readShaderFile("shaders/frag.spv");
    auto vertModule = createShaderModule(device, vertCode);
    auto fragModule = createShaderModule(device, fragCode);

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertModule,
            .pName = "main",
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragModule,
            .pName = "main",
        },
    };

    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{};

    auto inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False,
    };

    auto viewportStateInfo = vk::PipelineViewportStateCreateInfo{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    auto rasterizerInfo = vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    auto colorBlendInfo = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    std::array<vk::DynamicState, 2> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    auto dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    pipelineLayout = std::make_shared<vk::raii::PipelineLayout>(device, vk::PipelineLayoutCreateInfo{});

    auto pipelineChain = vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo>{
        vk::GraphicsPipelineCreateInfo{
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pViewportState = &viewportStateInfo,
            .pRasterizationState = &rasterizerInfo,
            .pMultisampleState = &multisampleInfo,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = **pipelineLayout,
        },
        vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainFormat,
        },
    };

    pipeline = std::make_shared<vk::raii::Pipeline>(device, nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
}

std::vector<char> GraphicsPipeline::readShaderFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    auto fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
    return buffer;
}

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const vk::raii::Device &device, const std::vector<char> &code) {
    return vk::raii::ShaderModule{
        device, vk::ShaderModuleCreateInfo{
                    .codeSize = code.size(),
                    .pCode = reinterpret_cast<const uint32_t *>(code.data()),
                }
    };
}

} // namespace VulkanHelpers
