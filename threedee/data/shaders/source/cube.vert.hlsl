cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
    float3 camera_position : packoffset(c8);
};

struct Material {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
    float emissive;
};

struct InstanceData
{
    float4x4 transform_matrix;
    int tex_index[6];
    Material material;
    uint visibility;
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Input
{
    float3 position : POSITION0;
    float2 tex_coord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

struct Output
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
	int tex_index : TEXCOORD1;
    int emissive_index : TEXCOORD2;
    float3 world_position : POSITION0;
    Material material;
    uint visiblity;
};

float3 scale_from_transform(float4x4 transform)
{
    return float3(
        length(float3(transform._11, transform._21, transform._31)),
        length(float3(transform._12, transform._22, transform._32)),
        length(float3(transform._13, transform._23, transform._33))
    );
}

Output main(Input input, uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    float4x4 transform = instance_data[instance_id].transform_matrix;
    float3 scale = scale_from_transform(transform);

    int face_index = vertex_id % 6;

    float2 tiling;
    // Determine tiling axes based on face normal
    if (abs(input.normal.x) > 0.5)
        tiling = scale.zy;
    else if (abs(input.normal.y) > 0.5)
        tiling = scale.xz;
    else
        tiling = scale.xy;

    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

	float4 world_position = mul(transform, float4(input.position, 1.0f));
	// Snap to grid to avoid floating point precision issues
	// world_position.xyz = round(world_position.xyz * 100.0f) / 100.0f;

	// Add slight overlap
	// world_position.xyz += normalize(input.normal) * 0.0001f;

    Output output;
    output.tex_coord = input.tex_coord * tiling;
	output.tex_index = instance_data[instance_id].tex_index[face_index];
    output.emissive_index = -1;
    output.position = mul(projection_view_matrix, world_position);
    output.normal = normalize(mul((float3x3)transform, input.normal));
    output.tangent = normalize(mul((float3x3)transform, input.tangent));
    output.world_position = world_position.xyz;

    output.material = instance_data[instance_id].material;
    output.visiblity = instance_data[instance_id].visibility;

    return output;
}