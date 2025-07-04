#pragma once
#include "Game/WeaponDefinition.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Timer.hpp"

class Actor;
struct EulerAngles;
struct Vec3;
struct Vertex_PCU;
class Map;
class Clock;

class Weapon
{
public:
	Weapon(WeaponDefinition def);
	~Weapon();

	void Update();
	void Render() const;

	void Fire(Actor* const& user);
	void FireRay(Actor* const& user);
	void FireProjectile(Actor* const& user);
	void FireMelee(Actor* const& user);
	void SecondaryFire(Actor* const& user);

	void Reload();
	void StartCooldown();
	void FinishCooldown();
	void ResetAutoCooldown();

	Vec3 GetRandomDirectionInCone(float coneDegrees, EulerAngles const& orientation);
	void PlayAnimationByName(std::string const& animName, Clock* const& clock);
	void AddCurrentAnimationFrame();

	//Events
	void OnHoldFire();
	void OnReleaseFire();
	void OnHoldSecondaryFire();
	void OnReleaseSecondaryFire();

	void PlayRapidSoundEffect(std::string const& soundName, float volume);
	void PlayOneSoundEffect(std::string const& soundName, float volume);
	void PlayLoopingSoundEffect(std::string const& soundName, float volume);

	WeaponDefinition		m_weaponDef;
	Weapon*					m_secondaryWeapon;
	std::vector<Vertex_PCU> m_reticleVerts;
	std::vector<Vertex_PCU> m_HUDVerts;
	std::vector<Vertex_PCU> m_weaponVerts;
	SpriteAnimDefinition*	m_currentAnimation;
	Clock*					m_animationClock;
	
	Timer*					m_reloadTimer;
	Timer*					m_cooldownTimer;
	Timer*					m_autoCooldownTimer;
	int						m_roundsInMag;
	int						m_roundsInBag;
	float					m_heatValue = 0.f;
	bool					m_primaryHeld = false;

	SoundPlaybackID			m_permanentID1;
	SoundPlaybackID			m_permanentID2;
};