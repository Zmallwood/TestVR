#pragma once
#include "Constants.h"
#include "types/ovrApp.h"
#include <string.h>
#include <stdlib.h>

namespace nar
{
    inline static ovrApp appState;
    inline static XrView *projections = (XrView *)(malloc(ovrMaxNumEyes * sizeof(XrView)));
    inline static XrAction vibrateLeftFeedback;
    inline static XrAction vibrateRightFeedback;
}