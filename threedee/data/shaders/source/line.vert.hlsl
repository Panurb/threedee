cbuffer TransformBlock : register(b0, space1)
{
    float4x4 projection_matrix : packoffset(c0);
    float4x4 view_matrix : packoffset(c4);
    float3 camera_position : packoffset(c8);
};

struct InstanceData
{
    float3 start_position;
    float3 end_position;
    float thickness;
    float4 color;
};

StructuredBuffer<InstanceData> instance_data : register(t0, space0);

struct Output
{
    float4 color : TEXCOORD0;
    float4 position : SV_Position;
};

Output main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    float4x4 projection_view_matrix = mul(projection_matrix, view_matrix);

    float3 start_position = instance_data[instance_id].start_position;
    float3 end_position = instance_data[instance_id].end_position;

    float4 clip_start = mul(projection_view_matrix, float4(start_position, 1.0f));
    float4 clip_end = mul(projection_view_matrix, float4(end_position, 1.0f));

    float2 dir = normalize(clip_end.xy / clip_end.w - clip_start.xy / clip_start.w);
    float2 perp = float2(-dir.y, dir.x);

    // TODO: thickness in pixels rather than clip space units
    float thickness = instance_data[instance_id].thickness * 0.1f;

    float2 offset = perp * ((vertex_id == 0 || vertex_id == 2) ? -1.0f : 1.0f) * (thickness / 2.0f);

    float4 clip_pos = (vertex_id < 2) ? clip_start : clip_end;
    clip_pos.xy += offset * clip_pos.w;

    Output output;
    output.color = instance_data[instance_id].color;
    output.position = clip_pos;

    return output;
}
