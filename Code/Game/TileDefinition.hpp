#pragma once
#include <string>
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"

struct IntVec2;

class TileDefinition
{
public:
	TileDefinition() :
		m_name("default"),
		m_isSolid(true),
		m_mapImagePixelColor(Rgba8()),
		m_floorSpriteCoords(IntVec2()),
		m_wallSpriteCoords(IntVec2()),
		m_ceilingSpriteCoords(IntVec2())
	{};
	TileDefinition(std::string name, bool solid, Rgba8 pixelColor, IntVec2 floor, IntVec2 wall, IntVec2 ceiling);
	~TileDefinition() {};

	std::string GetName() const;
	bool GetIsSolid() const;
	Rgba8 GetMapPixelColor() const;
	IntVec2 GetFloorCoords() const;
	IntVec2 GetWallCoords() const;
	IntVec2 GetSecondaryCoords() const;
	IntVec2 GetCeilingCoords() const;

	std::string m_name;
	bool m_isSolid;
	Rgba8 m_mapImagePixelColor;
	IntVec2 m_floorSpriteCoords;
	IntVec2 m_wallSpriteCoords;
	IntVec2 m_ceilingSpriteCoords;
	IntVec2 m_secondaryRandom;
};