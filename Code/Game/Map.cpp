#include "Map.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/ActorDefinition.hpp"
#include "AI.hpp"

extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

Map::Map()
{
	m_definition = nullptr;
}

Map::Map(Game* game, const MapDefinition* definition)
{
	m_game = game;
	m_definition = definition;
	m_texture = definition->GetTexture();
	m_shader = definition->GetShader();
	CreateTiles();
	CreateGeometry();
	CreateBuffers();

	Texture* skyBoxTexture = g_theRenderer->CreateOrGetTextureFromFile(m_definition->m_skyBoxFilePath.c_str());
	m_skyBoxSheet = new SpriteSheet(*skyBoxTexture, IntVec2(4, 3));
	CreateSkybox();
}

Map::~Map()
{
	delete m_definition;
	m_verts.clear();
	m_vertIndexes.clear();
	m_tiles.clear();

	for (int i = 0; i < (int)m_actors.size(); i++)
	{
		delete m_actors[i];
	}
	m_actors.clear();
	delete m_vBuffer;
	delete m_iBuffer;
	delete m_lightBuffer;
	delete m_skyBoxSheet;
	m_game = nullptr;
}

void Map::CreateTiles()
{
	std::string imageFilePath = "Data/Maps/" + m_definition->GetName() + ".png";
	Image* mapImage = g_theRenderer->CreateImageFromFile(imageFilePath.c_str());
	m_dimensions = mapImage->GetDimensions();
	std::vector<Rgba8> texels = mapImage->GetDataAsRgba8Vector();

	for (int rows = 0; rows < m_dimensions.y; rows++)
	{
		for (int cols = 0; cols < m_dimensions.x; cols++)
		{
			int index = (rows*m_dimensions.y) + cols;
			TileDefinition* selectedTile;
			for (int defIndex = 0; defIndex < m_game->m_tileDefs.size(); defIndex++)
			{
				Rgba8 texelColor = texels[index];
				if (m_game->m_tileDefs[defIndex]->GetMapPixelColor() == texelColor)
				{
					selectedTile = m_game->m_tileDefs[defIndex];
					m_tiles.push_back(Tile(cols, rows, *selectedTile));
					break;
				}
			}
		}
	}

	delete mapImage;
}

void Map::CreateGeometry()
{
	for (int i = 0; i < m_tiles.size(); i++)
	{
		SpriteSheet* sheet = m_game->m_spriteSheet;
		IntVec2 sheetDims = m_definition->GetSpriteCellCount();
		AABB3 bounds = m_tiles[i].m_bounds;
		IntVec2 spriteCoords = m_tiles[i].m_tileDef.GetWallCoords();
		IntVec2 secondaryCoords = m_tiles[i].m_tileDef.GetSecondaryCoords();

		if (spriteCoords.x != -1)
		{
			AddGeometryForWall(bounds, sheet->GetSpriteUVs(spriteCoords), sheet->GetSpriteUVs(secondaryCoords));
		}

		spriteCoords = m_tiles[i].m_tileDef.GetFloorCoords();
		if (spriteCoords.x != -1)
		{
			AddGeometryForFloor(bounds, sheet->GetSpriteUVs(spriteCoords));
		}

		spriteCoords = m_tiles[i].m_tileDef.GetCeilingCoords();
		if (spriteCoords.x != -1)
		{
			AddGeometryForCeiling(bounds, sheet->GetSpriteUVs(spriteCoords));
		}
	}
}

