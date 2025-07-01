cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_view_matrix : packoffset(c0);
};

struct InstanceData
{
    float4x4 transform_matrix : packoffset(c0);
    float specular : packoffset(c4);
    float diffuse : packoffset(c4.y);
    float ambient : packoffset(c4.z);
    float shininess : packoffset(c4.w);
    int tex_index : packoffset(c5);
    uint visibility : packoffset(c5.y);
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
    int normal_index : TEXCOORD2;
    float3 world_position : POSITION0;
    float specular;
    float diffuse;
    float ambient;
    float shininess;
    int visiblity;
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
	output.tex_index = instance_data[instance_id].tex_index;
    output.normal_index = instance_data[instance_id].tex_index + 1; // Assuming normal map is next in texture array
    output.position = mul(mul(projection_view_matrix, transform), float4(input.position, 1.0f));
    output.normal = normalize(mul((float3x3)transform, input.normal));
    output.tangent = normalize(mul((float3x3)transform, input.tangent));
    output.world_position = mul(transform, float4(input.position, 1.0f)).xyz;

    output.specular = instance_data[instance_id].specular;
    output.diffuse = instance_data[instance_id].diffuse;
    output.ambient = instance_data[instance_id].ambient;
    output.shininess = instance_data[instance_id].shininess;
    output.visiblity = instance_data[instance_id].visibility;

    return output;
}