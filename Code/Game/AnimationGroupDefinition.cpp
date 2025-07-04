#include "AnimationGroupDefinition.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

void AnimationGroupDefinition::AddDirectionAnimation(XmlElement* const& element, SpriteSheet* const& sheet, Texture* const& texture)
{
	Direction newDirection = Direction();
	NamedStrings attributes;
	attributes.PopulateFromXmlElementAttributes(*element);

	newDirection.m_direction = attributes.GetValue("vector", Vec3());
	XmlElement* animation = element->FirstChildElement();
	attributes.PopulateFromXmlElementAttributes(*animation);
	SpriteAnimPlaybackType type;
	if (m_playbackMode == "Once")
	{
		type = SpriteAnimPlaybackType::ONCE;
	}
	else if (m_playbackMode == "Loop")
	{
		type = SpriteAnimPlaybackType::LOOP;
	}
	else if (m_playbackMode == "Pingpong")
	{
		type = SpriteAnimPlaybackType::PINGPONG;
	}
	else
	{
		type = SpriteAnimPlaybackType::ONCE;
	}
	SpriteAnimDefinition* animationDef = new SpriteAnimDefinition(*sheet, attributes.GetValue("startFrame", 0), attributes.GetValue("endFrame", 0), 1.f/m_secondsPerFrame, type);
	animationDef->LoadFromXmlElement(*element);
	newDirection.m_animation = animationDef;;
	texture;

	m_directionAnims.push_back(newDirection);
}

const std::vector<Direction>& AnimationGroupDefinition::GetDirectionAnimations() const
{
	return m_directionAnims;
}

const std::string& AnimationGroupDefinition::GetName() const
{
	return m_name;
}

float AnimationGroupDefinition::GetSecondsPerFrame() const
{
	return m_secondsPerFrame;
}

bool AnimationGroupDefinition::IsScaledBySpeed() const
{
	return m_scaleBySpeed;
}

const std::string& AnimationGroupDefinition::GetPlaybackMode() const
{
	return m_playbackMode;
}
