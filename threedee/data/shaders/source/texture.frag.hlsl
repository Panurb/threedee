Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

struct Input
{
    float2 tex_coord : TEXCOORD0;
};

struct Output
{
    float4 color : SV_Target0;
};

Output main(Input input)
{
    return tex.Sample(sampler_tex, input.tex_coord);
}
