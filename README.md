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
* Bloom
* Postprocessing

## TODO:
* Shadow optimizations
* Fixed size 3D billboards
* Fog
* UI/Text rendering

## Should be implemented but unnecessary for now:
* Cubemap reflections
* Animations
* Particle effects
* Linux support (currently only tested on Windows)

## Might be implemented eventually:
* SSAO/GTAO
* SMAA
* Volumetric fog
* Volumetric light shafts
* DOF
* Soft stencil shadows (probably requires TAA or some form of temporal accumulation)
* Cascaded shadow maps
* Parallax maps
* Tesselation
* LOD/billboards
* Deferred rendering
* Order independent transparency
* GI model

## Dependencies:
* make [(Windows)](https://gnuwin32.sourceforge.net/packages/make.htm)
* [GLFW](https://github.com/glfw/glfw)
* [GLEW](https://github.com/nigels-com/glew)
* [GLM](https://github.com/g-truc/glm)
* [assimp](https://github.com/assimp/assimp)
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
