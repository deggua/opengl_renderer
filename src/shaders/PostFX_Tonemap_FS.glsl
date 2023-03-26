#version 450 core

// in
in vec2 vo_vtx_pos;
in vec2 vo_vtx_texcoord;

// out
out vec3 fo_color;

// uniform
uniform sampler2D g_screen;
uniform uint      g_tonemapper;

#define TONEMAP_REINHARD    0
#define TONEMAP_ACES_APPROX 1

vec3 Tonemap_Reinhard(vec3 hdr)
{
    return hdr / (hdr + vec3(1.0));
}

vec3 Tonemap_AcesApprox(vec3 hdr)
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

vec3 Tonemap_Clamp(vec3 hdr)
{
    return clamp(hdr, 0, 1);
}

vec3 Tonemap(vec3 hdr)
{
    if (g_tonemapper == TONEMAP_REINHARD) {
        return Tonemap_Reinhard(hdr);
    } else if (g_tonemapper == TONEMAP_ACES_APPROX) {
        return Tonemap_AcesApprox(hdr);
    } else {
        return Tonemap_Clamp(hdr);
    }
}

void main()
{
    vec3 hdr = texture(g_screen, vo_vtx_texcoord).rgb;
    fo_color = Tonemap(hdr);
}
