struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

struct VertexInput {
    [[vk::location(0)]] float2 inPosition;
    [[vk::location(1)]] float3 inColor;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 fragColor;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 0.0, 1.0))));
    output.fragColor = input.inColor;
    return output;
}

[shader("fragment")]
float4 fragmentMain([[vk::location(0)]] float3 fragColor) : SV_Target {
    return float4(fragColor, 1.0);
}
