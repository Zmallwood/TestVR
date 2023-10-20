#pragma once
#include "ovrBackgroundType.h"
#include "ovrGLProgram.h"
#include "ovrGeometry.h"
#include "ovrTrackedController.h"
#include "ovrSwapChain.h"

namespace nar
{
    struct ovrScene {
        bool CreatedScene;
        bool CreatedVAOs;
        ovrGLProgram Program;
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