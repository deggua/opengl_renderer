#version 450 core
out vec4 fo_color;

in vec2 vo_vtx_texcoord;

uniform sampler2D g_sprite;
uniform float     g_intensity;
uniform vec3      g_tint;

void main()
{
    fo_color = g_intensity * vec4(g_tint, 1.0) * texture(g_sprite, vo_vtx_texcoord);
}
