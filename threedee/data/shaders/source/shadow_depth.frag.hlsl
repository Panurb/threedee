struct Input
{
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    int tex_index : TEXCOORD1;
    int normal_index : TEXCOORD2;
    float3 world_position : POSITION0;
    float specular;
    float diffuse;
    float ambient;
    float shininess;
};

float main(Input input) : SV_Depth {
    return 0.0f; // Just needed to satisfy the pipeline; depth comes from SV_POSITION
}