#pragma once
#include "ovrFramebuffer.h"

namespace nar
{
    struct ovrRenderer {
        ovrFramebuffer FrameBuffer[ovrMaxNumEyes];
    };
}