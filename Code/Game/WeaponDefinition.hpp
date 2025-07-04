#pragma once
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "ActorDefinition.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"

class Shader;
class Texture;

#include <string>

struct HUDElement
{
	Shader*		m_shader = nullptr;
	Texture*	m_baseTexture = nullptr;
	Texture*	m_reticleTexture = nullptr;
	Vec2		m_reticleSize = Vec2(1.f,1.f);
	Vec2		m_spriteSize = Vec2(1.f,1.f);
	Vec2		m_spritePivot = Vec2(.5f, .5f);
	std::vector<SpriteAnimDefinition*> m_animationDefs;
};

class WeaponDefinition
{
public:
	std::string m_name = "uninitialized WeaponDef";

	//Time
	float m_refireTime = 0.f;

	//Ray
	int m_rayCount = 0;
	float m_rayConeDegrees = 0.f;
	float m_rayRange = 0.f;
	FloatRange m_rayDamage = FloatRange(0.f, 0.f);
	float m_rayImpulse = 0.f;
	bool m_rayRender = false;
	Rgba8 m_rayColor = Rgba8::WHITE;

	//Projectile
	int m_projectileCount = 0;
	float m_projectileConeDegrees = 0.f;
	float m_projectileSpeed = 0.f;
	std::string m_projectileActor = "";
	Rgba8 m_projectileColor = Rgba8::WHITE;
	bool m_projectileFallSpeed = 0.f;

	//Melee
	int m_meleeCount = 0;
	float m_meleeRange = 0.f;
	float m_meleeArcDegrees = 0.f;
	FloatRange m_meleeDamage = FloatRange(0.f, 0.f);
	float m_meleeImpulse = 0.f;

	//Ammo Management
	int m_magSize = -1; //-1 means infinite
	float m_reloadTime = -1.f; //-number means no reload
	float m_heatPerShot = -1.f;
	float m_maxHeat = -1.f;
	float m_cooldownTime = -1.f; //-number means no cooldown
	bool m_isEnergyBased = false;

	//Special
	float m_chargeShotTime = -1.f;
	bool  m_isExplosive = false;
	float m_blastRadius = -1.f;
	float m_blastDamage = 0.f;
	float m_blastImpulse = 0.f;

	//Visuals+Sounds
	HUDElement m_HUDElement;
	std::vector<SoundDefinition> m_sounds;
	bool m_loopSoundOnHold = false;
};