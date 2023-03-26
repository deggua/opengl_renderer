#version 450 core

// in
in vec2 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec3 fo_color;

// uniform
uniform sampler2D g_screen;
uniform float     g_gamma;

vec3 GammaCorrect(vec3 sdr, float gamma)
{
    return pow(sdr, vec3(1.0 / gamma));
}

void main()
{
    vec3 sdr = texture(g_screen, vo_vtx_texcoord).rgb;
    fo_color = GammaCorrect(sdr, g_gamma);
}
