#pragma once

namespace nar
{
#pragma pack(1)
    struct GlHeaderKTX_t {
        unsigned char identifier[12];
        unsigned int endianness;
        unsigned int glType;
        unsigned int glTypeSize;
        unsigned int glFormat;
        unsigned int glInternalFormat;
        unsigned int glBaseInternalFormat;
        unsigned int pixelWidth;
        unsigned int pixelHeight;
        unsigned int pixelDepth;
        unsigned int numberOfArrayElements;
        unsigned int numberOfFaces;
        unsigned int numberOfMipmapLevels;
        unsigned int bytesOfKeyValueData;
    };
#pragma pack()
}