#include "Weapon.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Player.hpp"
#include "Game/ActorHandle.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

extern RandomNumberGenerator* g_rng;
extern Renderer* g_theRenderer;
extern BitmapFont* g_testFont;
extern AudioSystem* g_theAudioSystem;

Weapon::Weapon(WeaponDefinition def)
{
	m_weaponDef = def;
	PlayAnimationByName("Idle", g_theSystemClock);
	m_roundsInBag = m_weaponDef.m_magSize;
	m_roundsInMag = m_weaponDef.m_magSize;

	m_autoCooldownTimer = new Timer(1.f, g_theGameClock);
	m_autoCooldownTimer->Start();
}

Weapon::~Weapon()
{
}

void Weapon::Update()
{
	//Manage Animations --------------------------------------------------------------------------------------------------------------------------------------------------------------------
	float secondsElapsed = (float)m_animationClock->GetTotalSeconds();
	if (m_currentAnimation->DidFinishPlayingOnce(secondsElapsed))
	{
		PlayAnimationByName("Idle", g_theSystemClock);
	}

	//Auto Reload/Cooldown -----------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (!m_weaponDef.m_isEnergyBased && m_roundsInMag == 0 && m_roundsInBag > 0)
	{
		Reload();
	}
	else if(m_weaponDef.m_isEnergyBased && m_heatValue >= m_weaponDef.m_maxHeat)
	{
		StartCooldown();
		OnReleaseFire();
	}

	//Manage Reload/Cooldown -----------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (m_reloadTimer && m_reloadTimer->HasPeriodElapsed())
	{
		delete m_reloadTimer;
		m_reloadTimer = nullptr;
	}
	if (m_cooldownTimer && m_cooldownTimer->HasPeriodElapsed())
	{
		FinishCooldown();
	}
	else if(m_autoCooldownTimer == nullptr || m_autoCooldownTimer->HasPeriodElapsed())
	{
		float heatByTime = RangeMap(m_heatValue, 0.f, m_weaponDef.m_maxHeat, 0.f, m_weaponDef.m_cooldownTime);
		float newHeatT = heatByTime - (float)g_theGameClock->GetDeltaSeconds();
		m_heatValue = RangeMap(newHeatT, 0.f, m_weaponDef.m_cooldownTime, 0.f, m_weaponDef.m_maxHeat);
		m_heatValue = GetClamped(m_heatValue, 0.f, m_weaponDef.m_maxHeat);
	}

	//Update Verts ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	m_reticleVerts.clear();
	float reticleHalfSize = m_weaponDef.m_HUDElement.m_reticleSize.x * .5f;
	Vec2 reticleBottomLeft = Vec2(SCREEN_CENTER_X - reticleHalfSize, SCREEN_CENTER_Y - reticleHalfSize);
	Vec2 reticleBottomRight = Vec2(SCREEN_CENTER_X + reticleHalfSize, SCREEN_CENTER_Y - reticleHalfSize);
	Vec2 reticleTopLeft = Vec2(SCREEN_CENTER_X - reticleHalfSize, SCREEN_CENTER_Y + reticleHalfSize);
	Vec2 reticleTopRight = Vec2(SCREEN_CENTER_X + reticleHalfSize, SCREEN_CENTER_Y + reticleHalfSize);
	AddVertsForAABB2D(m_reticleVerts, AABB2(reticleBottomLeft,reticleTopRight), AABB2::ZERO_TO_ONE, Rgba8::WHITE);

	//HUD
	m_HUDVerts.clear();
	float HUDHeight = SCREEN_SIZE_Y / 6.f;
	Vec2 HUDBottomLeft = Vec2(0.f, 0.f);
	Vec2 HUDBottomRight = Vec2(SCREEN_SIZE_X, 0.f);
	Vec2 HUDTopLeft = Vec2(0.f, HUDHeight);
	Vec2 HUDTopRight = Vec2(SCREEN_SIZE_X, HUDHeight);
	AddVertsForAABB2D(m_HUDVerts, AABB2(HUDBottomLeft, HUDTopRight), AABB2::ZERO_TO_ONE, Rgba8::WHITE);

	m_weaponVerts.clear();
	AddCurrentAnimationFrame();
}

