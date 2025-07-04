#include "Player.hpp"
#include "Game/Game.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

extern Renderer* g_theRenderer;
extern InputSystem* g_theInputSystem;
extern Window* g_theWindow;
extern BitmapFont* g_testFont;

Player::Player(Game* owner, Vec3 const& startPos)
{
	m_game = owner;
	m_playerCamPosition = startPos;

	m_camera = new Camera();
	m_camera->SetOrthographicView(Vec2(WORLD_MINS_X, WORLD_MINS_Y), Vec2(WORLD_MAXS_X, WORLD_MAXS_Y));
	m_camera->SetPerspectiveView(g_theWindow->GetConfig().m_aspectRatio, 60.f, .1f, 100.f);
	m_camera->SetCameraToRenderTransform(Mat44(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 0.f)));
}

Player::~Player()
{
	m_game = nullptr;
}

void Player::Update()
{
	//Setup ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	float textWidth = 40.f;
	float HUDHeight = SCREEN_SIZE_Y / 6.f;
	Vec2 HUDBottomLeft = Vec2(0.f, 0.f);
	Vec2 HUDBottomRight = Vec2(SCREEN_SIZE_X, 0.f);
	Vec2 HUDTopLeft = Vec2(0.f, HUDHeight);
	Vec2 HUDTopRight = Vec2(SCREEN_SIZE_X, HUDHeight);

	//Text ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	m_textVerts.clear();
	m_verts.clear();
	std::string health = Stringf("%.f",  m_map->GetActorByHandle(m_possessedActor)->m_health);
	std::string kills = Stringf("%i", m_kills);
	std::string deaths = Stringf("%i", m_deaths);
	std::string ammo;
	if (GetActor()->m_equippedWeapon && !GetActor()->m_equippedWeapon->m_weaponDef.m_isEnergyBased)
	{
		int ammoMag = GetActor()->m_equippedWeapon->m_roundsInMag;
		int ammoTotal = GetActor()->m_equippedWeapon->m_roundsInBag;
		ammo = Stringf("%i/%i", ammoMag, ammoTotal);
		g_testFont->AddVertsForTextInBox2D(m_textVerts, ammo, AABB2(SCREEN_SIZE_X * .15f, HUDHeight * .5f, (SCREEN_SIZE_X * .15f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), textWidth,
			Rgba8::WHITE, .6f, Vec2(.5f, .5f));
	}
	else if(GetActor()->m_equippedWeapon && GetActor()->m_equippedWeapon->m_cooldownTimer == nullptr)
	{
		float currentHeat = GetActor()->m_equippedWeapon->m_heatValue;
		float maxtHeat = GetActor()->m_equippedWeapon->m_weaponDef.m_maxHeat;
		AddVertsForAABB2D(m_verts, AABB2(SCREEN_SIZE_X * .15f, HUDHeight * .5f, (SCREEN_SIZE_X * .15f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), Rgba8::GRAY);
		float heatBar = RangeMap(currentHeat, 0.f, maxtHeat, 0.f, (textWidth * 3.f));
		AddVertsForAABB2D(m_verts, AABB2(SCREEN_SIZE_X * .15f, HUDHeight * .5f, (SCREEN_SIZE_X * .15f) + heatBar, (HUDHeight * .5f) + textWidth), Rgba8::BLUE);
	}
	else if(GetActor()->m_equippedWeapon)
	{
		float fractionOfTimeElapsed = (float)GetActor()->m_equippedWeapon->m_cooldownTimer->GetElapsedFraction();
		float heatBar = (textWidth * 3.f) * (1.f - fractionOfTimeElapsed);
		AddVertsForAABB2D(m_verts, AABB2(SCREEN_SIZE_X * .15f, HUDHeight * .5f, (SCREEN_SIZE_X * .15f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), Rgba8::GRAY);
		AddVertsForAABB2D(m_verts, AABB2(SCREEN_SIZE_X * .15f, HUDHeight * .5f, (SCREEN_SIZE_X * .15f) + heatBar, (HUDHeight * .5f) + textWidth), Rgba8::RED);
	}

	//Add Verts ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	g_testFont->AddVertsForTextInBox2D(m_textVerts, health, AABB2(SCREEN_SIZE_X * .27f, HUDHeight * .5f, (SCREEN_SIZE_X * .27f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), textWidth,
		Rgba8::WHITE, 1.f, Vec2(.5f, .5f));
	g_testFont->AddVertsForTextInBox2D(m_textVerts, kills, AABB2(SCREEN_SIZE_X * .03f, HUDHeight * .5f, (SCREEN_SIZE_X * .03f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), textWidth,
		Rgba8::WHITE, 1.f, Vec2(.5f, .5f));
	g_testFont->AddVertsForTextInBox2D(m_textVerts, deaths, AABB2(SCREEN_SIZE_X * .9f, HUDHeight * .5f, (SCREEN_SIZE_X * .9f) + (textWidth * 3.f), (HUDHeight * .5f) + textWidth), textWidth,
		Rgba8::WHITE, 1.f, Vec2(.5f, .5f));

	//Camera ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	if ((m_map->GetActorByHandle(m_possessedActor)->m_deathTimer)->IsStopped())
	{
		HandleInput();
		UpdateCamera();
	}
	else
	{
		AddVertsForAABB2D(m_verts, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(180, 180, 180, 80));
		UpdateCamera();
	}
}

void Player::HandleInput()
{
	switch (m_currentControlMode)
	{
	case ACTOR:
	{
		m_playerCamPosition = GetActor()->m_position;
		DebugAddScreenText(Stringf("FPS: %.2f",(1.f / (float)g_theGameClock->GetDeltaSeconds())), AABB2(SCREEN_SIZE_X - SCREEN_SIZE_X / 2.5, SCREEN_SIZE_Y - SCREEN_SIZE_Y / 45.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), SCREEN_SIZE_Y / 45.f, Vec2(0, 1), 0);
		HandleInputActorMode();
		break;
	}
	}
}

void Player::HandleInputActorMode()
{
	XboxController controller = g_theInputSystem->GetController(0);
	Vec3 forward;
	Vec3 left;
	Vec3 up;
	m_playerCamOrientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
	Weapon* weapon = GetActor()->m_equippedWeapon;

	if (m_playerIndex == -1)
	{
		//Weapons
		if (g_theInputSystem->WasKeyJustReleased(KEYCODE_MOUSEWHEEL_DOWN))
		{
			GetActor()->CycleWeaponDown();
		}
		if (g_theInputSystem->WasKeyJustReleased(KEYCODE_MOUSEWHEEL_UP))
		{
			GetActor()->CycleWeaponUp();
		}
		if (g_theInputSystem->IsKeyDown(KEYCODE_LMB))
		{
			GetActor()->Attack();
			if (GetActor()->m_equippedWeapon)
			{
				GetActor()->m_equippedWeapon->ResetAutoCooldown();
			}
			//Check for hold
			if (!g_theInputSystem->WasKeyJustPressed(KEYCODE_LMB))
			{
				if (GetActor()->m_equippedWeapon)
				{
					GetActor()->m_equippedWeapon->OnHoldFire();
				}
			}
		}
		else if (g_theInputSystem->WasKeyJustReleased(KEYCODE_LMB))
		{
			if (GetActor()->m_equippedWeapon)
			{
				GetActor()->m_equippedWeapon->OnReleaseFire();
			}
		}
		if (g_theInputSystem->IsKeyDown('R'))
		{
			weapon->Reload();
		}

		//Shift to run
		if (g_theInputSystem->WasKeyJustPressed(KEYCODE_SHIFT))
		{
			m_speed = GetActor()->m_definition.m_physicsElement.m_runSpeed;
		}
		if (g_theInputSystem->WasKeyJustReleased(KEYCODE_SHIFT))
		{
			m_speed = GetActor()->m_definition.m_physicsElement.m_walkSpeed;
		}

		//Movement
		if (g_theInputSystem->IsKeyDown('W'))
		{
			GetActor()->MoveInDirection(forward.GetFlattenedNormalXY(), m_speed);
		}
		if (g_theInputSystem->IsKeyDown('S'))
		{
			GetActor()->MoveInDirection(-forward.GetFlattenedNormalXY(), m_speed);
		}
		if (g_theInputSystem->IsKeyDown('A'))
		{
			GetActor()->MoveInDirection(left.GetFlattenedNormalXY(), m_speed);
		}
		if (g_theInputSystem->IsKeyDown('D'))
		{
			GetActor()->MoveInDirection(-left.GetFlattenedNormalXY(), m_speed);
		}

		//Orientation
		m_playerCamOrientation.m_yawDegrees -= m_lookSpeed * g_theInputSystem->GetCursorClientDelta().x;
		m_playerCamOrientation.m_pitchDegrees = GetClamped(m_playerCamOrientation.m_pitchDegrees + m_lookSpeed * g_theInputSystem->GetCursorClientDelta().y, -85.f, 85.f);
		GetActor()->m_orientation.m_yawDegrees -= m_lookSpeed * g_theInputSystem->GetCursorClientDelta().x;
		GetActor()->m_orientation.m_pitchDegrees  = GetClamped(m_playerCamOrientation.m_pitchDegrees + m_lookSpeed * g_theInputSystem->GetCursorClientDelta().y, -85.f, 85.f);
	}

	else if (m_playerIndex == 0)
	{
		//Weapons
		if (controller.WasButtonJustPressed(XBOX_BUTTON_RIGHT_BUMPER))
		{
			GetActor()->CycleWeaponDown();
		}
		if (controller.WasButtonJustPressed(XBOX_BUTTON_LEFT_BUMPER))
		{
			GetActor()->CycleWeaponUp();
		}
		if (controller.GetRightTrigger() > .5f)
		{
			GetActor()->Attack();
			if (GetActor()->m_equippedWeapon)
			{
				GetActor()->m_equippedWeapon->ResetAutoCooldown();
			}
			//Check for hold
			if (!g_theInputSystem->WasKeyJustPressed(KEYCODE_LMB))
			{
				if (GetActor()->m_equippedWeapon)
				{
					GetActor()->m_equippedWeapon->OnHoldFire();
				}
			}
		}
		else if (controller.GetRightTrigger() < .5f)
		{
			if (GetActor()->m_equippedWeapon)
			{
				GetActor()->m_equippedWeapon->OnReleaseFire();
			}
		}
		if (controller.WasButtonJustPressed(XBOX_BUTTON_X))
		{
			weapon->Reload();
		}

		//Movement
		if (controller.GetLeftStick().GetPosition().y > 0.6f)
		{
			GetActor()->MoveInDirection(forward.GetFlattenedNormalXY(), m_speed);
		}
		if (controller.GetLeftStick().GetPosition().y < -0.6f)
		{
			GetActor()->MoveInDirection(-forward.GetFlattenedNormalXY(), m_speed);
		}
		if (controller.GetLeftStick().GetPosition().x < -0.6f)
		{
			GetActor()->MoveInDirection(left.GetFlattenedNormalXY(), m_speed);
		}
		if (controller.GetLeftStick().GetPosition().x > 0.6f)
		{
			GetActor()->MoveInDirection(-left.GetFlattenedNormalXY(), m_speed);
		}

		//Shift to run
		if ((controller.GetButton(XBOX_BUTTON_B).m_wasPressedLastFrame && !controller.GetButton(XBOX_BUTTON_B).m_isPressed))
		{
			if (m_speed > GetActor()->m_definition.m_physicsElement.m_walkSpeed)
			{
				m_speed = GetActor()->m_definition.m_physicsElement.m_walkSpeed;
			}
			else
			{
				m_speed = GetActor()->m_definition.m_physicsElement.m_runSpeed;
			}
		}

		if (controller.GetRightStick().GetPosition().y > 0.3f)
		{
			float adjust = controller.GetRightStick().GetMagnitude() * m_lookSpeed * 1500.f * (float)g_theGameClock->GetDeltaSeconds();
			m_playerCamOrientation.m_pitchDegrees = GetClamped(m_playerCamOrientation.m_pitchDegrees - adjust, -85.f, 85.f);
		}
		if (controller.GetRightStick().GetPosition().y < -0.3f)
		{
			float adjust = controller.GetRightStick().GetMagnitude() * m_lookSpeed * 1500.f * (float)g_theGameClock->GetDeltaSeconds();
			m_playerCamOrientation.m_pitchDegrees = GetClamped(m_playerCamOrientation.m_pitchDegrees + adjust, -85.f, 85.f);
		}
		if (controller.GetRightStick().GetPosition().x < -0.3f)
		{
			m_playerCamOrientation.m_yawDegrees += controller.GetRightStick().GetMagnitude() * m_lookSpeed * 2000.f * (float)g_theGameClock->GetDeltaSeconds();
		}
		if (controller.GetRightStick().GetPosition().x > 0.3f)
		{
			m_playerCamOrientation.m_yawDegrees -= controller.GetRightStick().GetMagnitude() * m_lookSpeed * 2000.f * (float)g_theGameClock->GetDeltaSeconds();
		}
		GetActor()->m_orientation = EulerAngles(m_playerCamOrientation.m_yawDegrees, 0.f, 0.f);
	}

	m_playerCamPosition = GetActor()->m_position;
	m_camera->SetPosition(GetActor()->m_position + Vec3(0.f,0.f, GetActor()->m_definition.m_cameraElement.m_eyeHeight));
	m_camera->SetOrientation(m_playerCamOrientation);
	m_camera->SetPerspectiveView(g_theWindow->GetConfig().m_aspectRatio, GetActor()->m_definition.m_cameraElement.m_cameraFOVDegrees, .1f, 100.f);
	m_game->m_worldCamera = *m_camera;
}

void Player::UpdateCamera()
{
	if (m_currentControlMode == ControlMode::ACTOR)
	{
		float fraction = (float)(m_map->GetActorByHandle(m_possessedActor)->m_deathTimer)->GetElapsedFraction();
		float height = (1 - fraction) * GetActor()->m_definition.m_cameraElement.m_eyeHeight;
		m_camera->SetPosition(GetActor()->m_position + Vec3(0.f, 0.f, height));
		m_camera->SetOrientation(m_playerCamOrientation);
		float aspect = g_theWindow->GetConfig().m_aspectRatio;
		if (m_map->m_game->m_nextPlayerIndex > 0)
		{
			aspect *= 2.f;
		}
		m_camera->SetPerspectiveView(aspect, GetActor()->m_definition.m_cameraElement.m_cameraFOVDegrees, .1f, 100.f);
	}
	else if (m_currentControlMode == ControlMode::CAMERA)
	{
		m_camera->SetPosition(m_playerCamPosition);
		m_camera->SetOrientation(m_playerCamOrientation);
		m_camera->SetPerspectiveView(g_theWindow->GetConfig().m_aspectRatio, GetActor()->m_definition.m_cameraElement.m_cameraFOVDegrees, .1f, 100.f);
	}
}

void Player::Render() const
{
	g_theRenderer->BindTexture(&g_testFont->GetTexture());
	g_theRenderer->DrawVertexArray(m_textVerts);

	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->DrawVertexArray(m_verts);
}

Mat44 Player::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44::MakeTranslation3D(m_playerCamPosition);
	modelToWorld.Append(m_playerCamOrientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorld;
}

void Player::CycleControlMode()
{
	if (m_currentControlMode == (ControlMode)((int)(ControlMode::ACTOR)))
	{
		m_currentControlMode = ControlMode::CAMERA;
	}
	else
	{
		m_currentControlMode = (ControlMode)((int)(m_currentControlMode)+1);
	}
}

void Player::ResetKillCount()
{
	m_kills = 0;
}

void Player::ResetDeathCount()
{
	m_deaths = 0;
}

void Player::AddKillCount()
{
	m_kills++;
}

void Player::AddDeathCount()
{
	m_deaths++;
}

int Player::GetKillCount() const
{
	return m_kills;
}

int Player::GetDeathCount() const
{
	return m_deaths;
}

float Player::GetActorHealth() const
{
	if (GetActor() != nullptr)
	{
		return GetActor()->m_health;
	}
	else
	{
		return 0.f;
	}
}