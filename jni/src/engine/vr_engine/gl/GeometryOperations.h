#pragma once
#include "engine/types/VertexAttributeLocation.h"
#include "engine/types/ovrVertexAttribute.h"

namespace nar
{
    struct ovrGeometry;

    static ovrVertexAttribute ProgramVertexAttributes[] = {
        {VERTEX_ATTRIBUTE_LOCATION_POSITION, "vertexPosition"},
        {VERTEX_ATTRIBUTE_LOCATION_COLOR, "vertexColor"},
        {VERTEX_ATTRIBUTE_LOCATION_UV, "vertexUv"},
    };

    extern void ovrGeometry_Clear(ovrGeometry *geometry);

    extern void ovrGeometry_CreateGroundPlane(ovrGeometry *geometry);

    extern void ovrGeometry_CreateStagePlane(ovrGeometry *geometry, float minx, float minz, float maxx, float maxz);

    extern void ovrGeometry_CreateBox(ovrGeometry *geometry);

    extern void ovrGeometry_Destroy(ovrGeometry *geometry);

    extern void ovrGeometry_CreateVAO(ovrGeometry *geometry);

    extern void ovrGeometry_DestroyVAO(ovrGeometry *geometry);
}