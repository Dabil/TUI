#pragma once

#include <string>

#include "Rendering/Styles/Style.h"

class SgrEmitter
{
public:
    SgrEmitter() = default;

    void reset();
    const Style& currentStyle() const;

    void appendTransitionTo(std::string& out, const Style& targetStyle);
    void appendReset(std::string& out);

private:
    void appendFullStyle(std::string& out, const Style& style) const;
    void appendDeltaStyle(std::string& out, const Style& targetStyle) const;

private:
    Style m_currentStyle{};
};