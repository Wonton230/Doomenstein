#include "Controller.hpp"
#include "Game/Actor.hpp"

Controller::Controller(Map* map)
{
	m_map = map;
}

Controller::~Controller()
{
	m_map = nullptr;
}

void Controller::Possess(ActorHandle newActor)
{
	GetActor()->OnUnpossessed();
	m_possessedActor = newActor;
	GetActor()->OnPossessed(this);
}

Actor* Controller::GetActor() const
{
	return m_map->GetActorByHandle(m_possessedActor);
}