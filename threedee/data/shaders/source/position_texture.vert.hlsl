cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
};

struct InstanceData
{
    float4x4 transform_matrix;
	float4 tex_rect;
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Input
{
    float3 position : POSITION0;
    float2 tex_coord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct Output
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
	float4 tex_rect : TEXCOORD1;
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
    float4x4 transform = instance_data[instance_id].transform_matrix;
    float3 scale = scale_from_transform(transform);

    // Determine tiling axes based on face normal
    float2 tiling;
    if (abs(input.normal.x) > 0.5)
        tiling = scale.zy; // YZ face
    else if (abs(input.normal.y) > 0.5)
        tiling = scale.xz; // XZ face
    else
        tiling = scale.xy; // XY face

    Output output;
    output.tex_coord = input.tex_coord * tiling;
	output.tex_rect = instance_data[instance_id].tex_rect;
    output.position = mul(mul(projection_matrix, transform), float4(input.position, 1.0f));
    output.normal = normalize(mul((float3x3)transform, input.normal));
    return output;
}