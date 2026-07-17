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

struct Output
{
    float4 position : SV_Position;
    float2 tex_coord : TEXCOORD0;
    int tex_index;
};

Output main(Input input, uint instance_id : SV_InstanceID)
{
    InstanceData instance;
    if (use_instance_buffer == 0) {
        instance = model_data;
    } else {
        instance = instance_data[instance_id];
    }

    if ((instance.visibility & visibility_mask) == 0)
    {
        Output output;
        output.position = float4(0.0f, 0.0f, -1e6f, 0.0f);
        output.tex_coord = float2(0.0f, 0.0f);
        output.tex_index = 0;
        return output;
    }

    float4x4 transform = instance.transform_matrix;
    float2 tex_scale = instance.tex_scale;
    float2 tiling = 1.0f / tex_scale;

    Output output;
    output.position = mul(mul(projection_view_matrix, transform), float4(input.position, 1.0f));
    output.tex_coord = input.tex_coord * tiling;
    output.tex_index = instance.tex_index;

    return output;
}
