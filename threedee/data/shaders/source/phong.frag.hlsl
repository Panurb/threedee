Texture2DArray<float4> tex : register(t0, space2);
Texture2DArray<float> shadow_maps : register(t1, space2);
SamplerState sampler_tex : register(s0, space2);
SamplerState sampler_shadow_maps : register(s1, space2);

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
    float3 position : packoffset(c0);
    uint visibility_mask : packoffset(c0.w);
    float3 direction : packoffset(c1);
    float cutoff_cos : packoffset(c1.w);
    float3 diffuse_color : packoffset(c2);
    float3 specular_color : packoffset(c3);
    float4x4 projection_view_matrix : packoffset(c4);
};

cbuffer LightBuffer : register(b1, space3)
{
    LightData light_data[32];  // Should match MAX_LIGHTS
};

struct Input
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    int tex_index : TEXCOORD1;
    int normal_index : TEXCOORD2;
    float3 world_position : POSITION0;
    float specular;
    float diffuse;
    float ambient;
    float shininess;
    uint visibility;
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
    float2 tex_coord = input.tex_coord;
    float4 position = input.position;
    float3 normal = input.normal;
    float3 tangent = input.tangent;
    float3 world_position = input.world_position;
    float2 texel_size = 1.0 / float2(shadow_map_resolution, shadow_map_resolution);

    float3 base_color = tex.Sample(sampler_tex, float3(tex_coord, input.tex_index)).rgb;

    float3 normal_map = tex.Sample(sampler_tex, float3(tex_coord, input.normal_index)).rgb;
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

    float3 ambient = input.ambient * ambient_light * base_color;
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    float combined_spot_intensity = 0.0;

    for (int i = 0; i < num_lights; ++i)
    {
        if ((light_data[i].visibility_mask & input.visibility) == 0) {
            continue;
        }

        float3 l = normalize(light_data[i].position - world_position);
        float3 r = reflect(-l, n);

        float diff = max(dot(n, l), 0.0);

        float spot_cos = dot(-l, light_data[i].direction);
        float spot_intensity = saturate((spot_cos - light_data[i].cutoff_cos) / (1.0 - light_data[i].cutoff_cos));

        if (diff <= 0.0 || spot_intensity <= 0.0) {
            continue;
        }

        float spec = pow(max(dot(r, v), 0.0), input.shininess);

        float4 shadow_coord = mul(light_data[i].projection_view_matrix, float4(world_position, 1.0));
        shadow_coord.xyz /= shadow_coord.w;
        float2 shadow_uv = shadow_coord.xy * 0.5 + 0.5;
        shadow_uv.y = 1.0 - shadow_uv.y;
        float shadow_depth = shadow_coord.z;

        bool in_bounds = all(shadow_uv >= 0.0) && all(shadow_uv <= 1.0) && (shadow_coord.z >= 0.0) && (shadow_coord.z <= 1.0);

        float shadow = shadow_pcf(shadow_uv, i, shadow_depth, texel_size, 3);

        float shadow_strength = lerp(1.0, 0.25, shadow);
        float light_shadow_factor = lerp(0.25, shadow_strength, shadow_uv);

        combined_spot_intensity = max(combined_spot_intensity, spot_intensity);
        diff *= spot_intensity;
        spec *= spot_intensity;

        diffuse += base_color * diff * light_data[i].diffuse_color * light_shadow_factor;
        specular += input.specular * spec * light_data[i].specular_color * light_shadow_factor;
    }

    float3 lit_color = ambient + diffuse + specular;
    if (length(lit_color) < 0.001) {
        discard;
    }

    // Only hidden entities (ambient = 0) should be faded out
    float alpha = saturate(combined_spot_intensity);
    if (input.ambient > 0.0) {
        alpha = 1.0;
    }

    float fog_factor = saturate((fog_end - distance) / (fog_end - fog_start));
    float3 fogged_color = lerp(fog_color, lit_color, fog_factor);

    Output result;
    result.color = float4(fogged_color, alpha);
    //result.color = float4(fade.xxx, 1.0);
    result.depth = position.z;
    return result;
}
