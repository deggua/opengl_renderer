#version 450 core

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec2 vi_vtx_texcoord;

// out
out vec3 vo_vtx_pos;
out vec2 vo_vtx_texcoord;

void main()
{
    vo_vtx_pos      = vi_vtx_pos;
    vo_vtx_texcoord = vi_vtx_texcoord;

    gl_Position = vec4(vi_vtx_pos, 0.0);
}
