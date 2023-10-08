#version 450 core

out vec4 fo_color;

uniform             layout(binding = 0, r32f) restrict coherent image2DMS g_shadow_depth;
uniform sampler2DMS g_framebuffer_depth;

#define DENSITY   6.0
#define CLIP_NEAR 0.1
#define CLIP_FAR  50.0

float FogIntensity(float norm_linear_illum_depth)
{
    const float linear_start = 0.0;
    const float linear_end   = 0.5;

    if (norm_linear_illum_depth < linear_start) {
        return 0.0;
    } else if (norm_linear_illum_depth > linear_end) {
        return 1.0;
    } else {
        return 1.0 - (linear_end - norm_linear_illum_depth) / (linear_end - linear_start);
    }
}

float LinearDepth(float z_depth, float z_near, float z_far)
{
    float z_ndc = 2.0 * z_depth - 1.0;
    return 2.0 * z_near * z_far / (z_far + z_near - z_ndc * (z_far - z_near));
}

void main()
{
    // TODO: probably use a uniform for CLIP_NEAR and CLIP_FAR
    ivec2 pix_coord = ivec2(gl_FragCoord.xy);

    float z_buffer_depth      = texelFetch(g_framebuffer_depth, pix_coord, gl_SampleID).r;
    float linear_buffer_depth = LinearDepth(z_buffer_depth, CLIP_NEAR, CLIP_FAR);
    float linear_shadow_depth = imageLoad(g_shadow_depth, pix_coord, gl_SampleID).r;
    float linear_illum_depth
        = clamp(linear_buffer_depth - linear_shadow_depth, CLIP_NEAR, CLIP_FAR);

    float norm_linear_illum_depth = (linear_illum_depth - CLIP_NEAR) / (CLIP_FAR - CLIP_NEAR);
    float fog_intensity           = FogIntensity(norm_linear_illum_depth);

    // TODO: fog color and intensity relative to the dist between the fragment and the light
    fo_color = vec4(1.0, 1.0, 1.0, fog_intensity);
}
