/*
#version 450 core

#define AMBIENT_LIGHT 0
#define POINT_LIGHT   1
#define SPOT_LIGHT    2
#define SUN_LIGHT     3
*/

// TODO: I think we should inset the shadow geometry by the vertex normals, not the direction from
// the light source needs testing

#define EPSILON 0.001

struct PointLight {
    vec3 pos;
};

struct SpotLight {
    vec3  pos;
    vec3  dir;          // must be pre-normalized
    float inner_cutoff; // dot(dir, inner_dir)
    float outer_cutoff; // dot(dit, outer_dir)
};

struct SunLight {
    vec3 dir; // must be pre-normalized
};

struct Edges {
    vec3 e1, e2, e3, e4, e5, e6;
};

struct Vertices {
    vec3 v0, v1, v2, v3, v4, v5;
};

layout(triangles_adjacency) in; // six vertices in

#if LIGHT_TYPE == SUN_LIGHT
layout(triangle_strip, max_vertices = 12) out; // 12 out for sun, 3*3 + 3
#else
layout(triangle_strip, max_vertices = 18) out; // 18 out for the rest, 4*3 + 3 + 3
#endif

in vec3 vo_vtx_pos[]; // an array of 6 vertices (triangle with adjacency)
in vec3 vo_vtx_normal[];
in vec2 vo_vtx_texcoord[];

uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_wvp;    // obj    -> screen

layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    vec3 g_view_pos;
};

#if LIGHT_TYPE == SUN_LIGHT
uniform SunLight g_light_source;

#elif LIGHT_TYPE == AMBIENT_LIGHT
uniform AmbientLight g_light_source;

#elif LIGHT_TYPE == POINT_LIGHT
uniform PointLight g_light_source;

#elif LIGHT_TYPE == SPOT_LIGHT
uniform SpotLight g_light_source;

#endif

