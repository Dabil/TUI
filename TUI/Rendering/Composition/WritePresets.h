#pragma once

#include "Rendering/Composition/DepthPolicy.h"
#include "Rendering/Composition/WritePolicy.h"

namespace Composition::WritePresets
{
    WritePolicy visibleOnly(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy authoredOnly(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy solidObject(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy transparentOverlay(DepthPolicy depthPolicy = DepthPolicy::Ignore);

    WritePolicy visibleObject(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy transparentGlyphOverlay(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy glyphsOnly(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy styleMask(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy styleBlock(DepthPolicy depthPolicy = DepthPolicy::Ignore);
}