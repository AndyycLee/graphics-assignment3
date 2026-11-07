# CMake SFML Project Template (with 3D OpenGL)

Use this template repository for SFML windowing and input with 3D rendering handled by OpenGL.

This template is loosely based on the [original SFML CMake template](https://github.com/SFML/cmake-sfml-project). SFML is used for window creation and interaction; rendering is handled by OpenGL. Audio, networking, and SFML graphics components are disabled.

OpenGL is loaded via [GLAD](https://wikis.khronos.org/opengl/OpenGL_Loading_Library), and GLM supplies model, view, and projection matrices. Executables use an OpenGL 4.0 Core context and the GLSL shaders are copied by CMake into a `shaders` directory beside each executable.

## Meshes, Materials, and Shaders

The renderer separates mesh data, material settings, and shader programs:

- `Vertex` groups a position, normal, texture coordinate, and color. The current `Mesh` uses this fixed vertex layout for indexed triangle geometry; a position-only constructor fills in default values for the other attributes.
- `Material` holds basic ambient, diffuse, and specular lighting values, shininess, and a switch to enable or disable lighting.
- `Mesh` uploads vertices and indices to OpenGL buffers, optionally loads a diffuse texture, applies an object transform, and draws using a `Shader`. This supports textured or untextured meshes and lit or unlit rendering with the current shader pair.
- `Shader` loads, compiles, and links vertex and fragment GLSL files, then provides helpers for setting uniforms such as transforms and material properties.

The current implementation has one standard set of vertex attributes and supports one optional diffuse texture per mesh. It is a starting point for extending to other attribute layouts, multiple texture maps, or specialized shaders as needed.

## Executables

Two textured mesh executables are provided:

- `sphere` procedurally generates vertices, normals, triangle indices, and texture coordinates for a 64-by-32 latitude/longitude sphere.
- `cube` hard-codes the cube's vertices and texture coordinates. Each face has its own vertices and maps the full texture to that face.

Both programs require a texture image path as their only argument:

```sh
./build/bin/sphere path/to/texture.png
./build/bin/cube path/to/texture.png
```

Texture images are loaded by [`stb_image`](https://github.com/nothings/stb) (fetched by CMake), converted to RGBA, and vertically flipped to match OpenGL texture coordinates. Supported image formats are those supported by `stb_image`. The sphere expects a square texture image.

Both meshes rotate automatically using elapsed time measured with an SFML clock. The left and right arrow keys rotate the mesh in either direction, and Escape closes the window.

For questions or concerns, please get in touch with the instructor or one of the TAs.
