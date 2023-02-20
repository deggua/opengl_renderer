#version 330 core

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D texture_0;
uniform sampler2D texture_1;

void main()
{
    vec4 tex0 = texture(texture_0, tex_coord);
    vec4 tex1 = texture(texture_1, tex_coord);

    frag_color = mix(tex0, tex1, 0.2);
}