void Weapon::Render() const
{
	g_theRenderer->BindTexture(m_weaponDef.m_HUDElement.m_reticleTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_reticleVerts);

	g_theRenderer->BindTexture(m_weaponDef.m_HUDElement.m_baseTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_HUDVerts);

	float secondsForAnim = (float)m_animationClock->GetTotalSeconds();
	g_theRenderer->BindTexture(&m_currentAnimation->GetSpriteDefAtTime(secondsForAnim).GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_weaponVerts);
}

void Weapon::Fire(Actor* const& user)
{
	if ((m_weaponDef.m_isEnergyBased || (m_weaponDef.m_magSize > 0 && m_roundsInMag > 0)) && //Physical and Mag size + round count is valid
		(!m_weaponDef.m_isEnergyBased || m_weaponDef.m_maxHeat > m_heatValue) && //Energy based and heat is not overheat
		(m_cooldownTimer == nullptr) && (m_reloadTimer == nullptr)) //Not in reload or overheat cooldown
	{
		if (!m_weaponDef.m_loopSoundOnHold)
		{
			PlayRapidSoundEffect("Fire", .15f);
		}		

		if (!m_weaponDef.m_isEnergyBased)
		{
			m_roundsInMag--;
		}
		else
		{
			m_heatValue += m_weaponDef.m_heatPerShot;
		}

		if (m_weaponDef.m_rayCount > 0)
		{
			FireRay(user);
		}

		if (m_weaponDef.m_projectileCount > 0.f)
		{
			FireProjectile(user);
		}
	}
	else if (m_weaponDef.m_meleeCount > 0.f) //If Melee
	{
		FireMelee(user);
	}
}

void Weapon::FireRay(Actor* const& user)
{
	Vec3 forward;
	Vec3 left;
	Vec3 up;
	Player* playerRef = (Player*)user->m_controller;

	for (int j = 0; j < (int)m_weaponDef.m_rayCount; j++)
	{
		Actor* hitTarget = nullptr;
		playerRef->m_playerCamOrientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
		Vec3 randomConeDirection = GetRandomDirectionInCone(m_weaponDef.m_rayConeDegrees, playerRef->m_playerCamOrientation);
		RaycastResult3D result = user->m_spawnMap->RaycastAll(user->GetVisionStartPoint(), randomConeDirection,
			m_weaponDef.m_rayRange, hitTarget, user);

		if (hitTarget != nullptr) //hitTarget->m_definition.m_faction != user->m_definition.m_faction
		{
			float damage = g_rng->RollRandomFloatInRange(m_weaponDef.m_rayDamage.m_min, m_weaponDef.m_rayDamage.m_max);
			hitTarget->TakeDamage(user, damage);
			hitTarget->AddImpulse(forward * m_weaponDef.m_rayImpulse);

			SpawnInfo sp;
			sp.m_actor = "BloodSplatter";
			sp.m_orientation = EulerAngles();
			sp.m_position = result.m_impactPos;
			sp.m_velocity = Vec3();
			user->m_spawnMap->SpawnActor(sp);
			//DebugAddMessage("PISTOL HIT!", 2.f);
		}
		else
		{
			SpawnInfo sp;
			sp.m_actor = "BulletHit";
			sp.m_orientation = EulerAngles();
			sp.m_position = result.m_impactPos;
			sp.m_velocity = Vec3();
			user->m_spawnMap->SpawnActor(sp);
		}
		if (m_weaponDef.m_rayRender)
		{
			DebugAddWorldCylinder(user->GetVisionStartPoint() - Vec3(0.f, 0.f, .25f), result.m_impactPos, .01f, 0.f, m_weaponDef.m_rayColor);
		}
	}

	PlayAnimationByName("Attack", g_theGameClock);
}

