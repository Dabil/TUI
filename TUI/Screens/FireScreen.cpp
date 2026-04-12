#include "Screens/FireScreen.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "Core/Rect.h"
#include "Rendering/Surface.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Styles/StyleBuilder.h"
#include "Rendering/Styles/Themes.h"
#include "Rendering/Objects/AsciiBanner.h"

namespace
{
    constexpr int MaxHeat = 65;

    int randomInt(int minValue, int maxValue)
    {
        if (maxValue <= minValue)
        {
            return minValue;
        }

        return minValue + (std::rand() % ((maxValue - minValue) + 1));
    }

    int clampHeat(int value)
    {
        return std::clamp(value, 0, MaxHeat);
    }
}

FireScreen::FireScreen(Assets::AssetLibrary& assetLibrary)
    : m_assetLibrary(assetLibrary)
{
}

void FireScreen::onEnter()
{
    m_elapsedSeconds = 0.0;
    m_sourceFlickerTimer = 0.0;
    m_fireBuffer.clear();
    m_nextFireBuffer.clear();
    m_tuiFireLogoObject.clear();
    m_tuiFireLogoObjectShadow.clear();

    m_fontResult = m_assetLibrary.loadBannerFont(m_tuiFireLogoKey);

    if (!m_fontResult.success || !m_fontResult.hasFont())
    {
        return;
    }

    AsciiBanner::RenderOptions options;
    options.composeMode = AsciiBanner::ComposeMode::Kern;
    options.transparentSpaces = true;

    m_tuiFireLogoObject = AsciiBanner::generateTextObject(
        *m_fontResult.asset.font,
        "TUI",
        m_bannerStyle1,
        options);

    m_tuiFireLogoObjectShadow = AsciiBanner::generateTextObject(
        *m_fontResult.asset.font,
        "TUI",
        m_bannerStyle2,
        options);
}

void FireScreen::onExit()
{
    m_tuiFireLogoObject.clear();
    m_tuiFireLogoObjectShadow.clear();
}

void FireScreen::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;
    m_sourceFlickerTimer += deltaTime;

    if (m_fireWidth <= 0 || m_fireHeight <= 0)
    {
        return;
    }

    updateFire(deltaTime);
}

void FireScreen::draw(Surface& surface)
{
    ScreenBuffer& buffer = surface.buffer();

    const int screenWidth = buffer.getWidth();
    const int screenHeight = buffer.getHeight();

    if (screenWidth < 30 || screenHeight < 12)
    {
        buffer.writeString(1, 1, "FireScreen needs a larger console window.", Themes::Warning);
        return;
    }

    m_fireLeft = 2;
    m_fireTop = 3;

    const int visibleWidth = std::max(0, screenWidth - 4);
    const int visibleHeight = std::max(0, screenHeight - 2);

    ensureSimulationSize(visibleWidth, visibleHeight);

    buffer.fillRect(
        Rect{ Point{ m_fireLeft, m_fireTop }, Size{ m_fireWidth, m_fireHeight } },
        U' ',
        style::Bg(Color::FromBasic(Color::Basic::Black)));

    for (int y = 0; y < m_fireHeight; ++y)
    {
        for (int x = 0; x < m_fireWidth; ++x)
        {
            const int linearIndex = index(x, y);
            const int intensity = clampHeat(m_fireBuffer[static_cast<std::size_t>(linearIndex)]);
            const char32_t glyph = glyphForIntensity(intensity);
            const Style style = styleForIntensity(intensity);

            buffer.writeCodePoint(m_fireLeft + x, m_fireTop + y, glyph, style);
        }
    }

    const Style frameStyle = style::Fg(Color::FromBasic(Color::Basic::Red))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    const Style borderText = style::Fg(Color::FromBasic(Color::Basic::Yellow))
        + style::Bg(Color::FromBasic(Color::Basic::Black));

    buffer.drawFrame(
        Rect{ Point{ 0, 0 }, Size{ screenWidth, screenHeight } },
        frameStyle,
        U'╔', U'╗', U'╚', U'╝', U'═', U'║');

    buffer.drawFrame(
        Rect{ Point{ 2, 1 }, Size{ screenWidth - 4, screenHeight - 2 } },
        borderText,
        U'┌', U'┐', U'└', U'┘', U'─', U'│');

    buffer.writeString(4, 0, "[                 ]", frameStyle);
    buffer.writeString(5, 0, " Fire Simulation ", borderText);
    buffer.writeString(4, screenHeight - 1, "[                 ]", frameStyle);
    buffer.writeString(5, screenHeight - 1, " Buffered Flames ", borderText);

    if (!m_fontResult.success || !m_fontResult.hasFont())
    {
        const int errorX = (screenWidth - strlen("Failed to load banner font.")) / 2;
        const int errorY = screenHeight / 4;

        buffer.writeString(errorX, errorY, "Failed to load banner font.", Themes::Warning);
        return;
    }


    const int bannerWidth = m_tuiFireLogoObject.getWidth();
    const int bannerHeight = m_tuiFireLogoObject.getHeight();

    const int bannerX = (screenWidth - bannerWidth) / 2;
    const int bannerY = (screenHeight - bannerHeight) / 4;

    m_tuiFireLogoObjectShadow.draw(buffer, bannerX, bannerY);
    m_tuiFireLogoObject.draw(buffer, bannerX - 1, bannerY - 1);
}

