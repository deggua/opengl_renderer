#version 330 core

// This shader performs downsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.
// This particular method was customly designed to eliminate
// "pulsating artifacts and temporal stability issues".

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
uniform sampler2D g_tex_input;
uniform vec2      g_resolution;
uniform int       g_mip;

in vec2 vo_vtx_texcoord;

layout(location = 0) out vec3 fo_fragcolor;

float Luma(vec3 color)
{
    return dot(color, vec3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));
}

float KarisAverage(vec3 color)
{
    return 1.0 / (1.0 + Luma(color));
}

void main()
{
    vec2  src_texel_size = 1.0 / g_resolution;
    float x              = src_texel_size.x;
    float y              = src_texel_size.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(g_tex_input, vec2(vo_vtx_texcoord.x - 2 * x, vo_vtx_texcoord.y + 2 * y)).rgb;
    vec3 b = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y + 2 * y)).rgb;
    vec3 c = texture(g_tex_input, vec2(vo_vtx_texcoord.x + 2 * x, vo_vtx_texcoord.y + 2 * y)).rgb;

    vec3 d = texture(g_tex_input, vec2(vo_vtx_texcoord.x - 2 * x, vo_vtx_texcoord.y)).rgb;
    vec3 e = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y)).rgb;
    vec3 f = texture(g_tex_input, vec2(vo_vtx_texcoord.x + 2 * x, vo_vtx_texcoord.y)).rgb;

    vec3 g = texture(g_tex_input, vec2(vo_vtx_texcoord.x - 2 * x, vo_vtx_texcoord.y - 2 * y)).rgb;
    vec3 h = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y - 2 * y)).rgb;
    vec3 i = texture(g_tex_input, vec2(vo_vtx_texcoord.x + 2 * x, vo_vtx_texcoord.y - 2 * y)).rgb;

    vec3 j = texture(g_tex_input, vec2(vo_vtx_texcoord.x - x, vo_vtx_texcoord.y + y)).rgb;
    vec3 k = texture(g_tex_input, vec2(vo_vtx_texcoord.x + x, vo_vtx_texcoord.y + y)).rgb;
    vec3 l = texture(g_tex_input, vec2(vo_vtx_texcoord.x - x, vo_vtx_texcoord.y - y)).rgb;
    vec3 m = texture(g_tex_input, vec2(vo_vtx_texcoord.x + x, vo_vtx_texcoord.y - y)).rgb;

    if (g_mip == 0) {
        vec3 s1 = 0.25 * (a + b + d + e);
        vec3 s2 = 0.25 * (b + c + e + f);
        vec3 s3 = 0.25 * (d + e + g + h);
        vec3 s4 = 0.25 * (e + f + h + i);
        vec3 s5 = 0.25 * (j + k + l + m);

        float s1w = KarisAverage(s1);
        float s2w = KarisAverage(s2);
        float s3w = KarisAverage(s3);
        float s4w = KarisAverage(s4);
        float s5w = KarisAverage(s5);

        // I don't think it matters that we're oversampling certain pixels, it biases the average
        // but it should be energy conserving. Although this could be non-ideal since we heavily
        // oversample e, which might be the pixel that causes the fireflies
        vec3 karis_avg = s1w * s1 + s2w * s2 + s3w * s3 + s4w * s4 + s5w * s5;
        karis_avg /= s1w + s2w + s3w + s4w + s5w;

        fo_fragcolor = karis_avg;
    } else {
        // Apply weighted distribution:
        // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
        // a,b,d,e * 0.125
        // b,c,e,f * 0.125
        // d,e,g,h * 0.125
        // e,f,h,i * 0.125
        // j,k,l,m * 0.5
        // This shows 5 square areas that are being sampled. But some of them overlap,
        // so to have an energy preserving downsample we need to make some adjustments.
        // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
        // contribute 0.5 to the final color output. The code below is written
        // to effectively yield this sum. We get:
        // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
        fo_fragcolor = e * 0.125;
        fo_fragcolor += (a + c + g + i) * 0.03125;
        fo_fragcolor += (b + d + f + h) * 0.0625;
        fo_fragcolor += (j + k + l + m) * 0.125;
    }

    fo_fragcolor = max(fo_fragcolor, 0.0001);
}
