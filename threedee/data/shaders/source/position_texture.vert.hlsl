cbuffer CameraUniformData : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
    float3 camera_position : packoffset(c8);
};

struct InstanceData
{
    float4x4 transform_matrix;
    int tex_index;
    int emissive_index;
    float2 tex_scale;
    int material_index;
    uint visibility;
    float emissive;
};

cbuffer ModelUniformData : register(b1, space1)
{
    InstanceData model_data;
    int use_instance_buffer = 1;
};

StructuredBuffer<InstanceData> instance_datas : register(t0, space0);

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
    int material_index : TEXCOORD3;
    uint visiblity : TEXCOORD4;
    float emissive : TEXCOORD5;
};

float3 scale_from_transform(float4x4 transform)
{
    return float3(
        length(float3(transform._11, transform._21, transform._31)),
        length(float3(transform._12, transform._22, transform._32)),
        length(float3(transform._13, transform._23, transform._33))
    );
}

Output main(Input input, uint instance_id : SV_InstanceID)
{
    InstanceData instance_data;
    if (use_instance_buffer == 0) {
        instance_data = model_data;
    } else {
        instance_data = instance_datas[instance_id];
    }

    float4x4 transform = instance_data.transform_matrix;
    float3 scale = scale_from_transform(transform);

    float2 tex_scale = instance_data.tex_scale;

    float2 tiling;
    if (tex_scale.x == 0.0f || tex_scale.y == 0.0f) {
        // Determine tiling axes based on face normal
        if (abs(input.normal.x) > 0.5)
            tiling = scale.zy;
        else if (abs(input.normal.y) > 0.5)
            tiling = scale.xz;
        else
            tiling = scale.xy;
    } else {
        tiling = 1.0f / tex_scale;
    }

    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

	float4 world_position = mul(transform, float4(input.position, 1.0f));
	// Snap to grid to avoid floating point precision issues
	// world_position.xyz = round(world_position.xyz * 100.0f) / 100.0f;

	// Add slight overlap
	// world_position.xyz += normalize(input.normal) * 0.0001f;

    Output output;
    output.tex_coord = input.tex_coord * tiling;
	output.tex_index = instance_data.tex_index;
    output.emissive_index = instance_data.emissive_index;
    output.position = mul(projection_view_matrix, world_position);
    output.normal = normalize(mul((float3x3)transform, input.normal));
    output.tangent = normalize(mul((float3x3)transform, input.tangent));
    output.world_position = world_position.xyz;

    output.material_index = instance_data.material_index;
    output.visiblity = instance_data.visibility;
    output.emissive = instance_data.emissive;

    return output;
}