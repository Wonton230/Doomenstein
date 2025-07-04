#pragma once
#include <vector>
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/ActorDefinition.hpp"
#include "Game/AnimationGroupDefinition.hpp"
#include "Game/Map.hpp"
#include "Game/Weapon.hpp"

class  Game;
struct EulerAngles;
struct Vec3;
struct Mat44;
class  Camera;
struct Mat44;
class AI;
class Controller;
class Timer;
class Clock;

class Actor
{
public:
	Actor(Map* owningMap, ActorHandle handle, ActorDefinition def, Vec3 startPos, EulerAngles startOrientation, Vec3 startVelocity);
	~Actor();

	void Update();
	void UpdateVerts();
	void UpdateProjectileVerts();
	void UpdateActorVerts();
	void UpdatePhysics();

	void Render() const;

	void TakeDamage(Actor* sourceActor, float damage);
	bool CheckIfHealthIsZero();
	void StartDeath();

	void OnCollide(Actor* other = nullptr);
	void OnPossessed(Controller* controller);
	void OnUnpossessed();

	void AddForce(Vec3 const& acceleration);
	void AddImpulse(Vec3 const& impulse);
	void MoveInDirection(Vec3 const& direction, float speed);
	void TurnInDirection(Vec3 const& point, float maxTurn);
	Vec3 GetVisionStartPoint() const;

	void HandlePickupLogic(Actor* const& other);

	void Attack();
	void CycleWeaponUp();
	void CycleWeaponDown();

	void PlaySoundEffect(std::string const& soundName, float volume);

	//Rendering Helpers
	Direction	GetDirectionOfActorAnimationToCamera(Vec3 const& referencePoint, AnimationGroupDefinition const& animGroup);
	void		BindTextureToRenderer() const;
	Mat44		GetModelToWorldBillboardTransform() const;
	Mat44		GetModelToWorldTransform() const;
	void		PlayAnimationByName(std::string const& animName);
	void		AddCurrentAnimationFrame();
	void		DrawVertexPCUTBNs(std::vector<Vertex_PCUTBN> const& verts) const;

	Controller*					m_controller;
	AI*							m_AIController;
	bool						m_isAI = true;

	ActorHandle  				m_handle;
	ActorDefinition				m_definition;
	Map*						m_spawnMap;

	Vec3						m_position = Vec3();
	EulerAngles					m_orientation = EulerAngles();
	Vec3						m_velocity = Vec3();
	Vec3						m_acceleration = Vec3();

	Timer*						m_deathTimer;
	float						m_health = 1.0f;
	bool						m_isDead = false;
	bool						m_expired = false;
	bool						m_didCollide = false;

	Clock*						m_animationClock;
	Timer*						m_animationTimer;
	Timer*						m_damageTakenHUDTimer;
	AnimationGroupDefinition*	m_currentAnimation = nullptr;
	Rgba8						m_color = Rgba8::RED;
	VertexBuffer*				m_vertexPCUTBNBuffer;
	std::vector<Vertex_PCUTBN>	m_vertTBNs;

	ActorHandle					m_owningActor;
	std::vector<Weapon*>		m_weaponInventory;
	int							m_weaponIndex;
	Weapon*						m_equippedWeapon;
	Timer*						m_refireTimer;

	float						m_height = 1.f;
	float						m_radius = .5f;
	bool						m_isStatic = true;

	SoundPlaybackID				m_permanentID1;
	SoundPlaybackID				m_permanentID2;
};