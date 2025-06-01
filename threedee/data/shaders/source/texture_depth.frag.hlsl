Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float near_plane;
    float far_plane;
};

struct Output
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

float linearize_depth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return ((2.0 * near * far) / (far + near - z * (far - near))) / far;
}

Output main(float2 tex_coord : TEXCOORD0, float4 position : SV_Position)
{
    Output result;
    result.color = tex.Sample(sampler_tex, tex_coord);
    result.depth = linearize_depth(position.z, near_plane, far_plane);
    return result;
}
