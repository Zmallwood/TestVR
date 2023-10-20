#pragma once
#include <GLES3/gl3.h>

namespace nar
{
    struct ovrVertexAttribPointer {
        GLint Index;
        GLint Size;
        GLenum Type;
        GLboolean Normalized;
        GLsizei Stride;
        const GLvoid *Pointer;
    };
}