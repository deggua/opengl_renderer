#version 450 core

struct SunLight {
    vec3 dir; // must be pre-normalized

    vec3 color;
};

struct AmbientLight {
    vec3 color;
};

in vec3 vo_view_dir;

out vec4 fo_color;

uniform             layout(binding = 0, r32f) restrict coherent image2DMS g_shadow_depth;
uniform sampler2DMS g_framebuffer_depth;
uniform sampler2D   g_noise;

uniform AmbientLight g_ambient_light;
uniform SunLight     g_direct_light;

uniform float g_scattering_coef;
uniform float g_density;
uniform float g_asymmetry;

#define CLIP_NEAR 0.1
#define CLIP_FAR  50.0
#define PI        3.14159265359

#if 0
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
#else
float Phase_HG(float VdotL)
{
    const float g         = g_asymmetry;
    const float g2        = g * g;
    const float cos_theta = VdotL;

    float a = (1.0 + g2 - 2.0 * g * cos_theta);

    float numerator   = 1.0 - g2;
    float denominator = 4.0 * PI * a * sqrt(a);

    return numerator / denominator;
}

float Phase_Schlick(float VdotL)
{
    const float k         = g_asymmetry;
    const float k2        = k * k;
    const float cos_theta = VdotL;

    float a = 1.0 - k * cos_theta;

    float numerator   = 1.0 - k2;
    float denominator = 4.0 * PI * a * a;

    return numerator / denominator;
}

vec4 VolumetricLighting(float norm_linear_illum_depth)
{
    const float scattering_coef = g_scattering_coef;
#    if 0
    const float density
        = g_density + (texture(g_noise, gl_FragCoord.xy / vec2(512, 512)).r - 0.5) / (5.0 / g_density);
#    else
    const float density = g_density;
#    endif

    float depth = norm_linear_illum_depth * 100.0;

    vec3 in_scattering
        = g_ambient_light.color
          + g_direct_light.color * Phase_Schlick(dot(normalize(vo_view_dir), g_direct_light.dir));
    float out_scattering = scattering_coef * density;
    vec3  light          = in_scattering * out_scattering;
    vec3  illum          = light * depth;

    return vec4(illum, 1.0);
}
#endif

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

#if 0
    float fog_intensity           = FogIntensity(norm_linear_illum_depth);
    fo_color = vec4(1.0, 1.0, 1.0, fog_intensity);
#else
    fo_color = VolumetricLighting(norm_linear_illum_depth);
#endif
}