void Map::AddGeometryForWall(const AABB3& bounds, const AABB2& UVs, const AABB2& UV2s)
{
	for (int i = 0; i < (int)m_definition->m_ceilingHeight; i++)
	{
		float random = g_rng->RollRandomFloatInRange(0.f, 100.f);
		if (random <= 33.f)
		{
			//Facing -y
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(0, 0, (float)i), bounds.m_mins + Vec3(1, 0, (float)i), bounds.m_mins + Vec3(1, 0, (float)i + 1), bounds.m_mins + Vec3(0, 0, (float)i + 1),
				Vec3(0, -1, 0), Rgba8::WHITE, UV2s);
			//Facing +x
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(1, 0, (float)i), bounds.m_mins + Vec3(1, 1, (float)i), bounds.m_maxs + Vec3(0, 0, (float)i), bounds.m_maxs + Vec3(0, -1, (float)i),
				Vec3(1, 0, 0), Rgba8::WHITE, UV2s);
			//Facing +y
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_maxs + Vec3(0, 0, (float)i - 1), bounds.m_mins + Vec3(0, 1, (float)i), bounds.m_mins + Vec3(0, 1, (float)i + 1), bounds.m_maxs + Vec3(0, 0, (float)i),
				Vec3(0, 1, 0), Rgba8::WHITE, UV2s);
			//Facing -x
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(0, 1, (float)i), bounds.m_mins + Vec3(0, 0, (float)i), bounds.m_mins + Vec3(0, 0, (float)i + 1), bounds.m_mins + Vec3(0, 1, (float)i + 1),
				Vec3(-1, 0, 0), Rgba8::WHITE, UV2s);
		}
		else
		{
			//Facing -y
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(0, 0, (float)i), bounds.m_mins + Vec3(1, 0, (float)i), bounds.m_mins + Vec3(1, 0, (float)i + 1), bounds.m_mins + Vec3(0, 0, (float)i + 1),
				Vec3(0, -1, 0), Rgba8::WHITE, UVs);
			//Facing +x
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(1, 0, (float)i), bounds.m_mins + Vec3(1, 1, (float)i), bounds.m_maxs + Vec3(0, 0, (float)i), bounds.m_maxs + Vec3(0, -1, (float)i),
				Vec3(1, 0, 0), Rgba8::WHITE, UVs);
			//Facing +y
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_maxs + Vec3(0, 0, (float)i - 1), bounds.m_mins + Vec3(0, 1, (float)i), bounds.m_mins + Vec3(0, 1, (float)i + 1), bounds.m_maxs + Vec3(0, 0, (float)i),
				Vec3(0, 1, 0), Rgba8::WHITE, UVs);
			//Facing -x
			AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins + Vec3(0, 1, (float)i), bounds.m_mins + Vec3(0, 0, (float)i), bounds.m_mins + Vec3(0, 0, (float)i + 1), bounds.m_mins + Vec3(0, 1, (float)i + 1),
				Vec3(-1, 0, 0), Rgba8::WHITE, UVs);
		}
	}
}

void Map::AddGeometryForFloor(const AABB3& bounds, const AABB2& UVs)
{
	AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_mins, bounds.m_mins + Vec3(1, 0, 0), bounds.m_maxs + Vec3(0, 0, -1), bounds.m_maxs + Vec3(-1, 0, -1),
		Vec3(0, 0, 1), Rgba8::WHITE, UVs);
}

void Map::AddGeometryForCeiling(const AABB3& bounds, const AABB2& UVs)
{
	AddVertsForQuad3D(m_verts, m_vertIndexes, bounds.m_maxs + Vec3(-1, 0, m_definition->m_ceilingHeight - 1), bounds.m_maxs + Vec3(0,0, m_definition->m_ceilingHeight - 1), bounds.m_mins + Vec3(1, 0, m_definition->m_ceilingHeight), bounds.m_mins + Vec3(0, 0, m_definition->m_ceilingHeight),
		Vec3(0, 0, -1), Rgba8::WHITE, UVs);
}

