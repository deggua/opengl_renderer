/*
#version 450 core

#define AMBIENT_LIGHT 0
#define POINT_LIGHT   1
#define SPOT_LIGHT    2
#define SUN_LIGHT     3
*/

struct PointLight {
    vec3 pos;

    vec3 color;
};

struct SpotLight {
    vec3  pos;
    vec3  dir;          // must be pre-normalized
    float inner_cutoff; // dot(dir, inner_dir)
    float outer_cutoff; // dot(dit, outer_dir)

    vec3 color;
};

struct SunLight {
    vec3 dir; // must be pre-normalized

    vec3 color;
};

struct AmbientLight {
    vec3 color;
};

// in
layout(location = 0) in vec3 vi_vtx_pos;
layout(location = 1) in vec3 vi_vtx_normal;
layout(location = 2) in vec3 vi_vtx_tangent;
layout(location = 3) in vec3 vi_vtx_bitangent;
layout(location = 4) in vec2 vi_vtx_texcoord;

// out
#if LIGHT_TYPE != AMBIENT_LIGHT
out vec3 vo_light_dir;
out vec3 vo_view_dir;
#endif
out vec3 vo_vtx_pos;
out vec3 vo_vtx_normal;
out vec2 vo_vtx_texcoord;

// uniform
layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    mat4 g_mtx_view;
    mat4 g_mtx_proj;
    vec3 g_pos_view;
};

uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_wvp;    // obj    -> screen

#if LIGHT_TYPE == SUN_LIGHT
uniform SunLight g_light_source;

#elif LIGHT_TYPE == AMBIENT_LIGHT
uniform AmbientLight g_light_source;

#elif LIGHT_TYPE == POINT_LIGHT
uniform PointLight g_light_source;

#elif LIGHT_TYPE == SPOT_LIGHT
uniform SpotLight g_light_source;

#endif

#if LIGHT_TYPE != AMBIENT_LIGHT
void main()
{
    vec3 tangent   = normalize(g_mtx_normal * vi_vtx_tangent);
    vec3 bitangent = normalize(g_mtx_normal * vi_vtx_bitangent);
    vec3 normal    = normalize(g_mtx_normal * vi_vtx_normal);
    mat3 mtx_tbn   = transpose(mat3(tangent, bitangent, normal));

    vec3 vtx_pos    = vec3(g_mtx_world * vec4(vi_vtx_pos, 1.0));
    vo_vtx_pos      = vtx_pos;
    vo_vtx_normal   = normalize(mtx_tbn * normal);
    vo_vtx_texcoord = vi_vtx_texcoord;
    vo_view_dir     = normalize(mtx_tbn * (g_pos_view - vtx_pos));

#    if LIGHT_TYPE == SUN_LIGHT
    vo_light_dir = normalize(mtx_tbn * -g_light_source.dir);
#    else
    vo_light_dir = normalize(mtx_tbn * (g_light_source.pos - vtx_pos));
#    endif

    gl_Position = g_mtx_wvp * vec4(vi_vtx_pos, 1.0);
}
#else

void main()
{
    vo_vtx_pos      = vec3(g_mtx_world * vec4(vi_vtx_pos, 1.0));
    vo_vtx_normal   = g_mtx_normal * vi_vtx_normal;
    vo_vtx_texcoord = vi_vtx_texcoord;

    gl_Position = g_mtx_wvp * vec4(vi_vtx_pos, 1.0);
}
#endif
