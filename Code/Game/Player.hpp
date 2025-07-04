#pragma once
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Controller.hpp"

class  Game;
struct EulerAngles;
struct Vec3;
struct Mat44;
class  Camera;
struct Mat44;

enum ControlMode
{
	CAMERA,
	ACTOR
};

class Player: public Controller
{
public:
	Player(Game* owner, Vec3 const& startPos);
	~Player();

	void Update() override;
	void HandleInput();
	void HandleInputCameraMode();
	void HandleInputActorMode();
	void UpdateCamera();

	void Render() const;
	Mat44 GetModelToWorldTransform() const;

	void CycleControlMode();

	void ResetKillCount();
	void ResetDeathCount();
	void AddKillCount();
	void AddDeathCount();
	int GetKillCount() const;
	int GetDeathCount() const;
	float GetActorHealth() const;


	Game*	m_game = nullptr;
	Camera* m_camera = nullptr;
	int		m_playerIndex = 0;
	int		m_playerNumber = 0;
	AABB2	m_viewportBox;

	Vec3		m_playerCamPosition = Vec3(0.f, 0.f, 0.f);
	EulerAngles	m_playerCamOrientation = EulerAngles(0.f, 0.f, 0.f);

	Vec3		m_velocity = Vec3(0.f, 0.f, 0.f);
	EulerAngles	m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);

	Rgba8 m_color = Rgba8::WHITE;
	float m_speed = 1.f;
	float m_sprintSpeedMult = 15.f;
	float m_lookSpeed = .075f;

	int   m_deaths = 0;
	int	  m_kills = 0;
	std::vector<Vertex_PCU> m_textVerts;
	std::vector<Vertex_PCU> m_verts;

	ControlMode m_currentControlMode = ControlMode::ACTOR;
};