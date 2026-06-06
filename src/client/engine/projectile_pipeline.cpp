#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#include <glm/glm.hpp>

#include "projectile_pipeline.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

// Push constant layout shared between vertex and fragment stages:
//   offset  0: float4x4 model  (64 bytes)
//   offset 64: float     time  ( 4 bytes)
//   offset 68: uint  shaderType( 4 bytes)
//   total: 72 bytes
struct ProjectilePushConstants {
    glm::mat4 model;
    float time;
    uint32_t shaderType;
};

ProjectilePipeline::ProjectilePipeline(
    const vk::raii::Device &device, vk::Format swapChainFormat, vk::Format depthFormat, vk::DescriptorSetLayout uboLayout, vk::DescriptorSetLayout textureLayout
) {
    auto vertCode = readShaderFile("shaders/projectile_vert.spv");
    auto fragCode = readShaderFile("shaders/projectile_frag.spv");
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

    // Depth test on (occluded by terrain) but no depth write (transparent effect).
    auto depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
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
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(ProjectilePushConstants),
    };

    std::array<vk::DescriptorSetLayout, 2> rawLayouts = {uboLayout, textureLayout};
    m_pipelineLayout = std::make_shared<vk::raii::PipelineLayout>(
        device, vk::PipelineLayoutCreateInfo{
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

    m_pipeline = std::make_shared<vk::raii::Pipeline>(device, nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
}

std::vector<char> ProjectilePipeline::readShaderFile(const std::string &filename) {
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

vk::raii::ShaderModule ProjectilePipeline::createShaderModule(const vk::raii::Device &device, const std::vector<char> &code) {
    return vk::raii::ShaderModule{
        device, vk::ShaderModuleCreateInfo{
                    .codeSize = code.size(),
                    .pCode = reinterpret_cast<const uint32_t *>(code.data()),
                }
    };
}

} // namespace VulkanHelpers
