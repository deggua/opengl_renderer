/*
#version 450 core

#define AMBIENT_LIGHT 0
#define POINT_LIGHT   1
#define SPOT_LIGHT    2
#define SUN_LIGHT     3
*/

#define PI 3.14159265

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    float     gloss;
};

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
#if LIGHT_TYPE != AMBIENT_LIGHT
in vec3 vo_light_dir;
in vec3 vo_view_dir;
#endif
in vec3 vo_vtx_pos;
in vec3 vo_vtx_normal;
in vec2 vo_vtx_texcoord;

// out
out vec4 fo_color;

// uniform
uniform mat3 g_mtx_normal; // normal -> world
uniform mat4 g_mtx_world;  // obj    -> world
uniform mat4 g_mtx_wvp;    // obj    -> screen

layout(std140, binding = 0) uniform Shared
{
    mat4 g_mtx_vp;
    mat4 g_mtx_view;
    mat4 g_mtx_proj;
    vec3 g_pos_view;
};

uniform Material g_material;

#if LIGHT_TYPE == SUN_LIGHT
uniform SunLight g_light_source;

#elif LIGHT_TYPE == AMBIENT_LIGHT
uniform AmbientLight g_light_source;

#elif LIGHT_TYPE == POINT_LIGHT
uniform PointLight g_light_source;

#elif LIGHT_TYPE == SPOT_LIGHT
uniform SpotLight g_light_source;

#endif

// Phong lighting model
vec3 ComputeAmbientLight(vec3 light_color, vec3 frag_diffuse)
{
    return light_color * frag_diffuse;
}

vec3 ComputeDiffuseLight(vec3 light_color, vec3 frag_diffuse, vec3 frag_norm, vec3 light_dir)
{
    float diffuse_intensity = max(dot(frag_norm, light_dir), 0.0);
    return light_color * frag_diffuse * diffuse_intensity;
}

vec3 ComputeSpecularLight(
    vec3  light_color,
    vec3  frag_specular,
    float frag_gloss,
    vec3  frag_norm,
    vec3  light_dir,
    vec3  view_dir)
{
    // energy conserving factor for Blinn-Phong
    // see: https://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
    float energy_factor = (frag_gloss + 8.0) / (8.0 * PI);

    // TODO: since we're now in tangent space, I think there is a simpler way to compute this
    // backfacing check, e.g. check if Z < 0
    // TODO: I don't know if this is actually required, some SO posts seemed to recommend this
    // but the above link just has the cos term from the dot product, so maybe it isn't
    // would be good to test by looking at a glossy sphere near the discontinuity
    // backfacing lights and backfacing view shouldn't produce any specular effects
    // attenuating by cos_term avoids the discontinuity
    float cos_term = dot(frag_norm, light_dir);
    if (cos_term <= 0.0 || dot(frag_norm, view_dir) <= 0.0) {
        return vec3(0.0);
    }

    vec3 halfway_dir = normalize(light_dir + view_dir);
    // TODO: the max here might be unnecesary with the above checks
    float specular_intensity = pow(max(dot(frag_norm, halfway_dir), 0.0), frag_gloss);
    return energy_factor * light_color * frag_specular * specular_intensity * cos_term;
}

float distance2(vec3 v1, vec3 v2)
{
    vec3 tmp = v1 - v2;
    return dot(tmp, tmp);
}

// Inverse square falloff
float ComputeLightFalloff(vec3 light_pos, vec3 frag_pos)
{
    float frag2light_dist2 = distance2(light_pos, frag_pos);

    float falloff = 1.0;
    falloff /= 1.0 + frag2light_dist2;

    return falloff;
}

// Light source computation
#if LIGHT_TYPE == AMBIENT_LIGHT
vec3 ComputeLighting(
    AmbientLight light,
    vec3         frag_diffuse,
    vec3         frag_specular,
    vec3         frag_norm,
    float        frag_gloss,
    vec3         frag_pos,
    vec3         view_pos)
{
    return ComputeAmbientLight(light.color, frag_diffuse);
}
#endif

