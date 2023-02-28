/*
#version 330 core

#define AMBIENT_LIGHT 0
#define POINT_LIGHT   1
#define SPOT_LIGHT    2
#define SUN_LIGHT     3
*/

#define PI 3.14159265

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     gloss;
};

struct DistanceFalloff {
    float k0, k1, k2;
};

struct PointLight {
    vec3            pos;
    DistanceFalloff falloff;

    vec3 color;
};

struct SpotLight {
    vec3            pos;
    vec3            dir;          // must be pre-normalized
    float           inner_cutoff; // dot(dir, inner_dir)
    float           outer_cutoff; // dot(dit, outer_dir)
    DistanceFalloff falloff;

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
in vec2 g_frag_tex_coord;
in vec3 g_frag_normal;
in vec3 g_frag_pos;

// out
out vec4 g_frag_color;

// Phong lighting model
vec3 ComputeAmbientLight(vec3 light_color, vec3 obj_diffuse)
{
    return light_color * obj_diffuse;
}

vec3 ComputeDiffuseLight(vec3 light_color, vec3 obj_diffuse, vec3 frag_norm, vec3 light_dir)
{
    float diffuse_intensity = max(dot(frag_norm, light_dir), 0.0);
    return light_color * obj_diffuse * diffuse_intensity;
}

vec3 ComputeSpecularLight(
    vec3  light_color,
    vec3  obj_specular,
    float obj_gloss,
    vec3  frag_norm,
    vec3  light_dir,
    vec3  view_dir)
{
    // gloss compensation for Blinn-Phong
    const float energy_factor      = 24.0 / (8.0 * PI);
    vec3        halfway_dir        = normalize(light_dir + view_dir);
    float       specular_intensity = pow(max(dot(frag_norm, halfway_dir), 0.0), obj_gloss);
    return energy_factor * light_color * obj_specular * specular_intensity;
}

// Helper functions
float ComputeDistanceFalloff(DistanceFalloff falloff, vec3 light_pos, vec3 frag_pos)
{
    float frag2light_dist = distance(light_pos, frag_pos);

    float dist_falloff = 1.0;
    dist_falloff /= falloff.k0 + falloff.k1 * frag2light_dist + falloff.k2 * frag2light_dist * frag2light_dist;

    return dist_falloff;
}

// uniform
uniform vec3     g_view_pos;
uniform Material g_material;

// SUN LIGHT
#if LIGHT_TYPE == SUN_LIGHT
uniform SunLight g_light_source;

vec3 ComputeLighting(
    SunLight light,
    vec3     obj_diffuse,
    vec3     obj_specular,
    float    obj_gloss,
    vec3     frag_pos,
    vec3     frag_norm,
    vec3     view_pos)
{
    vec3 frag2view_dir  = normalize(view_pos - frag_pos);
    vec3 frag2light_dir = -light.dir;

    vec3 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec3 total_light = diffuse_light + specular_light;

    return total_light;
}

// AMBIENT LIGHT
#elif LIGHT_TYPE == AMBIENT_LIGHT
uniform AmbientLight g_light_source;

vec3 ComputeLighting(
    AmbientLight light,
    vec3         obj_diffuse,
    vec3         obj_specular,
    float        obj_gloss,
    vec3         frag_pos,
    vec3         frag_norm,
    vec3         view_pos)
{
    vec3 total_light = ComputeAmbientLight(light.color, obj_diffuse);

    return total_light;
}

// POINT LIGHT
#elif LIGHT_TYPE == POINT_LIGHT
uniform PointLight g_light_source;

vec3 ComputeLighting(
    PointLight light,
    vec3       obj_diffuse,
    vec3       obj_specular,
    float      obj_gloss,
    vec3       frag_pos,
    vec3       frag_norm,
    vec3       view_pos)
{
    vec3 frag2view_dir  = normalize(view_pos - frag_pos);
    vec3 frag2light_dir = normalize(light.pos - frag_pos);

    // inverse square law falloff
    float dist_falloff = ComputeDistanceFalloff(light.falloff, light.pos, frag_pos);

    // specular + diffuse light contribution
    vec3 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec3 total_light = dist_falloff * (diffuse_light + specular_light);

    return total_light;
}

// SPOT LIGHT
#elif LIGHT_TYPE == SPOT_LIGHT
uniform SpotLight g_light_source;

vec3 ComputeLighting(
    SpotLight light,
    vec3      obj_diffuse,
    vec3      obj_specular,
    float     obj_gloss,
    vec3      frag_pos,
    vec3      frag_norm,
    vec3      view_pos)
{
    vec3 frag2view_dir  = normalize(view_pos - frag_pos);
    vec3 frag2light_dir = normalize(light.pos - frag_pos);

    // linear radial falloff
    float frag_cos_theta  = dot(-frag2light_dir, light.dir);
    float inner_cos_phi   = light.inner_cutoff;
    float outer_cos_gamma = light.outer_cutoff;
    float radial_falloff  = clamp((frag_cos_theta - outer_cos_gamma) / (inner_cos_phi - outer_cos_gamma), 0.0, 1.0);

    // inverse square law distance falloff
    float dist_falloff = ComputeDistanceFalloff(light.falloff, light.pos, frag_pos);

    // specular + diffuse contribution
    vec3 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec3 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec3 total_light = radial_falloff * dist_falloff * (diffuse_light + specular_light);

    return total_light;
}

#endif

void main()
{
    vec3 frag_norm = normalize(g_frag_normal); // TODO: is normalizing necessary?

    vec3  obj_diffuse  = vec3(texture(g_material.diffuse, g_frag_tex_coord));
    vec3  obj_specular = vec3(texture(g_material.specular, g_frag_tex_coord));
    float obj_gloss    = g_material.gloss;

    vec3 light_color
        = ComputeLighting(g_light_source, obj_diffuse, obj_specular, obj_gloss, g_frag_pos, frag_norm, g_view_pos);

    g_frag_color = vec4(light_color, 1.0);
}
