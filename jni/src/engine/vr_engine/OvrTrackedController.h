#pragma once
#include <openxr/openxr_oculus_helpers.h>
#include "engine/types/ovrTrackedController.h"

namespace nar
{
    static void ovrTrackedController_Clear(ovrTrackedController *controller) {
        controller->Active = false;
        controller->Pose = XrPosef_Identity();
    }
}