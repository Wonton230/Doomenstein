#include "AI.hpp"
#include "Game/Game.hpp"

AI::AI()
{
}

AI::~AI()
{
}

void AI::DamagedBy(Actor* attacker)
{
	if (attacker != nullptr)
	{
		m_targetActorHandle = attacker->m_handle;
	}
}

void AI::Update()
{
	//TODO: Implement aggro logic
	if (GetActor())
	{
		if(m_targetActorHandle == ActorHandle::INVALID)
		{
			Actor* ref = m_map->GetClosestVisibleEnemy(GetActor());
			if (ref != nullptr && !ref->m_isDead && !ref->m_definition.m_isPickup)
			{
				m_targetActorHandle = ref->m_handle;
			}
			else
			{
				m_targetActorHandle = ActorHandle::INVALID;
			}
		}

		if (m_targetActorHandle != ActorHandle::INVALID)
		{
			Actor* targetActor = m_map->GetActorByHandle(m_targetActorHandle);
			if (targetActor && targetActor->m_isDead)
			{
				m_targetActorHandle = ActorHandle::INVALID;
			}
			else
			{
				float deltaSec = (float)g_theGameClock->GetDeltaSeconds();//m_gameClock->GetDeltaSeconds();
				float distanceToTarget = (m_map->GetActorByHandle(m_targetActorHandle)->m_position - GetActor()->m_position).GetLength();

				float attackRange = GetActor()->m_radius;
				if (GetActor()->m_equippedWeapon != nullptr)
				{
					attackRange = GetActor()->m_equippedWeapon->m_weaponDef.m_meleeRange;
				}

				if (GetActor()->m_equippedWeapon != nullptr && attackRange > distanceToTarget)
				{
					GetActor()->TurnInDirection(targetActor->m_position, GetActor()->m_definition.m_physicsElement.turnSpeed * deltaSec);
					GetActor()->Attack();
				}
				else
				{
					GetActor()->TurnInDirection(targetActor->m_position, GetActor()->m_definition.m_physicsElement.turnSpeed * deltaSec);
					GetActor()->MoveInDirection(GetActor()->m_orientation.GetForwardNormal(), GetActor()->m_definition.m_physicsElement.m_runSpeed);
				}			
			}
		}
	}
}