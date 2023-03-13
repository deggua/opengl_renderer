# A real-time renderer using OpenGL
Currently implements:
* Mesh importing (via Assimp)
* Texture importing (via stb_image)
* Blinn-Phong shading
* Point lights, directional lights, and spot lights
* Real-time shadow volumes (stencil shadows) for all light types via a geometry shader

TODO:
* Shadow optimizations
* Normal maps
* HDR + tonemapping
* Anti-aliasing
* Anistropic filtering
* Skyboxes
* Cubemap reflections
* Bloom
* Animations
* Particle effects
* Depth fog
* UI
* Linux support (currently only tested on Windows)

Might be implemented eventually:
* SSAO
* Soft stencil shadows
* Shadow maps
* Planar shadows
* Parallax maps
* Tesselation
* LOD/billboards
* Forward+ rendering
* Order independent transparency

Dependencies:
* GLFW
* GLEW
* GLM
* Assimp
* stb_image
