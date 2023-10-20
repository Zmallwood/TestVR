#include "TrackedControllerOperations.h"
#include <openxr/openxr_oculus_helpers.h>

namespace nar
{
    void ovrTrackedController_Clear(ovrTrackedController *controller) {
        controller->Active = false;
        controller->Pose = XrPosef_Identity();
    }
}