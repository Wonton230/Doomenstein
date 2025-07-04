#pragma once
#include <string>
#include <vector>
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Rgba8.hpp"

class Image;
class Shader;
class Texture;
class AnimationGroupDefinition;

enum Faction
{
	GOOD,
	EVIL,
	NEUTRAL,
	COUNT
};

struct SoundDefinition
{
	std::string m_soundName;
	std::string m_soundFilePath;
};

struct ActorCollision
{
	float m_physicsRadius = 0.f;
	float m_physicsHeight = 0.f;
	bool  m_collidesWithWorld = false;
	bool  m_collidesWithActors = false;
	bool  m_dieOnCollision = false;
	FloatRange m_damageOnCollide = FloatRange(0.f, 0.f);
	float m_impulseOnCollide = 0.f;
};

struct ActorPhysics
{
	bool m_simulated = false;
	bool m_flying = false;
	float m_walkSpeed = 0.f;
	float m_runSpeed = 0.f;
	float m_drag = 0.f;
	float turnSpeed = 0.f;
};

struct ActorCameraSettings
{
	float m_eyeHeight = 0.f;
	float m_cameraFOVDegrees = 60.f;
};

struct ActorAISettings
{
	bool m_aiEnabled = false;
	float m_sightRadius = 0.f;
	float m_sightAngle = 0.f;
};

struct ActorWeapon
{
	std::vector<std::string> m_weaponNames;
};

struct ActorVisuals
{
	Vec2			m_spriteWorldSize = Vec2(1.f, 1.f);
	Vec2			m_pivot = Vec2(0.5f, 0.5f);
	BillBoardType	m_billBoardType = BillBoardType::NONE;
	bool			m_renderLit = false;
	bool			m_renderRounded = false;
	Shader*			m_shader = nullptr;
	SpriteSheet*	m_sheet = nullptr;
	std::vector<AnimationGroupDefinition*> m_groupDefinitions;
};

class ActorDefinition
{
public:
	float	GetHeight() const;
	float	GetRadius() const;
	Faction	GetFaction() const;
	bool	IsSimulated() const;
	bool	CanBeAI() const;
	void	AddGroupDefinition(SpriteSheet* const& sheet, Texture* const& texture, XmlElement* const& element);

	std::string m_name = "uninitialized ActorDef";
	bool m_visible = false;
	bool m_dieOnSpawn = false;
	float m_health = 1.f;
	float m_corpseLifetime = 0.f;
	Rgba8 m_color = Rgba8::WHITE;
	Faction m_faction = Faction::NEUTRAL;
	bool m_canBePossessed = false;
	bool m_isPickup = false;
	int  m_spawnCost = 0;

	ActorCollision m_collisionElement;
	ActorPhysics m_physicsElement;
	ActorCameraSettings m_cameraElement;
	ActorAISettings m_AIElement;
	ActorVisuals m_VisualElement;
	ActorWeapon m_weaponElement;
	std::vector<SoundDefinition> m_sounds;
};