#version 330 core

// This shader performs upsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
uniform sampler2D g_tex_input;
uniform float     g_radius;

in vec2 vo_vtx_texcoord;
layout(location = 0) out vec3 fo_fragcolor;

void main()
{
    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = g_radius;
    float y = g_radius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(g_tex_input, vec2(vo_vtx_texcoord.x - x, vo_vtx_texcoord.y + y)).rgb;
    vec3 b = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y + y)).rgb;
    vec3 c = texture(g_tex_input, vec2(vo_vtx_texcoord.x + x, vo_vtx_texcoord.y + y)).rgb;

    vec3 d = texture(g_tex_input, vec2(vo_vtx_texcoord.x - x, vo_vtx_texcoord.y)).rgb;
    vec3 e = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y)).rgb;
    vec3 f = texture(g_tex_input, vec2(vo_vtx_texcoord.x + x, vo_vtx_texcoord.y)).rgb;

    vec3 g = texture(g_tex_input, vec2(vo_vtx_texcoord.x - x, vo_vtx_texcoord.y - y)).rgb;
    vec3 h = texture(g_tex_input, vec2(vo_vtx_texcoord.x, vo_vtx_texcoord.y - y)).rgb;
    vec3 i = texture(g_tex_input, vec2(vo_vtx_texcoord.x + x, vo_vtx_texcoord.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    fo_fragcolor = e * 4.0;
    fo_fragcolor += (b + d + f + h) * 2.0;
    fo_fragcolor += (a + c + g + i);
    fo_fragcolor *= 1.0 / 16.0;
}
