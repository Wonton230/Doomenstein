#include "Actor.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/AI.hpp"

extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern AudioSystem* g_theAudioSystem;

Actor::Actor(Map* owningMap, ActorHandle handle, ActorDefinition def, Vec3 startPos, EulerAngles startOrientation, Vec3 startVelocity)
{
	//Controls
	m_handle = handle;
	m_definition = def;
	m_spawnMap = owningMap;
	m_AIController = new AI();
	m_controller = m_AIController;
	m_controller->m_map = owningMap;
	m_controller->m_possessedActor = m_handle;
	m_isAI = true;
	
	//Physics
	m_position = startPos;
	m_orientation = startOrientation;
	m_velocity = startVelocity;
	m_acceleration = Vec3();
	m_height = def.m_collisionElement.m_physicsHeight;
	m_radius = def.m_collisionElement.m_physicsRadius;

	//Health
	m_health = def.m_health;
	m_deathTimer = new Timer(m_definition.m_corpseLifetime, g_theGameClock);

	//Weapons
	m_weaponIndex = 0;
	m_equippedWeapon = nullptr;
	m_refireTimer = new Timer(0.f, g_theGameClock);
	std::vector<std::string> weaponNames = m_definition.m_weaponElement.m_weaponNames;
	for (int i = 0; i < (int)weaponNames.size(); i++)
	{
		WeaponDefinition useThis;
		useThis.m_name = "undefined";
		for (int j = 0; j < (int)m_spawnMap->m_game->m_weaponDefs.size(); j++)
		{
			if (weaponNames[i] == m_spawnMap->m_game->m_weaponDefs[j]->m_name)
			{
				useThis = *m_spawnMap->m_game->m_weaponDefs[j];
			}
		}
		if (useThis.m_name == "undefined")
		{
			ERROR_AND_DIE("Error: Could not give Weapons due to invalid actor definition (SpawnActor)");
		}
		m_weaponInventory.push_back(new Weapon(useThis));
	}

	if ((int)m_weaponInventory.size() > 0)
	{
		m_equippedWeapon = m_weaponInventory.front();
		m_refireTimer->m_period = m_equippedWeapon->m_weaponDef.m_refireTime;
		m_refireTimer->Start();
	}

	//Render
	m_color = def.m_color;
	m_vertexPCUTBNBuffer = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_animationClock = new Clock();
	m_animationTimer = new Timer(0.f, m_animationClock);
	if (m_definition.m_visible)
	{
		m_currentAnimation = m_definition.m_VisualElement.m_groupDefinitions[0];
	}
	
	m_animationClock = new Clock(*g_theGameClock);
	m_animationTimer = new Timer(0.f, m_animationClock);
	m_animationTimer->Start();
}

Actor::~Actor()
{
	m_controller = nullptr;
	m_vertTBNs.clear();
	delete m_vertexPCUTBNBuffer;
}

void Actor::Update()
{
	if (m_definition.m_dieOnSpawn)
	{
		StartDeath();
	}

	if (m_controller && m_isAI)
	{
		m_controller->Update();
	}
	//UpdateVerts();
	//if (m_equippedWeapon != nullptr)
	//{
	//	m_equippedWeapon->Update();
	//}
	for (int weaponIndex = 0; weaponIndex < (int)m_weaponInventory.size(); weaponIndex++)
	{
		if (m_weaponInventory[weaponIndex])
		{
			m_weaponInventory[weaponIndex]->Update();
		}
	}

	if (!m_isDead)
	{
		UpdatePhysics();
	}
	//Audio Update
	if (m_deathTimer->DecrementPeriodIfElapsed())
	{
		m_expired = true;
	}
}

void Actor::UpdateVerts()
{
	//Projectile
	if (m_owningActor != ActorHandle::INVALID && m_definition.m_visible)
	{
		UpdateProjectileVerts();
	}
	//Actor
	else if (m_definition.m_visible)
	{
		UpdateActorVerts();
	}
	std::string health = Stringf("Health: %.2f", m_health);
}

