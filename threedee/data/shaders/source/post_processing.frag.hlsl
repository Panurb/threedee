Texture2D tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);


struct Input {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};


float4 main(Input input) : SV_Target {
    float3 color = tex.Sample(sampler_tex, input.tex_coord).rgb;

    // ACES Filmic Tone Mapping
    color = max(color, 0.0);
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
    color = saturate(color);
    color = pow(color, float3(1.0 / 2.2)); // Gamma

    return float4(color, 1.0);
}
