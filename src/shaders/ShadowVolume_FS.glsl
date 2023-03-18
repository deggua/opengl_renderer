#version 330 core

out vec4 fo_color;

void main()
{
#if 0
    discard;
#else
    fo_color = vec4(1.0, 1.0, 1.0, 1.0);
#endif
}
