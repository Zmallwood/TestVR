#pragma once

#define MAX_PROGRAM_UNIFORMS 8
#define MAX_PROGRAM_TEXTURES 8
#define MAX_VERTEX_ATTRIB_POINTERS 3

namespace nar
{
    static const int CPU_LEVEL = 2;
    static const int GPU_LEVEL = 3;
    static const int NUM_MULTI_SAMPLES = 4;

    enum { ovrMaxLayerCount = 16 };
    enum { ovrMaxNumEyes = 2 };
}