void Map::CreateBuffers()
{
	unsigned int numVerts = static_cast<unsigned int>(m_verts.size());
	const Vertex_PCUTBN* addressOfFirstElement = &m_verts.front();
	m_vBuffer = g_theRenderer->CreateVertexBuffer(numVerts * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	g_theRenderer->CopyCPUToGPU(addressOfFirstElement, numVerts, m_vBuffer);

	unsigned int numIndexes = static_cast<unsigned int>(m_vertIndexes.size());
	const unsigned int* addressOfIndex = &m_vertIndexes.front();
	m_iBuffer = g_theRenderer->CreateIndexBuffer(numIndexes);
	g_theRenderer->CopyCPUToGPU(addressOfIndex, numIndexes, m_iBuffer);

	m_lightBuffer = g_theRenderer->CreateConstantBuffer(sizeof(LightConstants));
	LightConstants light;
	light.c_sunDirection = m_game->m_lightDirection.GetNormalized();
	light.c_sunIntensity = m_game->m_lightIntensity;
	light.c_ambientIntensity = m_game->m_ambientIntensity;
	m_lightConstants = light;
	g_theRenderer->CopyCPUToGPU(&m_lightConstants, sizeof(LightConstants), m_lightBuffer);
}

void Map::CreateSkybox()
{
	m_skyBoxLocalBounds.m_mins = m_skyBoxLocalBounds.m_mins + Vec3(m_dimensions.x * .5f, m_dimensions.y * .5f, 0.f);
	m_skyBoxLocalBounds.m_maxs = m_skyBoxLocalBounds.m_maxs + Vec3(m_dimensions.x * .5f, m_dimensions.y * .5f, 0.f);
	AddVertsForSkyBoxAABB3D(m_skyBoxVerts, m_skyBoxLocalBounds, *m_skyBoxSheet);
}

Actor* Map::SpawnPlayer(Player* possessingPlayer, Vec3 const& location)
{
	//Load Marine Actor
	SpawnInfo sp;
	sp.m_actor = "Marine";
	sp.m_orientation = EulerAngles();
	
	sp.m_position = location;
	sp.m_velocity = Vec3();

	//Spawn Marine
	Actor* player = SpawnActor(sp);

	//Set controller variables
	possessingPlayer->m_playerCamPosition = sp.m_position;
	possessingPlayer->m_playerCamOrientation = sp.m_orientation;
	possessingPlayer->m_possessedActor = player->m_handle;
	possessingPlayer->m_map = this;

	//Set actor variables
	player->m_controller = possessingPlayer;
	player->m_isAI = false;
	player->m_AIController = new AI();
	player->m_AIController->m_possessedActor = player->m_handle;
	player->m_AIController->m_map = player->m_spawnMap;
	
	return player;
}

void Map::KillPlayer(Player* possessingPlayer)
{
	m_actors[possessingPlayer->m_possessedActor.GetIndex()]->m_controller = nullptr;
	delete m_actors[possessingPlayer->m_possessedActor.GetIndex()];
	m_actors[possessingPlayer->m_possessedActor.GetIndex()] = nullptr;
	possessingPlayer->m_possessedActor = ActorHandle::INVALID;
}

Actor* Map::SpawnActor(const SpawnInfo& spawnInfo)
{
	//Manage and check Handle
	unsigned int index = (int)m_actors.size();
	unsigned int uid = m_nextActorUID;
	m_nextActorUID++;
	if (uid >= ActorHandle::MAX_ACTOR_UID)
	{
		ERROR_AND_DIE("UIDs have run out of space. Error in SpawnActor");
	}

	//Find next available index. Error if out of range
	for (int i = 0; i < (int)m_actors.size(); i++)
	{
		if (m_actors[i] == nullptr)
		{
			index = i;
			break;
		}
	}
	if (index >= ActorHandle::MAX_ACTOR_INDEX)
	{
		ERROR_AND_DIE("Actor vector has run out of space. Error in SpawnActor");
	}
	ActorHandle newHandle = ActorHandle(uid, index);
	ActorDefinition useThis;
	useThis.m_name = "undefined";
	for (int i = 0; i < (int)m_game->m_actorDefs.size(); i++)
	{
		if (spawnInfo.m_actor == m_game->m_actorDefs[i]->m_name)
		{
			useThis = *m_game->m_actorDefs[i];
		}
	}
	if (useThis.m_name == "undefined")
	{
		ERROR_AND_DIE("Error: Could not spawn actor due to invalid actor definition (SpawnActor)");
	}

	//Create the new Actor
	Actor* newActor = new Actor(this, newHandle, useThis, spawnInfo.m_position, spawnInfo.m_orientation, spawnInfo.m_velocity);
	if (index == (int)m_actors.size())
	{
		m_actors.push_back(newActor);
	}
	else
	{
		m_actors[index] = newActor;
	}

	newActor->m_owningActor = ActorHandle::INVALID;
	return newActor;
}

void Map::KillAIActor(const ActorHandle handle)
{
	delete m_actors[handle.GetIndex()]->m_controller;
	m_actors[handle.GetIndex()]->m_controller = nullptr;
	delete m_actors[handle.GetIndex()];
	m_actors[handle.GetIndex()] = nullptr;
}

Actor* Map::GetActorByHandle(const ActorHandle handle) const
{
	unsigned int index = handle.GetIndex();

	if (m_actors[index] != nullptr)
	{
		if (handle == m_actors[index]->m_handle)
		{
			return m_actors[index];
		}
	}
	return nullptr;
}

void Map::DeleteDestroyedActors()
{
	for (int i = 0; i < (int)m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr)
		{
			if (m_actors[i]->m_expired)
			{
				delete m_actors[i];
				m_actors[i] = nullptr;
			}
		}
	}
}

