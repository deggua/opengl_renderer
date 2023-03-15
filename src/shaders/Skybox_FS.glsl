#version 450 core
out vec4 fo_color;

in vec3 vo_vtx_texcoords;

uniform samplerCube g_skybox;

void main()
{
    fo_color = texture(g_skybox, vo_vtx_texcoords);
}
