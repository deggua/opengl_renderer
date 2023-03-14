#version 450 core

// in
in vec3 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec4 fo_color;

// uniform
uniform sampler2D g_tex_framebuffer;
uniform float     g_gamma;

void main()
{
    fo_color = pow(texture(g_tex_framebuffer, vo_vtx_texcoord).rgb, vec3(1.0 / g_gamma));
}
