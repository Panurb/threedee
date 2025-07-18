Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float4 color : packoffset(c0);
};

struct Input
{
    float2 tex_coord : TEXCOORD0;
};

float4 main(Input input) : SV_Target0
{
    float alpha = tex.Sample(sampler_tex, input.tex_coord).a;
    return float4(color.rgb, alpha * color.a);
}
