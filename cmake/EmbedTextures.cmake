# Embed compressed JPEGs, not decoded pixels. The binary is self-contained and
# can be launched by Finder/Explorer or from any working directory.
function(embed_textures output)
    set(content "// Generated from assets/textures; do not edit.\n#pragma once\nnamespace textures {\n")
    foreach(name earth moon sun)
        set(path "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/${name}.jpg")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
        file(READ "${path}" bytes HEX)
        # Keep generated lines short for MSVC as well as Clang/GCC.
        string(REGEX REPLACE "(................................)" "\\1\n" bytes "${bytes}")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${bytes}")
        string(APPEND content "inline constexpr unsigned char ${name}[] = {${bytes}};\n")
    endforeach()
    string(APPEND content "}\n")
    file(CONFIGURE OUTPUT "${output}" CONTENT "${content}" @ONLY)
endfunction()
