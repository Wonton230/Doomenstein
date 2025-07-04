#pragma once
#include <string>
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Vec3.hpp"

class Image;
class Shader;
class Texture;
struct IntVec2;

struct SpawnInfo
{
	std::string m_actor = "default spawn";
	Vec3 m_position = Vec3();
	EulerAngles m_orientation = EulerAngles();
	Vec3 m_velocity = Vec3();
};

class MapDefinition
{
public:
	MapDefinition() :
		m_name("default"),
		m_spriteSheetCellCount(IntVec2())
	{};
	MapDefinition(std::string const& name, Shader* shader, Texture* sheetTexture, IntVec2 cellCount);
	~MapDefinition();

	Texture* GetTexture() const;
	Shader* GetShader() const;
	std::string GetName() const;
	IntVec2 GetSpriteCellCount() const;

	std::string m_name;
	std::string m_skyBoxFilePath;
	Shader* m_shader = nullptr;
	Texture* m_spriteSheetTexture = nullptr;
	IntVec2 m_spriteSheetCellCount;
	float m_ceilingHeight;
	std::vector<SpawnInfo*>	m_spawnInfo;
};