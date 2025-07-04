#include "MapDefinition.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Shader.hpp"

MapDefinition::MapDefinition(std::string const& name, Shader* shader, Texture* sheetTexture, IntVec2 cellCount)
{
	m_name = name;
	m_shader = shader;
	m_spriteSheetTexture = sheetTexture;
	m_spriteSheetCellCount = cellCount;
}

MapDefinition::~MapDefinition()
{
	m_spawnInfo.clear();
}

Texture* MapDefinition::GetTexture() const
{
	return m_spriteSheetTexture;
}

Shader* MapDefinition::GetShader() const
{
	return m_shader;
}

std::string MapDefinition::GetName() const
{
	return m_name;
}

IntVec2 MapDefinition::GetSpriteCellCount() const
{
	return m_spriteSheetCellCount;
}