void Weapon::FireProjectile(Actor* const& user)
{
	for (int j = 0; j < (int)m_weaponDef.m_projectileCount; j++)
	{
		SpawnInfo spawnInfo;
		Player* ref = (Player*)(user->m_controller);
		spawnInfo.m_actor = m_weaponDef.m_projectileActor;
		spawnInfo.m_position = user->m_position + Vec3(0.f, 0.f, user->m_definition.m_collisionElement.m_physicsHeight * .7f) + (.3f * user->m_orientation.GetForwardNormal());
		spawnInfo.m_velocity = GetRandomDirectionInCone(m_weaponDef.m_projectileConeDegrees, ref->m_playerCamOrientation) * m_weaponDef.m_projectileSpeed;
		Actor* refToActor = user->m_spawnMap->SpawnActor(spawnInfo);
		refToActor->m_owningActor = user->m_handle;
	}

	PlayAnimationByName("Attack", g_theGameClock);
}

void Weapon::FireMelee(Actor* const& user)
{
	Vec3 forward;
	Vec3 left;
	Vec3 up;

	std::vector<Actor*> targets = user->m_spawnMap->GetActorsInSector(user, m_weaponDef.m_meleeArcDegrees, m_weaponDef.m_meleeRange);
	for (int i = 0; i < (int)targets.size(); i++)
	{
		float damage = 0.f;
		for (int j = 0; j < (int)m_weaponDef.m_meleeCount; j++)
		{
			damage += g_rng->RollRandomFloatInRange(m_weaponDef.m_meleeDamage.m_min, m_weaponDef.m_meleeDamage.m_max);
		}

		if (user->m_definition.m_faction != targets[i]->m_definition.m_faction)
		{
			user->m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
			targets[i]->TakeDamage(user, damage);
			targets[i]->AddImpulse(m_weaponDef.m_meleeImpulse * forward);
		}
	}

	PlayAnimationByName("Attack", g_theGameClock);
}

void Weapon::SecondaryFire(Actor* const& user)
{
	m_secondaryWeapon->Fire(user);
}

void Weapon::Reload()
{
	if (!m_weaponDef.m_isEnergyBased)
	{
		if (m_reloadTimer == nullptr && m_weaponDef.m_magSize > m_roundsInMag && m_roundsInBag > 0)
		{
			PlayAnimationByName("Reload", g_theGameClock);
			PlayOneSoundEffect("Reload", .3f);
			m_reloadTimer = new Timer(m_weaponDef.m_reloadTime, g_theGameClock);
			m_reloadTimer->Start();
			//Play sound

			float roundsToAdd = GetClamped((float)m_weaponDef.m_magSize - (float)m_roundsInMag, 0.f, (float)m_roundsInBag);
			m_roundsInMag += (int)roundsToAdd;
			m_roundsInBag -= (int)roundsToAdd;
		}
	}
}

void Weapon::StartCooldown()
{
	if (m_cooldownTimer == nullptr)
	{
		m_cooldownTimer = new Timer(m_weaponDef.m_cooldownTime, g_theGameClock);
		m_cooldownTimer->Start();
		PlayOneSoundEffect("Beep", .3f);
		PlayOneSoundEffect("Steam", .3f);
	}
}

void Weapon::FinishCooldown()
{
	if (m_cooldownTimer != nullptr)
	{
		delete m_cooldownTimer;
		m_cooldownTimer = nullptr;
		m_heatValue = 0;
		PlayOneSoundEffect("Recharge", .5f);
	}
}

void Weapon::ResetAutoCooldown()
{
	if (m_autoCooldownTimer)
	{
		delete m_autoCooldownTimer;
		m_autoCooldownTimer = new Timer(1.f, g_theGameClock);
		m_autoCooldownTimer->Start();
	}
}

Vec3 Weapon::GetRandomDirectionInCone(float coneDegrees, EulerAngles const& orientation)
{
	float halfCone = coneDegrees * 0.5f;

	float pitch = g_rng->RollRandomFloatInRange(-halfCone, halfCone);
	float yaw = g_rng->RollRandomFloatInRange(-halfCone, halfCone);
	EulerAngles spreadOffset(yaw, pitch, 0.f);

	spreadOffset.m_yawDegrees += orientation.m_yawDegrees;
	spreadOffset.m_pitchDegrees += orientation.m_pitchDegrees;

	Vec3 forward;
	Vec3 left;
	Vec3 up;
	spreadOffset.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
	return forward;
}

