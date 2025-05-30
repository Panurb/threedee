cbuffer UBO : register(b0, space3)
{
    float near_plane;
    float far_plane;
};

struct Output
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

float linearize_depth(float depth, float near, float far)
{
    float z = depth * 2.0 - 1.0;
    return ((2.0 * near * far) / (far + near - z * (far - near))) / far;
}

Output main(float4 color : TEXCOORD0, float4 position : SV_Position)
{
    Output result;
    result.color = color;
    result.depth = linearize_depth(position.z, near_plane, far_plane);
    return result;
}
