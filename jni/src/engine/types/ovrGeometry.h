#pragma once
#include "engine/Constants.h"
#include "ovrVertexAttribPointer.h"
#include <GLES3/gl3.h>

namespace nar
{
    struct ovrGeometry {
        GLuint VertexBuffer;
        GLuint IndexBuffer;
        GLuint VertexArrayObject;
        int VertexCount;
        int IndexCount;
        ovrVertexAttribPointer VertexAttribs[MAX_VERTEX_ATTRIB_POINTERS];
    };
}