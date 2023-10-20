#pragma once
#include "engine/Macros.h"
#include "engine/types/ktxImageInfo_t.h"
#include <android/asset_manager.h>
#include "engine/types/GlHeaderKTX_t.h"

#if !defined(GL_TEXTURE_CUBE_MAP_ARRAY)
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

namespace nar
{
    static bool LoadImageDataFromKTXFile(AAssetManager *amgr, const char *assetName, ktxImageInfo_t *outInfo) {
        AAsset *asset = AAssetManager_open(amgr, assetName, AASSET_MODE_BUFFER);
        if (asset == NULL) {
            ALOGE("Failed to open %s", assetName);
            return false;
        }
        const void *buffer = AAsset_getBuffer(asset);
        if (buffer == NULL) {
            ALOGE("Failed to read %s", assetName);
            return false;
        }
        size_t bufferSize = AAsset_getLength(asset);

        if (bufferSize < sizeof(GlHeaderKTX_t)) {
            ALOGE("%s: Invalid KTX file", assetName);
            return false;
        }

        const unsigned char fileIdentifier[12] = {(unsigned char)'\xAB', 'K',  'T',  'X',    ' ', '1', '1',
                                                  (unsigned char)'\xBB', '\r', '\n', '\x1A', '\n'};

        const GlHeaderKTX_t *header = (GlHeaderKTX_t *)buffer;
        if (memcmp(header->identifier, fileIdentifier, sizeof(fileIdentifier)) != 0) {
            ALOGE("%s: Invalid KTX file", assetName);
            return false;
        }
        // only support little endian
        if (header->endianness != 0x04030201) {
            ALOGE("%s: KTX file has wrong endianness", assetName);
            return false;
        }
        // only support unsigned byte
        if (header->glType != GL_UNSIGNED_BYTE) {
            ALOGE("%s: KTX file has unsupported glType %d", assetName, header->glType);
            return false;
        }
        // skip the key value data
        const size_t startTex = sizeof(GlHeaderKTX_t) + header->bytesOfKeyValueData;
        if ((startTex < sizeof(GlHeaderKTX_t)) || (startTex >= bufferSize)) {
            ALOGE("%s: Invalid KTX header sizes", assetName);
            return false;
        }

        outInfo->width = header->pixelWidth;
        outInfo->height = header->pixelHeight;
        outInfo->depth = header->pixelDepth;
        outInfo->internalFormat = header->glInternalFormat;
        outInfo->numberOfArrayElements = (header->numberOfArrayElements >= 1) ? header->numberOfArrayElements : 1;
        outInfo->numberOfFaces = (header->numberOfFaces >= 1) ? header->numberOfFaces : 1;
        outInfo->numberOfMipmapLevels = header->numberOfMipmapLevels;
        outInfo->mipSizeStored = true;
        outInfo->data = buffer;
        outInfo->dataOffset = startTex;
        outInfo->dataSize = bufferSize;

        outInfo->target =
            ((outInfo->depth > 1)
                 ? GL_TEXTURE_3D
                 : ((outInfo->numberOfFaces > 1)
                        ? ((outInfo->numberOfArrayElements > 1) ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP)
                        : ((outInfo->numberOfArrayElements > 1) ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D)));

        return true;
    }

