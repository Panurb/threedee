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

float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

struct Input {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};

float4 main(Input input) : SV_Target {
    float3 result = tex.Sample(sampler_tex, input.tex_coord).rgb * weights[0];

    for (int i = 1; i < 5; ++i) {
        float2 offset = vertical ? float2(0.0, texel_size.y * i) : float2(texel_size.x * i, 0.0);
        result += tex.Sample(sampler_tex, input.tex_coord + offset).rgb * weights[i];
        result += tex.Sample(sampler_tex, input.tex_coord - offset).rgb * weights[i];
    }

    return float4(result, 1.0);
}