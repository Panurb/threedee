Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

struct Input
{
    float2 tex_coord : TEXCOORD0;
};

float4 main(Input input) : SV_Target0
{
    float alpha = tex.Sample(sampler_tex, input.tex_coord).a;
    return float4(1.0f, 1.0f, 1.0f, alpha);
}
