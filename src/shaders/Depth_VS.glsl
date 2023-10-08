#version 450 core

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec3 vi_vtx_normal;
layout(location = 2) in vec3 vi_vtx_tangent;
layout(location = 3) in vec3 vi_vtx_bitangent;
layout(location = 4) in vec2 vi_vtx_texcoord;

uniform mat4 g_mtx_wvp; // obj -> screen

void main()
{
    gl_Position = g_mtx_wvp * vec4(vi_vtx_pos, 1.0);
}
