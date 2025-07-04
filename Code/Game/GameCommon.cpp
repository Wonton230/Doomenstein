#include "GameCommon.hpp"

extern Renderer* g_theRenderer;
extern AudioSystem* g_theAudioSystem;

void DrawDebugRing(Vec2 const& center, float const radius, float const thickness, Rgba8 const color)
{
	constexpr int NUM_SIDES = 12;
	constexpr int NUM_TRIS = NUM_SIDES * 2;
	constexpr int NUM_VERTS = NUM_TRIS * 3;
	constexpr float DEG_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	float innerRadius = radius - (thickness / 2);
	float outerRadius = radius + (thickness / 2);
	Vertex_PCU verts[NUM_VERTS];

	for (int i = 0; i < NUM_SIDES; i++)
	{
		float startDegrees = DEG_PER_SIDE * static_cast<float>(i);
		float endDegrees = DEG_PER_SIDE * (static_cast<float>((i + 1) % NUM_SIDES));

		//Compute inner and outer corners of trapezoid on each side
		Vec3 innerStart = Vec3(center.x + innerRadius * CosDegrees(startDegrees), center.y + innerRadius * SinDegrees(startDegrees), 0.f);
		Vec3 outerStart = Vec3(center.x + outerRadius * CosDegrees(startDegrees), center.y + outerRadius * SinDegrees(startDegrees), 0.f);
		Vec3 innerEnd = Vec3(center.x + innerRadius * CosDegrees(endDegrees), center.y + innerRadius * SinDegrees(endDegrees), 0.f);
		Vec3 outerEnd = Vec3(center.x + outerRadius * CosDegrees(endDegrees), center.y + outerRadius * SinDegrees(endDegrees), 0.f);

		int indexA = 6 * i;
		int indexB = 6 * i + 1;
		int indexC = 6 * i + 2;
		int indexD = 6 * i + 3;
		int indexE = 6 * i + 4;
		int indexF = 6 * i + 5;
		//Tri 1
		verts[indexA].m_position = innerStart;
		verts[indexB].m_position = outerStart;
		verts[indexC].m_position = innerEnd;
		//Tri 2
		verts[indexD].m_position = outerStart;
		verts[indexE].m_position = outerEnd;
		verts[indexF].m_position = innerEnd;
	}

	for (int i = 0; i < NUM_VERTS; i++)
	{
		verts[i].m_color = color;
	}
	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DrawDebugLine(Vec2 const& pos, Vec2 const& vector, float const thickness, Rgba8 const color)
{
	constexpr int NUM_VERTS = 6;
	Vec2 extendlength = vector.GetNormalized() * thickness/2;
	Vec2 extendWidth = extendlength.GetRotated90Degrees();
	Vec2 vertA = pos + vector + extendlength + extendWidth;
	Vec2 vertB = pos + vector + extendlength - extendWidth;
	Vec2 vertC = pos - extendlength + extendWidth;
	Vec2 vertD = pos - extendlength - extendWidth;

	Vertex_PCU verts[NUM_VERTS];
	verts[0].m_position = Vec3(vertA.x,vertA.y,0.f);
	verts[1].m_position = Vec3(vertB.x,vertB.y,0.f);
	verts[2].m_position = Vec3(vertC.x,vertC.y,0.f);
	verts[3].m_position = Vec3(vertC.x,vertC.y,0.f);
	verts[4].m_position = Vec3(vertD.x,vertD.y,0.f);
	verts[5].m_position = Vec3(vertB.x,vertB.y,0.f);

	for (int i = 0; i < NUM_VERTS; i++)
	{
		verts[i].m_color = color;
	}
	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}