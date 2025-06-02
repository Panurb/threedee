Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float near_plane : packoffset(c0);
    float far_plane : packoffset(c0.y);
    float3 light_direction : packoffset(c1);
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

Output main(float2 tex_coord : TEXCOORD0, float4 position : SV_Position, float3 normal : NORMAL0)
{
    float ambient_light = 0.2;

    Output result;
    float3 n = normalize(normal);
    float3 l = normalize(-light_direction);
    float diff = max(dot(n, l), 0.0);

    float3 base_color = tex.Sample(sampler_tex, tex_coord).rgb;
    float3 lit_color = base_color * diff + ambient_light * base_color;

    result.color = float4(lit_color, 1.0);
    result.depth = linearize_depth(position.z, near_plane, far_plane);
    return result;
}
