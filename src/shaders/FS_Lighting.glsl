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
in vec3 vo_vtx_pos;
in vec3 vo_vtx_normal;
in vec2 vo_vtx_texcoord;

// out
out vec4 fo_color;

// Phong lighting model
vec4 ComputeAmbientLight(vec3 light_color, vec4 obj_diffuse)
{
    return vec4(light_color, 1.0) * obj_diffuse;
}

vec4 ComputeDiffuseLight(vec3 light_color, vec4 obj_diffuse, vec3 frag_norm, vec3 light_dir)
{
    float diffuse_intensity = max(dot(frag_norm, light_dir), 0.0);
    return vec4(light_color, 1.0) * obj_diffuse * diffuse_intensity;
}

vec4 ComputeSpecularLight(
    vec3  light_color,
    vec4  obj_specular,
    float obj_gloss,
    vec3  frag_norm,
    vec3  light_dir,
    vec3  view_dir)
{
    // gloss compensation for Blinn-Phong
    const float energy_factor      = 24.0 / (8.0 * PI);
    vec3        halfway_dir        = normalize(light_dir + view_dir);
    float       specular_intensity = pow(max(dot(frag_norm, halfway_dir), 0.0), obj_gloss);
    return energy_factor * vec4(light_color, 1.0) * obj_specular * specular_intensity;
}

// Helper functions
float ComputeLightFalloff(vec3 light_pos, vec3 frag_pos)
{
    float frag2light_dist = distance(light_pos, frag_pos);

    float falloff = 1.0;
    falloff /= 1.0 + frag2light_dist * frag2light_dist;

    return falloff;
}

// uniform
uniform vec3     g_view_pos;
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

// Light source computation
vec4 ComputeLighting(
    AmbientLight light,
    vec4         obj_diffuse,
    vec4         obj_specular,
    float        obj_gloss,
    vec3         frag_pos,
    vec3         frag_norm,
    vec3         view_pos)
{
    vec4 total_light = ComputeAmbientLight(light.color, obj_diffuse);

    return total_light;
}

vec4 ComputeLighting(
    PointLight light,
    vec4       obj_diffuse,
    vec4       obj_specular,
    float      obj_gloss,
    vec3       frag_pos,
    vec3       frag_norm,
    vec3       view_pos)
{
    vec3 frag2view_dir  = normalize(view_pos - frag_pos);
    vec3 frag2light_dir = normalize(light.pos - frag_pos);

    // inverse square law falloff
    float dist_falloff = ComputeLightFalloff(light.pos, frag_pos);

    // specular + diffuse light contribution
    vec4 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec4 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec4 total_light = dist_falloff * (diffuse_light + specular_light);

    return total_light;
}

vec4 ComputeLighting(
    SpotLight light,
    vec4      obj_diffuse,
    vec4      obj_specular,
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
    float dist_falloff = ComputeLightFalloff(light.pos, frag_pos);

    // specular + diffuse contribution
    vec4 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec4 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec4 total_light = radial_falloff * dist_falloff * (diffuse_light + specular_light);

    return total_light;
}

vec4 ComputeLighting(
    SunLight light,
    vec4     obj_diffuse,
    vec4     obj_specular,
    float    obj_gloss,
    vec3     frag_pos,
    vec3     frag_norm,
    vec3     view_pos)
{
    vec3 frag2view_dir  = normalize(view_pos - frag_pos);
    vec3 frag2light_dir = -light.dir;

    vec4 diffuse_light = ComputeDiffuseLight(light.color, obj_diffuse, frag_norm, frag2light_dir);
    vec4 specular_light
        = ComputeSpecularLight(light.color, obj_specular, obj_gloss, frag_norm, frag2light_dir, frag2view_dir);
    vec4 total_light = diffuse_light + specular_light;

    return total_light;
}

// Main program
void main()
{
    vec3 frag_norm = normalize(vo_vtx_normal); // TODO: is normalizing necessary?

    vec4  obj_diffuse  = texture(g_material.diffuse, vo_vtx_texcoord);
    vec4  obj_specular = texture(g_material.specular, vo_vtx_texcoord);
    float obj_gloss    = g_material.gloss;

    if (obj_diffuse.a < 0.5) {
        discard;
    } else {
        vec4 light_color
            = ComputeLighting(g_light_source, obj_diffuse, obj_specular, obj_gloss, vo_vtx_pos, frag_norm, g_view_pos);

        fo_color = light_color;
    }
}
