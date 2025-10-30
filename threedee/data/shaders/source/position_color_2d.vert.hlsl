cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
};

struct InstanceData
{
    float4 transform_row0;
    float4 transform_row1;
    float4 color;
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Input
{
    float2 position : TEXCOORD0;
    uint instance_id : SV_InstanceID;
};

struct Output
{
    float4 color : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input)
{
    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);
    InstanceData instance = instance_data[input.instance_id];

    Output output;
    output.color = instance.color;
    float2x3 transform = float2x3(
        instance.transform_row0.xyz,
        instance.transform_row1.xyz
    );
    float2 transformed_position = mul(transform, float3(input.position, 1.0f));
    output.position = mul(projection_view_matrix, float4(transformed_position.xy, 0.0f, 1.0f));
    return output;
}
