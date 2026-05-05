#pragma once

#include <vector>
#include <random>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/ScreenBuffer.h"
#include "Rendering/Diagnostics/StartupDiagnosticsContext.h"

class Surface;

class DigitalRainEffect
{
public:
	DigitalRainEffect(TerminalHostKind hostKind = TerminalHostKind::Unknown);
    ~DigitalRainEffect() = default;

	void onEnter();
	void onExit();
	void update(double deltaTime);
	void draw(Surface& surface, const Rect& viewport);
    const std::u32string& getGlyphPool() const;

private:

    enum class DropComposition
    {
        BrightWhiteWhite,   // 25%
        WhiteOnly,          // 25%
        GreenOnly           // 50%
    };

    struct DeadGlyph
    {
        int x = 0;
        int y = 0;
        char32_t glyph = U' ';
    };

    struct Stream
    {
        bool active = false;
        float headY = 0.0f;
        float speed = 0.0f;
        int length = 0;
        double respawnDelay = 0.0;
        double mutationTimer = 0.0;
        DropComposition composition = DropComposition::GreenOnly;
        std::vector<char32_t> glyphs;
    };

private:
    static std::u32string buildConsoleGlyphPool();
    static std::u32string buildTerminalGlyphPool();
    static std::u32string buildGlyphPoolForHost(TerminalHostKind hostKind);

    void ensureLayout(const Rect& viewport);
    void rebuildStreams();
    void resetStream(Stream& stream, bool staggerStart);
    void configureActiveStream(Stream& stream);
    void updateStreams(double deltaTime);
    void spawnDeadGlyphs(double deltaTime);
    void drawRain(ScreenBuffer& buffer) const;
    int countActiveStreams() const;
    char32_t randomGlyph();
    Style styleForTrailIndex(int trailIndex, int streamLength, DropComposition composition) const;

private:
    int m_rainLeft = 1;
    int m_rainTop = 1;
    int m_rainWidth = 0;
    int m_rainHeight = 0;

    double m_elapsedSeconds = 0.0;

    TerminalHostKind m_hostKind = TerminalHostKind::Unknown;
    std::u32string m_glyphPool;
    std::vector<Stream> m_streams;

    Style m_backgroundStyle;
    Style m_headStyle;
    Style m_secondStyle;
    Style m_thirdStyle;
    Style m_fourthStyle;
    Style m_tailStyle;

    std::vector<DeadGlyph> m_deadGlyphs;
    double m_deadGlyphSpawnAccumulator = 0.0;

    std::mt19937 m_rng;
};