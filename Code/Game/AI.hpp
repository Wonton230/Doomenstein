#pragma once
#include "Game/Controller.hpp"
#include "Game/Actor.hpp"
#include "Game/ActorHandle.hpp"

class AI: public Controller
{
public:
	AI();
	~AI();

	void DamagedBy(Actor* attacker);
	void Update() override;

	ActorHandle m_targetActorHandle = ActorHandle::INVALID;
};