#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Rendering/Styles/Style.h"
#include "UI/Panels/ScrollablePanel.h"

class Surface;

class TextView : public ScrollablePanel
{
public:
    TextView() = default;
    explicit TextView(std::string title);

    void setText(std::string_view text);
    void setLines(const std::vector<std::string>& lines);
    void setLines(std::vector<std::string>&& lines);
    void appendLine(std::string line);
    void clear();

    const std::vector<std::string>& lines() const;
    bool empty() const;
    std::size_t lineCount() const;

    void setTextStyle(const Style& style);
    const Style& textStyle() const;

    void draw(Surface& surface) override;

private:
    void updateContentSizeFromLines();
    Rect resolveTextRect() const;
    static std::vector<std::string> splitLines(std::string_view text);

private:
    std::vector<std::string> m_lines;
    Style m_textStyle;
};