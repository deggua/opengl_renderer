#version 330 core

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec3 vtx_rgb;
layout(location = 2) in vec2 vtx_tex;

out vec3 our_color;
out vec2 tex_coord;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(vtx_pos, 1.0);
    our_color   = vtx_rgb;
    tex_coord   = vtx_tex;
}