void FireScreen::ensureSimulationSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        m_fireWidth = width;
        m_fireHeight = height;
        m_fireBuffer.clear();
        m_nextFireBuffer.clear();
        return;
    }

    const int paddedSize = (width * height) + width + 1;

    if (width == m_fireWidth &&
        height == m_fireHeight &&
        static_cast<int>(m_fireBuffer.size()) == paddedSize)
    {
        return;
    }

    m_fireWidth = width;
    m_fireHeight = height;

    m_fireBuffer.assign(static_cast<std::size_t>(paddedSize), 0);
    m_nextFireBuffer.clear();

    seedFireSource();
}

void FireScreen::seedFireSource()
{
    if (m_fireWidth <= 0 || m_fireHeight <= 0)
    {
        return;
    }

    std::fill(m_fireBuffer.begin(), m_fireBuffer.end(), 0);

    const int sparksPerTick = std::max(1, m_fireWidth / 9);
    const int bottomRowOffset = m_fireWidth * (m_fireHeight - 1);

    for (int i = 0; i < sparksPerTick; ++i)
    {
        const int x = randomInt(0, m_fireWidth - 1);
        const int linearIndex = bottomRowOffset + x;

        if (linearIndex >= 0 && linearIndex < static_cast<int>(m_fireBuffer.size()))
        {
            m_fireBuffer[static_cast<std::size_t>(linearIndex)] = MaxHeat;
        }
    }
}

void FireScreen::updateFire(double deltaTime)
{
    (void)deltaTime;

    if (m_fireWidth <= 0 || m_fireHeight <= 0)
    {
        return;
    }

    const int visibleSize = m_fireWidth * m_fireHeight;
    const int sparksPerTick = std::max(1, m_fireWidth / 9);
    const int bottomRowOffset = m_fireWidth * (m_fireHeight - 1);

    if (m_sourceFlickerTimer >= 0.03)
    {
        m_sourceFlickerTimer = 0.0;

        for (int i = 0; i < sparksPerTick; ++i)
        {
            const int x = randomInt(0, m_fireWidth - 1);
            const int linearIndex = bottomRowOffset + x;

            if (linearIndex >= 0 && linearIndex < static_cast<int>(m_fireBuffer.size()))
            {
                m_fireBuffer[static_cast<std::size_t>(linearIndex)] = MaxHeat;
            }
        }
    }

    for (int i = 0; i < visibleSize; ++i)
    {
        const int iRight = i + 1;
        const int iBelow = i + m_fireWidth;
        const int iBelowRight = i + m_fireWidth + 1;

        const int current = m_fireBuffer[static_cast<std::size_t>(i)];
        const int right = (iRight < static_cast<int>(m_fireBuffer.size()))
            ? m_fireBuffer[static_cast<std::size_t>(iRight)]
            : 0;
        const int below = (iBelow < static_cast<int>(m_fireBuffer.size()))
            ? m_fireBuffer[static_cast<std::size_t>(iBelow)]
            : 0;
        const int belowRight = (iBelowRight < static_cast<int>(m_fireBuffer.size()))
            ? m_fireBuffer[static_cast<std::size_t>(iBelowRight)]
            : 0;

        int averaged = (current + right + below + belowRight) / 4;
        averaged = clampHeat(averaged);

        m_fireBuffer[static_cast<std::size_t>(i)] = averaged;
    }

    coolTopRows();
}

void FireScreen::coolTopRows()
{
    const int rowsToCool = std::min(2, m_fireHeight);

    for (int y = 0; y < rowsToCool; ++y)
    {
        for (int x = 0; x < m_fireWidth; ++x)
        {
            const int linearIndex = index(x, y);
            int& heat = m_fireBuffer[static_cast<std::size_t>(linearIndex)];

            if (heat > 0)
            {
                heat = std::max(0, heat - 1);
            }
        }
    }
}

int FireScreen::index(int x, int y) const
{
    return (y * m_fireWidth) + x;
}

char32_t FireScreen::glyphForIntensity(int intensity) const
{
    static constexpr char32_t Glyphs[] =
    {
        U' ',
        U'.',
        U':',
        U'^',
        U'*',
        U'x',
        U's',
        U'S',
        U'#',
        U'$'
    };

    intensity = clampHeat(intensity);

    if (intensity <= 0)
    {
        return Glyphs[0];
    }

    if (intensity > 9)
    {
        return Glyphs[9];
    }

    return Glyphs[intensity];
}

Style FireScreen::styleForIntensity(int intensity) const
{
    const Color black = Color::FromBasic(Color::Basic::Black);

    intensity = clampHeat(intensity);

    if (intensity <= 0)
    {
        return style::Bg(black);
    }

    if (intensity <= 4)
    {
        return style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
             + style::Bg(black);
    }

    if (intensity <= 9)
    {
        return style::Bold
             + style::Fg(Color::FromBasic(Color::Basic::Red))
             + style::Bg(black);
    }

    if (intensity <= 15)
    {
        return style::Bold
             + style::Fg(Color::FromBasic(Color::Basic::Yellow))
             + style::Bg(black);
    }

    if (intensity <= 50)
    {
        return style::Bold
             + style::Fg(Color::FromBasic(Color::Basic::White))
             + style::Bg(black);
    }

    return style::Bold
         + style::Fg(Color::FromBasic(Color::Basic::White))
         + style::Bg(black);
}