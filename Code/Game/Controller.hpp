#pragma once
#include "Game/ActorHandle.hpp"
#include "Game/Map.hpp"

class Controller
{
public:
	Controller() = default;
	Controller(Map* map);
	virtual ~Controller();

	void Possess(ActorHandle newActor);
	Actor* GetActor() const;
	virtual void Update() = 0;

	ActorHandle m_possessedActor = ActorHandle::INVALID;
	Map* m_map;
};