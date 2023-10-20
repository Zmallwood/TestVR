#include "engine/Game.h"
#include <android_native_app_glue.h>

/*
This example demonstrates the use of the supported compositor layer types: projection,
cylinder, quad, cubemap, and equirect.

Compositor layers provide advantages such as increased resolution and better sampling
qualities over rendering to the eye buffers. Layers are composited strictly in order,
with no depth interaction, so it is the applications responsibility to not place the
layers such that they would be occluded by any world geometry, which can result in
eye straining depth paradoxes.

Layer rendering may break down at extreme grazing angles, and as such, they should be
faded out or converted to normal surfaces which render to the eye buffers if the viewer
can get too close.

All texture levels must have a 0 alpha border to avoid edge smear.
*/

void android_main(struct android_app *app) {
    nar::Game::Get()->Run(app);
}
