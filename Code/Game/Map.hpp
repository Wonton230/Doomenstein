#pragma once
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/Tile.hpp"
#include "Game/ActorHandle.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

class Tile;
class Texture;
class Shader;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class MapDefinition;
class Game;
class Actor;
struct AABB3;
struct AABB2;
struct Vec3;
class Player;
class Actor;

class Map
{
public:
	Map();
	Map(Game* game, const MapDefinition* definition);
	~Map();

	//Creation Functions
	void CreateTiles();
	void CreateGeometry();
	void AddGeometryForWall(const AABB3& bounds, const AABB2& UVs, const AABB2& UV2s);
	void AddGeometryForFloor(const AABB3& bounds, const AABB2& UVs);
	void AddGeometryForCeiling(const AABB3& bounds, const AABB2& UVs);
	void CreateSkybox();
	void CreateBuffers();

	//Actor Functions
	Actor* SpawnPlayer(Player* possessingPlayer, Vec3 const& location);
	void   KillPlayer(Player* possessingPlayer);
	Actor* SpawnActor(const SpawnInfo& spawnInfo);
	void   KillAIActor(const ActorHandle handle);
	Actor* GetActorByHandle(const ActorHandle handle) const;
	void   DeleteDestroyedActors();
	Actor* GetClosestVisibleEnemy(Actor* searchingActor) const;
	void   DebugPossessNext();

	//Tile Functions
	bool		IsPositionInBounds(const Vec3& position) const;
	IntVec2		GetCoordFromPosition(const Vec3& position) const;
	bool		AreCoordsInBounds(int x, int y) const;
	const Tile* GetTile(int x, int y) const;

	//Updates
	void Update();
	void UpdateLightBuffer();
	void UpdateActors();
	void UpdateAllActorVerts();

	//Renders
	void Render() const;
	void RenderActors() const;
	int  AddPointLightToMap(Vec3 const& location, float intensity, Rgba8 const& color);
	void ClearPointLights();

	//Events
	void OnDemonKilled();
	void OnPlayerKilled();

	//Collision
	void CollideActors();
	void CollideActors(Actor* a, Actor* b);
	void CollideActorsWithMap();
	void CollideActorWithMap(Actor* a);
	std::vector<Actor*> GetActorsInSector(Actor* actorReference, float sectorAngle, float radius);

	//Raycasts
	RaycastResult3D RaycastAll(const Vec3& start, const Vec3& direction, float distance, Actor*& hit, Actor* owner = nullptr) const;
	RaycastResult3D RaycastWorldXY(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldZ(const Vec3& start, const Vec3& direction, float distance) const;
	RaycastResult3D RaycastWorldActors(const Vec3& start, const Vec3& direction, float distance, Actor*& hit, Actor* owner = nullptr) const;

	//Game Management
	Game* m_game = nullptr;
	const MapDefinition* m_definition;

	//Skybox Stuff
	SpriteSheet* m_skyBoxSheet = nullptr;
	std::vector<Vertex_PCU> m_skyBoxVerts;
	AABB3		 m_skyBoxLocalBounds = AABB3(Vec3(-35.f, -35.f, -35.f), Vec3(35.f, 35.f, 35.f));


protected:
	std::vector<Tile>		m_tiles;
	IntVec2					m_dimensions;

	std::vector<Actor*>		m_actors;
	unsigned int			m_nextActorUID = 0;

	std::vector<Vertex_PCUTBN> m_verts;
	std::vector<unsigned int> m_vertIndexes;
	Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	VertexBuffer* m_vBuffer = nullptr;
	IndexBuffer* m_iBuffer = nullptr;
	ConstantBuffer* m_lightBuffer = nullptr;
	LightConstants m_lightConstants;
};