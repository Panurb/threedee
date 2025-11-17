cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
    float3 camera_position : packoffset(c8);
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
    int material_index;
    uint visibility;
    int billboard_type; // 0 = spherical, 1 = cylindrical, 2 = screen-aligned
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
    int material_index : TEXCOORD3;
    uint visiblity;
};


Output main(Input input, uint instance_id : SV_InstanceID)
{
    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

    float3 position = instance_data[instance_id].position;
    float width = instance_data[instance_id].width;
    float height = instance_data[instance_id].height;

    float3 camera_right = normalize(float3(view_matrix._11, view_matrix._12, view_matrix._13));
    float3 camera_up = normalize(float3(view_matrix._21, view_matrix._22, view_matrix._23));

    if (instance_data[instance_id].billboard_type != 2) {
        float3 to_camera = camera_position - position;
        float3 world_up = float3(0.0f, 1.0f, 0.0f);

        if (instance_data[instance_id].billboard_type == 1) {
            to_camera = float3(to_camera.x, 0.0f, to_camera.z);
        }

        camera_right = normalize(cross(world_up, to_camera));
        camera_up = normalize(cross(to_camera, camera_right));
    }

    float3 camera_forward = normalize(cross(camera_right, camera_up));

    float3 world_position = position
        + input.position.x * width * camera_right
        + input.position.y * height * camera_up;

    Output output;
    output.tex_coord = input.tex_coord;
	output.tex_index = instance_data[instance_id].tex_index;
    output.emissive_index = -1;
    output.position = mul(projection_view_matrix, float4(world_position, 1.0f));

    // For billboards, normal always faces the camera
    output.normal = camera_forward;
    output.tangent = camera_right;

    output.world_position = world_position;

    output.material_index = instance_data[instance_id].material_index;
    output.visiblity = instance_data[instance_id].visibility;

    return output;
}