Actor* Map::GetClosestVisibleEnemy(Actor* searchingActor) const
{
	float shortestDistanceHit = 9999.f;
	Actor* output = nullptr;

	for (int i = 0; i < (int)m_actors.size(); i++)
	{
		//Filter nulls
		if (m_actors[i] != nullptr)
		{
			//Filter same factions and invisibles
			//
			if (m_actors[i]->m_definition.m_visible && searchingActor->m_definition.m_AIElement.m_aiEnabled && !m_actors[i]->m_definition.m_isPickup &&
				searchingActor->m_definition.m_faction != m_actors[i]->m_definition.m_faction &&
				m_actors[i]->m_definition.m_faction != Faction::NEUTRAL)
			{
				//If the point is well outside of vision cone skip
				Vec3 sightLine = m_actors[i]->m_position - searchingActor->GetVisionStartPoint();
				if (IsPointInsideVisionCone(searchingActor->GetVisionStartPoint(), searchingActor->m_orientation, searchingActor->m_definition.m_AIElement.m_sightAngle, m_actors[i]->m_position))
				{
					Actor* detection = nullptr;
					//std::vector<Actor*> potentialTargets = m_game->m_map->GetActorsInSector(searchingActor, searchingActor->m_definition.m_AIElement.m_sightAngle, searchingActor->m_definition.m_AIElement.m_sightRadius);
					RaycastResult3D result = RaycastAll(searchingActor->GetVisionStartPoint(), sightLine.GetNormalized(),
						searchingActor->m_definition.m_AIElement.m_sightRadius, detection, searchingActor);
					//If the actor output is not null (the raycast hit the actor)
					if (detection && result.m_impactDist <= shortestDistanceHit)
					{
						shortestDistanceHit = result.m_impactDist;
						output = detection;
					}
				}
			}
		}
	}

	return output;
}

void Map::DebugPossessNext()
{
	for (int i = 1; i <= (int)m_actors.size(); i++)
	{
		int index = (m_game->m_player->m_possessedActor.GetIndex() + i) % ((int)m_actors.size());

		//Filter nulls
		if (m_actors[index] != nullptr && m_actors[index]->m_definition.m_canBePossessed && m_actors[index]->m_isAI)
		{
			m_game->m_player->Possess(m_actors[index]->m_handle);
			break;
		}
	}
}

bool Map::IsPositionInBounds(const Vec3& position) const
{
	FloatRange xRange = FloatRange(0.f, (float)m_dimensions.x + 1.f);
	FloatRange yRange = FloatRange(0.f, (float)m_dimensions.y + 1.f);
	return xRange.IsOnRange(position.x) && yRange.IsOnRange(position.y);
}

IntVec2 Map::GetCoordFromPosition(const Vec3& position) const
{
	return IntVec2((int)position.x, (int)position.y);
}

bool Map::AreCoordsInBounds(int x, int y) const
{
	if (m_dimensions.x >= x && 0 <= x && m_dimensions.y >= y && 0 <= y)
	{
		return true;
	}
	return false;
}

const Tile* Map::GetTile(int x, int y) const
{
	int index = (y * m_dimensions.y) + x;
	float maxIndex = (float)(m_dimensions.x - 1) + (float)((m_dimensions.y - 1) * (m_dimensions.y));
	float indexF = GetClamped((float)index, 0.f, maxIndex);
	return &m_tiles[(int)indexF];
}

void Map::Update()
{
	UpdateLightBuffer();
	UpdateActors();
	DeleteDestroyedActors();
}

void Map::UpdateLightBuffer()
{
	m_lightConstants.c_sunDirection = m_game->m_lightDirection.GetNormalized();
	m_lightConstants.c_sunIntensity = m_game->m_lightIntensity;
	m_lightConstants.c_ambientIntensity = m_game->m_ambientIntensity;
	g_theRenderer->CopyCPUToGPU(&m_lightConstants, sizeof(LightConstants), m_lightBuffer);
	ClearPointLights();
}

