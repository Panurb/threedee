struct Output
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

Output main(float4 color : TEXCOORD0, float4 position : SV_Position)
{
    Output result;
    result.color = color;
    result.depth = position.z;
    return result;
}
