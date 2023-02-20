#version 330 core

out vec4 frag_color;
in vec3  our_color;
in vec2  tex_coord;

uniform sampler2D texture_0;
uniform sampler2D texture_1;
uniform float     blend_amt;

void main()
{
    vec4 tex0 = texture(texture_0, tex_coord);
    vec4 tex1 = texture(texture_1, tex_coord);

    frag_color = mix(tex0, tex1, clamp(blend_amt, 0.0, 1.0));
}
