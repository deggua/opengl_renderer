#version 330 core

layout(location = 0) in vec3 vtx_pos;
layout(location = 1) in vec2 vtx_tex;

out vec2 tex_coord;

uniform mat4 mat_model; // obj    -> world
uniform mat4 mat_view;  // world  -> camera
uniform mat4 mat_proj;  // camera -> screen

void main()
{
    gl_Position = mat_proj * mat_view * mat_model * vec4(vtx_pos, 1.0);
    tex_coord   = vtx_tex;
}
