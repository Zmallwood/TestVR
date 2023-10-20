#pragma once
#include "ovrBackgroundType.h"
#include "ovrProgram.h"
#include "ovrGeometry.h"
#include "ovrTrackedController.h"
#include "ovrSwapChain.h"

namespace nar
{
    struct ovrScene {
        bool CreatedScene;
        bool CreatedVAOs;
        ovrProgram Program;
        ovrGeometry GroundPlane;
        ovrGeometry Box;
        ovrTrackedController TrackedController[4]; // left aim, left grip, right aim, right grip

        ovrSwapChain CubeMapSwapChain;
        XrSwapchainImageOpenGLESKHR *CubeMapSwapChainImage;
        ovrSwapChain EquirectSwapChain;
        XrSwapchainImageOpenGLESKHR *EquirectSwapChainImage;
        ovrSwapChain CylinderSwapChain;
        XrSwapchainImageOpenGLESKHR *CylinderSwapChainImage;
        ovrSwapChain QuadSwapChain;
        XrSwapchainImageOpenGLESKHR *QuadSwapChainImage;

        ovrBackgroundType BackGroundType;
    };
}