void Actor::UpdateProjectileVerts()
{
	m_vertTBNs.clear();
	if (!m_isDead)
	{
		PlayAnimationByName("Walk");
		AddCurrentAnimationFrame();
		m_spawnMap->AddPointLightToMap(m_position, .15f, m_color);
	}
	else
	{
		PlayAnimationByName("Death");
		AddCurrentAnimationFrame();
		m_spawnMap->AddPointLightToMap(m_position, .15f, m_color);
	}
}

void Actor::UpdateActorVerts()
{
	m_vertTBNs.clear();
	if (!m_isDead)
	{
		if (!m_animationTimer->IsStopped() && m_animationTimer->HasPeriodElapsed())
		{
			PlayAnimationByName("Walk");
		}
		AddCurrentAnimationFrame();
	}
	else
	{
		PlayAnimationByName("Death");
		AddCurrentAnimationFrame();
	}
}

void Actor::UpdatePhysics()
{	
	float deltaSeconds = (float)g_theGameClock->GetDeltaSeconds();

	Vec3 dragAcceleration = -m_definition.m_physicsElement.m_drag * m_velocity;
	AddForce(dragAcceleration);
	m_velocity += m_acceleration * deltaSeconds;
	m_position += m_velocity * deltaSeconds;
	m_acceleration = Vec3();

	if (!m_definition.m_physicsElement.m_flying)
	{
		m_position.z = 0.f;
	}
}

void Actor::Render() const
{
	if ((m_spawnMap->m_game->m_player->m_possessedActor == m_handle && m_spawnMap->m_game->m_player->m_currentControlMode != ControlMode::CAMERA) || !m_definition.m_visible)
	{
		return;
	}
	
	//Set Texture and Shader
	BindTextureToRenderer();
	g_theRenderer->BindShader(m_definition.m_VisualElement.m_shader);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	//Evaluate Billboard transform
	if (m_definition.m_VisualElement.m_billBoardType == BillBoardType::NONE)
	{
		g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);
	}
	else
	{
		if (m_owningActor != ActorHandle::INVALID)
		{
			g_theRenderer->SetModelConstants(GetModelToWorldBillboardTransform());
		}
		else
		{
			g_theRenderer->SetModelConstants(GetModelToWorldBillboardTransform(), m_color);
		}
		
	}

	g_theRenderer->SetStatesIfChanged();
	DrawVertexPCUTBNs(m_vertTBNs);
}

void Actor::TakeDamage(Actor* sourceActor, float damage)
{
	if (m_definition.m_isPickup || (sourceActor && sourceActor->m_definition.m_isPickup))
	{
		return;
	}

	if (damage > 0.f && !m_isDead)
	{
		PlayAnimationByName("Hurt");
		PlaySoundEffect("Hurt", .5f);
	}
	m_health -= damage;

	if (CheckIfHealthIsZero())
	{
		if (sourceActor != nullptr && sourceActor != this)
		{
			if (sourceActor->m_owningActor != ActorHandle::INVALID)
			{
				Actor* attacker = m_spawnMap->GetActorByHandle(sourceActor->m_owningActor);
				if (!attacker->m_isAI && !m_isAI)
				{
					static_cast<Player*>(attacker->m_controller)->AddKillCount();
					static_cast<Player*>(m_controller)->AddDeathCount();
				}
			}
			else
			{
				Actor* attacker = sourceActor;
				if (!attacker->m_isAI && !m_isAI)
				{
					static_cast<Player*>(attacker->m_controller)->AddKillCount();
					static_cast<Player*>(m_controller)->AddDeathCount();
				}
			}
		}
	}

	if (sourceActor != nullptr && sourceActor != this)
	{
		if (sourceActor->m_owningActor != ActorHandle::INVALID)
		{
			Actor* attacker = m_spawnMap->GetActorByHandle(sourceActor->m_owningActor);
			if (m_isAI)
			{
				static_cast<AI*>(m_controller)->DamagedBy(attacker);
			}
		}
		else
		{
			Actor* attacker = sourceActor;
			if (m_isAI && attacker->m_definition.m_faction != m_definition.m_faction)
			{
				static_cast<AI*>(m_controller)->DamagedBy(attacker);
			}
		}
	}
}

