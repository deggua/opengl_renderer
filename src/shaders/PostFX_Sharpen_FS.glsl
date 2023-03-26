#version 450 core

// in
in vec2 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec3 fo_color;

// uniform
uniform sampler2D g_screen;
uniform float     g_strength;
uniform vec2      g_resolution;

vec3 Min5(vec3 a, vec3 b, vec3 c, vec3 d, vec3 e)
{
    vec3 ab = min(a, b);
    vec3 cd = min(c, d);
    return min(min(ab, cd), e);
}

vec3 Max5(vec3 a, vec3 b, vec3 c, vec3 d, vec3 e)
{
    vec3 ab = max(a, b);
    vec3 cd = max(c, d);
    return max(max(ab, cd), e);
}

vec3 SampleScreen(vec2 uv, float x_offset, float y_offset, vec2 dxdy)
{
    vec2 offset = vec2(x_offset, y_offset);
    return clamp(texture(g_screen, uv + offset * dxdy).rgb, 0.0, 1.0);
}

// this is based on Acerola's implementation:
// https://github.com/GarrettGunnell/AcerolaFX/blob/main/Shaders/Includes/AcerolaFX_Sharpness.fxh
vec3 ContrastAdaptiveSharpening(float strength, vec2 uv, vec2 dxdy)
{
    float sharpness = -(1.0 / mix(10.0, 7.0, clamp(strength, 0.0, 1.0)));

    /*
     g h i
     d e f
     a b c
    */

    vec3 a = SampleScreen(uv, -1, -1, dxdy);
    vec3 b = SampleScreen(uv, 0, -1, dxdy);
    vec3 c = SampleScreen(uv, 1, -1, dxdy);
    vec3 d = SampleScreen(uv, -1, 0, dxdy);
    vec3 e = SampleScreen(uv, 0, 0, dxdy);
    vec3 f = SampleScreen(uv, 1, 0, dxdy);
    vec3 g = SampleScreen(uv, -1, 1, dxdy);
    vec3 h = SampleScreen(uv, 0, 1, dxdy);
    vec3 i = SampleScreen(uv, 1, 1, dxdy);

    vec3 min1    = Min5(d, e, f, b, h);
    vec3 min2    = Min5(min1, a, c, g, i);
    vec3 min_rgb = min1 + min2;

    vec3 max1    = Max5(d, e, f, b, h);
    vec3 max2    = Max5(max1, a, c, g, i);
    vec3 max_rgb = max1 + max2;

    vec3 inv_max_rgb = 1.0 / max_rgb;
    vec3 amp         = min(min_rgb, 2.0 - max_rgb) * inv_max_rgb;
    amp              = sqrt(clamp(amp, 0, 1));

    vec3 w     = amp * sharpness;
    vec3 inv_w = 1.0 / (1.0 + 4.0 * w);

    vec3 total = w * (b + d + f + h) + e;
    return clamp(total * inv_w, 0, 1);
}

void main()
{
    vec2 dxdy = 1.0 / g_resolution;
    fo_color  = ContrastAdaptiveSharpening(g_strength, vo_vtx_texcoord, dxdy);
}
