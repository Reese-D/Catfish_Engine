struct UniformBufferObject {
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

// Unused for procedural rendering; kept for pipeline layout compatibility.
[[vk::binding(0, 1)]]
Sampler2D texSampler;

struct PushConstants {
    float4x4 model;
    float time;
    uint shaderType; // 0 = fireball, 1 = gravity well
};

[[vk::push_constant]]
PushConstants push;

struct VertexInput {
    [[vk::location(0)]] float3 inPosition;
    [[vk::location(1)]] float3 inColor;
    [[vk::location(2)]] float2 inTexCoord;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(push.model, float4(input.inPosition, 1.0))));
    output.uv = input.inTexCoord;
    return output;
}

// ---- Fireball: hot white core → yellow → orange → red → transparent ----------

float4 fireball(float2 uv, float t) {
    float2 c = uv - 0.5;
    float raw = length(c) * 2.0; // 0 at center, 1 at rim

    // Turbulent flicker: offset distance slightly with angle/time noise
    float angle = atan2(c.y, c.x);
    float noise = 0.06 * sin(t * 13.0 + angle * 4.0)
                + 0.03 * sin(t * 20.0 + raw * 9.0);
    float d = clamp(raw + noise, 0.0, 1.05);

    if (d >= 1.0) return float4(0.0, 0.0, 0.0, 0.0);

    float4 col;
    if (d < 0.15) {
        col = float4(1.0, 1.0, 0.92, 1.0);
    } else if (d < 0.35) {
        float f = (d - 0.15) / 0.20;
        col = lerp(float4(1.0, 1.0, 0.6, 1.0), float4(1.0, 0.48, 0.04, 1.0), f);
    } else if (d < 0.65) {
        float f = (d - 0.35) / 0.30;
        col = lerp(float4(1.0, 0.48, 0.04, 1.0), float4(0.85, 0.06, 0.0, 1.0), f);
    } else {
        float f = (d - 0.65) / 0.35;
        col = lerp(float4(0.85, 0.06, 0.0, 1.0), float4(0.0, 0.0, 0.0, 0.0), f);
    }

    float pulse = 0.85 + 0.15 * sin(t * 7.0);
    col.rgb *= pulse;
    return col;
}

// ---- Gravity well: black event horizon + blue/purple accretion disk ----------

float4 gravityWell(float2 uv, float t) {
    float2 c = uv - 0.5;
    float dist = length(c) * 2.0;

    if (dist >= 1.0) return float4(0.0, 0.0, 0.0, 0.0);

    // Swirl UV: rotate faster near the centre to fake frame-dragging
    float swirl = t * 2.5 / max(dist, 0.15);
    float cosS = cos(swirl);
    float sinS = sin(swirl);
    float2 rot = float2(c.x * cosS - c.y * sinS,
                        c.x * sinS + c.y * cosS);
    float swirlAngle = atan2(rot.y, rot.x);

    // Azimuthal brightness variation in the disk
    float ringVar = 0.5 + 0.5 * sin(swirlAngle * 6.0 + t * 2.0);

    float4 col;
    if (dist < 0.22) {
        // Event horizon — pure black
        col = float4(0.0, 0.0, 0.0, 1.0);
    } else if (dist < 0.50) {
        float f = (dist - 0.22) / 0.28;
        float brightness = (1.0 - f * 0.6) * (0.65 + 0.35 * ringVar);
        // white/lavender near horizon → deep purple/blue at disk edge
        float3 disk = lerp(float3(0.95, 0.80, 1.0), float3(0.18, 0.04, 0.90), f) * brightness;
        col = float4(disk, 1.0);
    } else if (dist < 0.80) {
        float f = (dist - 0.50) / 0.30;
        float alpha = 1.0 - f;
        float brightness = 0.35 * (0.4 + 0.6 * ringVar) * (1.0 - f);
        float3 glow = float3(0.10, 0.02, 0.60) * brightness;
        col = float4(glow, alpha);
    } else {
        float f = (dist - 0.80) / 0.20;
        col = float4(0.0, 0.0, 0.0, 1.0 - f);
    }

    return col;
}

// ---- Lightning bolt: jagged white/blue streak on an elongated quad -----------
// uv.x = cross-axis [0,1], uv.y = along-axis [0,1]

float4 lightning(float2 uv, float t) {
    // Jagged bolt centerline — layered sin waves of increasing frequency
    float jag = 0.08 * sin(uv.y * 14.0 + t * 22.0)
              + 0.04 * sin(uv.y * 29.0 - t * 35.0)
              + 0.02 * sin(uv.y * 55.0 + t * 18.0);
    float center = 0.5 + jag;
    float du = abs(uv.x - center);

    // Core and glow bands
    float core = max(0.0, 1.0 - du * 18.0);
    float glow = max(0.0, 1.0 - du *  6.0);

    // Secondary branch — offset from main bolt
    float branchJag = jag * 0.6 + 0.05 * sin(uv.y * 22.0 + t * 19.0 + 1.7);
    float branchDu = abs(uv.x - 0.5 - branchJag);
    float branch = max(0.0, 0.5 * (1.0 - branchDu * 22.0));

    // Fade at the tip ends so the bolt doesn't hard-clip
    float endFade = smoothstep(0.0, 0.08, uv.y) * (1.0 - smoothstep(0.92, 1.0, uv.y));

    float3 col = float3(1.0, 1.0, 1.0) * core
               + float3(0.45, 0.65, 1.0) * glow * 0.7
               + float3(0.35, 0.55, 1.0) * branch;

    float alpha = (glow + branch * 0.6) * endFade;

    // High-frequency flicker
    float flicker = 0.75 + 0.25 * sin(t * 50.0 + uv.y * 5.0);
    return float4(col * flicker, alpha * flicker);
}

[shader("fragment")]
float4 fragmentMain([[vk::location(0)]] float2 uv) : SV_Target {
    if (push.shaderType == 0u)
        return fireball(uv, push.time);
    if (push.shaderType == 1u)
        return gravityWell(uv, push.time);
    return lightning(uv, push.time);
}
