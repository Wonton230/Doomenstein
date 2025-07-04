#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/MAth/AABB3.hpp"
#include "Game/TileDefinition.hpp"

enum TileType
{
	NUM_TILE_TYPES
};

class Tile
{
public:
	Tile() :
		m_tileDef(TileDefinition()),
		m_bounds(AABB3())
	{};
	Tile(int colX, int rowY, TileDefinition def);
	~Tile();

	TileDefinition m_tileDef;
	AABB3 m_bounds;
	float m_unitSize = 1.f;
};