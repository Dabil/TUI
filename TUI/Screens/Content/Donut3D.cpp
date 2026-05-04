#include "Screens/Content/Donut3D.h"

#include <cmath>
#include <algorithm>
#include <vector>

#include "Rendering/Surface.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Styles/StyleBuilder.h"

namespace
{
    constexpr float Pi = 3.1415926535f;
    constexpr float ThetaSpacing = 0.07f;
    constexpr float PhiSpacing = 0.02f;

    constexpr float R1 = 1.0f;
    constexpr float R2 = 2.0f;
    constexpr float K2 = 5.0f;

    constexpr float RotationASpeed = 1.15f;
    constexpr float RotationBSpeed = 0.45f;

    constexpr float SizeScale = 0.4f;

    constexpr char32_t LuminanceRamp[] = U".,-~:;=!*#$@";
    constexpr int LuminanceRampCount = static_cast<int>((sizeof(LuminanceRamp) / sizeof(LuminanceRamp[0])) - 1);

    constexpr Color::Basic CyclePalette[] =
    {
        Color::Basic::Blue,
        Color::Basic::Cyan,
        Color::Basic::Magenta,
        Color::Basic::Red,
        Color::Basic::Yellow,
        Color::Basic::Green
    };

    constexpr Color::Basic CyclePaletteBright[] =
    {
        Color::Basic::BrightBlue,
        Color::Basic::BrightCyan,
        Color::Basic::BrightMagenta,
        Color::Basic::BrightRed,
        Color::Basic::BrightYellow,
        Color::Basic::BrightGreen
    };

    constexpr int CyclePaletteCount = static_cast<int>(sizeof(CyclePalette) / sizeof(CyclePalette[0]));
}

Donut3D::Donut3D()
{
}

void Donut3D::onEnter()
{
    m_elapsedSeconds = 0.0;
    m_rotationA = 0.0f;
    m_rotationB = 0.0f;
}

void Donut3D::onExit()
{
}

void Donut3D::update(double deltaTime)
{
    m_elapsedSeconds += deltaTime;

    m_rotationA += static_cast<float>(deltaTime) * RotationASpeed;
    m_rotationB += static_cast<float>(deltaTime) * RotationBSpeed;

    if (m_rotationA > (Pi * 2.0f))
    {
        m_rotationA = std::fmod(m_rotationA, Pi * 2.0f);
    }

    if (m_rotationB > (Pi * 2.0f))
    {
        m_rotationB = std::fmod(m_rotationB, Pi * 2.0f);
    }
}

void Donut3D::draw(Surface& surface, const Rect& viewPort)
{
//    ScreenBuffer& buffer = surface.buffer();

    renderDonut(surface, viewPort);
}

void Donut3D::ensureBuffers(int width, int height)
{
    const std::size_t cellCount = static_cast<std::size_t>(std::max(0, width) * std::max(0, height));

    if (m_depthBuffer.size() != cellCount)
    {
        m_depthBuffer.assign(cellCount, 0.0f);
    }
    else
    {
        std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 0.0f);
    }

    if (m_glyphBuffer.size() != cellCount)
    {
        m_glyphBuffer.assign(cellCount, U' ');
    }
    else
    {
        std::fill(m_glyphBuffer.begin(), m_glyphBuffer.end(), U' ');
    }
}