    static bool LoadTextureFromKTXImageMemory(const ktxImageInfo_t *info, const int textureId) {
        if (info == NULL) {
            return false;
        }

        if (info->data != NULL) {
            GL(glBindTexture(info->target, textureId));

            const int numDataLevels = info->numberOfMipmapLevels;
            const unsigned char *levelData = (const unsigned char *)(info->data) + info->dataOffset;
            const unsigned char *endOfBuffer = levelData + (info->dataSize - info->dataOffset);

            for (int i = 0; i < numDataLevels; i++) {
                const int w = (info->width >> i) >= 1 ? (info->width >> i) : 1;
                const int h = (info->height >> i) >= 1 ? (info->height >> i) : 1;
                const int d = (info->depth >> i) >= 1 ? (info->depth >> i) : 1;

                size_t mipSize = 0;
                bool compressed = false;
                GLenum glFormat = GL_RGBA;
                GLenum glDataType = GL_UNSIGNED_BYTE;
                switch (info->internalFormat) {
                case GL_R8: {
                    mipSize = w * h * d * 1 * sizeof(unsigned char);
                    glFormat = GL_RED;
                    glDataType = GL_UNSIGNED_BYTE;
                    break;
                }
                case GL_RG8: {
                    mipSize = w * h * d * 2 * sizeof(unsigned char);
                    glFormat = GL_RG;
                    glDataType = GL_UNSIGNED_BYTE;
                    break;
                }
                case GL_RGB8: {
                    mipSize = w * h * d * 3 * sizeof(unsigned char);
                    glFormat = GL_RGB;
                    glDataType = GL_UNSIGNED_BYTE;
                    break;
                }
                case GL_RGBA8: {
                    mipSize = w * h * d * 4 * sizeof(unsigned char);
                    glFormat = GL_RGBA;
                    glDataType = GL_UNSIGNED_BYTE;
                    break;
                }
                }
                if (mipSize == 0) {
                    ALOGE("Unsupported image format %d", info->internalFormat);
                    GL(glBindTexture(info->target, 0));
                    return false;
                }

                if (info->numberOfArrayElements > 1) {
                    mipSize = mipSize * info->numberOfArrayElements * info->numberOfFaces;
                }

                // ALOGV( "mipSize%d = %zu (%d)", i, mipSize, info->numberOfMipmapLevels );

                if (info->mipSizeStored) {
                    if (levelData + 4 > endOfBuffer) {
                        ALOGE("Image data exceeds buffer size");
                        GL(glBindTexture(info->target, 0));
                        return false;
                    }
                    const size_t storedMipSize = (size_t) * (const unsigned int *)levelData;
                    // ALOGV( "storedMipSize = %zu", storedMipSize );
                    mipSize = storedMipSize;
                    levelData += 4;
                }

                if (info->depth <= 1 && info->numberOfArrayElements <= 1) {
                    for (int face = 0; face < info->numberOfFaces; face++) {
                        if (mipSize <= 0 || mipSize > (size_t)(endOfBuffer - levelData)) {
                            ALOGE("Mip %d data exceeds buffer size (%zu > %zu)", i, mipSize, (endOfBuffer - levelData));
                            GL(glBindTexture(info->target, 0));
                            return false;
                        }

                        const GLenum uploadTarget =
                            (info->target == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
                        if (compressed) {
                            GL(glCompressedTexSubImage2D(
                                uploadTarget + face, i, 0, 0, w, h, info->internalFormat, (GLsizei)mipSize, levelData));
                        }
                        else {
                            GL(glTexSubImage2D(uploadTarget + face, i, 0, 0, w, h, glFormat, glDataType, levelData));
                        }

                        levelData += mipSize;

                        if (info->mipSizeStored) {
                            levelData += 3 - ((mipSize + 3) % 4);
                            if (levelData > endOfBuffer) {
                                ALOGE("Image data exceeds buffer size");
                                GL(glBindTexture(info->target, 0));
                                return false;
                            }
                        }
                    }
                }
                else {
                    if (mipSize <= 0 || mipSize > (size_t)(endOfBuffer - levelData)) {
                        ALOGE("Mip %d data exceeds buffer size (%zu > %zu)", i, mipSize, (endOfBuffer - levelData));
                        GL(glBindTexture(info->target, 0));
                        return false;
                    }

                    if (compressed) {
                        GL(glCompressedTexSubImage3D(
                            info->target, i, 0, 0, 0, w, h, d * info->numberOfArrayElements, info->internalFormat,
                            (GLsizei)mipSize, levelData));
                    }
                    else {
                        GL(glTexSubImage3D(
                            info->target, i, 0, 0, 0, w, h, d * info->numberOfArrayElements, glFormat, glDataType,
                            levelData));
                    }

                    levelData += mipSize;

                    if (info->mipSizeStored) {
                        levelData += 3 - ((mipSize + 3) % 4);
                        if (levelData > endOfBuffer) {
                            ALOGE("Image data exceeds buffer size");
                            GL(glBindTexture(info->target, 0));
                            return false;
                        }
                    }
                }
            }

            GL(glTexParameteri(
                info->target, GL_TEXTURE_MIN_FILTER,
                (info->numberOfMipmapLevels > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR));
            GL(glTexParameteri(info->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

            GL(glBindTexture(info->target, 0));
        }

        return true;
    }
}