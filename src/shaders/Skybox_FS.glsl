#version 450 core
out vec4 fo_color;

in vec3 vo_vtx_texcoords;

uniform samplerCube skybox;

void main()
{
    fo_color = texture(skybox, vo_vtx_texcoords);
}
