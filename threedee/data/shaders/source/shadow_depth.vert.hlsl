cbuffer UBO : register(b0, space1)
{
    float4x4 projection_view_matrix : packoffset(c0);
    uint visibility_mask : packoffset(c4);
};

struct InstanceData
{
    float4x4 transform_matrix;
    int tex_index;
    int emissive_index;
    float2 tex_scale;
    int material_index;
    uint visibility;
    float emissive;
};

cbuffer ModelUniformData : register(b1, space1)
{
    InstanceData model_data;
    int use_instance_buffer = 1;
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Input
{
    float3 position : POSITION0;
    float2 tex_coord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

float4 main(Input input, uint instance_id : SV_InstanceID) : SV_Position
{
    InstanceData instance;
    if (use_instance_buffer == 0) {
        instance = model_data;
    } else {
        instance = instance_data[instance_id];
    }

    if ((instance.visibility & visibility_mask) == 0)
    {
        return float4(0.0f, 0.0f, -1e6f, 0.0f);
    }

    float4x4 transform = instance.transform_matrix;

    float4 position = mul(mul(projection_view_matrix, transform), float4(input.position, 1.0f));

    return position;
}