void EmitQuad(PointLight light, vec3 start_vertex, vec3 end_vertex, mat4 mtx_vp)
{
    vec3 light_dir = normalize(start_vertex - light.pos);

    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((start_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #2: the starting vertex projected to infinity
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir = normalize(end_vertex - light.pos);

    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((end_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #4: the ending vertex projected to infinity
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    EndPrimitive();
}

void EmitVolume(PointLight light, Vertices verts, Edges edges, mat4 mtx_vp)
{
    vec3 normal    = cross(edges.e1, edges.e2);
    vec3 light_dir = light.pos - verts.v0;

    // Handle only light facing triangles
    if (dot(normal, light_dir) <= 0) {
        return;
    }

    normal = cross(edges.e3, edges.e1);

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v0;
        vec3 vtx_end   = verts.v2;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    normal    = cross(edges.e4, edges.e5);
    light_dir = light.pos - verts.v2;

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v2;
        vec3 vtx_end   = verts.v4;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    normal    = cross(edges.e2, edges.e6);
    light_dir = light.pos - verts.v4;

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v4;
        vec3 vtx_end   = verts.v0;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    // render the front cap
    light_dir   = normalize(verts.v0 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v0 + light_dir * EPSILON), 1.0);
    EmitVertex();

    light_dir   = normalize(verts.v2 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v2 + light_dir * EPSILON), 1.0);
    EmitVertex();

    light_dir   = normalize(verts.v4 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v4 + light_dir * EPSILON), 1.0);
    EmitVertex();
    EndPrimitive();

    // render the back cap
    light_dir   = verts.v0 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir   = verts.v4 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir   = verts.v2 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();
}

void EmitTri(SunLight light, vec3 start_vertex, vec3 end_vertex, mat4 mtx_vp)
{
    vec3 light_dir = light.dir;

    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((start_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #2: the starting vertex projected to infinity
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((end_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    EndPrimitive();
}

void EmitVolume(SunLight light, Vertices verts, Edges edges, mat4 mtx_vp)
{
    vec3 normal    = cross(edges.e1, edges.e2);
    vec3 light_dir = -light.dir;

    // Handle only light facing triangles
    if (dot(normal, light_dir) <= 0) {
        return;
    }

    normal = cross(edges.e3, edges.e1);

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v0;
        vec3 vtx_end   = verts.v2;
        EmitTri(light, vtx_start, vtx_end, mtx_vp);
    }

    normal = cross(edges.e4, edges.e5);

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v2;
        vec3 vtx_end   = verts.v4;
        EmitTri(light, vtx_start, vtx_end, mtx_vp);
    }

    normal = cross(edges.e2, edges.e6);

    if (dot(normal, light_dir) <= 0) {
        vec3 vtx_start = verts.v4;
        vec3 vtx_end   = verts.v0;
        EmitTri(light, vtx_start, vtx_end, mtx_vp);
    }

    // render the front cap
    light_dir   = light.dir;
    gl_Position = mtx_vp * vec4((verts.v0 + light_dir * EPSILON), 1.0);
    EmitVertex();

    gl_Position = mtx_vp * vec4((verts.v2 + light_dir * EPSILON), 1.0);
    EmitVertex();

    gl_Position = mtx_vp * vec4((verts.v4 + light_dir * EPSILON), 1.0);
    EmitVertex();
    EndPrimitive();
}

void EmitQuad(SpotLight light, vec3 start_vertex, vec3 end_vertex, mat4 mtx_vp)
{
    vec3 light_dir = normalize(start_vertex - light.pos);

    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((start_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #2: the starting vertex projected to infinity
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir = normalize(end_vertex - light.pos);

    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    gl_Position = mtx_vp * vec4((end_vertex + light_dir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #4: the ending vertex projected to infinity
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    EndPrimitive();
}

void EmitVolume(SpotLight light, Vertices verts, Edges edges, mat4 mtx_vp)
{
    vec3 normal    = cross(edges.e1, edges.e2);
    vec3 light_dir = light.pos - verts.v0;

    // Handle only light facing triangles
    if (dot(normal, light_dir) <= 0) {
        return;
    }

    vec3 light_dir_v0 = normalize(light.pos - verts.v0);
    vec3 light_dir_v1 = normalize(light.pos - verts.v1);
    vec3 light_dir_v2 = normalize(light.pos - verts.v2);
    vec3 light_dir_v3 = normalize(light.pos - verts.v3);
    vec3 light_dir_v4 = normalize(light.pos - verts.v4);
    vec3 light_dir_v5 = normalize(light.pos - verts.v5);

    // expand the outer cone a bit since the exact cone still has artifacts
    // TODO: play with factor, larger values are better, 1/1.4 didn't work
    // TODO: still had weird artifacts even for relatively small values, 0 seems to work but
    // generates a lot of extra geometry
    // TODO: still needs proper testing to see if its faster since we have to do a lot more compute
    // per vertex float expanded_outer_cutoff = 0.3 * light.outer_cutoff;
    float expanded_outer_cutoff = 0;

    bool is_outside_v0 = dot(-light_dir_v0, light.dir) < expanded_outer_cutoff;
    bool is_outside_v1 = dot(-light_dir_v1, light.dir) < expanded_outer_cutoff;
    bool is_outside_v2 = dot(-light_dir_v2, light.dir) < expanded_outer_cutoff;
    bool is_outside_v3 = dot(-light_dir_v3, light.dir) < expanded_outer_cutoff;
    bool is_outside_v4 = dot(-light_dir_v4, light.dir) < expanded_outer_cutoff;
    bool is_outside_v5 = dot(-light_dir_v5, light.dir) < expanded_outer_cutoff;

    // Handle only faces where the face is at least partially within the outer cone
    if (is_outside_v0 && is_outside_v1 && is_outside_v2 && is_outside_v3 && is_outside_v4
        && is_outside_v5) {
        return;
    }

    normal = cross(edges.e3, edges.e1);

    if (dot(normal, light_dir) <= 0 && (!is_outside_v0 || !is_outside_v2)) {
        vec3 vtx_start = verts.v0;
        vec3 vtx_end   = verts.v2;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    normal    = cross(edges.e4, edges.e5);
    light_dir = light.pos - verts.v2;

    if (dot(normal, light_dir) <= 0 && (!is_outside_v2 || !is_outside_v4)) {
        vec3 vtx_start = verts.v2;
        vec3 vtx_end   = verts.v4;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    normal    = cross(edges.e2, edges.e6);
    light_dir = light.pos - verts.v4;

    if (dot(normal, light_dir) <= 0 && (!is_outside_v4 || !is_outside_v0)) {
        vec3 vtx_start = verts.v4;
        vec3 vtx_end   = verts.v0;
        EmitQuad(light, vtx_start, vtx_end, mtx_vp);
    }

    // render the front cap
    light_dir   = normalize(verts.v0 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v0 + light_dir * EPSILON), 1.0);
    EmitVertex();

    light_dir   = normalize(verts.v2 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v2 + light_dir * EPSILON), 1.0);
    EmitVertex();

    light_dir   = normalize(verts.v4 - light.pos);
    gl_Position = mtx_vp * vec4((verts.v4 + light_dir * EPSILON), 1.0);
    EmitVertex();
    EndPrimitive();

    // render the back cap
    light_dir   = verts.v0 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir   = verts.v4 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();

    light_dir   = verts.v2 - light.pos;
    gl_Position = mtx_vp * vec4(light_dir, 0.0);
    EmitVertex();
}

void main()
{
    Edges edges;
    edges.e1 = vo_vtx_pos[2] - vo_vtx_pos[0];
    edges.e2 = vo_vtx_pos[4] - vo_vtx_pos[0];
    edges.e3 = vo_vtx_pos[1] - vo_vtx_pos[0];
    edges.e4 = vo_vtx_pos[3] - vo_vtx_pos[2];
    edges.e5 = vo_vtx_pos[4] - vo_vtx_pos[2];
    edges.e6 = vo_vtx_pos[5] - vo_vtx_pos[0];

    Vertices verts;
    verts.v0 = vo_vtx_pos[0];
    verts.v1 = vo_vtx_pos[1];
    verts.v2 = vo_vtx_pos[2];
    verts.v3 = vo_vtx_pos[3];
    verts.v4 = vo_vtx_pos[4];
    verts.v5 = vo_vtx_pos[5];

    EmitVolume(g_light_source, verts, edges, g_mtx_vp);
}
