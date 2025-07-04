#include "TileDefinition.hpp"

TileDefinition::TileDefinition(std::string name, bool solid, Rgba8 pixelColor, IntVec2 floor, IntVec2 wall, IntVec2 ceiling)
{
	m_name = name;
	m_isSolid = solid;
	m_mapImagePixelColor = pixelColor;
	m_floorSpriteCoords = floor;
	m_wallSpriteCoords = wall;
	m_ceilingSpriteCoords = ceiling;
}

std::string TileDefinition::GetName() const
{
	return m_name;
}

bool TileDefinition::GetIsSolid() const
{
	return m_isSolid;
}

Rgba8 TileDefinition::GetMapPixelColor() const
{
	return m_mapImagePixelColor;
}

IntVec2 TileDefinition::GetFloorCoords() const
{
	return m_floorSpriteCoords;
}

IntVec2 TileDefinition::GetWallCoords() const
{
	return m_wallSpriteCoords;
}

IntVec2 TileDefinition::GetSecondaryCoords() const
{
	return m_secondaryRandom;
}

IntVec2 TileDefinition::GetCeilingCoords() const
{
	return m_ceilingSpriteCoords;
}