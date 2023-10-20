#pragma once
#include "types/ovrApp.h"
#include "Constants.h"

namespace nar
{
    inline static ovrApp appState;
    inline static XrView *projections = (XrView *)(malloc(ovrMaxNumEyes * sizeof(XrView)));
}