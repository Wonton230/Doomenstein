#pragma once
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Audio/AudioSystem.hpp"

constexpr float WORLD_SIZE_X = 2.f;
constexpr float WORLD_SIZE_Y = 2.f;
constexpr float WORLD_MINS_X = -1.f;
constexpr float WORLD_MINS_Y = -1.f;
constexpr float WORLD_MAXS_X = 1.f;
constexpr float WORLD_MAXS_Y = 1.f;
constexpr float WORLD_CENTER_X = WORLD_MINS_X + WORLD_SIZE_X / 2.f;
constexpr float WORLD_CENTER_Y = WORLD_MINS_Y + WORLD_SIZE_Y / 2.f;

extern Clock* g_theGameClock;

void DrawDebugRing(Vec2 const& center, float const radius, float const thickness, Rgba8 const color);
void DrawDebugLine(Vec2 const& pos, Vec2 const& vector, float const thickness, Rgba8 const color);