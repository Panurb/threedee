Texture2DArray<float4> tex : register(t0, space2);
SamplerState sampler_tex : register(s0, space2);

cbuffer UBO : register(b0, space3)
{
    float near_plane : packoffset(c0);
    float far_plane : packoffset(c0.y);
    float ambient_light : packoffset(c0.z);
    int num_lights : packoffset(c0.w);
    float3 camera_position : packoffset(c1);
};

struct LightData
{
    float3 position : packoffset(c0);
    float3 diffuse_color : packoffset(c1);
    float3 specular_color : packoffset(c2);
};

StructuredBuffer<LightData> light_data : register(t1, space2);

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

Output main(Input input)
{
    float2 tex_coord = input.tex_coord;
    float4 position = input.position;
    float3 normal = input.normal;
    float3 tangent = input.tangent;
    float3 world_position = input.world_position;

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
    float3 v = normalize(view_direction);

    float3 ambient = input.ambient * ambient_light * base_color;
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);

	for (int i = 0; i < num_lights; ++i)
    {
        float3 l = normalize(light_data[i].position - world_position);
        float3 r = reflect(-l, n);

        float diff = max(dot(n, l), 0.0);
        float spec = pow(max(dot(r, v), 0.0), input.shininess);

        diffuse += base_color * diff * light_data[i].diffuse_color;
        specular += input.specular * spec * light_data[i].specular_color;
    }

    float3 lit_color = ambient + diffuse + specular;

    Output result;
    result.color = float4(lit_color, 1.0);
    result.depth = linearize_depth(position.z, near_plane, far_plane);
    return result;
}
