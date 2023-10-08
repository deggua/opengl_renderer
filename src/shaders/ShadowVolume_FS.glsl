#version 450 core
#extension GL_NV_shader_atomic_float : require

out vec4 fo_color;

uniform             layout(binding = 0, r32f) restrict coherent image2DMS g_shadow_depth;
uniform sampler2DMS g_framebuffer_depth;

float LinearDepth(float z_depth, float z_near, float z_far)
{
    float z_ndc = 2.0 * z_depth - 1.0;
    return 2.0 * z_near * z_far / (z_far + z_near - z_ndc * (z_far - z_near));
}

void main()
{
    // TODO: probably use a uniform for CLIP_NEAR and CLIP_FAR
    bool  frontface    = gl_FrontFacing;
    ivec2 pix_coord    = ivec2(gl_FragCoord.xy);
    float depth_buffer = texelFetch(g_framebuffer_depth, pix_coord, gl_SampleID).r;

    float depth;
    if (gl_FragCoord.z < depth_buffer) {
        depth = LinearDepth(gl_FragCoord.z, 0.1, 50.0);
    } else {
        depth = LinearDepth(depth_buffer, 0.1, 50.0);
    }

    if (frontface) {
        imageAtomicAdd(g_shadow_depth, pix_coord, gl_SampleID, -depth);
    } else {
        imageAtomicAdd(g_shadow_depth, pix_coord, gl_SampleID, depth);
    }

    fo_color = vec4(0.0, 0.0, 0.0, 0.0);
}

// Render front face fog
// Subtract back face fog

// Add fog up to front face
// Subtract fog up to back face

// Blend onto the main framebuffer
