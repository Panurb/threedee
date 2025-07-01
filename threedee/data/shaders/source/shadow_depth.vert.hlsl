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

float4 main(Input input, uint instance_id : SV_InstanceID) : SV_Position
{
    float4x4 transform = instance_data[instance_id].transform_matrix;

    float4 position = mul(mul(projection_view_matrix, transform), float4(input.position, 1.0f));

    return position;
}