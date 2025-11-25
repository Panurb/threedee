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

cbuffer LightBuffer : register(b1, space3)
{
    LightData light_data[32];  // Should match MAX_LIGHTS
};

struct Material {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
};

StructuredBuffer<Material> materials : register(t4, space2);

StructuredBuffer<LightData> lights : register(t5, space2);

struct Input
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    int tex_index : TEXCOORD1;
    int emissive_index : TEXCOORD2;
    float3 world_position : POSITION0;
    int material_index : TEXCOORD3;
    uint visibility : TEXCOORD4;
    float emissive : TEXCOORD5;
};

struct Output
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

float radial_fade(float2 uv) {
    float2 center_uv = float2(0.5, 0.5);
    float dist = distance(uv, center_uv);
    return 1.0 - smoothstep(0.4, 0.5, dist);
}

float shadow_pcf(float2 uv, int light_index, float shadow_depth, float2 texel_size, int kernel_radius = 1)
{
    float shadow = 0.0;
    for (int x = -kernel_radius; x <= kernel_radius; ++x) {
        for (int y = -kernel_radius; y <= kernel_radius; ++y) {
            float2 offset = float2(x, y) * texel_size;
            float sample_depth = shadow_maps.Sample(sampler_shadow_maps, float3(uv + offset, light_index)).r;
            if (shadow_depth - 0.0005 > sample_depth)
                shadow += 1.0;
        }
    }
    return shadow / ((2 * kernel_radius + 1) * (2 * kernel_radius + 1));
}

Output main(Input input)
{
    Material material = materials[input.material_index];
    float2 tex_coord = input.tex_coord;
    float4 position = input.position;
    float3 normal = input.normal;
    float3 tangent = input.tangent;
    float3 world_position = input.world_position;
    float2 texel_size = 1.0 / float2(shadow_map_resolution, shadow_map_resolution);

    float4 sampled_color = tex.Sample(sampler_tex, float3(tex_coord, input.tex_index));
    float alpha = sampled_color.a;
    if (alpha == 0.0) {
        discard;
    }
    float3 base_color = sampled_color.rgb;
    base_color = pow(base_color, float3(2.2)); // Convert to linear space

    float3 normal_map = normal_tex.Sample(sampler_normal_tex, float3(tex_coord, input.tex_index)).rgb;
    normal_map = normal_map * 2.0 - 1.0; // Convert to range [-1, 1]

    // Transform normal from tangent space to world space
    float3 N = normalize(normal);
    float3 T = normalize(tangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt orthogonalization
    float3 B = cross(N, T);

    float3x3 tangent_to_world = float3x3(T, B, N);

    // Multiply from right
    float3 n = normalize(mul(normal_map, tangent_to_world));

    float3 view_direction = camera_position - world_position;
    float distance = length(view_direction);
    float3 v = view_direction / distance;

    float3 ambient = material.ambient * ambient_light * base_color;
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    float combined_spot_intensity = 0.0;

    for (int i = 0; i < num_lights; ++i) {
        LightData light = lights[i];

        if ((light.visibility_mask & input.visibility) == 0) {
            continue;
        }

        float3 l = normalize(light.position - world_position);
        float3 r = reflect(-l, n);

        float diff = max(dot(n, l), 0.0);

        float spot_cos = dot(-l, light.direction);
        float spot_intensity = saturate((spot_cos - light.cutoff_cos) / (1.0 - light.cutoff_cos));

        float light_distance = length(light.position - world_position);
        float attenuation = 1.0 / (1.0 + 0.1 * light_distance + 0.01 * light_distance * light_distance);
        float fade = smoothstep(light.range, 0.8 * light.range, light_distance);
        attenuation *= fade;
        spot_intensity *= attenuation;

        if (diff <= 0.0 || spot_intensity <= 0.0) {
            continue;
        }

        float spec = pow(max(dot(r, v), 0.0), material.shininess);

        float4 shadow_coord = mul(light.projection_view_matrix, float4(world_position, 1.0));
        shadow_coord.xyz /= shadow_coord.w;
        float2 shadow_uv = shadow_coord.xy * 0.5 + 0.5;

        // Need to flip Y coordinate, origin is top left in Vulkan
        shadow_uv.y = 1.0 - shadow_uv.y;
        float shadow_depth = shadow_coord.z;

        bool in_bounds = all(shadow_uv >= 0.0) && all(shadow_uv <= 1.0) && (shadow_coord.z >= 0.0) && (shadow_coord.z <= 1.0);

        float shadow = shadow_pcf(shadow_uv, i, shadow_depth, texel_size, 3);

        float shadow_strength = lerp(1.0, 0.25, shadow);
        float light_shadow_factor = lerp(0.25, shadow_strength, shadow_uv);

        combined_spot_intensity = max(combined_spot_intensity, spot_intensity);
        diff *= spot_intensity;
        spec *= spot_intensity;

        float3 diffuse_color = light.diffuse_color;
        float3 specular_color = light.specular_color;

        // Hidden entities light up in different color
        // TODO: parametrize the color
        if (material.ambient == 0.0) {
            diffuse_color = float3(0.2, 5.0, 2.0);
            specular_color = float3(0.2, 5.0, 2.0);
            base_color = float3(1.0, 1.0, 1.0);
        }

        diffuse += base_color * diff * diffuse_color * light_shadow_factor;
        specular += material.specular * spec * specular_color * light_shadow_factor;
    }

    float base_emissive = 1.0;
    if (input.emissive_index != -1) {
        base_emissive = emissive_maps.Sample(sampler_emissive_maps, float3(tex_coord, input.emissive_index)).r;
    }
    float3 emissive = input.emissive * base_emissive * base_color;

    float3 lit_color = ambient + diffuse + specular + emissive;

    // Only hidden entities (ambient = 0) should be faded out
    if (material.ambient == 0.0) {
        alpha *= saturate(combined_spot_intensity);
    }

    float fog_factor = saturate((fog_end - distance) / (fog_end - fog_start));
    float3 fogged_color = lerp(fog_color, lit_color, fog_factor);

    Output result;
    result.color = float4(fogged_color, alpha);
    result.depth = position.z;
    return result;
}
