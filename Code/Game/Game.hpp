#pragma once
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/VertexUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/Player.hpp"
#include "Game/Map.hpp"
#include "Game/WeaponDefinition.hpp"
#include "GameCommon.hpp"
#include <vector>

class Player;

enum class GameState
{
	NONE,
	ATTRACT,
	MENU,
	SURVIVALMENU,
	LOBBY,
	SURVIVAL,
	MULTIPLAYER,
	COUNT
};

class Game
{
public:
	Game();
	~Game();

	void Startup();
	void Update();
	void Render();
	void Shutdown();

	void StartGameMode();
	void ShutdownGameMode();
	void HandleInputGame();
	void UpdateGameMode();
	void OnMultiplayerGameEnd(int winnerIndex);
	void RenderGameModeWorld() const;
	void RenderGameModeScreen() const;
	void RenderHUDElements() const;

	void StartAttractMode();
	void ShutdownAttractMode();
	void HandleInputAttract();
	void UpdateAttractMode();
	void RenderAttractMode() const;

	void StartMenuMode();
	void UpdateMenuMode();
	void HandleMenuInput();
	void RenderMenuMode();
	void ShutdownMenuMode();

	void StartSurvivalMenuMode();
	void UpdateSurvivalMenuMode();
	void RenderSurvivalMenuMode();
	void ShutdownSurvivalMenuMode();

	void StartSurvivalMode();
	void UpdateSurvivalMode();
	void RenderSurvivalMode();
	void RenderSurvivalModeScreen() const;
	void ShutdownSurvivalMode();
	void OnWaveClear();

	void StartLobbyMode();
	void ShutdownLobbyMode();
	void HandleInputLobby();
	void UpdateLobbyMode();
	void RenderLobbyMode() const;

	void OnCloseGameMode();
	void OnOpenGameMode(GameState const& newGameState);
	void OnMenuInputUp();
	void OnMenuInputDown();

	void AddPlayer(int controllerIndex);
	void RemovePlayer(int controllerIndex);
	void SetViewPortsForPlayers();
	Camera GetActiveWorldCamera() const;

	void InitializeMaps();
	void InitializeTileDefs();
	void InitializeActors();
	void InitializeProjectileActor();
	void InitializeWeapons();
	void InitializeActor();
	void CreateAllSounds();

	GameState				m_gameState = GameState::ATTRACT;
	SoundPlaybackID			m_currentSongID;
	bool					m_isGameOver = false;
	Timer*					m_timerForReset = new Timer(5.f, g_theSystemClock);

	Camera					m_screenCamera;
	Camera					m_worldCamera;

	SpriteSheet*				m_spriteSheet;
	std::vector<Vertex_PCU>		m_verts;
	std::vector<Vertex_PCU>		m_verts2;
	std::vector<Vertex_PCUTBN>	m_vertsTBN;
	std::vector<unsigned int>	m_indexes;
	Vec3						m_lightDirection = Vec3(2.f, 1.f, -1.f);
	float						m_lightIntensity = .35f;
	float						m_ambientIntensity = .25f;

	Map*						 m_map = nullptr;
	std::vector<MapDefinition*>	 m_mapDefs;
	std::vector<TileDefinition*> m_tileDefs;
	std::vector<ActorDefinition*> m_actorDefs;
	std::vector<WeaponDefinition*> m_weaponDefs;
	std::vector<Player*>		 m_playerList;
	Player*						 m_player = nullptr;

	bool						 m_keyboardActive = false;
	bool						 m_controllerActive = false;
	int							 m_nextPlayerIndex = -1;

	//Multiplayer
	int							m_winnerIndex = -1;

	//Survival
	int							m_waveNumber = 1;
	int							m_waveBudget = 1;
	int							m_enemiesRemainingOnMap = 0;
	constexpr static int		m_maxEnemisOnMap = 15;
	float						m_waveScaleFactor = 1.5f;
	Timer*						m_waveResetTimer;
	std::vector<ActorDefinition*> m_enemyDefs;

	//Menu Variables
	int							m_menuOptionIndex = 0; //0 = Survival, 1 = Multiplayer, 2 = Quit
	Timer*						m_menuSynchroTimer = new Timer(.2f, g_theSystemClock);
	float						m_revolverStartOrientation = 90.f;
	float						m_revolverOrientation = 90.f;
	float						m_revolverTargetOrientation = 90.f;
	Vec2						m_revolverCenter = Vec2(0.f, SCREEN_CENTER_Y);
	float						m_revolverRadius = SCREEN_SIZE_Y * .6f;
	Vec2						m_pivotLeftCoordOfSurvivalButton;
	Vec2						m_pivotLeftCoordOfMultiplayerButton;
	Vec2						m_pivotLeftCoordOfQuitButton;
	float						m_textHeight = SCREEN_SIZE_Y * .15f;
private:
	float m_ringSize = 50.f;
	float m_elapsedTime = 0.f;
};