#version 330

layout(triangles_adjacency) in; // six vertices in
layout(triangle_strip, max_vertices = 18) out;

in vec3 vo_vtx_pos[]; // an array of 6 vertices (triangle with adjacency)

// TODO: this only works for point lights
uniform vec3 g_light_pos;

uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_view;   // world  -> view
uniform mat4 g_mtx_screen; // view   -> screen

float EPSILON = 0.001;

// Emit a quad using a triangle strip
void EmitQuad(vec3 StartVertex, vec3 EndVertex, mat4 mtx_wvp)
{
    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    vec3 LightDir = normalize(StartVertex - g_light_pos);
    gl_Position   = mtx_wvp * vec4((StartVertex + LightDir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #2: the starting vertex projected to infinity
    gl_Position = mtx_wvp * vec4(LightDir, 0.0);
    EmitVertex();

    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    LightDir    = normalize(EndVertex - g_light_pos);
    gl_Position = mtx_wvp * vec4((EndVertex + LightDir * EPSILON), 1.0);
    EmitVertex();

    // Vertex #4: the ending vertex projected to infinity
    gl_Position = mtx_wvp * vec4(LightDir, 0.0);
    EmitVertex();

    EndPrimitive();
}

void main()
{
    mat4 mtx_wvp = g_mtx_screen * g_mtx_view;

    vec3 e1 = vo_vtx_pos[2] - vo_vtx_pos[0];
    vec3 e2 = vo_vtx_pos[4] - vo_vtx_pos[0];
    vec3 e3 = vo_vtx_pos[1] - vo_vtx_pos[0];
    vec3 e4 = vo_vtx_pos[3] - vo_vtx_pos[2];
    vec3 e5 = vo_vtx_pos[4] - vo_vtx_pos[2];
    vec3 e6 = vo_vtx_pos[5] - vo_vtx_pos[0];

    vec3 Normal   = cross(e1, e2);
    vec3 LightDir = g_light_pos - vo_vtx_pos[0];

    // Handle only light facing triangles
    if (dot(Normal, LightDir) > 0) {
        Normal = cross(e3, e1);

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = vo_vtx_pos[0];
            vec3 EndVertex   = vo_vtx_pos[2];
            EmitQuad(StartVertex, EndVertex, mtx_wvp);
        }

        Normal   = cross(e4, e5);
        LightDir = g_light_pos - vo_vtx_pos[2];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = vo_vtx_pos[2];
            vec3 EndVertex   = vo_vtx_pos[4];
            EmitQuad(StartVertex, EndVertex, mtx_wvp);
        }

        Normal   = cross(e2, e6);
        LightDir = g_light_pos - vo_vtx_pos[4];

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = vo_vtx_pos[4];
            vec3 EndVertex   = vo_vtx_pos[0];
            EmitQuad(StartVertex, EndVertex, mtx_wvp);
        }

        // render the front cap
        LightDir    = (normalize(vo_vtx_pos[0] - g_light_pos));
        gl_Position = mtx_wvp * vec4((vo_vtx_pos[0] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir    = (normalize(vo_vtx_pos[2] - g_light_pos));
        gl_Position = mtx_wvp * vec4((vo_vtx_pos[2] + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir    = (normalize(vo_vtx_pos[4] - g_light_pos));
        gl_Position = mtx_wvp * vec4((vo_vtx_pos[4] + LightDir * EPSILON), 1.0);
        EmitVertex();
        EndPrimitive();

        // render the back cap
        LightDir    = vo_vtx_pos[0] - g_light_pos;
        gl_Position = mtx_wvp * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir    = vo_vtx_pos[4] - g_light_pos;
        gl_Position = mtx_wvp * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir    = vo_vtx_pos[2] - g_light_pos;
        gl_Position = mtx_wvp * vec4(LightDir, 0.0);
        EmitVertex();
    }
}