void Map::UpdateActors()
{
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr)
		{
			m_actors[i]->Update();
		}
	}
	CollideActors();
	CollideActorsWithMap();
}

void Map::UpdateAllActorVerts()
{
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr)
		{
			m_actors[i]->UpdateVerts();
		}
	}
}

void Map::CollideActors()
{
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr && m_actors[i]->m_definition.m_collisionElement.m_collidesWithActors)
		{
			for (int j = 0; j < m_actors.size(); j++)
			{
				if (i != j && m_actors[j] != nullptr && m_actors[j]->m_definition.m_collisionElement.m_collidesWithActors)
				{
					CollideActors(m_actors[i], m_actors[j]);
				}
			}
		}
	}
}

void Map::CollideActors(Actor* a, Actor* b)
{
	bool collided = false;
	Vec2 aCenterXY = Vec2(a->m_position.x, a->m_position.y);
	Vec2 bCenterXY = Vec2(b->m_position.x, b->m_position.y);
	FloatRange aRange = FloatRange(a->m_position.z, a->m_position.z + a->m_height-.001f);
	FloatRange bRange = FloatRange(b->m_position.z, b->m_position.z + b->m_height-.001f);

	bool aOwnsb = b->m_owningActor == a->m_handle;
	bool bOwnsa = a->m_owningActor == b->m_handle;
	bool aIsDead = a->m_isDead;
	bool bIsDead = b->m_isDead;

	if (aRange.IsOverlappingWith(bRange) && !aOwnsb && !bOwnsa && !aIsDead && !bIsDead)
	{
		if (a->m_isStatic && !b->m_isStatic)
		{
			bool thisImpact = PushDiscOutOfDisc2D(bCenterXY, b->m_radius, aCenterXY, a->m_radius);
			b->m_position = Vec3(bCenterXY.x, bCenterXY.y, b->m_position.z);
			if (!collided)
			{
				collided = thisImpact;
			}
		}
		else if (!a->m_isStatic && b->m_isStatic)
		{
			bool thisImpact = PushDiscOutOfDisc2D(aCenterXY, a->m_radius, bCenterXY, b->m_radius);
			a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
			if (!collided)
			{
				collided = thisImpact;
			}
		}
		else
		{
			bool thisImpact = PushDiscsOutOfEachOther2D(aCenterXY, a->m_radius, bCenterXY, b->m_radius);
			a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
			b->m_position = Vec3(bCenterXY.x, bCenterXY.y, b->m_position.z);
			if (!collided)
			{
				collided = thisImpact;
			}
		}
	}

	if (collided)
	{
		a->OnCollide(b);
		b->OnCollide(a);
	}
}

void Map::CollideActorsWithMap()
{
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr)
		{
			for (int j = 0; j < m_actors.size(); j++)
			{
				CollideActorWithMap(m_actors[i]);
			}
		}
	}
}

