Texture2DArray<float4> tex : register(t0, space2);
Texture2DArray<float3> normal_tex : register(t1, space2);
Texture2DArray<float> shadow_maps : register(t2, space2);
Texture2DArray<float> emissive_maps : register(t3, space2);
SamplerState sampler_tex : register(s0, space2);
SamplerState sampler_normal_tex : register(s1, space2);
SamplerState sampler_shadow_maps : register(s2, space2);
SamplerState sampler_emissive_maps : register(s3, space2);

cbuffer UBO : register(b0, space3)
{
    float near_plane : packoffset(c0);
    float far_plane : packoffset(c0.y);
    float ambient_light : packoffset(c0.z);
    int num_lights : packoffset(c0.w);
    float3 camera_position : packoffset(c1);
    int shadow_map_resolution : packoffset(c1.w);
    float3 fog_color : packoffset(c2);
    float fog_start : packoffset(c3);
    float fog_end : packoffset(c3.y);
};

struct Material {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
};

StructuredBuffer<Material> materials : register(t4, space2);

struct LightData
{
    float3 position;
    uint visibility_mask;
    float3 direction;
    float cutoff_cos;
    float3 diffuse_color;
    float range;
    float3 specular_color;
    float4x4 projection_view_matrix;
};

StructuredBuffer<LightData> lights : register(t5, space2);

struct Input
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    int tex_index : TEXCOORD1;
    float3 world_position : TEXCOORD2;
    uint visibility : TEXCOORD3;
	float4 color : COLOR0;
};

struct Output
{
    float4 color : SV_Target0;
};

Output main(Input input)
{
    float4 sampled_color = tex.Sample(sampler_tex, float3(input.tex_coord, input.tex_index));

    float3 view_dir = normalize(camera_position - input.world_position);

    float3 final_color = float3(ambient_light, ambient_light, ambient_light);

    for (int i = 0; i < num_lights; ++i) {
        LightData light = lights[i];

        if ((light.visibility_mask & input.visibility) == 0) {
            continue;
        }

        float3 light_dir = normalize(light.position - input.world_position);

        // Angle-based forward scattering
        float scatter = saturate(dot(view_dir, light_dir));
        scatter = pow(scatter, 8.0f);

        // Distance attenuation
        float distance = length(light.position - input.world_position);
        float attenuation = saturate(1.0f - (distance / light.range));

        float spot_atten = saturate((dot(-light_dir, light.direction) - light.cutoff_cos) / (1.0 - light.cutoff_cos));
        attenuation *= spot_atten;

        if (attenuation <= 0.0f) {
            continue;
        }

        final_color += light.diffuse_color * attenuation * scatter;
    }

    Output result;
	float4 color = input.color;
    result.color = float4(color.rgb * final_color, sampled_color.a * color.a);
    return result;
}
