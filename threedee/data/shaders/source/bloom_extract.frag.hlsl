// Common post-processing descriptor set
Texture2D tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3) {
    float near_plane;
    float far_plane;
    float2 screen_size;
};
// End of common descriptor set

cbuffer BloomParams : register(b1, space3) {
    float bloom_threshold;
    float bloom_knee;
    float bloom_intensity;
    float bloom_strength;
    float2 texel_size;
    bool vertical;
};

struct Input {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};

float4 main(Input input) : SV_Target {
    float3 color = tex.Sample(sampler_tex, input.tex_coord).rgb;

    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));

    float bloom = saturate((luminance - bloom_threshold + bloom_knee) / (2.0 * bloom_knee));

    return float4(color * bloom * bloom_intensity, 1.0);
}