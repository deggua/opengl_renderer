#version 450 core

// in
layout(location = 0) in vec2 vi_vtx_pos;
layout(location = 1) in vec2 vi_vtx_texcoord;

void main()
{
    gl_Position = vec4(vi_vtx_pos.x, vi_vtx_pos.y, 0.0, 1.0);
}