#if LIGHT_TYPE == POINT_LIGHT
vec3 ComputeLighting(
    PointLight light,
    vec3       frag_diffuse,
    vec3       frag_specular,
    vec3       frag_norm,
    float      frag_gloss,
    vec3       frag_pos,
    vec3       view_pos)
{
    vec3 frag2view_dir  = vo_view_dir;
    vec3 frag2light_dir = vo_light_dir;

    float dist_falloff = ComputeLightFalloff(light.pos, frag_pos);

    // specular + diffuse light contribution
    vec3 diffuse_light  = ComputeDiffuseLight(light.color, frag_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light = ComputeSpecularLight(
        light.color,
        frag_specular,
        frag_gloss,
        frag_norm,
        frag2light_dir,
        frag2view_dir);
    vec3 total_light = dist_falloff * (diffuse_light + specular_light);

    return total_light;
}
#endif

#if LIGHT_TYPE == SPOT_LIGHT
vec3 ComputeLighting(
    SpotLight light,
    vec3      frag_diffuse,
    vec3      frag_specular,
    vec3      frag_norm,
    float     frag_gloss,
    vec3      frag_pos,
    vec3      view_pos)
{
    vec3 frag2view_dir  = vo_view_dir;
    vec3 frag2light_dir = vo_light_dir;

    // linear radial falloff
    float frag_cos_theta  = dot(-normalize(light.pos - frag_pos), light.dir);
    float inner_cos_phi   = light.inner_cutoff;
    float outer_cos_gamma = light.outer_cutoff;
    float radial_falloff
        = clamp((frag_cos_theta - outer_cos_gamma) / (inner_cos_phi - outer_cos_gamma), 0.0, 1.0);

    // inverse square law distance falloff
    float dist_falloff = ComputeLightFalloff(light.pos, frag_pos);

    // specular + diffuse contribution
    vec3 diffuse_light  = ComputeDiffuseLight(light.color, frag_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light = ComputeSpecularLight(
        light.color,
        frag_specular,
        frag_gloss,
        frag_norm,
        frag2light_dir,
        frag2view_dir);
    vec3 total_light = radial_falloff * dist_falloff * (diffuse_light + specular_light);

    return total_light;
}
#endif

#if LIGHT_TYPE == SUN_LIGHT
vec3 ComputeLighting(
    SunLight light,
    vec3     frag_diffuse,
    vec3     frag_specular,
    vec3     frag_norm,
    float    frag_gloss,
    vec3     frag_pos,
    vec3     view_pos)
{
    vec3 frag2view_dir  = vo_view_dir;
    vec3 frag2light_dir = vo_light_dir;

    vec3 diffuse_light  = ComputeDiffuseLight(light.color, frag_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light = ComputeSpecularLight(
        light.color,
        frag_specular,
        frag_gloss,
        frag_norm,
        frag2light_dir,
        frag2view_dir);
    vec3 total_light = diffuse_light + specular_light;

    return total_light;
}
#endif

// Main program
void main()
{
    vec4  frag_diffuse  = texture(g_material.diffuse, vo_vtx_texcoord);
    vec4  frag_specular = texture(g_material.specular, vo_vtx_texcoord);
    vec3  frag_normal   = 2.0 * texture(g_material.normal, vo_vtx_texcoord).rgb - 1.0;
    float frag_gloss    = g_material.gloss;

    if (frag_diffuse.a < 0.5) {
        discard;
    } else {
        vec3 light_color = ComputeLighting(
            g_light_source,
            frag_diffuse.rgb,
            frag_specular.rgb,
            frag_normal,
            frag_gloss,
            vo_vtx_pos,
            g_pos_view);

        fo_color = vec4(light_color, 1.0);
    }
}