bool Actor::CheckIfHealthIsZero()
{
	if (!m_isDead && m_health <= 0.f)
	{
		StartDeath();
		return true;
	}
	return false;
}

void Actor::StartDeath()
{
	if (!m_isDead)
	{
		g_theAudioSystem->StopSound(m_permanentID1);
		g_theAudioSystem->StopSound(m_permanentID2);
		PlaySoundEffect("Death", .5f);
		m_deathTimer->Start();
		m_isDead = true;

		if (m_definition.m_faction == Faction::EVIL)
		{
			m_spawnMap->OnDemonKilled();

			float randomPercent = g_rng->RollRandomFloatInRange(0.f, 100.f);
			if (randomPercent <= 30.f)
			{
				SpawnInfo info = SpawnInfo();
				info.m_actor = "AmmoPickup";
				info.m_position = m_position;
				m_spawnMap->SpawnActor(info);
			}
		}
		else if(m_definition.m_faction == Faction::GOOD)
		{
			SpawnInfo info = SpawnInfo();
			info.m_actor = "AmmoPickup";
			info.m_position = m_position;
			m_spawnMap->SpawnActor(info);
		}
	}
}

void Actor::OnCollide(Actor* other)
{
	m_didCollide = true;

	if (m_definition.m_isPickup)
	{
		return;
	}

	if (other != nullptr)
	{
		if (other->m_definition.m_isPickup && !m_isAI)
		{
			HandlePickupLogic(other);
		}
		else
		{

			float damageMe = g_rng->RollRandomFloatInRange(other->m_definition.m_collisionElement.m_damageOnCollide.m_min, other->m_definition.m_collisionElement.m_damageOnCollide.m_max);
			TakeDamage(other, damageMe);
			float damageOther = g_rng->RollRandomFloatInRange(m_definition.m_collisionElement.m_damageOnCollide.m_min, m_definition.m_collisionElement.m_damageOnCollide.m_max);
			TakeDamage(this, damageOther);

			if ((other->m_position != m_position))
			{
				Vec3 direction = (m_position - other->m_position).GetNormalized();
				AddImpulse(direction * other->m_definition.m_collisionElement.m_impulseOnCollide);
				other->AddImpulse(-direction * other->m_definition.m_collisionElement.m_impulseOnCollide);
			}
		}
	}

	if (m_definition.m_collisionElement.m_dieOnCollision && !m_isDead)
	{
		StartDeath();
	}
}

void Actor::OnPossessed(Controller* controller)
{
	if (m_isAI)
	{
		m_AIController = (AI*)m_controller;
	}
	else
	{
		m_AIController = nullptr;
	}
	m_controller = controller;
	controller->m_possessedActor = m_handle;
	m_isAI = false;
}

void Actor::OnUnpossessed()
{
	m_controller->m_possessedActor = ActorHandle::INVALID;
	m_controller = m_AIController;
	m_isAI = true;
}

void Actor::AddForce(Vec3 const& acceleration)
{
	m_acceleration += acceleration;
}

void Actor::AddImpulse(Vec3 const& impulse)
{
	m_velocity += impulse;
}

void Actor::MoveInDirection(Vec3 const& direction, float speed)
{
	float magnitude = speed * m_definition.m_physicsElement.m_drag;
	direction.GetNormalized();
	AddForce(direction * magnitude);
}

void Actor::TurnInDirection(Vec3 const& point, float maxTurn)
{
	Vec2 forward = m_orientation.GetForwardNormal().GetFlattenedXY();
	Vec2 toTarget = (point - m_position).GetFlattenedXY().GetNormalized();
	float targetYaw = Atan2Degrees(toTarget.y, toTarget.x);
	float newYaw = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, targetYaw, maxTurn);
	m_orientation.m_yawDegrees = newYaw;
}

