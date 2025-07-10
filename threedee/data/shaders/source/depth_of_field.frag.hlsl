Texture2D tex : register(t0, space2);
Texture2DMS<float> depth_tex : register(t1, space2);
SamplerState sampler_tex : register(s0, space2);


cbuffer UBO : register(b0, space3) {
    float near_plane;
    float far_plane;
    float focal_distance;
    float focal_range;
    float2 screen_size;
    bool vertical;
};


struct Input {
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};


float linearize_depth(float depth) {
    return (2.0 * near_plane) / (far_plane + near_plane - depth * (far_plane - near_plane));
}


float compute_blur_amount(float linear_depth) {
    float blur = abs(linear_depth - focal_distance) / focal_range;
    return saturate(blur);
}


float4 main(Input input) : SV_Target {
    float3 color = float3(0.0, 0.0, 0.0);

    float raw_depth = depth_tex.Load(input.position.xy, 0);
    float linear_depth = linearize_depth(raw_depth);

    float blur_amount = compute_blur_amount(linear_depth);
    blur_amount = pow(blur_amount, 0.5);
    int blur_radius = int(blur_amount * 20);
    blur_radius = max(blur_radius, 1);

    // return float4(blur_amount.xxx, 1.0);

    float total_weight = 0.0;
    float sigma = blur_radius / 2.0;

    for (int i = -blur_radius; i <= blur_radius; ++i) {
        int x = vertical ? 0 : i;
        int y = vertical ? i : 0;

        float2 offset = float2(x, y);
        float dist2 = dot(offset, offset);
        if (dist2 > blur_radius * blur_radius) continue; // circle cutoff

        float2 uv_offset = offset / screen_size;
        float weight = exp(-dist2 / (2.0 * sigma * sigma));

        color += tex.Sample(sampler_tex, input.tex_coord + uv_offset).rgb * weight;
        total_weight += weight;
    }

    color /= max(total_weight, 1e-6);

    return float4(color, 1.0);
}
