# A real-time renderer using OpenGL

![screenshot](assets/screenshot.png)

## Currently implements:
* Mesh importing (via Assimp)
* Texture importing (via stb_image)
* Blinn-Phong shading
* Point lights, directional lights, and spot lights
* Real-time shadow volumes (stencil shadows) for all light types
* MSAA + AF
* Skyboxes
* HDR Tonemapping
* Normal maps

## TODO:
* Shadow optimizations
* Cubemap reflections
* Bloom
* Animations
* Particle effects
* Fog
* UI/Text
* Linux support (currently only tested on Windows)

## Might be implemented eventually:
* SSAO
* SMAA
* DOF
* Soft stencil shadows (probably requires TAA or some form of temporal accumulation)
* Shadow maps
* Planar shadows
* Parallax maps
* Tesselation
* LOD/billboards
* Forward+ rendering
* Order independent transparency
* GI model

## Dependencies:
* make [(Windows)](https://gnuwin32.sourceforge.net/packages/make.htm)
* [GLFW](https://github.com/glfw/glfw)
* [GLEW](https://github.com/nigels-com/glew)
* [GLM](https://github.com/g-truc/glm)
* [assimp](https://github.com/assimp/assimp)
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