void Actor::Attack()
{
	while (m_refireTimer->DecrementPeriodIfElapsed())
	{
		m_equippedWeapon->Fire(this);
		PlayAnimationByName("Attack");
	}
}

void Actor::CycleWeaponUp()
{
	int index = (m_weaponIndex + 1) % (int)m_weaponInventory.size();
	m_equippedWeapon->OnReleaseFire();
	m_equippedWeapon = m_weaponInventory[index];
	m_refireTimer->m_period = m_equippedWeapon->m_weaponDef.m_refireTime;
	m_weaponIndex = index;
}

void Actor::CycleWeaponDown()
{
	int index = (m_weaponIndex - 1);
	if (index < 0)
	{
		index = (int)m_weaponInventory.size() - 1;
	}
	m_equippedWeapon->OnReleaseFire();
	m_equippedWeapon = m_weaponInventory[index];
	m_refireTimer->m_period = m_equippedWeapon->m_weaponDef.m_refireTime;
	m_weaponIndex = index;
}

void Actor::PlaySoundEffect(std::string const& soundName, float volume)
{
	for (int i = 0; i < (int)m_definition.m_sounds.size(); i++)
	{
		if (m_definition.m_sounds[i].m_soundName == soundName)
		{
			SoundID id = g_theAudioSystem->CreateOrGetSound(m_definition.m_sounds[i].m_soundFilePath, 3);
			if (g_theAudioSystem->IsPlaying(m_permanentID1))
			{
				break;
			}
			else
			{
				m_permanentID1 = g_theAudioSystem->StartSoundAt(id, m_position, false, volume);
			}
		}
	}
}

Mat44 Actor::GetModelToWorldTransform() const
{
	Mat44 modelToWorld = Mat44::MakeTranslation3D(m_position);
	modelToWorld.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorld;
}

Vec3 Actor::GetVisionStartPoint() const
{
	return m_position + Vec3(0.f, 0.f, m_definition.m_cameraElement.m_eyeHeight);
}

void Actor::HandlePickupLogic(Actor* const& other)
{
	if (other->m_definition.m_name == "AmmoPickup")
	{
		for (int i = 0; i < (int)m_weaponInventory.size(); i++)
		{
			if (!m_weaponInventory[i]->m_weaponDef.m_isEnergyBased)
			{
				m_weaponInventory[i]->m_roundsInBag += m_weaponInventory[i]->m_weaponDef.m_magSize;
			}
		}
	}

	other->PlaySoundEffect("PickedUp", .3f);
	other->m_isDead = true;
	other->m_expired = true;
}

void Actor::DrawVertexPCUTBNs(std::vector<Vertex_PCUTBN> const& verts) const
{
	if (!verts.empty())
	{
		unsigned int numVerts = static_cast<unsigned int>(verts.size());
		const Vertex_PCUTBN* addressOfFirstElement = &verts.front();

		g_theRenderer->CopyCPUToGPU(addressOfFirstElement, numVerts, m_vertexPCUTBNBuffer);
		g_theRenderer->DrawVertexBuffer(m_vertexPCUTBNBuffer, numVerts);
	}
}

Direction Actor::GetDirectionOfActorAnimationToCamera(Vec3 const& referencePoint, AnimationGroupDefinition const& animGroup)
{
	Vec3 cameraToActorXY = m_position - referencePoint;
	Vec3 cameraToActor = Vec3(cameraToActorXY.x, cameraToActorXY.y, 0.f).GetNormalized();

	//Get the actor's model to world then inverse it to get a world to actor model matrix
	Mat44 transformation = GetModelToWorldTransform().GetOrthonormalInverse();
	cameraToActor = transformation.TransformVectorQuantity3D(cameraToActor);

	std::vector<Direction> directions = animGroup.m_directionAnims;
	float largestDot = 0.f;
	Direction closestMatch = directions[0];
	for (int i = 0; i < (int)directions.size(); i++)
	{
		float dot = DotProduct3D(cameraToActor.GetNormalized(), directions[i].m_direction.GetNormalized());
		if (dot >= largestDot)
		{
			largestDot = dot;
			closestMatch = directions[i];
		}
	}
	return closestMatch;
}

