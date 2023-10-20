#pragma once
#include "engine/types/ovrUniform.h"

namespace nar
{
    struct ovrGLProgram;

    extern void ovrGLProgram_Clear(ovrGLProgram *program);

    extern bool ovrGLProgram_Create(ovrGLProgram *program, const char *vertexSource, const char *fragmentSource);

    extern void ovrGLProgram_Destroy(ovrGLProgram *program);

    static ovrUniform ProgramUniforms[] = {
        {MODEL_MATRIX, MATRIX4X4, "modelMatrix"},
        {VIEW_PROJ_MATRIX, MATRIX4X4, "viewProjectionMatrix"},
    };

    static const char VERTEX_SHADER[] =
        "#version 300 es\n"
        "in vec3 vertexPosition;\n"
        "in vec4 vertexColor;\n"
        "uniform mat4 viewProjectionMatrix;\n"
        "uniform mat4 modelMatrix;\n"
        "out vec4 fragmentColor;\n"
        "void main()\n"
        "{\n"
        " gl_Position = viewProjectionMatrix * ( modelMatrix * vec4( vertexPosition, 1.0 ) );\n"
        " fragmentColor = vertexColor;\n"
        "}\n";

    static const char FRAGMENT_SHADER[] = "#version 300 es\n"
                                          "in lowp vec4 fragmentColor;\n"
                                          "out lowp vec4 outColor;\n"
                                          "void main()\n"
                                          "{\n"
                                          " outColor = fragmentColor;\n"
                                          "}\n";

}