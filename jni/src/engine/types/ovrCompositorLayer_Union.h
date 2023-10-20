#pragma once

namespace nar
{
    union ovrCompositorLayer_Union {
        XrCompositionLayerProjection Projection;
        XrCompositionLayerQuad Quad;
        XrCompositionLayerCylinderKHR Cylinder;
        XrCompositionLayerCubeKHR Cube;
        XrCompositionLayerEquirect2KHR Equirect2;
    };
}