void Actor::BindTextureToRenderer() const
{
	if (m_definition.m_VisualElement.m_sheet != nullptr)
	{
		g_theRenderer->BindTexture(&m_definition.m_VisualElement.m_sheet->GetTexture());
	}
	else
	{
		g_theRenderer->BindTexture(nullptr);
	}
}

Mat44 Actor::GetModelToWorldBillboardTransform() const
{
	Mat44 cameraTransform = m_spawnMap->m_game->m_player->GetModelToWorldTransform();
	Vec2 spriteDims = Vec2(m_definition.m_VisualElement.m_spriteWorldSize.x, m_definition.m_VisualElement.m_spriteWorldSize.y);
	Vec3 pivotPoint = Vec3(0, spriteDims.x * (.5f - m_definition.m_VisualElement.m_pivot.x), spriteDims.y * (.5f - m_definition.m_VisualElement.m_pivot.y));
	Mat44 billBoardTransform = GetBillboardTransform(m_definition.m_VisualElement.m_billBoardType, cameraTransform, m_position, spriteDims);
	billBoardTransform.AppendTranslation3D(pivotPoint);
	return billBoardTransform;
}

void Actor::PlayAnimationByName(std::string const& animName)
{
	if (m_currentAnimation != nullptr && m_currentAnimation->m_name == animName)
	{
		return;
	}

	std::vector<AnimationGroupDefinition*> groups = m_definition.m_VisualElement.m_groupDefinitions;
	AnimationGroupDefinition* select = nullptr;
	for (int i = 0; i < (int)groups.size(); i++)
	{
		if (groups[i]->m_name == animName)
		{
			select = groups[i];
		}
	}
	if (select != nullptr)
	{
		if (m_animationClock)
		{
			delete m_animationClock;
			delete m_animationTimer;
		}
		m_currentAnimation = select;
		m_animationClock = new Clock(*g_theGameClock);
		m_animationTimer = new Timer(GetDirectionOfActorAnimationToCamera(m_position, *m_currentAnimation).m_animation->GetLengthSeconds(), m_animationClock);
		if (animName != groups[0]->m_name)
		{
			m_animationTimer->Start();
		}
		m_animationClock->Unpause();
	}
}

void Actor::AddCurrentAnimationFrame()
{
	Vec3 cameraPos = m_spawnMap->m_game->m_worldCamera.GetPosition();

	Direction animationDirection = GetDirectionOfActorAnimationToCamera(cameraPos, *m_currentAnimation);
	Vec2 spriteDims = Vec2(m_definition.m_VisualElement.m_spriteWorldSize.x, m_definition.m_VisualElement.m_spriteWorldSize.y);

	if (m_currentAnimation->m_scaleBySpeed && m_definition.m_physicsElement.m_runSpeed > 0.f)
	{
		m_animationClock->SetTimeScale((double)m_velocity.GetLength() / m_definition.m_physicsElement.m_runSpeed);
	}
	float secondsForAnim = (float)m_animationClock->GetTotalSeconds();
	SpriteDefinition sprite = animationDirection.m_animation->GetSpriteDefAtTime(secondsForAnim);
	if (m_definition.m_VisualElement.m_renderRounded)
	{
		AddVertsForRoundedQuad3D(m_vertTBNs, Vec3(0, 0, 0), Vec3(0, spriteDims.x, 0), Vec3(0, spriteDims.x, spriteDims.y), Vec3(0, 0, spriteDims.y), Vec3(1, 0, 0),
			Rgba8::WHITE, sprite.GetUVs());
	}
	else
	{
		AddVertsForQuad3D(m_vertTBNs, Vec3(0, 0, 0), Vec3(0, spriteDims.x, 0), Vec3(0, spriteDims.x, spriteDims.y), Vec3(0, 0, spriteDims.y), Vec3(1, 0, 0),
			Rgba8::WHITE, sprite.GetUVs());
	}
}