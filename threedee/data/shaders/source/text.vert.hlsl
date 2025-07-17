cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
};

struct Input
{
    float2 position : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};

struct Output
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.tex_coord = input.tex_coord;
    output.position = mul(projection_matrix, float4(input.position, 0.0f, 1.0f));
    return output;
}
