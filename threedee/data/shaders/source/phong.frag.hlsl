Texture2D<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float near_plane : packoffset(c0);
    float far_plane : packoffset(c0.y);
    float3 camera_position : packoffset(c1);
    float3 light_position : packoffset(c2);
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

Output main(
    float2 tex_coord : TEXCOORD0,
    float4 position : SV_Position,
    float3 normal : NORMAL0,
    float4 tex_rect : TEXCOORD1,
    float3 world_position : TEXCOORD2
)
{
    float ambient_light = 0.2;

    float2 atlas_uv = tex_rect.xy + frac(tex_coord) * tex_rect.zw;

    float3 light_direction = light_position - world_position;
    float3 view_direction = camera_position - world_position;

    Output result;
    float3 n = normalize(normal);
    float3 l = normalize(light_direction);
    float3 v = normalize(view_direction);
    float3 r = reflect(-l, n);

    float diff = max(dot(n, l), 0.0);

    // Phong specular
    float specular_strength = 0.5;
    float shininess = 32.0;
    float spec = pow(max(dot(r, v), 0.0), shininess);

    float3 base_color = tex.Sample(sampler_tex, atlas_uv).rgb;
    float3 ambient = ambient_light * base_color;
    float3 diffuse = base_color * diff;
    float3 specular = specular_strength * spec * float3(1.0, 1.0, 1.0);

    float3 lit_color = ambient + diffuse + specular;

    result.color = float4(lit_color, 1.0);
    result.depth = linearize_depth(position.z, near_plane, far_plane);
    return result;
}
