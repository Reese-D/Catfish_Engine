#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#include <glm/glm.hpp>

#include "graphics_pipeline.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

GraphicsPipeline::GraphicsPipeline(const vk::raii::Device &mDevice, vk::Format swapChainFormat, vk::Format depthFormat) {
    // Set 0: UBO (view + proj), vertex stage
    auto uboBinding = vk::DescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
    };
    m_uboLayout = std::make_shared<vk::raii::DescriptorSetLayout>(mDevice, vk::DescriptorSetLayoutCreateInfo{.bindingCount = 1, .pBindings = &uboBinding});

    // Set 1: combined image sampler, fragment stage
    auto texBinding = vk::DescriptorSetLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };
    m_textureLayout = std::make_shared<vk::raii::DescriptorSetLayout>(mDevice, vk::DescriptorSetLayoutCreateInfo{.bindingCount = 1, .pBindings = &texBinding});

    auto vertCode = readShaderFile("shaders/vert.spv");
    auto fragCode = readShaderFile("shaders/frag.spv");
    auto vertModule = createShaderModule(mDevice, vertCode);
    auto fragModule = createShaderModule(mDevice, fragCode);

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

    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDesc,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size()),
        .pVertexAttributeDescriptions = attrDescs.data(),
    };

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
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    auto depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
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

    auto pushConstantRange = vk::PushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4),
    };
    std::array<vk::DescriptorSetLayout, 2> rawLayouts = {**m_uboLayout, **m_textureLayout};
    m_pipelineLayout = std::make_shared<vk::raii::PipelineLayout>(
        mDevice, vk::PipelineLayoutCreateInfo{
                     .setLayoutCount = static_cast<uint32_t>(rawLayouts.size()),
                     .pSetLayouts = rawLayouts.data(),
                     .pushConstantRangeCount = 1,
                     .pPushConstantRanges = &pushConstantRange,
                 }
    );

    auto pipelineChain = vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo>{
        vk::GraphicsPipelineCreateInfo{
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pViewportState = &viewportStateInfo,
            .pRasterizationState = &rasterizerInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = &depthStencilInfo,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = **m_pipelineLayout,
        },
        vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainFormat,
            .depthAttachmentFormat = depthFormat,
        },
    };

    m_pipeline = std::make_shared<vk::raii::Pipeline>(mDevice, nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
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

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const vk::raii::Device &mDevice, const std::vector<char> &code) {
    return vk::raii::ShaderModule{
        mDevice, vk::ShaderModuleCreateInfo{
                     .codeSize = code.size(),
                     .pCode = reinterpret_cast<const uint32_t *>(code.data()),
                 }
    };
}

} // namespace VulkanHelpers