void Map::CollideActorWithMap(Actor* a)
{
	bool didImpact = false;
	if (!a->m_definition.m_collisionElement.m_collidesWithWorld || !IsPositionInBounds(a->m_position))
	{
		return;
	}
	IntVec2 tileCoordinate = GetCoordFromPosition(a->m_position);
	const Tile* up = GetTile(tileCoordinate.x, tileCoordinate.y + 1);
	const Tile* down = GetTile(tileCoordinate.x, tileCoordinate.y - 1);
	const Tile* left = GetTile(tileCoordinate.x - 1, tileCoordinate.y);
	const Tile* right = GetTile(tileCoordinate.x + 1, tileCoordinate.y);
	const Tile* upLeft = GetTile(tileCoordinate.x - 1, tileCoordinate.y + 1);
	const Tile* downLeft = GetTile(tileCoordinate.x - 1, tileCoordinate.y - 1);
	const Tile* upRight = GetTile(tileCoordinate.x + 1, tileCoordinate.y + 1);
	const Tile* downRight = GetTile(tileCoordinate.x + 1, tileCoordinate.y - 1);

	Vec2 aCenterXY = Vec2(a->m_position.x, a->m_position.y);

	if (up->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 upBoundsXY = AABB2(Vec2(up->m_bounds.m_mins.x, up->m_bounds.m_mins.y), Vec2(up->m_bounds.m_maxs.x, up->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, upBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (down->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 downBoundsXY = AABB2(Vec2(down->m_bounds.m_mins.x, down->m_bounds.m_mins.y), Vec2(down->m_bounds.m_maxs.x, down->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, downBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (left->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 leftBoundsXY = AABB2(Vec2(left->m_bounds.m_mins.x, left->m_bounds.m_mins.y), Vec2(left->m_bounds.m_maxs.x, left->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, leftBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (right->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 rightBoundsXY = AABB2(Vec2(right->m_bounds.m_mins.x, right->m_bounds.m_mins.y), Vec2(right->m_bounds.m_maxs.x, right->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, rightBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (upRight->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 upBoundsXY = AABB2(Vec2(upRight->m_bounds.m_mins.x, upRight->m_bounds.m_mins.y), Vec2(upRight->m_bounds.m_maxs.x, upRight->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, upBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (upLeft->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 downBoundsXY = AABB2(Vec2(upLeft->m_bounds.m_mins.x, upLeft->m_bounds.m_mins.y), Vec2(upLeft->m_bounds.m_maxs.x, upLeft->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, downBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (downLeft->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 leftBoundsXY = AABB2(Vec2(downLeft->m_bounds.m_mins.x, downLeft->m_bounds.m_mins.y), Vec2(downLeft->m_bounds.m_maxs.x, downLeft->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, leftBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}
	if (downRight->m_tileDef.GetIsSolid())
	{
		bool collided;
		AABB2 rightBoundsXY = AABB2(Vec2(downRight->m_bounds.m_mins.x, downRight->m_bounds.m_mins.y), Vec2(downRight->m_bounds.m_maxs.x, downRight->m_bounds.m_maxs.y));
		collided = PushDiscOutOfAABB2D(aCenterXY, a->m_radius, rightBoundsXY);
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, a->m_position.z);
		if (collided)
		{
			didImpact = collided;
		}
	}

	FloatRange aRange = FloatRange(a->m_position.z, a->m_position.z + a->m_height);

	if (aRange.IsOnRange(0.f))
	{
		bool collided;
		float newZ =  0.f;
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, newZ);
		collided = true;
		if (collided)
		{
			didImpact = collided;
		}
	}

	if (aRange.IsOnRange(m_definition->m_ceilingHeight))
	{
		bool collided;
		float newZ = 1.f - a->m_height;
		a->m_position = Vec3(aCenterXY.x, aCenterXY.y, newZ);
		collided = true;
		if (collided)
		{
			didImpact = collided;
		}
	}

	if (didImpact)
	{
		a->OnCollide();
	}
}

std::vector<Actor*> Map::GetActorsInSector(Actor* actorReference, float sectorAngle, float radius)
{
	std::vector<Actor*> returnedActors;
	for (int i = 0; i < (int)m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr && m_actors[i]->m_handle != actorReference->m_handle)
		{
			if (IsPointInsideDirectedSector2D(m_actors[i]->m_position.GetFlattenedXY(), actorReference->m_position.GetFlattenedXY(),
				actorReference->m_orientation.GetForwardNormal().GetFlattenedXY(), sectorAngle, radius))
			{
				returnedActors.push_back(m_actors[i]);
			}
		}
	}
	return returnedActors;
}

void Map::Render() const
{
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetStatesIfChanged();
	g_theRenderer->BindTexture(&m_skyBoxSheet->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_skyBoxVerts);

	Vec3 lightBasis = m_game->m_lightDirection.GetNormalized();
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->SetStatesIfChanged();
	g_theRenderer->DrawLitIndexBuffer(m_vBuffer, m_iBuffer, m_lightBuffer, (int)m_vertIndexes.size());
	RenderActors();
	DebugRenderWorld(m_game->m_worldCamera);
}

void Map::RenderActors() const
{
	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr)
		{
			m_actors[i]->Render();
		}
	}
}

int Map::AddPointLightToMap(Vec3 const& location, float intensity, Rgba8 const& color)
{
	PointLight point;
	float rgba[4];
	color.GetAsFloats(rgba);
	point.LightColor[0] = rgba[0];
	point.LightColor[1] = rgba[1];
	point.LightColor[2] = rgba[2];
	point.LightColor[3] = rgba[3];

	point.LightPosition = location;
	point.Intensity = intensity;

	m_lightConstants.c_pointLightList[m_lightConstants.c_numPointLights] = point;
	m_lightConstants.c_numPointLights++;
	return m_lightConstants.c_numPointLights - 1;
}

void Map::ClearPointLights()
{
	m_lightConstants.c_numPointLights = 0;
}

void Map::OnDemonKilled()
{
	m_game->m_enemiesRemainingOnMap--;
	if (m_game->m_player)
	{
		m_game->m_player->m_kills++;
	}
}

void Map::OnPlayerKilled()
{
}

RaycastResult3D Map::RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor*& hit, Actor* owner) const
{
	RaycastResult3D resultTotal;
	resultTotal.m_didImpact = false;
	resultTotal.m_impactPos = start + direction * distance;
	resultTotal.m_rayMaxLength = distance;
	resultTotal.m_impactDist = 10000.f;
	if (distance == 0.f)
	{
		return resultTotal;
	}
	Vec3 ray3D = direction * distance;

	RaycastResult3D resultWorldZ = RaycastWorldZ(start, direction, distance);
	RaycastResult3D resultWorldXY = RaycastWorldXY(start, direction, distance);
	Actor* temp = nullptr;
	RaycastResult3D resultActors = RaycastWorldActors(start, direction, distance, temp, owner);

	if (resultWorldZ.m_impactDist < resultWorldXY.m_impactDist)
	{
		resultTotal = resultWorldZ;
	}
	else
	{
		resultTotal = resultWorldXY;
	}

	if (resultActors.m_impactDist < resultTotal.m_impactDist)
	{
		resultTotal = resultActors;
		hit = temp;
	}

	return resultTotal;
}

RaycastResult3D Map::RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D resultTotal;
	resultTotal.m_didImpact = false;
	Vec3 end = start + direction * distance;
	resultTotal.m_impactPos = end;
	resultTotal.m_rayMaxLength = distance;
	resultTotal.m_impactDist = 9999999.f;	

	Vec3 ray3D = direction * distance;
	Vec3 ray2D = Vec3(ray3D.x, ray3D.y, 0.f);
	Vec3 currentPos = start;
	IntVec2 currentCoord = GetCoordFromPosition(start);

	
	if (IsPositionInBounds(currentPos) && GetTile(currentCoord.x, currentCoord.y)->m_tileDef.GetIsSolid())
	{
		resultTotal.m_didImpact = true;
		resultTotal.m_impactNormal = -direction;
		resultTotal.m_impactPos = start;
		resultTotal.m_impactDist = 0.f;
		return resultTotal;
	}

	float stepX;
	float stepY;
	float tMaxX;
	float tMaxY;
	if (end.x - start.x > 0.f)
	{
		stepX = 1.f;
		tMaxX = RangeMap((float)currentCoord.x + stepX, start.x, end.x, 0.f, 1.f);
	}
	else if (end.x - start.x < 0.f)
	{
		stepX = -1.f;
		tMaxX = RangeMap((float)currentCoord.x, start.x, end.x, 0.f, 1.f);
	}
	else
	{
		stepX = 0.f;
		tMaxX = RangeMap((float)currentCoord.x, start.x, end.x, 0.f, 1.f);
	}
	if (end.y - start.y > 0.f)
	{
		stepY = 1.f;
		tMaxY = RangeMap((float)currentCoord.y + stepY, start.y, end.y, 0.f, 1.f);
	}
	else if (end.y - start.y < 0.f)
	{
		stepY = -1.f;
		tMaxY = RangeMap((float)currentCoord.y, start.y, end.y, 0.f, 1.f);
	}
	else
	{
		stepY = 0.f;
		tMaxY = RangeMap((float)currentCoord.y, start.y, end.y, 0.f, 1.f);
	}

	float tDeltaX = (stepX != 0.f) ? RangeMap(start.x + stepX, start.x, end.x, 0.f, 1.f) : 9999999.f;
	float tDeltaY = (stepY != 0.f) ? RangeMap(start.y + stepY, start.y, end.y, 0.f, 1.f) : 9999999.f;

	while (tMaxX <= 1.f || tMaxY <= 1.f)
	{
		if (tMaxX < tMaxY)
		{
			if (stepX == 0.f)
			{
				break;
			}
			currentPos = currentPos + Vec3(stepX, 0.f, 0.f);
			currentCoord = GetCoordFromPosition(currentPos);
			if (IsPositionInBounds(currentPos) && GetTile(currentCoord.x, currentCoord.y)->m_tileDef.GetIsSolid())
			{
				resultTotal.m_didImpact = true;
				resultTotal.m_impactNormal = Vec3(-stepX, 0.f, 0.f);
				resultTotal.m_impactPos = (tMaxX * ray3D) + start;
				resultTotal.m_impactDist = (tMaxX * ray3D).GetLength();
				return resultTotal;
			}
			tMaxX = tMaxX + (tDeltaX);	
		}
		else
		{
			if (stepY == 0.f)
			{
				break;
			}
			currentPos = currentPos + Vec3(0.f, stepY, 0.f);
			currentCoord = GetCoordFromPosition(currentPos);
			if (IsPositionInBounds(currentPos) && GetTile(currentCoord.x, currentCoord.y)->m_tileDef.GetIsSolid())
			{
				resultTotal.m_didImpact = true;
				resultTotal.m_impactNormal = Vec3(0.f, -stepY, 0.f);
				resultTotal.m_impactPos = (tMaxY * ray3D) + start;
				resultTotal.m_impactDist = (tMaxY * ray3D).GetLength();
				return resultTotal;
			}
			tMaxY = tMaxY + (tDeltaY);
		}
	}

	return resultTotal;
}

RaycastResult3D Map::RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const
{
	RaycastResult3D resultTotal;
	resultTotal.m_didImpact = false;
	resultTotal.m_impactPos = start + direction * distance;
	resultTotal.m_rayMaxLength = distance;
	resultTotal.m_impactDist = 9999999.f;

	if (direction.z == 0.f)
	{
		return resultTotal;
	}
	
	Vec3 ray3D = direction * distance;

	if (direction.z > 0.f) //Check Ceiling
	{
		float t = RangeMap(1.f, start.z, resultTotal.m_impactPos.z, 0.f, m_definition->m_ceilingHeight);
		if (t > 1.f)
		{
			return resultTotal;
		}
		else
		{
			Vec3 position = start + direction * (distance * t);
			if (!IsPositionInBounds(position))
			{
				return resultTotal;
			}
			resultTotal.m_didImpact = true;
			resultTotal.m_impactPos = position;
			resultTotal.m_rayMaxLength = distance;
			resultTotal.m_impactDist = (distance * t);
			resultTotal.m_impactNormal = Vec3(0.f, 0.f, -1.f);
			return resultTotal;
		}
	}
	else //Check Floor
	{
		float t = RangeMap(0.f, start.z, resultTotal.m_impactPos.z, 0.f, 1.f);
		if (t > 1.f)
		{
			return resultTotal;
		}
		else
		{
			Vec3 position = start + direction * (distance * t);
			if (!IsPositionInBounds(position))
			{
				return resultTotal;
			}
			resultTotal.m_didImpact = true;
			resultTotal.m_impactPos = position;
			resultTotal.m_rayMaxLength = distance;
			resultTotal.m_impactDist = (distance * t);
			resultTotal.m_impactNormal = Vec3(0.f, 0.f, 1.f);
			return resultTotal;
		}
	}
}

RaycastResult3D Map::RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor*& hit, Actor* owner) const
{
	RaycastResult3D resultTotal;
	resultTotal.m_didImpact = false;
	resultTotal.m_impactPos = start + direction * distance;
	resultTotal.m_rayMaxLength = distance;
	resultTotal.m_impactDist = 9999999.f;

	Vec3 ray3D = direction * distance;

	for (int i = 0; i < m_actors.size(); i++)
	{
		if (m_actors[i] != nullptr && (m_actors[i] != owner || owner == nullptr))
		{
			RaycastResult3D result = RaycastVsCylinderZ3D(start, direction, distance,
				m_actors[i]->m_position + Vec3(0.f, 0.f, m_actors[i]->m_height * .5f), 
				FloatRange(m_actors[i]->m_position.z, m_actors[i]->m_position.z + m_actors[i]->m_height), 
				m_actors[i]->m_radius);

			if (result.m_didImpact && result.m_impactDist < resultTotal.m_impactDist)
			{
				resultTotal = result;
				hit = m_actors[i];
			}
		}
	}
	return resultTotal;
}
