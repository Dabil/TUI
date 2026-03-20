#pragma once

#include "Rendering/Styles/Style.h"

enum class StyleMergeMode
{
    Replace,
    MergePreserveDestination,
    MergePreserveSource
};

class StyleMerge
{
public:
    static Style merge(
        const Style& destination,
        const Style& source,
        StyleMergeMode mode);
};