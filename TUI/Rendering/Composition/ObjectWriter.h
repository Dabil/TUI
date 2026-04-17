#pragma once

#include <optional>

#include "Rendering/Composition/WritePolicy.h"
#include "Rendering/Objects/TextObject.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"

namespace Composition
{
    class ObjectWriter
    {
    public:
        ObjectWriter(ScreenBuffer& target, int offsetX = 0, int offsetY = 0);

        void setOffset(int offsetX, int offsetY);

        int offsetX() const;
        int offsetY() const;

        void writeObject(const TextObject& source, const WritePolicy& policy);
        void writeObject(
            const TextObject& source,
            const WritePolicy& policy,
            const std::optional<Style>& sourceStyleOverride);

    private:
        void writeObjectInternal(
            const TextObject& source,
            const WritePolicy& policy,
            const std::optional<Style>& sourceStyleOverride);

    private:
        ScreenBuffer& m_target;
        int m_offsetX = 0;
        int m_offsetY = 0;
    };
}