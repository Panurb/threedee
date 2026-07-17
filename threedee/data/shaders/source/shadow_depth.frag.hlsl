Texture2DArray<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

struct Input
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD0;
    int tex_index;
};

struct Output
{
    float4 color : SV_Target0;
};

Output main(Input input)
{
    float4 sampled_color = tex.Sample(sampler_tex, float3(input.tex_coord, input.tex_index));
    float alpha = sampled_color.a;
    if (alpha == 0.0) {
        discard;
    }

    Output output;
    output.color = float4(1.0, 0.0, 0.0, 1.0); // Color doesn't matter for depth-only pass
    return output;
}