void Weapon::PlayAnimationByName(std::string const& animName, Clock* const& clock)
{
	if (m_currentAnimation != nullptr && m_currentAnimation->GetName() == animName)
	{
		return;
	}

	std::vector<SpriteAnimDefinition*> defs = m_weaponDef.m_HUDElement.m_animationDefs;
	m_currentAnimation = nullptr;
	for (int i = 0; i < (int)defs.size(); i++)
	{
		if (defs[i] != nullptr && defs[i]->GetName() == animName)
		{
			m_currentAnimation = defs[i];
			if(m_animationClock)
			{
				delete m_animationClock;
			}
			m_animationClock = new Clock(*clock);
		}
	}

	if (m_currentAnimation == nullptr && (int)defs.size() > 0)
	{
		m_currentAnimation = defs[0];
		if (m_animationClock)
		{
			delete m_animationClock;
		}
		m_animationClock = new Clock(*clock);
	}
}

void Weapon::AddCurrentAnimationFrame()
{
	float HUDHeight = SCREEN_SIZE_Y / 6.f;
	float centerX = SCREEN_CENTER_X;

	Vec2 spriteDims = m_weaponDef.m_HUDElement.m_spriteSize;
	Vec2 pivot = m_weaponDef.m_HUDElement.m_spritePivot;
	Vec2 min = Vec2(centerX - (spriteDims.x * pivot.x), HUDHeight - (spriteDims.y * pivot.y));
	Vec2 max = min + spriteDims;

	float secondsForAnim = (float)m_animationClock->GetTotalSeconds();
	SpriteDefinition sprite = m_currentAnimation->GetSpriteDefAtTime(secondsForAnim);

	if (m_cooldownTimer)
	{
		AddVertsForAABB2D(m_weaponVerts, AABB2(min, max), sprite.GetUVs(), Rgba8::RED);
	}
	else
	{
		AddVertsForAABB2D(m_weaponVerts, AABB2(min, max), sprite.GetUVs(), Rgba8::WHITE);
	}
}

void Weapon::OnHoldFire()
{
	if (!m_primaryHeld && m_weaponDef.m_loopSoundOnHold)
	{
		m_primaryHeld = true;
		PlayLoopingSoundEffect("Fire", .3f);
	}
}

void Weapon::OnReleaseFire()
{
	if (m_primaryHeld && m_weaponDef.m_loopSoundOnHold)
	{
		m_primaryHeld = false;
		g_theAudioSystem->StopSound(m_permanentID2);
	}
}

void Weapon::OnHoldSecondaryFire()
{
}

void Weapon::OnReleaseSecondaryFire()
{
}

void Weapon::PlayRapidSoundEffect(std::string const& soundName, float volume)
{
	for (int i = 0; i < (int)m_weaponDef.m_sounds.size(); i++)
	{
		if (m_weaponDef.m_sounds[i].m_soundName == soundName)
		{
			SoundID id = g_theAudioSystem->CreateOrGetSound(m_weaponDef.m_sounds[i].m_soundFilePath, 2);
			if (g_theAudioSystem->IsPlaying(m_permanentID1))
			{
				g_theAudioSystem->StopSound(m_permanentID1);
			}
			m_permanentID1 = g_theAudioSystem->StartSound(id, false, volume);
		}
	}
}

void Weapon::PlayOneSoundEffect(std::string const& soundName, float volume)
{
	for (int i = 0; i < (int)m_weaponDef.m_sounds.size(); i++)
	{
		if (m_weaponDef.m_sounds[i].m_soundName == soundName)
		{
			SoundID id = g_theAudioSystem->CreateOrGetSound(m_weaponDef.m_sounds[i].m_soundFilePath, 2);
			g_theAudioSystem->StartSound(id, false, volume);
		}
	}
}

void Weapon::PlayLoopingSoundEffect(std::string const& soundName, float volume)
{
	for (int i = 0; i < (int)m_weaponDef.m_sounds.size(); i++)
	{
		if (m_weaponDef.m_sounds[i].m_soundName == soundName)
		{
			SoundID id = g_theAudioSystem->CreateOrGetSound(m_weaponDef.m_sounds[i].m_soundFilePath, 2);
			if (g_theAudioSystem->IsPlaying(m_permanentID2))
			{
				g_theAudioSystem->StopSound(m_permanentID2);
			}
			m_permanentID2 = g_theAudioSystem->StartSound(id, true, volume);
		}
	}
}