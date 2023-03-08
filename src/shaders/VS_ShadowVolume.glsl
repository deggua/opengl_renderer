#version 330 core

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec3 vi_vtx_normal;
layout(location = 2) in vec2 vi_vtx_texcoord;

// out
out vec3 vo_vtx_pos;

// uniform
uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_view;   // world  -> view
uniform mat4 g_mtx_screen; // view   -> screen

void main()
{
    vo_vtx_pos = vec3(g_mtx_world * vec4(vi_vtx_pos, 1.0));
}
