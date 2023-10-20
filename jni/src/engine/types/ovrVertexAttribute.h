#pragma once
#include "VertexAttributeLocation.h"

namespace nar
{
    struct ovrVertexAttribute {
        VertexAttributeLocation location;
        const char *name;
    };
}