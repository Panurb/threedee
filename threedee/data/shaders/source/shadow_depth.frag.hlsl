struct Output
{
    float4 color : SV_Target0;
};

Output main()
{
    Output output;
    output.color = float4(1.0, 0.0, 0.0, 1.0); // Color doesn't matter for depth-only pass
    return output;
}