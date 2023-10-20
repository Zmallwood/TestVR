#pragma once

namespace nar
{
    struct ktxImageInfo_t {
        int width;
        int height;
        int depth;
        GLenum internalFormat;
        GLenum target;
        int numberOfArrayElements;
        int numberOfFaces;
        int numberOfMipmapLevels;
        bool mipSizeStored;
        const void *data; // caller responsible for freeing memory
        int dataOffset;
        int dataSize;
    };
}