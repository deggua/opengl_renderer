#version 450 core

// in
in vec2 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec4 fo_color;

// uniform
uniform sampler2D g_screen;
uniform float     g_gamma;

void main()
{
    fo_color = vec4(pow(texture(g_screen, vo_vtx_texcoord).rgb, vec3(1.0 / g_gamma)), 1.0);
    // fo_color = texture(g_screen, vo_vtx_texcoord);
}
