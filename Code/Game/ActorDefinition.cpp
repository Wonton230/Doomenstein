#include "ActorDefinition.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "AnimationGroupDefinition.hpp"

float ActorDefinition::GetHeight() const
{
    return m_collisionElement.m_physicsHeight;
}

float ActorDefinition::GetRadius() const
{
    return m_collisionElement.m_physicsRadius;
}

Faction ActorDefinition::GetFaction() const
{
    return m_faction;
}

bool ActorDefinition::IsSimulated() const
{
    return m_physicsElement.m_simulated;
}

bool ActorDefinition::CanBeAI() const
{
    return m_AIElement.m_aiEnabled;
}

void ActorDefinition::AddGroupDefinition(SpriteSheet* const& sheet, Texture* const& texture, XmlElement* const& element)
{
	NamedStrings animGroupAttributes;
	animGroupAttributes.PopulateFromXmlElementAttributes(*element);
    AnimationGroupDefinition* newGroup = new AnimationGroupDefinition();
    newGroup->m_name = animGroupAttributes.GetValue("name", "-");
    newGroup->m_playbackMode = animGroupAttributes.GetValue("playbackMode", "Once");
    newGroup->m_scaleBySpeed = animGroupAttributes.GetValue("scaleBySpeed", false);
    newGroup->m_secondsPerFrame = animGroupAttributes.GetValue("secondsPerFrame", 1.f);

    XmlElement* child = element->FirstChildElement();
    while (child)
    {
        newGroup->AddDirectionAnimation(child, sheet, texture);
        child = child->NextSiblingElement();
    }

    m_VisualElement.m_groupDefinitions.push_back(newGroup);
}