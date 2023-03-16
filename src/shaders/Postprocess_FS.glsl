#version 450 core

// in
in vec2 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec4 fo_color;

// uniform
uniform sampler2D g_screen;
uniform float     g_gamma;

#define TONEMAP_REINHARD 0
#define TONEMAP_ACES     1
#define TONEMAP          TONEMAP_ACES

#if TONEMAP == TONEMAP_REINHARD
vec3 tonemap(vec3 hdr)
{
    return hdr / (hdr + vec3(1.0));
}

#elif TONEMAP == TONEMAP_ACES
vec3 tonemap(vec3 hdr)
{
    const vec3 a = vec3(2.51);
    const vec3 b = vec3(0.03);
    const vec3 c = vec3(2.43);
    const vec3 d = vec3(0.59);
    const vec3 e = vec3(0.14);

    vec3 numer = hdr * (a * hdr + b);
    vec3 denom = hdr * (c * hdr + d) + e;
    return clamp(numer / denom, 0.0, 1.0);
}

#else
vec3 tonemap(vec3 hdr)
{
    return clamp(hdr, 0.0, 1.0);
}
#endif

void main()
{
    vec3 hdr             = texture(g_screen, vo_vtx_texcoord).rgb;
    vec3 tonemapped      = tonemap(hdr);
    vec3 gamma_corrected = pow(tonemapped, vec3(1.0 / g_gamma));
    fo_color             = vec4(gamma_corrected, 1.0);
}
