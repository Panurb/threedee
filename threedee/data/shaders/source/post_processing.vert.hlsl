// draw fullscreen quad

static const float2 quad_vertices[4] = {
    float2(-1.0f, -1.0f),
    float2( 1.0f, -1.0f),
    float2(-1.0f,  1.0f),
    float2( 1.0f,  1.0f)
};

static const float2 quad_uvs[4] = {
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f)
};

struct Output {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Output main(uint vertex_id : SV_VertexID) {
    Output output;
    output.position = float4(quad_vertices[vertex_id], 0.0f, 1.0f);
    output.uv = quad_uvs[vertex_id];
    return output;
}