void Donut3D::renderDonut(Surface& surface, const Rect& viewPort)
{
    if (viewPort.size.width <= 0 || viewPort.size.height <= 0)
    {
        return;
    }

    ScreenBuffer& buffer = surface.buffer();

    ensureBuffers(viewPort.size.width, viewPort.size.height);

    buffer.fillRect(
        viewPort,
        U' ',
        style::Bg(Color::FromBasic(Color::Basic::Black)));

    std::vector<float> luminanceBuffer(static_cast<std::size_t>(viewPort.size.width * viewPort.size.height), 0.0f);
    std::vector<float> shadowBuffer(static_cast<std::size_t>(viewPort.size.width * viewPort.size.height), 0.0f);

    const float cosA = std::cos(m_rotationA);
    const float sinA = std::sin(m_rotationA);
    const float cosB = std::cos(m_rotationB);
    const float sinB = std::sin(m_rotationB);

    const float k1 = SizeScale * static_cast<float>(viewPort.size.width) * K2 * 3.0f / (8.0f * (R1 + R2));

    float minDepth = 1000000.0f;
    float maxDepth = -1000000.0f;

    const float floorY = (static_cast<float>(viewPort.size.height) * 0.9f);
    const float shadowShiftX = 6.0f;

    for (float theta = 0.0f; theta < (Pi * 2.0f); theta += ThetaSpacing)
    {
        const float costheta = std::cos(theta);
        const float sintheta = std::sin(theta);

        for (float phi = 0.0f; phi < (Pi * 2.0f); phi += PhiSpacing)
        {
            const float cosphi = std::cos(phi);
            const float sinphi = std::sin(phi);

            const float circlex = R2 + (R1 * costheta);
            const float circley = R1 * sintheta;

            const float x = circlex * ((cosB * cosphi) + (sinA * sinB * sinphi))
                - (circley * cosA * sinB);
            const float y = circlex * ((sinB * cosphi) - (sinA * cosB * sinphi))
                + (circley * cosA * cosB);
            const float z = K2 + (cosA * circlex * sinphi) + (circley * sinA);

            if (z <= 0.0f)
            {
                continue;
            }

            const float ooz = 1.0f / z;

            const int xp = static_cast<int>((static_cast<float>(viewPort.size.width) * 0.5f) + (k1 * ooz * x));
            const int yp = static_cast<int>((static_cast<float>(viewPort.size.height) * 0.5f) - (k1 * ooz * y * 0.5f));

            if (xp < 0 || xp >= viewPort.size.width || yp < 0 || yp >= viewPort.size.height)
            {
                continue;
            }

            const float luminance =
                (cosphi * costheta * sinB)
                - (cosA * costheta * sinphi)
                - (sinA * sintheta)
                + (cosB * ((cosA * sintheta) - (costheta * sinA * sinphi)));

            if (luminance <= 0.0f)
            {
                continue;
            }

            const int bufferIndex = index(xp, yp, viewPort.size.width);
            const std::size_t cellIndex = static_cast<std::size_t>(bufferIndex);

            if (ooz <= m_depthBuffer[cellIndex])
            {
                continue;
            }

            m_depthBuffer[cellIndex] = ooz;
            luminanceBuffer[cellIndex] = luminance;

            int luminanceIndex = static_cast<int>(luminance * 8.0f);
            luminanceIndex = std::clamp(luminanceIndex, 0, LuminanceRampCount - 1);
            m_glyphBuffer[cellIndex] = LuminanceRamp[luminanceIndex];

            if (ooz < minDepth)
            {
                minDepth = ooz;
            }

            if (ooz > maxDepth)
            {
                maxDepth = ooz;
            }

            const float closeness = std::clamp((ooz - 0.10f) / 0.18f, 0.0f, 1.0f);
            const int shadowX = static_cast<int>(std::round(static_cast<float>(xp) + shadowShiftX + (closeness * 2.0f)));
            const int shadowY = static_cast<int>(std::round(floorY + ((static_cast<float>(yp) - (static_cast<float>(viewPort.size.height) * 0.5f)) * 0.18f)));

            if (shadowX >= 0 && shadowX < viewPort.size.width && shadowY >= 0 && shadowY < viewPort.size.height)
            {
                const std::size_t shadowIndex = static_cast<std::size_t>(index(shadowX, shadowY, viewPort.size.width));
                shadowBuffer[shadowIndex] = std::max(shadowBuffer[shadowIndex], 0.35f + (luminance * 0.45f));
            }

            const int softShadowX1 = shadowX - 1;
            const int softShadowX2 = shadowX + 1;

            if (softShadowX1 >= 0 && softShadowX1 < viewPort.size.width && shadowY >= 0 && shadowY < viewPort.size.height)
            {
                const std::size_t shadowIndex = static_cast<std::size_t>(index(softShadowX1, shadowY, viewPort.size.width));
                shadowBuffer[shadowIndex] = std::max(shadowBuffer[shadowIndex], 0.18f + (luminance * 0.18f));
            }

            if (softShadowX2 >= 0 && softShadowX2 < viewPort.size.width && shadowY >= 0 && shadowY < viewPort.size.height)
            {
                const std::size_t shadowIndex = static_cast<std::size_t>(index(softShadowX2, shadowY, viewPort.size.width));
                shadowBuffer[shadowIndex] = std::max(shadowBuffer[shadowIndex], 0.18f + (luminance * 0.18f));
            }
        }
    }

    if (maxDepth <= minDepth)
    {
        maxDepth = minDepth + 0.0001f;
    }

    for (int y = 0; y < viewPort.size.height; ++y)
    {
        for (int x = 0; x < viewPort.size.width; ++x)
        {
            const std::size_t cellIndex = static_cast<std::size_t>(index(x, y, viewPort.size.width));
            const float shadowIntensity = shadowBuffer[cellIndex];

            if (shadowIntensity <= 0.0f)
            {
                continue;
            }

            char32_t shadowGlyph = U'.';
            Style shadowStyle = style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
                + style::Bg(Color::FromBasic(Color::Basic::Black));

            if (shadowIntensity > 0.65f)
            {
                shadowGlyph = U':';
                shadowStyle = style::Fg(Color::FromBasic(Color::Basic::Blue))
                    + style::Bg(Color::FromBasic(Color::Basic::Black));
            }
            else if (shadowIntensity > 0.40f)
            {
                shadowGlyph = U'.';
                shadowStyle = style::Fg(Color::FromBasic(Color::Basic::BrightBlack))
                    + style::Bg(Color::FromBasic(Color::Basic::Black));
            }

            buffer.writeCodePoint(viewPort.position.x + x, viewPort.position.y + y, shadowGlyph, shadowStyle);
        }
    }

    for (int y = 0; y < viewPort.size.height; ++y)
    {
        for (int x = 0; x < viewPort.size.width; ++x)
        {
            const int bufferIndex = index(x, y, viewPort.size.width);
            const std::size_t cellIndex = static_cast<std::size_t>(bufferIndex);

            const char32_t glyph = m_glyphBuffer[cellIndex];
            const float depth = m_depthBuffer[cellIndex];
            const float luminance = luminanceBuffer[cellIndex];

            if (glyph == U' ')
            {
                continue;
            }

            const float normalizedDepth = (depth - minDepth) / (maxDepth - minDepth);
            buffer.writeCodePoint(viewPort.position.x + x, viewPort.position.y + y, glyph, buildShadedStyle(normalizedDepth, luminance));
        }
    }
}

