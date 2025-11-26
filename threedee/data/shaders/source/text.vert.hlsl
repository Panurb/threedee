cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
};

cbuffer InstanceData : register(b1, space1)
{
    float4 transform_row0;
    float4 transform_row1;
    float4 color;
};

struct Input
{
    float2 position : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
    uint instance_id : SV_InstanceID;
};

struct Output
{
    float2 tex_coord : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.tex_coord = input.tex_coord;
    output.color = color;
    float2x3 transform = float2x3(
        transform_row0.xyz,
        transform_row1.xyz
    );
    float2 transformed_position = mul(transform, float3(input.position, 1.0f));
    output.position = mul(projection_matrix, float4(transformed_position, 0.0f, 1.0f));
    return output;
}
