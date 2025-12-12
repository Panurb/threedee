cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
    float3 camera_position : packoffset(c8);
};

struct InstanceData
{
	float4 color;
    float3 position;
    float width;
    float height;
	float angle;
    int tex_index;
    uint visibility;
};

cbuffer ModelUniformData : register(b1, space1)
{
    InstanceData model_data;
    int use_instance_buffer = 1;
};

StructuredBuffer<InstanceData> instance_datas : register(t0, space0);

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
    float3 world_position : TEXCOORD2;
    uint visibility: TEXCOORD3;
	float4 color : COLOR0;
};


Output main(Input input, uint instance_id : SV_InstanceID)
{
    InstanceData instance_data = instance_datas[instance_id];

    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

    float3 position = instance_data.position;
    float width = instance_data.width;
    float height = instance_data.height;

    // This is like a spherical billboard
    float3 to_camera = camera_position - position;
    float3 world_up = float3(0.0f, 1.0f, 0.0f);

    float3 camera_right = normalize(cross(world_up, to_camera));
    float3 camera_up = normalize(cross(to_camera, camera_right));
    float3 camera_forward = normalize(cross(camera_right, camera_up));

    float c = cos(instance_data.angle);
    float s = sin(instance_data.angle);

    float2 local_pos = float2(width * input.position.x, height * input.position.y);
    float2 rotated = float2(c * local_pos.x - s * local_pos.y,
                            s * local_pos.x + c * local_pos.y);

    float3 world_position = position + rotated.x * camera_right + rotated.y *  camera_up;

    Output output;
    output.tex_coord = input.tex_coord;
	output.tex_index = instance_data.tex_index;
    output.position = mul(projection_view_matrix, float4(world_position, 1.0f));

    // For billboards, normal always faces the camera
    output.normal = camera_forward;
    output.tangent = camera_right;

    output.world_position = world_position;

    output.visibility = instance_data.visibility;
	output.color = instance_data.color;

    return output;
}