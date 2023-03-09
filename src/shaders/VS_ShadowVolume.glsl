#version 450 core

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec3 vi_vtx_normal;
layout(location = 2) in vec2 vi_vtx_texcoord;

// out
out vec3 vo_vtx_pos;
out vec3 vo_vtx_normal;
out vec2 vo_vtx_texcoord;

// uniform
uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_wvp;    // obj    -> screen

layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    vec3 g_view_pos;
};

void main()
{
    vo_vtx_pos      = vec3(g_mtx_world * vec4(vi_vtx_pos, 1.0));
    vo_vtx_normal   = g_mtx_normal * vi_vtx_normal;
    vo_vtx_texcoord = vi_vtx_texcoord;
}
