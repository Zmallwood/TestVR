#pragma once
#include "OvrGeometry.h"
#include "engine/Macros.h"
#include "engine/types/ovrProgram.h"
#include "engine/types/ovrUniform.h"
#include <openxr/openxr.h>
#include <stdio.h>
#include <string.h>

namespace nar
{
    static ovrUniform ProgramUniforms[] = {
        {MODEL_MATRIX, MATRIX4X4, "modelMatrix"},
        {VIEW_PROJ_MATRIX, MATRIX4X4, "viewProjectionMatrix"},
    };

    static void ovrProgram_Clear(ovrProgram *program) {
        program->Program = 0;
        program->VertexShader = 0;
        program->FragmentShader = 0;
        memset(program->UniformLocation, 0, sizeof(program->UniformLocation));
        memset(program->UniformBinding, 0, sizeof(program->UniformBinding));
        memset(program->Textures, 0, sizeof(program->Textures));
    }

    static bool ovrProgram_Create(ovrProgram *program, const char *vertexSource, const char *fragmentSource) {
        GLint r;

        GL(program->VertexShader = glCreateShader(GL_VERTEX_SHADER));

        GL(glShaderSource(program->VertexShader, 1, &vertexSource, 0));
        GL(glCompileShader(program->VertexShader));
        GL(glGetShaderiv(program->VertexShader, GL_COMPILE_STATUS, &r));
        if (r == GL_FALSE) {
            GLchar msg[4096];
            GL(glGetShaderInfoLog(program->VertexShader, sizeof(msg), 0, msg));
            ALOGE("%s\n%s\n", vertexSource, msg);
            return false;
        }

        GL(program->FragmentShader = glCreateShader(GL_FRAGMENT_SHADER));
        GL(glShaderSource(program->FragmentShader, 1, &fragmentSource, 0));
        GL(glCompileShader(program->FragmentShader));
        GL(glGetShaderiv(program->FragmentShader, GL_COMPILE_STATUS, &r));
        if (r == GL_FALSE) {
            GLchar msg[4096];
            GL(glGetShaderInfoLog(program->FragmentShader, sizeof(msg), 0, msg));
            ALOGE("%s\n%s\n", fragmentSource, msg);
            return false;
        }

        GL(program->Program = glCreateProgram());
        GL(glAttachShader(program->Program, program->VertexShader));
        GL(glAttachShader(program->Program, program->FragmentShader));

        // Bind the vertex attribute locations.
        for (size_t i = 0; i < sizeof(ProgramVertexAttributes) / sizeof(ProgramVertexAttributes[0]); i++) {
            GL(glBindAttribLocation(
                program->Program, ProgramVertexAttributes[i].location, ProgramVertexAttributes[i].name));
        }

        GL(glLinkProgram(program->Program));
        GL(glGetProgramiv(program->Program, GL_LINK_STATUS, &r));
        if (r == GL_FALSE) {
            GLchar msg[4096];
            GL(glGetProgramInfoLog(program->Program, sizeof(msg), 0, msg));
            ALOGE("Linking program failed: %s\n", msg);
            return false;
        }

        int numBufferBindings = 0;

        // Get the uniform locations.
        memset(program->UniformLocation, -1, sizeof(program->UniformLocation));
        for (size_t i = 0; i < sizeof(ProgramUniforms) / sizeof(ProgramUniforms[0]); i++) {
            const int uniformIndex = ProgramUniforms[i].index;
            if (ProgramUniforms[i].type == BUFFER) {
                GL(program->UniformLocation[uniformIndex] =
                       glGetUniformBlockIndex(program->Program, ProgramUniforms[i].name));
                program->UniformBinding[uniformIndex] = numBufferBindings++;
                GL(glUniformBlockBinding(
                    program->Program, program->UniformLocation[uniformIndex], program->UniformBinding[uniformIndex]));
            }
            else {
                GL(program->UniformLocation[uniformIndex] =
                       glGetUniformLocation(program->Program, ProgramUniforms[i].name));
                program->UniformBinding[uniformIndex] = program->UniformLocation[uniformIndex];
            }
        }

        GL(glUseProgram(program->Program));

        // Get the texture locations.
        for (int i = 0; i < MAX_PROGRAM_TEXTURES; i++) {
            char name[32];
            sprintf(name, "Texture%i", i);
            program->Textures[i] = glGetUniformLocation(program->Program, name);
            if (program->Textures[i] != -1) {
                GL(glUniform1i(program->Textures[i], i));
            }
        }

        GL(glUseProgram(0));

        return true;
    }

    static void ovrProgram_Destroy(ovrProgram *program) {
        if (program->Program != 0) {
            GL(glDeleteProgram(program->Program));
            program->Program = 0;
        }
        if (program->VertexShader != 0) {
            GL(glDeleteShader(program->VertexShader));
            program->VertexShader = 0;
        }
        if (program->FragmentShader != 0) {
            GL(glDeleteShader(program->FragmentShader));
            program->FragmentShader = 0;
        }
    }

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