int Donut3D::index(int x, int y, int width) const
{
    return (y * width) + x;
}

Style Donut3D::buildShadedStyle(float normalizedDepth, float luminance) const
{
    normalizedDepth = std::clamp(normalizedDepth, 0.0f, 1.0f);
    luminance = std::clamp(luminance, 0.0f, 1.0f);

    const int hueOffset = static_cast<int>(m_elapsedSeconds * 0.05) % CyclePaletteCount;

    int depthBand = 0;
    if (normalizedDepth < 0.20f)
    {
        depthBand = 0;
    }
    else if (normalizedDepth < 0.45f)
    {
        depthBand = 1;
    }
    else if (normalizedDepth < 0.75f)
    {
        depthBand = 2;
    }
    else
    {
        depthBand = 3;
    }

    const int paletteIndex = (hueOffset + depthBand) % CyclePaletteCount;

    const Color baseColor = Color::FromBasic(CyclePalette[paletteIndex]);
    const Color brightColor = Color::FromBasic(CyclePaletteBright[paletteIndex]);
    const Color hottestColor = (depthBand >= 3)
        ? Color::FromBasic(Color::Basic::BrightWhite)
        : brightColor;

    if (luminance < 0.25f)
    {
        return style::Fg(baseColor)
             + style::Bg(Color::FromBasic(Color::Basic::Black));
    }

    if (luminance < 0.55f)
    {
        return style::Fg(brightColor)
             + style::Bg(Color::FromBasic(Color::Basic::Black));
    }

    return style::Bold
         + style::Fg(hottestColor)
         + style::Bg(Color::FromBasic(Color::Basic::Black));
}