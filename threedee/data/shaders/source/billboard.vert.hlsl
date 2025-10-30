cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
};

struct Material {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
    float emissive;
};

struct InstanceData
{
    float3 position;
    float width;
    float height;
    int tex_index;
    int emissive_index;
    Material material;
    uint visibility;
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
    float2 tex_coord : TEXCOORD0;
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
	int tex_index : TEXCOORD1;
    int emissive_index : TEXCOORD2;
    float3 world_position : POSITION0;
    Material material;
    uint visiblity;
};


Output main(Input input, uint instance_id : SV_InstanceID)
{
    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

    float3 position = instance_data[instance_id].position;
    float width = instance_data[instance_id].width;
    float height = instance_data[instance_id].height;

    float3 camera_right = normalize(float3(view_matrix._11, view_matrix._21, view_matrix._31));
    float3 camera_up = normalize(float3(view_matrix._12, view_matrix._22, view_matrix._32));

    float3 world_position = position
        + (input.position.x - 0.5f) * width * camera_right
        + (input.position.y - 0.5f) * height * camera_up;

    Output output;
    output.tex_coord = input.tex_coord;
	output.tex_index = instance_data[instance_id].tex_index;
    output.emissive_index = instance_data[instance_id].emissive_index;
    output.position = mul(projection_view_matrix, float4(world_position, 1.0f));
    output.normal = input.normal;
    output.tangent = input.tangent;
    output.world_position = world_position;

    output.material = instance_data[instance_id].material;
    output.visiblity = instance_data[instance_id].visibility;

    return output;
}