#pragma once
#pragma once
#include <string>
#include <vector>
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"

class Texture;

struct Direction
{
	Vec3 m_direction = Vec3();
	SpriteAnimDefinition* m_animation = nullptr;

	Direction() = default;
};

class AnimationGroupDefinition
{
public:
	AnimationGroupDefinition() = default;

	void									AddDirectionAnimation(XmlElement* const& element, SpriteSheet* const& sheet, Texture* const& texture);
	const std::vector<Direction>&			GetDirectionAnimations() const;
	const std::string&						GetName() const;
	const std::string&						GetPlaybackMode() const;
	float									GetSecondsPerFrame() const;
	bool									IsScaledBySpeed() const;

	std::string m_name = "Default";
	std::string m_playbackMode = "Loop";
	bool		m_scaleBySpeed = false;
	float		m_secondsPerFrame = 0.25f;
	std::vector<Direction> m_directionAnims;
};