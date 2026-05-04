#pragma once

#include <vector>

#include "Screens/Screen.h"
#include "Core/Rect.h"
#include "Rendering/Styles/Style.h"

class Surface;

class Donut3D
{
public:
	explicit Donut3D();

	void onEnter();
	void onExit();
	void update(double deltaTime);
	void draw(Surface& surface, const Rect& viewPort);

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