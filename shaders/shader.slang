struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

[[vk::binding(1, 0)]]
Sampler2D texSampler;

struct VertexInput {
    [[vk::location(0)]] float3 inPosition;
    [[vk::location(1)]] float3 inColor;
    [[vk::location(2)]] float2 inTexCoord;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 fragColor;
    [[vk::location(1)]] float2 fragTexCoord;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0))));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragmentMain([[vk::location(0)]] float3 fragColor, [[vk::location(1)]] float2 fragTexCoord) : SV_Target {
    return texSampler.Sample(fragTexCoord);
}
