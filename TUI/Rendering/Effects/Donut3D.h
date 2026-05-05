#pragma once

#include <vector>

#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"
#include "Rendering/Effects/IEffects.h"

class Surface;

class Donut3D final : public IEffect
{
public:
	explicit Donut3D();
	~Donut3D() override = default;

	void onEnter() override;
	void onExit() override;
	void update(double deltaTime) override;
	void draw(Surface& surface, const Rect& viewPort) override;

private:
	void ensureBuffers(int width, int height);
	void renderDonut(Surface& surface, const Rect& viewPort);
	int index(int x, int y, int width) const;
	Style buildShadedStyle(float normalizedDepth, float luminance) const;

private:
	double m_elapsedSeconds = 0.0;
	float m_rotationA = 0.0f;
	float m_rotationB = 0.0f;

	std::vector<float> m_depthBuffer;
	std::vector<char32_t> m_glyphBuffer;
};