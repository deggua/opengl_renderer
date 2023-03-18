#version 450 core

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec3 vi_vtx_normal;
layout(location = 2) in vec2 vi_vtx_texcoord;

// out
out vec2 vo_vtx_texcoord;

// uniform
uniform mat4 g_mtx_wv; // obj  -> view

uniform vec3 g_scale;

layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    mat4 g_mtx_view;
    mat4 g_mtx_proj;
    vec3 g_pos_view;
};

void main()
{
    // TODO: I don't think this would work very well with lighting, we would have to do the
    // calculations in view space
    vo_vtx_texcoord = vi_vtx_texcoord;

    mat4 mtx_wv   = g_mtx_wv;
    mtx_wv[0].xyz = vec3(g_scale[0], 0.0, 0.0);
    mtx_wv[1].xyz = vec3(0.0, g_scale[1], 0.0);
    mtx_wv[2].xyz = vec3(0.0, 0.0, g_scale[2]);

    gl_Position = g_mtx_proj * mtx_wv * vec4(vi_vtx_pos, 1.0);
}
