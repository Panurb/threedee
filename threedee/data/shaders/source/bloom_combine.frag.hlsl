// Common post-processing descriptor set
Texture2D tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3) {
    float near_plane;
    float far_plane;
    float2 screen_size;
};
// End of common descriptor set

Texture2D bloom_tex : register(t1, space2);

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
    float3 scene = tex.Sample(sampler_tex, input.tex_coord).rgb;
    float3 bloom = bloom_tex.Sample(sampler_tex, input.tex_coord).rgb;

    return float4(scene + bloom * bloom_strength, 1.0);
}