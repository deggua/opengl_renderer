#version 450 core
#extension GL_NV_shader_atomic_float : require

out vec4 fo_color;

uniform layout(binding = 0, r32f) restrict coherent image2D g_shadow_depth;

#if 0
#    define DENSITY 1

float FogIntensity(float depth)
{
    float intensity = pow(exp(depth * DENSITY) - 1, 3);
    return clamp(intensity, 0.0, 1.0);
}
#endif

float LinearDepth(float z_depth, float z_near, float z_far)
{
    float z_ndc = 2.0 * z_depth - 1.0;
    return 2.0 * z_near * z_far / (z_far + z_near - z_ndc * (z_far - z_near));
}

void main()
{
    // TODO: probably use a uniform for CLIP_NEAR and CLIP_FAR
    float depth     = LinearDepth(gl_FragCoord.z, 0.1, 50.0);
    bool  frontface = gl_FrontFacing;
    ivec2 pix_coord = ivec2(gl_FragCoord.xy);

    if (!frontface) {
        imageAtomicAdd(g_shadow_depth, pix_coord, depth);
    } else {
        imageAtomicAdd(g_shadow_depth, pix_coord, -depth);
    }

    fo_color = vec4(0.0, 0.0, 0.0, 0.0);
}

// Render front face fog
// Subtract back face fog

// Add fog up to front face
// Subtract fog up to back face

// Blend onto the main framebuffer
