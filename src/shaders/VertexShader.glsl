#version 330 core

// in
layout(location = 0) in vec3 g_vtx_pos;
layout(location = 1) in vec3 g_vtx_norm;
layout(location = 2) in vec2 g_vtx_tex;

// out
out vec2 g_frag_tex_coord;
out vec3 g_frag_normal;
out vec3 g_frag_pos;

// uniform
uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_view;   // world  -> view
uniform mat4 g_mtx_screen; // view   -> screen

void main()
{
    g_frag_pos       = vec3(g_mtx_world * vec4(g_vtx_pos, 1.0));
    g_frag_normal    = g_mtx_normal * g_vtx_norm;
    g_frag_tex_coord = g_vtx_tex;

    gl_Position = g_mtx_screen * g_mtx_view * g_mtx_world * vec4(g_vtx_pos, 1.0);
}
