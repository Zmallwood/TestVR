#pragma once
#include "engine/Constants.h"
#include <GLES3/gl3.h>

namespace nar
{
    struct ovrGLProgram {
        GLuint Program;
        GLuint VertexShader;
        GLuint FragmentShader;
        // These will be -1 if not used by the program.
        GLint UniformLocation[MAX_PROGRAM_UNIFORMS]; // ProgramUniforms[].name
        GLint UniformBinding[MAX_PROGRAM_UNIFORMS];  // ProgramUniforms[].name
        GLint Textures[MAX_PROGRAM_TEXTURES];        // Texture%i
    };
}