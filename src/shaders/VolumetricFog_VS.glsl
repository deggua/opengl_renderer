#version 450 core

// in
layout(location = 0) in vec2 vi_vtx_pos;
layout(location = 1) in vec2 vi_vtx_texcoord;

uniform vec2 g_half_size_near_plane;

out vec3 vo_view_dir;

void main()
{
    vec2 k   = g_half_size_near_plane;
    vec2 ndc = vi_vtx_pos;

    vec3 view2frag_dir = vec3(k * (ndc + 1.0) - k, -1.0);
    vec3 frag2view_dir = -view2frag_dir;

    vo_view_dir = frag2view_dir;
    gl_Position = vec4(vi_vtx_pos.x, vi_vtx_pos.y, 0.0, 1.0);
}
