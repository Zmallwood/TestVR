#pragma once
#include "engine/types/MatrixTypes.h"
#include "engine/types/UniformTypes.h"

namespace nar
{
    struct ovrUniform {
        MatrixTypes index;
        UniformTypes type;
        const char *name;
    };
}