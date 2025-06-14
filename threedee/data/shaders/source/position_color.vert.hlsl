cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_view_matrix : packoffset(c0);
};

struct InstanceData
{
    float4x4 transform_matrix : packoffset(c0);
    float4 color : packoffset(c4);
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Input
{
    float3 position : TEXCOORD0;
};

struct Output
{
    float4 color : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input, uint instance_id : SV_InstanceID)
{
    Output output;
    output.color = instance_data[instance_id].color;
    float4x4 transform = instance_data[instance_id].transform_matrix;
    output.position = mul(mul(projection_view_matrix, transform), float4(input.position, 1.0f));
    return output;
}