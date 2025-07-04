#include "Tile.hpp"

Tile::Tile(int colX, int rowY, TileDefinition def)
{
	m_tileDef = def;
	m_bounds = AABB3(Vec3((colX) * m_unitSize, (rowY) * m_unitSize, 0.f), Vec3((colX + 1) * m_unitSize, (rowY + 1) * m_unitSize, 1.f));
}

Tile::~Tile()
{
}
