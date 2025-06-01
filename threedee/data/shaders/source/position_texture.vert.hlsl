cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
};

StructuredBuffer<float4x4> transform_matrices : register(t0, space0);

struct Input
{
    float3 position : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};

struct Output
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input, uint instance_id : SV_InstanceID)
{
    Output output;
    output.tex_coord = input.tex_coord;
    float4x4 transform = transform_matrices[instance_id];
    output.position = mul(mul(projection_matrix, transform), float4(input.position, 1.0f));
    return output;
}