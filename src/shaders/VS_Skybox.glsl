#version 450 core
layout(location = 0) in vec3 vi_vtx_pos;

out vec3 vo_vtx_texcoords;

layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    vec3 g_view_pos;
};

uniform mat4 g_mtx_vp_fixed;

void main()
{
    vo_vtx_texcoords = vec3(vi_vtx_pos.xy, -vi_vtx_pos.z);
    gl_Position      = (g_mtx_vp_fixed * vec4(vi_vtx_pos, 1.0)).xyww;
}
