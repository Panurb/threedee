StructuredBuffer<float4x4> TransformMatrices : register(t0, space0);

struct Input
{
    float3 Position : TEXCOORD0;
    float4 Color : TEXCOORD1;
};

struct Output
{
    float4 Color : TEXCOORD0;
    float4 Position : SV_Position;
};

Output main(Input input, uint instanceID : SV_InstanceID)
{
    Output output;
    output.Color = input.Color;
    float4x4 transform = TransformMatrices[instanceID];
    output.Position = mul(transform, float4(input.Position, 1.0f));
    return output;
}