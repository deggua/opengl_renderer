#version 330 core

in vec2  vo_vtx_texcoord;
out vec4 fo_color;

uniform sampler2D g_tex_hdr;
uniform sampler2D g_tex_bloom;

uniform float g_bloom_strength = 0.04;

void main()
{
    vec3 hdr   = texture(g_tex_hdr, vo_vtx_texcoord).rgb;
    vec3 bloom = texture(g_tex_bloom, vo_vtx_texcoord).rgb;
    fo_color   = vec4(mix(hdr, bloom, vec3(g_bloom_strength)), 1.0);
}
