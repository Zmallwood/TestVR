#include "OvrApp_GetInstance.h"
#include "engine/Global.h"

namespace nar
{
    XrInstance ovrApp_GetInstance() {
        return appState.Instance;
    }
}