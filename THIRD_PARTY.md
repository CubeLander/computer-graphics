# Third-party components

Project-authored source is MIT licensed. Dependencies retain their own licenses.

| Component | Pinned source | License | Use |
| --- | --- | --- | --- |
| GLFW 3.4 | https://github.com/glfw/glfw/tree/a74efa0d5628b74adc0426af4c5710e287fa7c2c | zlib/libpng | Native windows, input and context creation; built statically |
| GLAD | `deps/glad/gl.h` in the pinned GLFW source | See `third_party/glad/LICENSE` (upstream GLAD and Khronos notices) | Generated OpenGL loader; no legacy rendering calls used |
| GLM 1.0.1 | https://github.com/g-truc/glm/tree/0af55ccecd98d4e5a8d1fad7de25ba429d60e863 | MIT or Modified MIT, per `copying.txt` | Vector and matrix operations |
| stb | https://github.com/nothings/stb/tree/a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d | MIT or public domain | Vendored, unmodified `stb_easy_font.h` and `stb_image_write.h`; see `third_party/stb/LICENSE` |

GLFW and GLM are downloaded at first CMake configure, not at application runtime.
The release package includes dependency license texts. No external textures or fonts are needed for assignment one.

GLAD license text is from https://github.com/Dav1dde/glad/blob/v2.0.0/LICENSE; the exact Khronos platform notice embedded in the generated header is appended.
