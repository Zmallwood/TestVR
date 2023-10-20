#pragma once
#include <GLES3/gl3.h>
#include <stdio.h>
#include <string.h>
#include "engine/types/ovrGeometry.h"

namespace nar
{
    enum VertexAttributeLocation {
        VERTEX_ATTRIBUTE_LOCATION_POSITION,
        VERTEX_ATTRIBUTE_LOCATION_COLOR,
        VERTEX_ATTRIBUTE_LOCATION_UV,
        VERTEX_ATTRIBUTE_LOCATION_TRANSFORM
    };

    struct ovrVertexAttribute {
        enum VertexAttributeLocation location;
        const char *name;
    };

    static ovrVertexAttribute ProgramVertexAttributes[] = {
        {VERTEX_ATTRIBUTE_LOCATION_POSITION, "vertexPosition"},
        {VERTEX_ATTRIBUTE_LOCATION_COLOR, "vertexColor"},
        {VERTEX_ATTRIBUTE_LOCATION_UV, "vertexUv"},
    };

    static void ovrGeometry_Clear(ovrGeometry *geometry) {
        geometry->VertexBuffer = 0;
        geometry->IndexBuffer = 0;
        geometry->VertexArrayObject = 0;
        geometry->VertexCount = 0;
        geometry->IndexCount = 0;
        for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
            memset(&geometry->VertexAttribs[i], 0, sizeof(geometry->VertexAttribs[i]));
            geometry->VertexAttribs[i].Index = -1;
        }
    }

    static void ovrGeometry_CreateGroundPlane(ovrGeometry *geometry) {
        typedef struct {
            float positions[12][4];
            unsigned char colors[12][4];
        } ovrCubeVertices;

        static const ovrCubeVertices cubeVertices = {
            // positions
            {{4.5f, 0.0f, 4.5f, 1.0f},
             {4.5f, 0.0f, -4.5f, 1.0f},
             {-4.5f, 0.0f, -4.5f, 1.0f},
             {-4.5f, 0.0f, 4.5f, 1.0f},

             {4.5f, -10.0f, 4.5f, 1.0f},
             {4.5f, -10.0f, -4.5f, 1.0f},
             {-4.5f, -10.0f, -4.5f, 1.0f},
             {-4.5f, -10.0f, 4.5f, 1.0f},

             {4.5f, 10.0f, 4.5f, 1.0f},
             {4.5f, 10.0f, -4.5f, 1.0f},
             {-4.5f, 10.0f, -4.5f, 1.0f},
             {-4.5f, 10.0f, 4.5f, 1.0f}},
            // colors
            {{255, 0, 0, 255},
             {0, 255, 0, 255},
             {0, 0, 255, 255},
             {255, 255, 0, 255},

             {255, 128, 0, 255},
             {0, 255, 255, 255},
             {0, 0, 255, 255},
             {255, 0, 255, 255},

             {255, 128, 128, 255},
             {128, 255, 128, 255},
             {128, 128, 255, 255},
             {255, 255, 128, 255}},
        };

        static const unsigned short cubeIndices[18] = {
            0, 1, 2,  0, 2,  3,

            4, 5, 6,  4, 6,  7,

            8, 9, 10, 8, 10, 11,
        };

        geometry->VertexCount = 12;
        geometry->IndexCount = 18;

        geometry->VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
        geometry->VertexAttribs[0].Size = 4;
        geometry->VertexAttribs[0].Type = GL_FLOAT;
        geometry->VertexAttribs[0].Normalized = false;
        geometry->VertexAttribs[0].Stride = sizeof(cubeVertices.positions[0]);
        geometry->VertexAttribs[0].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, positions);

        geometry->VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
        geometry->VertexAttribs[1].Size = 4;
        geometry->VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
        geometry->VertexAttribs[1].Normalized = true;
        geometry->VertexAttribs[1].Stride = sizeof(cubeVertices.colors[0]);
        geometry->VertexAttribs[1].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, colors);

        GL(glGenBuffers(1, &geometry->VertexBuffer));
        GL(glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer));
        GL(glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

        GL(glGenBuffers(1, &geometry->IndexBuffer));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer));
        GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    static void ovrGeometry_CreateStagePlane(ovrGeometry *geometry, float minx, float minz, float maxx, float maxz) {
        typedef struct {
            float positions[12][4];
            unsigned char colors[12][4];
        } ovrCubeVertices;

        const ovrCubeVertices cubeVertices = {
            // positions
            {{maxx, 0.0f, maxz, 1.0f},
             {maxx, 0.0f, minz, 1.0f},
             {minx, 0.0f, minz, 1.0f},
             {minx, 0.0f, maxz, 1.0f},

             {maxx, -3.0f, maxz, 1.0f},
             {maxx, -3.0f, minz, 1.0f},
             {minx, -3.0f, minz, 1.0f},
             {minx, -3.0f, maxz, 1.0f},

             {maxx, 3.0f, maxz, 1.0f},
             {maxx, 3.0f, minz, 1.0f},
             {minx, 3.0f, minz, 1.0f},
             {minx, 3.0f, maxz, 1.0f}},
            // colors
            {{128, 0, 0, 255},
             {0, 128, 0, 255},
             {0, 0, 128, 255},
             {128, 128, 0, 255},

             {128, 64, 0, 255},
             {0, 128, 128, 255},
             {0, 0, 128, 255},
             {128, 0, 128, 255},

             {128, 64, 64, 255},
             {64, 128, 64, 255},
             {64, 64, 128, 255},
             {128, 128, 64, 255}},
        };

        static const unsigned short cubeIndices[18] = {
            0, 1, 2,  0, 2,  3,

            4, 5, 6,  4, 6,  7,

            8, 9, 10, 8, 10, 11,
        };

        geometry->VertexCount = 12;
        geometry->IndexCount = 18;

        geometry->VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
        geometry->VertexAttribs[0].Size = 4;
        geometry->VertexAttribs[0].Type = GL_FLOAT;
        geometry->VertexAttribs[0].Normalized = false;
        geometry->VertexAttribs[0].Stride = sizeof(cubeVertices.positions[0]);
        geometry->VertexAttribs[0].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, positions);

        geometry->VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
        geometry->VertexAttribs[1].Size = 4;
        geometry->VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
        geometry->VertexAttribs[1].Normalized = true;
        geometry->VertexAttribs[1].Stride = sizeof(cubeVertices.colors[0]);
        geometry->VertexAttribs[1].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, colors);

        GL(glGenBuffers(1, &geometry->VertexBuffer));
        GL(glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer));
        GL(glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

        GL(glGenBuffers(1, &geometry->IndexBuffer));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer));
        GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    static void ovrGeometry_CreateBox(ovrGeometry *geometry) {
        typedef struct {
            float positions[8][4];
            unsigned char colors[8][4];
        } ovrCubeVertices;

        static const ovrCubeVertices cubeVertices = {
            // positions
            {{-1.0f, -1.0f, -1.0f, 1.0f},
             {1.0f, -1.0f, -1.0f, 1.0f},
             {-1.0f, 1.0f, -1.0f, 1.0f},
             {1.0f, 1.0f, -1.0f, 1.0f},

             {-1.0f, -1.0f, 1.0f, 1.0f},
             {1.0f, -1.0f, 1.0f, 1.0f},
             {-1.0f, 1.0f, 1.0f, 1.0f},
             {1.0f, 1.0f, 1.0f, 1.0f}},
            // colors
            {
                {255, 0, 0, 255},
                {250, 255, 0, 255},
                {250, 0, 255, 255},
                {255, 255, 0, 255},
                {255, 0, 0, 255},
                {250, 255, 0, 255},
                {250, 0, 255, 255},
                {255, 255, 0, 255},
            },
        };

        //     6------7
        //    /|     /|
        //   2-+----3 |
        //   | |    | |
        //   | 4----+-5
        //   |/     |/
        //   0------1

        static const unsigned short cubeIndices[36] = {0, 1, 3, 0, 3, 2,

                                                       5, 4, 6, 5, 6, 7,

                                                       4, 0, 2, 4, 2, 6,

                                                       1, 5, 7, 1, 7, 3,

                                                       4, 5, 1, 4, 1, 0,

                                                       2, 3, 7, 2, 7, 6};

        geometry->VertexCount = 8;
        geometry->IndexCount = 36;

        geometry->VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
        geometry->VertexAttribs[0].Size = 4;
        geometry->VertexAttribs[0].Type = GL_FLOAT;
        geometry->VertexAttribs[0].Normalized = false;
        geometry->VertexAttribs[0].Stride = sizeof(cubeVertices.positions[0]);
        geometry->VertexAttribs[0].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, positions);

        geometry->VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
        geometry->VertexAttribs[1].Size = 4;
        geometry->VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
        geometry->VertexAttribs[1].Normalized = true;
        geometry->VertexAttribs[1].Stride = sizeof(cubeVertices.colors[0]);
        geometry->VertexAttribs[1].Pointer = (const GLvoid *)offsetof(ovrCubeVertices, colors);

        GL(glGenBuffers(1, &geometry->VertexBuffer));
        GL(glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer));
        GL(glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

        GL(glGenBuffers(1, &geometry->IndexBuffer));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer));
        GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    static void ovrGeometry_Destroy(ovrGeometry *geometry) {
        GL(glDeleteBuffers(1, &geometry->IndexBuffer));
        GL(glDeleteBuffers(1, &geometry->VertexBuffer));

        ovrGeometry_Clear(geometry);
    }

    static void ovrGeometry_CreateVAO(ovrGeometry *geometry) {
        GL(glGenVertexArrays(1, &geometry->VertexArrayObject));
        GL(glBindVertexArray(geometry->VertexArrayObject));

        GL(glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer));

        for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++) {
            if (geometry->VertexAttribs[i].Index != -1) {
                GL(glEnableVertexAttribArray(geometry->VertexAttribs[i].Index));
                GL(glVertexAttribPointer(
                    geometry->VertexAttribs[i].Index, geometry->VertexAttribs[i].Size, geometry->VertexAttribs[i].Type,
                    geometry->VertexAttribs[i].Normalized, geometry->VertexAttribs[i].Stride,
                    geometry->VertexAttribs[i].Pointer));
            }
        }

        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer));

        GL(glBindVertexArray(0));
    }

    static void ovrGeometry_DestroyVAO(ovrGeometry *geometry) {
        GL(glDeleteVertexArrays(1, &geometry->VertexArrayObject));
    }
}