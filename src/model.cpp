#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>
#include <span>
#include <stdexcept>

#include "model.h"
#include "texture_image.h"

namespace VulkanHelpers {

Model::Model(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const std::string &path
) {
    auto dataBuffer = fastgltf::GltfDataBuffer::FromPath(path);
    if (dataBuffer.error() != fastgltf::Error::None) {
        throw std::runtime_error("Failed to load model file: " + std::string(fastgltf::getErrorMessage(dataBuffer.error())));
    }

    fastgltf::Parser parser;
    auto asset = parser.loadGltfBinary(dataBuffer.get(), std::filesystem::path(path).parent_path(), fastgltf::Options::None);
    if (asset.error() != fastgltf::Error::None) {
        throw std::runtime_error("Failed to parse glTF: " + std::string(fastgltf::getErrorMessage(asset.error())));
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto &mesh : asset->meshes) {
        for (const auto &primitive : mesh.primitives) {
            uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

            // Positions (required)
            auto posIt = primitive.findAttribute("POSITION");
            if (posIt == primitive.attributes.end()) {
                continue;
            }
            const auto &posAccessor = asset->accessors[posIt->accessorIndex];
            std::size_t vertexCount = posAccessor.count;

            vertices.resize(vertices.size() + vertexCount);
            std::size_t i = 0;
            fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset.get(), posAccessor, [&](fastgltf::math::fvec3 pos) {
                auto &v = vertices[vertexOffset + i++];
                v.pos[0] = pos[0];
                v.pos[1] = pos[1];
                v.pos[2] = pos[2];
                v.color[0] = 1.0f;
                v.color[1] = 1.0f;
                v.color[2] = 1.0f;
                v.texCoord[0] = 0.0f;
                v.texCoord[1] = 0.0f;
            });

            // Texture coordinates (optional)
            auto texIt = primitive.findAttribute("TEXCOORD_0");
            if (texIt != primitive.attributes.end()) {
                const auto &texAccessor = asset->accessors[texIt->accessorIndex];
                std::size_t j = 0;
                fastgltf::iterateAccessor<fastgltf::math::fvec2>(asset.get(), texAccessor, [&](fastgltf::math::fvec2 uv) {
                    auto &v = vertices[vertexOffset + j++];
                    v.texCoord[0] = uv[0];
                    v.texCoord[1] = uv[1];
                });
            }

            // Indices
            if (primitive.indicesAccessor.has_value()) {
                const auto &indexAccessor = asset->accessors[*primitive.indicesAccessor];
                fastgltf::iterateAccessor<uint32_t>(asset.get(), indexAccessor, [&](uint32_t index) {
                    indices.push_back(vertexOffset + index);
                });
            }
        }
    }

    if (vertices.empty() || indices.empty()) {
        throw std::runtime_error("Model contains no geometry: " + path);
    }

    vertexBuffer = std::make_shared<VertexBuffer>(device, physicalDevice, commandPool, graphicsQueue, vertices);
    indexBuffer = std::make_shared<IndexBuffer>(device, physicalDevice, commandPool, graphicsQueue, indices);

    // Extract the base color texture from the first primitive's material
    fastgltf::DefaultBufferDataAdapter adapter;
    for (const auto &mesh : asset->meshes) {
        for (const auto &primitive : mesh.primitives) {
            if (!primitive.materialIndex.has_value()) continue;

            const auto &material = asset->materials[*primitive.materialIndex];
            if (!material.pbrData.baseColorTexture.has_value()) continue;

            std::size_t texIdx = material.pbrData.baseColorTexture->textureIndex;
            const auto &texture = asset->textures[texIdx];
            if (!texture.imageIndex.has_value()) continue;

            const auto &img = asset->images[*texture.imageIndex];
            if (const auto *bv = std::get_if<fastgltf::sources::BufferView>(&img.data)) {
                auto bytes = adapter(asset.get(), bv->bufferViewIndex);
                textureImage = std::make_shared<TextureImage>(
                    device, physicalDevice, commandPool, graphicsQueue,
                    std::span<const std::byte>(bytes.data(), bytes.size())
                );
                return;
            }
        }
    }

    throw std::runtime_error("Model has no usable base color texture: " + path);
}

} // namespace VulkanHelpers
