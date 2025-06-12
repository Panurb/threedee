struct Output
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

Output main(float4 position) {
    Output output;
    output.depth = position.z;
    output.color = float4(1.0, 0.0, 0.0, 1.0); // Placeholder color
    return output;
}
