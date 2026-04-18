#pragma once

#include "Rendering/Composition/DepthPolicy.h"
#include "Rendering/Composition/WritePolicy.h"

namespace Composition::WritePresets
{
    WritePolicy solidObject(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy visibleObject(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy glyphsOnly(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy styleMask(DepthPolicy depthPolicy = DepthPolicy::Ignore);
    WritePolicy styleBlock(DepthPolicy depthPolicy = DepthPolicy::Ignore);
}