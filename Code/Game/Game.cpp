#include "Game.hpp"
#include "Game/App.hpp"
#include "Game/Player.hpp"
#include "game/AI.hpp"
#include "GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Math/OBB2.hpp"

extern App*						g_theApp;
extern Renderer*				g_theRenderer;
extern RandomNumberGenerator*	g_rng;
extern InputSystem*				g_theInputSystem;
extern AudioSystem*				g_theAudioSystem;
extern Clock*					g_theSystemClock;
extern DevConsole*				g_theDevConsole;
extern Window*					g_theWindow;
extern BitmapFont*				g_testFont;
extern NamedStrings*			g_gameConfigBlackboard;

Clock* g_theGameClock = nullptr;

Game::Game()
{
  	CreateAllSounds();
	SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("mainMenuMusic", "default"), 2);
	m_currentSongID = g_theAudioSystem->StartSound(id, true, g_gameConfigBlackboard->GetValue("musicVolume", .1f));

	m_screenCamera = Camera();
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

Game::~Game()
{
}

void Game::Startup()
{
	g_theGameClock = new Clock(*g_theSystemClock);

	switch (m_gameState)
	{
	case GameState::NONE:
		break;
	case GameState::ATTRACT:
		StartAttractMode();
		break;
	case GameState::MENU:
		StartMenuMode();
		break;
	case GameState::SURVIVALMENU:
		StartSurvivalMenuMode();
		break;
	case GameState::LOBBY:
		StartLobbyMode();
		break;
	case GameState::SURVIVAL:
		StartSurvivalMode();
		break;
	case GameState::MULTIPLAYER:
		StartGameMode();
		break;
	case GameState::COUNT:
		break;
	default:
		break;
	}
}

void Game::Update()
{
	switch (m_gameState)
	{
	case GameState::NONE:
		break;
	case GameState::ATTRACT:
		UpdateAttractMode();
		break;
	case GameState::MENU:
		UpdateMenuMode();
		break;
	case GameState::SURVIVALMENU:
		UpdateSurvivalMenuMode();
		break;
	case GameState::LOBBY:
		UpdateLobbyMode();
		break;
	case GameState::SURVIVAL:
		UpdateSurvivalMode();
		break;
	case GameState::MULTIPLAYER:
		UpdateGameMode();
		break;
	case GameState::COUNT:
		break;
	default:
		break;
	}
}

void Game::Render()
{
	switch (m_gameState)
	{
	case GameState::NONE:
		break;
	case GameState::ATTRACT:
		RenderAttractMode();
		break;
	case GameState::MENU:
		RenderMenuMode();
		break;
	case GameState::SURVIVALMENU:
		RenderSurvivalMenuMode();
		break;
	case GameState::LOBBY:
		RenderLobbyMode();
		break;
	case GameState::SURVIVAL:
		RenderSurvivalMode();
		break;
	case GameState::MULTIPLAYER:
		g_theRenderer->ClearScreen(Rgba8(80, 80, 80, 255));

		// World Render
		for (int i = 0; i < (int)m_playerList.size(); i++)
		{
			if (m_playerList[i] != nullptr)
			{
				m_player = m_playerList[i];
				m_worldCamera = *m_playerList[i]->m_camera;
				m_map->UpdateAllActorVerts();
				g_theRenderer->BeginCamera(m_worldCamera);
				g_theRenderer->SetStatesIfChanged();
				RenderGameModeWorld();
				g_theRenderer->EndCamera(m_worldCamera);
			}
		}

		// Screen Render
		for (int i = 0; i < (int)m_playerList.size(); i++)
		{
			if (m_playerList[i] != nullptr)
			{
				m_player = m_playerList[i];
				m_screenCamera.SetViewPort(m_playerList[i]->m_camera->GetViewPort());
				g_theRenderer->BeginCamera(m_screenCamera);
				RenderGameModeScreen();
				g_theRenderer->EndCamera(m_screenCamera);
			}
		}
		break;
	case GameState::COUNT:
		break;
	default:
		break;
	}
}

void Game::OnCloseGameMode()
{
	switch (m_gameState)
	{
	case GameState::ATTRACT:
	{
		ShutdownAttractMode();
		break;
	}
	case GameState::MENU:
	{
		ShutdownMenuMode();
		break;
	}
	case GameState::SURVIVALMENU:
	{
		ShutdownSurvivalMenuMode();
		break;
	}
	case GameState::LOBBY:
	{
		ShutdownLobbyMode();
		break;
	}
	case GameState::SURVIVAL:
	{
		ShutdownSurvivalMode();
		break;
	}
	case GameState::MULTIPLAYER:
	{
		ShutdownGameMode();
		break;
	}
	case GameState::NONE:
	case GameState::COUNT:
	default:
		break;
	}
}

void Game::OnOpenGameMode(GameState const& newGameState)
{
	m_gameState = newGameState;
	Startup();
}

void Game::OnMenuInputUp()
{
	m_menuSynchroTimer->Start();
	m_revolverTargetOrientation = m_revolverOrientation + 60.f;
}

void Game::OnMenuInputDown()
{
	m_menuSynchroTimer->Start();
	m_revolverTargetOrientation = m_revolverOrientation - 60.f;
}

void Game::StartGameMode()
{
	g_theAudioSystem->StopSound(m_currentSongID);
	SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("gameMusic", "default"), 2);
	m_currentSongID = g_theAudioSystem->StartSound(id, true, g_gameConfigBlackboard->GetValue("musicVolume", .1f));

	//Renderer Setup
	g_theRenderer->CreateOrGetShader("Diffuse", VertexType::PCUTBN);
	g_theRenderer->BindShader(g_theRenderer->CreateOrGetShader("Diffuse", VertexType::PCUTBN));

	//Read tileDefXML
	InitializeTileDefs();
	//Read mapDefXML
	InitializeMaps();
	//Read actorDefXML
	InitializeActors();

	m_map = new Map(this, m_mapDefs[1]);

	//Player Setup
	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			int randomIndex = g_rng->RollRandomIntInRange(1, (int)(m_map->m_definition->m_spawnInfo.size() / 2));
			Vec3 position;
			if (i == 0)
			{
				position = m_map->m_definition->m_spawnInfo[(randomIndex * 2) - 1]->m_position;
			}
			else
			{
				position = m_map->m_definition->m_spawnInfo[(randomIndex * 2) - 2]->m_position;
			}
			m_map->SpawnPlayer(m_playerList[i], position);
		}
	}
	SetViewPortsForPlayers();
	g_theAudioSystem->SetNumListeners((int)m_playerList.size());

	for (int i = 0; i < (int)m_map->m_definition->m_spawnInfo.size(); i++)
	{
		m_map->SpawnActor(*m_map->m_definition->m_spawnInfo[i]);
	}
}

void Game::HandleInputGame()
{
	XboxController controller = g_theInputSystem->GetController(0);

	if (g_theInputSystem->WasKeyJustPressed('P'))
	{
		g_theGameClock->TogglePause();
	}

	if (g_theInputSystem->IsKeyDown('T') && !g_theGameClock->IsPaused())
	{
		g_theGameClock->SetTimeScale(0.1f);
	}

	if (!g_theInputSystem->IsKeyDown('T') && !g_theGameClock->IsPaused())
	{
		g_theGameClock->SetTimeScale(1.0f);
	}

	if (g_theInputSystem->WasKeyJustPressed('O'))
	{
		g_theGameClock->StepSingleFrame();
	}

	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC) || controller.GetButton(XBOX_BUTTON_BACK).m_isPressed)
	{
		Shutdown();
		ShutdownGameMode();
		m_gameState = GameState::ATTRACT;
		Startup();
	}
}

void Game::Shutdown()
{
	m_verts.clear();
	DebugRenderClear();
}

void Game::UpdateGameMode()
{
	m_verts.clear();
	m_indexes.clear();
	m_vertsTBN.clear();

	HandleInputGame();

	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			m_playerList[i]->Update();
			m_playerList[i]->UpdateCamera();
			Vec3 forward;
			Vec3 up;
			Vec3 left;
			m_playerList[i]->m_playerCamOrientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
			g_theAudioSystem->UpdateListener(i, m_playerList[i]->m_playerCamPosition, forward, up);
		}
	}

	if (m_map != nullptr)
	{
		m_map->Update();

		for (int i = 0; i < (int)m_playerList.size(); i++)
		{
			if (m_playerList[i] != nullptr && m_playerList[i]->GetActor() == nullptr)
			{
				int randomIndex = g_rng->RollRandomIntInRange(1, (int)(m_map->m_definition->m_spawnInfo.size() / 2));
				Vec3 position;
				if (i == 0)
				{
					position = m_map->m_definition->m_spawnInfo[(randomIndex * 2) - 1]->m_position;
				}
				else
				{
					position = m_map->m_definition->m_spawnInfo[(randomIndex * 2) - 2]->m_position;
				}
				m_map->SpawnPlayer(m_playerList[i], position);
			}
		}
	}

	if (m_timerForReset->IsStopped())
	{
		for (int i = 0; i < (int)m_playerList.size(); i++)
		{
			if (m_playerList[i] != nullptr)
			{
				if (m_playerList[i]->m_kills >= 5)
				{
					OnMultiplayerGameEnd(i);
				}
			}
		}
	}

	if (m_timerForReset->HasPeriodElapsed() && !m_timerForReset->IsStopped())
	{
		OnCloseGameMode();
		delete m_timerForReset;
		m_timerForReset = new Timer(5.f, g_theSystemClock);
		m_winnerIndex = -1;
		OnOpenGameMode(GameState::ATTRACT);
	}
}

void Game::OnMultiplayerGameEnd(int winnerIndex)
{
	m_timerForReset->Start();
	m_winnerIndex = winnerIndex;
}

void Game::RenderGameModeWorld() const
{
	m_map->Render();
}

void Game::RenderGameModeScreen() const
{
	g_theRenderer->SetStatesIfChanged();
	RenderHUDElements();
}

void Game::RenderHUDElements() const
{
	if (m_player != nullptr && m_player->m_currentControlMode != ControlMode::CAMERA)
	{
		if (m_player->GetActor() != nullptr)
		{
			if (m_player->GetActor()->m_equippedWeapon != nullptr)
			{
				m_player->GetActor()->m_equippedWeapon->Render();
			}
			m_player->Render();

			if (m_winnerIndex > -1)
			{
				std::vector<Vertex_PCU> tempArray;
				g_testFont->AddVertsForTextInBox2D(tempArray, Stringf("Player %i Wins!", m_winnerIndex + 1), AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), SCREEN_SIZE_Y * .2f, Rgba8::GREEN, 1.f);
				g_theRenderer->BeginCamera(m_screenCamera);
				g_theRenderer->BindTexture(&g_testFont->GetTexture());
				g_theRenderer->DrawVertexArray(tempArray);
				g_theRenderer->EndCamera(m_screenCamera);
			}
		}
	}
}

void Game::ShutdownGameMode()
{
	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			delete m_playerList[i];
			m_playerList[i] = nullptr;
		}
	}
	m_nextPlayerIndex = -1;
	m_keyboardActive = false;
	m_controllerActive = false;
	m_screenCamera.SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_verts.clear();
	delete m_map;
	m_map = nullptr;
	m_actorDefs.clear();
	m_weaponDefs.clear();
	m_mapDefs.clear();
	DebugRenderClear();
}

void Game::StartAttractMode()
{
	g_theAudioSystem->StopSound(m_currentSongID);
	SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("mainMenuMusic", "default"), 2);
	m_currentSongID = g_theAudioSystem->StartSound(id, true, g_gameConfigBlackboard->GetValue("musicVolume", .1f));

	m_gameState = GameState::ATTRACT;
	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			delete m_playerList[i];
			m_playerList[i] = nullptr;
		}
	}
	m_nextPlayerIndex = -1;
	m_keyboardActive = false;
	m_controllerActive = false;
	m_screenCamera.SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
}

void Game::ShutdownAttractMode()
{
	
}

void Game::HandleInputAttract()
{
	XboxController controller = g_theInputSystem->GetController(0);

	if (g_theInputSystem->WasKeyJustPressed(' ') || controller.GetButton(XBOX_BUTTON_START).m_isPressed)
	{
		OnCloseGameMode();
		OnOpenGameMode(GameState::MENU);
		SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
		g_theAudioSystem->StartSound(id, false);
	}

	if (g_theInputSystem->WasKeyJustPressed('P'))
	{
		g_theGameClock->TogglePause();
	}

	if (g_theInputSystem->IsKeyDown('T'))
	{
		g_theGameClock->SetTimeScale(0.1f);
	}

	if (!g_theInputSystem->IsKeyDown('T') && !g_theGameClock->IsPaused())
	{
		g_theGameClock->SetTimeScale(1.0f);
	}

	if (g_theInputSystem->WasKeyJustPressed('O'))
	{
		g_theGameClock->StepSingleFrame();
	}

	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
	{
		FireEvent("quit");
	}
}

void Game::UpdateAttractMode()
{
	HandleInputAttract();
	m_elapsedTime += static_cast<float>(g_theGameClock->GetDeltaSeconds());
	m_ringSize = abs(CosDegrees(m_elapsedTime * 50) * 500);
}

void Game::RenderAttractMode() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	g_theRenderer->ClearScreen(Rgba8(255, 25, 215, 255));

	g_theRenderer->SetStatesIfChanged();

	g_theRenderer->BindTexture(nullptr);
	DrawDebugRing(Vec2(SCREEN_CENTER_X, SCREEN_CENTER_Y), m_ringSize, 10, Rgba8(0, 255, 0, 255));

	Texture* testTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/AttractScreen.jpg");
	std::vector<Vertex_PCU> testTextureVerts;
	AABB2 texturedAABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y);
	AddVertsForAABB2D(testTextureVerts, texturedAABB2, Rgba8(255, 255, 255, 255));
	g_theRenderer->BindTexture(testTexture);
	g_theRenderer->DrawVertexArray(testTextureVerts);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::StartMenuMode()
{
}

void Game::UpdateMenuMode()
{
	HandleMenuInput();

	float baseAngleDeg = m_revolverOrientation - 90.f;
	const float angleOffset = 60.f;

	float angleSurvival = baseAngleDeg;
	float angleMultiplayer = baseAngleDeg - angleOffset;
	float angleQuit = baseAngleDeg - (2.f * angleOffset);

	// Compute coordinates on circle
	m_pivotLeftCoordOfSurvivalButton = m_revolverCenter + Vec2(CosDegrees(angleSurvival), SinDegrees(angleSurvival)) * m_revolverRadius;
	m_pivotLeftCoordOfMultiplayerButton = m_revolverCenter + Vec2(CosDegrees(angleMultiplayer), SinDegrees(angleMultiplayer)) * m_revolverRadius;
	m_pivotLeftCoordOfQuitButton = m_revolverCenter + Vec2(CosDegrees(angleQuit), SinDegrees(angleQuit)) * m_revolverRadius;

	if (!m_menuSynchroTimer->IsStopped())
	{
		float revolverOrientation = Interpolate(m_revolverStartOrientation, m_revolverTargetOrientation, (float)m_menuSynchroTimer->GetElapsedFraction());
		m_revolverOrientation = revolverOrientation;
	}

	if (m_menuSynchroTimer->HasPeriodElapsed())
	{
		m_menuSynchroTimer->Stop();
		m_revolverStartOrientation = m_revolverOrientation;
	}
}

void Game::HandleMenuInput()
{
	if (g_theInputSystem->WasKeyJustPressed(' ') && m_menuSynchroTimer->HasPeriodElapsed())
	{
		switch (m_menuOptionIndex)
		{
		case 0:
		{
			OnCloseGameMode();
			AddPlayer(-1);
			OnOpenGameMode(GameState::SURVIVAL);
			SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
			g_theAudioSystem->StartSound(id, false);
			break;
		}
		case 1:
		{
			OnCloseGameMode();
			OnOpenGameMode(GameState::LOBBY);
			SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
			g_theAudioSystem->StartSound(id, false);
			break;
		}
		case 2:
		{
			OnCloseGameMode();
			FireEvent("quit");
			break;
		}
		}
	}

	if (g_theInputSystem->WasKeyJustPressed('W'))
	{
		if (m_menuOptionIndex > 0 && m_menuSynchroTimer->HasPeriodElapsed())
		{
			m_menuOptionIndex--;
			SoundID id = g_theAudioSystem->CreateOrGetSound("Data/Audio/Revolver.mp3", 2);
			g_theAudioSystem->StartSound(id, false, 1.f, 0.f, 2.f);
			OnMenuInputDown();
		}
	}

	if (g_theInputSystem->WasKeyJustPressed('S') && m_menuSynchroTimer->HasPeriodElapsed())
	{
		if (m_menuOptionIndex < 2)
		{
			m_menuOptionIndex++;
			SoundID id = g_theAudioSystem->CreateOrGetSound("Data/Audio/Revolver.mp3", 2);
			g_theAudioSystem->StartSound(id, false, 1.f, 0.f, 2.f);
			OnMenuInputUp();
		}
	}

	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
	{
		OnCloseGameMode();
		OnOpenGameMode(GameState::ATTRACT);
	}
}

void Game::RenderMenuMode()
{
	g_theRenderer->BeginCamera(m_screenCamera);

	g_theRenderer->ClearScreen(Rgba8(50, 50, 50, 255));
	g_theRenderer->SetStatesIfChanged();
	g_theRenderer->BindTexture(nullptr);

	Texture* revolverTexture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/RevolverChamber.png");
	std::vector<Vertex_PCU> testTextureVerts;
	float squareSize = SCREEN_SIZE_Y * .95f;
	Vec2 iBasis = Vec2::MakeFromPolarDegrees(m_revolverOrientation);
	OBB2 texturedBox = OBB2(Vec2(0.f, SCREEN_CENTER_Y), Vec2(squareSize * .5f, squareSize * .5f), iBasis);
	AddVertsForOBB2D(testTextureVerts, texturedBox, Rgba8(255, 50, 50, 255));
	//AddVertsForAABB2D(testTextureVerts, texturedAABB2, Rgba8(255, 255, 255, 255));
	g_theRenderer->BindTexture(revolverTexture);
	g_theRenderer->DrawVertexArray(testTextureVerts);
	g_theRenderer->BindTexture(&g_testFont->GetTexture());

	AABB2 survivalButton = AABB2(m_pivotLeftCoordOfSurvivalButton.x, m_pivotLeftCoordOfSurvivalButton.y - m_textHeight * .5f, SCREEN_SIZE_X, m_pivotLeftCoordOfSurvivalButton.y + m_textHeight * .5f);
	AABB2 multiplayerButton = AABB2(m_pivotLeftCoordOfMultiplayerButton.x, m_pivotLeftCoordOfMultiplayerButton.y - m_textHeight * .5f, SCREEN_SIZE_X, m_pivotLeftCoordOfMultiplayerButton.y + m_textHeight * .5f);
	AABB2 quitButton = AABB2(m_pivotLeftCoordOfQuitButton.x, m_pivotLeftCoordOfQuitButton.y - m_textHeight * .5f, SCREEN_SIZE_X, m_pivotLeftCoordOfQuitButton.y + m_textHeight * .5f);

	std::vector<Vertex_PCU> buttonVerts;
	switch (m_menuOptionIndex)
	{
	case 0:
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Survival Mode", survivalButton, m_textHeight, Rgba8::WHITE, .6f, Vec2(0.f, .5f));
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Versus Mode", multiplayerButton, m_textHeight, Rgba8::GRAY, .6f, Vec2(0.f, .5f));
		//g_testFont->AddVertsForTextInBox2D(buttonVerts, "Exit Game", quitButton, m_textHeight, Rgba8::WHITE, .6f, Vec2(0.f, .5f));
		break;
	case 1:
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Survival Mode", survivalButton, m_textHeight, Rgba8::GRAY, .6f, Vec2(0.f, .5f));
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Versus Mode", multiplayerButton, m_textHeight, Rgba8::WHITE, .6f, Vec2(0.f, .5f));
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Exit Game", quitButton, m_textHeight, Rgba8::GRAY, .6f, Vec2(0.f, .5f));
		break;
	case 2:
		//g_testFont->AddVertsForTextInBox2D(buttonVerts, "Survival Mode", survivalButton, m_textHeight, Rgba8::WHITE, .6f, Vec2(0.f, .5f));
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Versus Mode", multiplayerButton, m_textHeight, Rgba8::GRAY, .6f, Vec2(0.f, .5f));
		g_testFont->AddVertsForTextInBox2D(buttonVerts, "Exit Game", quitButton, m_textHeight, Rgba8::WHITE, .6f, Vec2(0.f, .5f));
		break;
	}

	g_theRenderer->DrawVertexArray(buttonVerts);
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::ShutdownMenuMode()
{
}

void Game::StartSurvivalMenuMode()
{
}

void Game::UpdateSurvivalMenuMode()
{
}

void Game::RenderSurvivalMenuMode()
{
}

void Game::ShutdownSurvivalMenuMode()
{
}

void Game::StartSurvivalMode()
{
	g_theAudioSystem->StopSound(m_currentSongID);
	SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("gameMusic", "default"), 2);
	m_currentSongID = g_theAudioSystem->StartSound(id, true, g_gameConfigBlackboard->GetValue("musicVolume", .1f));

	//Renderer Setup
	g_theRenderer->CreateOrGetShader("Diffuse", VertexType::PCUTBN);
	g_theRenderer->BindShader(g_theRenderer->CreateOrGetShader("Diffuse", VertexType::PCUTBN));

	//Read tileDefXML
	InitializeTileDefs();
	//Read mapDefXML
	InitializeMaps();
	//Read actorDefXML
	InitializeActors();

	m_map = new Map(this, m_mapDefs[0]);

	if (m_waveResetTimer == nullptr)
	{
		m_waveResetTimer = new Timer(5.f, g_theGameClock);
		m_waveResetTimer->Start();
	}

	//Player Setup
	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			m_map->SpawnPlayer(m_playerList[i], m_map->m_definition->m_spawnInfo[0]->m_position);
		}
	}
	SetViewPortsForPlayers();
	g_theAudioSystem->SetNumListeners((int)m_playerList.size());

	for (int i = 0; i < (int)m_map->m_definition->m_spawnInfo.size(); i++)
	{
		m_map->SpawnActor(*m_map->m_definition->m_spawnInfo[i]);
	}
}

void Game::UpdateSurvivalMode()
{
	m_verts.clear();
	m_indexes.clear();
	m_vertsTBN.clear();

	if (m_timerForReset->IsStopped())
	{
		HandleInputGame();

		for (int i = 0; i < (int)m_playerList.size(); i++)
		{
			if (m_playerList[i] != nullptr)
			{
				m_playerList[i]->Update();
				m_playerList[i]->UpdateCamera();
				Vec3 forward;
				Vec3 up;
				Vec3 left;
				m_playerList[i]->m_playerCamOrientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
				g_theAudioSystem->UpdateListener(i, m_playerList[i]->m_playerCamPosition, forward, up);
			}
		}

		if (m_map != nullptr)
		{
			m_map->Update();

			//Spawn Demons
			if (m_waveResetTimer->IsStopped())
			{
				if (m_waveBudget > 0 && m_enemiesRemainingOnMap < m_maxEnemisOnMap)
				{
					int newEnemyIndex = g_rng->RollRandomIntInRange(0, (int)m_enemyDefs.size() - 1);
					if (m_enemyDefs[newEnemyIndex]->m_spawnCost > m_waveBudget)
					{
						newEnemyIndex = 0; // Just choose regular demon
					}
					int spawnLocationIndex = g_rng->RollRandomIntInRange(1, (int)m_map->m_definition->m_spawnInfo.size() - 1);
					SpawnInfo info = SpawnInfo();
					info.m_actor = m_enemyDefs[newEnemyIndex]->m_name;
					info.m_position = m_map->m_definition->m_spawnInfo[spawnLocationIndex]->m_position;
					info.m_orientation = m_map->m_definition->m_spawnInfo[spawnLocationIndex]->m_orientation;
					Actor* ref = m_map->SpawnActor(info);
					ref->m_AIController->m_targetActorHandle = m_player->m_possessedActor;

					m_waveBudget -= m_enemyDefs[newEnemyIndex]->m_spawnCost;
					m_enemiesRemainingOnMap++;
				}
				else if (m_waveBudget <= 0 && m_enemiesRemainingOnMap <= 0)
				{
					OnWaveClear();
				}
			}
		}

		if (m_waveResetTimer->HasPeriodElapsed() && !m_waveResetTimer->IsStopped())
		{
			delete m_waveResetTimer;
			m_waveResetTimer = new Timer(5.f, g_theGameClock);
		}

		if (m_timerForReset->IsStopped())
		{
			for (int i = 0; i < (int)m_playerList.size(); i++)
			{
				if (m_map && m_map->GetActorByHandle(m_player->m_possessedActor)->m_health <= 0.f)
				{
					m_timerForReset->Start();
				}
			}
		}
	}
	
	else if (m_timerForReset->HasPeriodElapsed() && !m_timerForReset->IsStopped())
	{
		OnCloseGameMode();
		delete m_timerForReset;
		m_timerForReset = new Timer(5.f, g_theSystemClock);
		OnOpenGameMode(GameState::ATTRACT);
	}
}

void Game::RenderSurvivalMode()
{
	g_theRenderer->ClearScreen(Rgba8(80, 80, 80, 255));

	m_player = m_playerList[0];
	m_worldCamera = *m_playerList[0]->m_camera;
	m_map->UpdateAllActorVerts();
	g_theRenderer->BeginCamera(m_worldCamera);
	g_theRenderer->SetStatesIfChanged();
	RenderGameModeWorld();
	g_theRenderer->EndCamera(m_worldCamera);

	m_screenCamera.SetViewPort(m_playerList[0]->m_camera->GetViewPort());
	g_theRenderer->BeginCamera(m_screenCamera);
	RenderSurvivalModeScreen();

	std::vector<Vertex_PCU> tempArray;
	g_testFont->AddVertsForTextInBox2D(tempArray, Stringf("Wave: %i", m_waveNumber), AABB2(0.f, SCREEN_SIZE_Y * .95f, SCREEN_SIZE_X, SCREEN_SIZE_Y), SCREEN_SIZE_Y * .05f, Rgba8::WHITE, .8f, Vec2(0.f, .5f));
	g_theRenderer->BindTexture(&g_testFont->GetTexture());
	g_theRenderer->DrawVertexArray(tempArray);

	if (!m_timerForReset->IsStopped())
	{
		std::vector<Vertex_PCU> tempNullTextureArray;
		float opacityFloat = 2.f * GetClamped((float)m_timerForReset->GetElapsedFraction(), 0.f, .35f);
		unsigned char opacity = DenormalizeByte(opacityFloat);

		tempArray.clear();
		AddVertsForAABB2D(tempNullTextureArray, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), Rgba8(75, 75, 75, opacity));
		g_testFont->AddVertsForTextInBox2D(tempArray, Stringf("Game Over\nYou Survived %i Waves", m_waveNumber), AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), SCREEN_SIZE_Y * .1f, Rgba8(255,40,40,opacity), .8f, Vec2(.5f, .5f));

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(tempNullTextureArray);
		g_theRenderer->BindTexture(&g_testFont->GetTexture());
		g_theRenderer->DrawVertexArray(tempArray);
	}

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderSurvivalModeScreen() const
{
	if (m_player != nullptr)
	{
		if (m_player->GetActor() != nullptr)
		{
			if (m_player->GetActor()->m_equippedWeapon != nullptr)
			{
				m_player->GetActor()->m_equippedWeapon->Render();
			}
			m_player->Render();
		}
	}
}

void Game::ShutdownSurvivalMode()
{
	for (int i = 0; i < (int)m_playerList.size(); i++)
	{
		if (m_playerList[i] != nullptr)
		{
			delete m_playerList[i];
			m_playerList[i] = nullptr;
		}
	}
	m_nextPlayerIndex = -1;
	m_keyboardActive = false;
	m_controllerActive = false;
	m_screenCamera.SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));

	m_verts.clear();
	delete m_map;
	m_map = nullptr;
	m_actorDefs.clear();
	m_weaponDefs.clear();
	m_mapDefs.clear();
	m_enemyDefs.clear();
	DebugRenderClear();

	m_waveNumber = 1;
	m_waveBudget = 1;
	m_enemiesRemainingOnMap = 0;
	m_waveScaleFactor = 1.5f;
}

void Game::OnWaveClear()
{
	m_waveNumber++;
	m_waveBudget = (int)(m_waveNumber * m_waveScaleFactor);
	m_enemiesRemainingOnMap = 0;

	m_waveResetTimer->Start();
}

void Game::StartLobbyMode()
{
	m_gameState = GameState::LOBBY;
}

void Game::ShutdownLobbyMode()
{
	
}

void Game::HandleInputLobby()
{
	XboxController controller = g_theInputSystem->GetController(0);

	if (g_theInputSystem->WasKeyJustPressed(' '))
	{
		SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
		g_theAudioSystem->StartSound(id, false);
		if (m_keyboardActive && m_controllerActive)
		{
			OnCloseGameMode();
			OnOpenGameMode(GameState::MULTIPLAYER);
		}
		else if(!m_keyboardActive)
		{
			AddPlayer(-1);
			m_nextPlayerIndex++;
			m_keyboardActive = true;
		}
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_START))
	{
		SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
		g_theAudioSystem->StartSound(id, false);
		if (m_controllerActive && m_keyboardActive)
		{
			OnCloseGameMode();
			OnOpenGameMode(GameState::MULTIPLAYER);
		}
		else if(!m_controllerActive)
		{
			AddPlayer(0);
			m_nextPlayerIndex++;
			m_controllerActive = true;
		}
	}

	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_ESC))
	{
		SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
		g_theAudioSystem->StartSound(id, false);
		if (!m_keyboardActive)
		{
			OnCloseGameMode();
			OnOpenGameMode(GameState::MENU);
		}
		else
		{
			RemovePlayer(-1);
			m_nextPlayerIndex--;
			m_keyboardActive = false;
		}
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		SoundID id = g_theAudioSystem->CreateOrGetSound(g_gameConfigBlackboard->GetValue("buttonClickSound", "default"), 2);
		g_theAudioSystem->StartSound(id, false);
		if (!m_controllerActive)
		{
			OnCloseGameMode();
			OnOpenGameMode(GameState::MENU);
		}
		else
		{
			RemovePlayer(0);
			m_nextPlayerIndex--;
			m_controllerActive = false;
		}
	}
}

void Game::UpdateLobbyMode()
{
	m_verts.clear();
	HandleInputLobby();
	std::string outputK;
	std::string outputC;
	float textHeight = 25.f;

	if (m_nextPlayerIndex == -1)
	{
		outputK = Stringf("Press SPACE to join on Keyboard\nPress START to join on Controller");
		g_testFont->AddVertsForTextInBox2D(m_verts, outputK, AABB2(0.f, 0.f, SCREEN_SIZE_X, textHeight * 2.f), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, 0.f));
	}
	else if (m_nextPlayerIndex == 0)
	{
		if (m_keyboardActive)
		{
			outputK = Stringf("Press START to join on Controller");
			g_testFont->AddVertsForTextInBox2D(m_verts, outputK, AABB2(0.f, 0.f, SCREEN_SIZE_X, textHeight * 2.f), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, 0.f));

			outputC = Stringf("Player 1 ready: Waiting for Player 2");
			g_testFont->AddVertsForTextInBox2D(m_verts, outputC, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
			m_playerList[0]->m_viewportBox = AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y);
		}
		else if(m_controllerActive)
		{
			outputK = Stringf("Press SPACE to join on Keyboard");
			g_testFont->AddVertsForTextInBox2D(m_verts, outputK, AABB2(0.f, 0.f, SCREEN_SIZE_X, textHeight * 2.f), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, 0.f));

			outputC = Stringf("Player 1 ready: Waiting for Player 2");
			g_testFont->AddVertsForTextInBox2D(m_verts, outputC, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
		}
	}
	else if (m_nextPlayerIndex == 1)
	{
		if (m_playerList[0]->m_playerIndex == 0)
		{
			outputC = Stringf("Player %i ready: Controller Press START to start.", m_playerList[0]->m_playerNumber);
			g_testFont->AddVertsForTextInBox2D(m_verts, outputC, AABB2(0.f, SCREEN_SIZE_Y * .5f, SCREEN_SIZE_X, SCREEN_SIZE_Y), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
			m_playerList[0]->m_viewportBox = AABB2(0.f, SCREEN_SIZE_Y * .5f, SCREEN_SIZE_X, SCREEN_SIZE_Y);

			outputK = Stringf("Player %i ready: Keyboard Press START to start.", m_playerList[1]->m_playerNumber);
			g_testFont->AddVertsForTextInBox2D(m_verts, outputK, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y * .5f), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
			m_playerList[1]->m_viewportBox = AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y * .5f);
		}
		else
		{
			outputC = Stringf("Player %i ready: Keyboard Press SPACE to start.", m_playerList[0]->m_playerNumber);
			g_testFont->AddVertsForTextInBox2D(m_verts, outputC, AABB2(0.f, SCREEN_SIZE_Y * .5f, SCREEN_SIZE_X, SCREEN_SIZE_Y), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
			m_playerList[0]->m_viewportBox = AABB2(0.f, SCREEN_SIZE_Y * .5f, SCREEN_SIZE_X, SCREEN_SIZE_Y);

			outputK = Stringf("Player %i ready: Controller Press START to start.", m_playerList[1]->m_playerNumber);
			g_testFont->AddVertsForTextInBox2D(m_verts, outputK, AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y * .5f), textHeight, Rgba8::WHITE, .75f, Vec2(.5f, .5f));
			m_playerList[1]->m_viewportBox = AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y * .5f);
		}
	}
}

void Game::RenderLobbyMode() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	g_theRenderer->ClearScreen(Rgba8::GRAY);
	g_theRenderer->BindTexture(&g_testFont->GetTexture());
	g_theRenderer->DrawVertexArray(m_verts);
	DebugRenderScreen(m_screenCamera);
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::AddPlayer(int controllerIndex)
{
	bool added = false;
	for (int j = 0; j < (int)m_playerList.size(); j++)
	{
		if (m_playerList[j] == nullptr && !added)
		{
			m_playerList[j] = new Player(this, Vec3(2.5f, 8.5f, .5f));
			m_playerList[j]->m_playerIndex = controllerIndex;
			if (m_nextPlayerIndex == -1)
			{
				m_playerList[j]->m_playerNumber = 1;
			}
			else
			{
				m_playerList[j]->m_playerNumber = 2;
			}
			added = true;
		}
	}
	if (!added)
	{
		m_playerList.push_back(new Player(this, Vec3(2.5f, 8.5f, .5f)));
		m_playerList[(int)m_playerList.size() - 1]->m_playerIndex = controllerIndex;
		if (m_nextPlayerIndex == -1)
		{
			m_playerList[(int)m_playerList.size() - 1]->m_playerNumber = 1;
		}
		else
		{
			m_playerList[(int)m_playerList.size() - 1]->m_playerNumber = 2;
		}
	}
}

void Game::RemovePlayer(int controllerIndex)
{
	for (int j = 0; j < (int)m_playerList.size(); j++)
	{
		if (m_playerList[j] != nullptr)
		{
			if (m_playerList[j]->m_playerIndex == controllerIndex)
			{
				if (m_nextPlayerIndex == 1)
				{
					if (j == 0)
					{
						m_playerList[0] = m_playerList[1];
						m_playerList[0]->m_playerNumber = 1;
						m_playerList[1] = nullptr;
					}
					else
					{
						delete m_playerList[j];
						m_playerList[j] = nullptr;
						m_playerList[0]->m_playerNumber = 1;
					}
				}
				else
				{
					delete m_playerList[j];
					m_playerList[j] = nullptr;
				}
			}
		}
	}
}

void Game::SetViewPortsForPlayers()
{
	if (m_nextPlayerIndex > 0)
	{
		m_playerList[0]->m_camera->SetViewPort(AABB2(0.f, SCREEN_SIZE_Y * .5f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
		m_playerList[1]->m_camera->SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y * .5f));
	}
	else if (m_nextPlayerIndex == 0)
	{
		m_playerList[0]->m_camera->SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
	}
}

Camera Game::GetActiveWorldCamera() const
{
	return m_worldCamera;
}

void Game::InitializeMaps()
{
	XmlDocument mapDefinition;
	std::string defPath = "Data/Definitions/MapDefinitions.xml";
	XmlResult eResult = mapDefinition.LoadFile(defPath.c_str());
	if (eResult != 0)
	{
		ERROR_AND_DIE("Map xml file not found or loaded incorrectly!");
	}
	XmlElement* mapElement = mapDefinition.RootElement()->FirstChildElement();
	while (mapElement)
	{
		NamedStrings* mapAttributes = new NamedStrings();
		mapAttributes->PopulateFromXmlElementAttributes(*mapElement);

		std::string lead = "Data/Shaders/";
		std::string shader = mapAttributes->GetValue("shader", "error");
		size_t pos = mapAttributes->GetValue("shader", "error").find(lead);
		if (pos != std::string::npos) {
			std::string result = shader;
			result.erase(pos, lead.length());
			shader = result;
		}

		MapDefinition* mapDef = new MapDefinition(mapAttributes->GetValue("name", "error"),
			g_theRenderer->CreateOrGetShader(shader.c_str(), VertexType::PCUTBN),
			g_theRenderer->CreateOrGetTextureFromFile(mapAttributes->GetValue("spriteSheetTexture", "error").c_str()),
			mapAttributes->GetValue("spriteSheetCellCount", IntVec2()));
		m_spriteSheet = new SpriteSheet(*mapDef->GetTexture(), mapDef->GetSpriteCellCount());
		mapDef->m_ceilingHeight = mapAttributes->GetValue("ceilingHeight", 1.f);
		mapDef->m_skyBoxFilePath = mapAttributes->GetValue("skyBoxFilePath", "None");

		//SpawnInfo section
		XmlElement* spawnInfosElement = mapElement->FirstChildElement("SpawnInfos");
		if (spawnInfosElement != nullptr)
		{
			XmlElement* spawnInfoElement = spawnInfosElement->FirstChildElement("SpawnInfo");
			while (spawnInfoElement != nullptr)
			{
				NamedStrings spawnAttributes;
				spawnAttributes.PopulateFromXmlElementAttributes(*spawnInfoElement);

				SpawnInfo* info = new SpawnInfo();
				info->m_actor = spawnAttributes.GetValue("actor", "default spawn");
				info->m_position = spawnAttributes.GetValue("position", Vec3());
				info->m_orientation = spawnAttributes.GetValue("orientation", EulerAngles());

				mapDef->m_spawnInfo.push_back(info);
				spawnInfoElement = spawnInfoElement->NextSiblingElement("SpawnInfo");
			}
		}
		m_mapDefs.push_back(mapDef);

		mapElement = mapElement->NextSiblingElement();
	}
}

void Game::InitializeTileDefs()
{
	XmlDocument tileDefinitions;
	std::string defPath = "Data/Definitions/TileDefinitions.xml";
	XmlResult eResult = tileDefinitions.LoadFile(defPath.c_str());
	if (eResult != 0)
	{
		ERROR_AND_DIE("TileDef xml file not found or loaded incorrectly!");
	}
	XmlElement* tileElement = tileDefinitions.RootElement()->FirstChildElement();
	while (tileElement)
	{
		NamedStrings tileAttributes;
		tileAttributes.PopulateFromXmlElementAttributes(*tileElement);

		TileDefinition* tileDef = new TileDefinition(
			tileAttributes.GetValue("name", "error"),
			tileAttributes.GetValue("isSolid", true),
			tileAttributes.GetValue("mapImagePixelColor", Rgba8::WHITE),
			tileAttributes.GetValue("floorSpriteCoords", IntVec2(-1, -1)),
			tileAttributes.GetValue("wallSpriteCoords", IntVec2(-1, -1)),
			tileAttributes.GetValue("ceilingSpriteCoords", IntVec2(-1, -1))
		);

		tileDef->m_secondaryRandom = tileAttributes.GetValue("secondaryRandom", IntVec2(-1, -1));
		m_tileDefs.push_back(tileDef);

		tileElement = tileElement->NextSiblingElement();
	}
}

void Game::InitializeActors()
{
	InitializeProjectileActor();
	InitializeWeapons();
	InitializeActor();

	for (int i = 0; i < (int)m_actorDefs.size(); i++)
	{
		if (m_actorDefs[i]->m_faction == Faction::EVIL)
		{
			m_enemyDefs.push_back(m_actorDefs[i]);
		}
	}
}

void Game::InitializeProjectileActor()
{
	XmlDocument actorDefinitions;
	std::string defPath = "Data/Definitions/ProjectileActorDefinitions.xml";
	XmlResult eResult = actorDefinitions.LoadFile(defPath.c_str());
	if (eResult != 0)
	{
		ERROR_AND_DIE("ActorDef xml file not found or loaded incorrectly!");
	}

	XmlElement* actorElement = actorDefinitions.RootElement()->FirstChildElement();
	while (actorElement)
	{
		NamedStrings actorAttributes;
		actorAttributes.PopulateFromXmlElementAttributes(*actorElement);

		ActorDefinition* newActorDef = new ActorDefinition();

		// Base attributes
		newActorDef->m_name = actorAttributes.GetValue("name", "uninitialized");
		newActorDef->m_visible = actorAttributes.GetValue("visible", false);
		newActorDef->m_health = actorAttributes.GetValue("health", 1.f);
		newActorDef->m_corpseLifetime = actorAttributes.GetValue("corpseLifetime", 0.f);
		newActorDef->m_canBePossessed = actorAttributes.GetValue("canBePossessed", false);
		newActorDef->m_color = actorAttributes.GetValue("color", Rgba8::WHITE);

		std::string factionStr = actorAttributes.GetValue("faction", "NEUTRAL");
		if (factionStr == "GOOD" || factionStr == "Marine")
		{
			newActorDef->m_faction = Faction::GOOD;
		}
		else if (factionStr == "EVIL" || factionStr == "Demon")
		{
			newActorDef->m_faction = Faction::EVIL;
		}
		else
		{
			newActorDef->m_faction = Faction::NEUTRAL;
		}

		// Child elements: Collision, Physics, Camera, AI, Weapon
		XmlElement* childElem = actorElement->FirstChildElement();
		while (childElem)
		{
			std::string tag = childElem->Name();
			NamedStrings childAttributes;
			childAttributes.PopulateFromXmlElementAttributes(*childElem);

			if (tag == "Collision")
			{
				newActorDef->m_collisionElement.m_physicsRadius = childAttributes.GetValue("radius", 0.f);
				newActorDef->m_collisionElement.m_physicsHeight = childAttributes.GetValue("height", 0.f);
				newActorDef->m_collisionElement.m_collidesWithWorld = childAttributes.GetValue("collidesWithWorld", false);
				newActorDef->m_collisionElement.m_collidesWithActors = childAttributes.GetValue("collidesWithActors", false);
				newActorDef->m_collisionElement.m_dieOnCollision = childAttributes.GetValue("dieOnCollide", false);
				newActorDef->m_collisionElement.m_impulseOnCollide = childAttributes.GetValue("impulseOnCollide", 0.f);
				newActorDef->m_collisionElement.m_damageOnCollide = childAttributes.GetValue("damageOnCollide", FloatRange());
			}
			else if (tag == "Physics")
			{
				newActorDef->m_physicsElement.m_simulated = childAttributes.GetValue("simulated", false);
				newActorDef->m_physicsElement.m_flying = childAttributes.GetValue("flying", false);
				newActorDef->m_physicsElement.m_walkSpeed = childAttributes.GetValue("walkSpeed", 0.f);
				newActorDef->m_physicsElement.m_runSpeed = childAttributes.GetValue("runSpeed", 0.f);
				newActorDef->m_physicsElement.m_drag = childAttributes.GetValue("drag", 0.f);
				newActorDef->m_physicsElement.turnSpeed = childAttributes.GetValue("turnSpeed", 0.f);
			}
			else if (tag == "Visuals")
			{
				newActorDef->m_VisualElement.m_spriteWorldSize = childAttributes.GetValue("size", Vec2(1.f, 1.f));
				newActorDef->m_VisualElement.m_pivot = childAttributes.GetValue("pivot", Vec2(.5f, .5f));
				std::string billboardString = childAttributes.GetValue("billboardType", "None");
				if (billboardString == "WorldUpFacing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::WORLD_UP_FACING;
				}
				else if (billboardString == "WorldUpOpposing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::WORLD_UP_OPPOSING;
				}
				else if (billboardString == "FullFacing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::FULL_FACING;
				}
				else if (billboardString == "FullOpposing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::FULL_OPPOSING;
				}
				else
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::NONE;
				}

				newActorDef->m_VisualElement.m_renderLit = childAttributes.GetValue("renderLit", false);
				newActorDef->m_VisualElement.m_renderRounded = childAttributes.GetValue("renderRounded", false);

				std::string shaderName = childAttributes.GetValue("shader", "Data/Shaders/Default");
				newActorDef->m_VisualElement.m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), VertexType::PCUTBN);
				Texture* sheetTexture = g_theRenderer->CreateOrGetTextureFromFile((childAttributes.GetValue("spriteSheet", "Data/Images/Test_StbiFlippedAndOpenGL.png")).c_str());
				SpriteSheet* spriteSheet = new SpriteSheet(*sheetTexture, childAttributes.GetValue("cellCount", IntVec2(1, 1)));
				newActorDef->m_VisualElement.m_sheet = spriteSheet;

				XmlElement* animGroup = childElem->FirstChildElement("AnimationGroup");
				while (animGroup)
				{
					newActorDef->AddGroupDefinition(spriteSheet, sheetTexture, animGroup);
					animGroup = animGroup->NextSiblingElement();
				}
			}

			childElem = childElem->NextSiblingElement();
		}

		m_actorDefs.push_back(newActorDef);

		actorElement = actorElement->NextSiblingElement();
	}
}

void Game::InitializeWeapons()
{
	XmlDocument weaponDefsDoc;
	std::string defPath = "Data/Definitions/WeaponDefinitions.xml";
	XmlResult result = weaponDefsDoc.LoadFile(defPath.c_str());

	if (result != 0)
	{
		ERROR_AND_DIE("WeaponDefinitions.xml failed to load!");
	}

	XmlElement* weaponElement = weaponDefsDoc.RootElement()->FirstChildElement();
	while (weaponElement)
	{
		NamedStrings attributes;
		attributes.PopulateFromXmlElementAttributes(*weaponElement);

		WeaponDefinition* weaponDef = new WeaponDefinition();
		weaponDef->m_name = attributes.GetValue("name", "unknown");

		// Time
		weaponDef->m_refireTime = attributes.GetValue("refireTime", 0.f);

		// Ray
		weaponDef->m_rayCount = attributes.GetValue("rayCount", 0);
		weaponDef->m_rayConeDegrees = attributes.GetValue("rayCone", 0.f);
		weaponDef->m_rayRange = attributes.GetValue("rayRange", 0.f);
		weaponDef->m_rayDamage = attributes.GetValue("rayDamage", FloatRange(0.f, 0.f));
		weaponDef->m_rayImpulse = attributes.GetValue("rayImpulse", 0.f);
		weaponDef->m_rayRender = attributes.GetValue("rayRender", false);
		weaponDef->m_rayColor = attributes.GetValue("rayColor", Rgba8::WHITE);

		// Projectile
		weaponDef->m_projectileCount = attributes.GetValue("projectileCount", 0);
		weaponDef->m_projectileConeDegrees = attributes.GetValue("projectileCone", 0.f);
		weaponDef->m_projectileSpeed = attributes.GetValue("projectileSpeed", 0.f);
		weaponDef->m_projectileActor = attributes.GetValue("projectileActor", "");
		weaponDef->m_projectileColor = attributes.GetValue("projectileColor", Rgba8::WHITE);
		weaponDef->m_projectileFallSpeed = attributes.GetValue("projectileFallSpeed", false);

		// Melee
		weaponDef->m_meleeCount = attributes.GetValue("meleeCount", 0);
		weaponDef->m_meleeRange = attributes.GetValue("meleeRange", 0.f);
		weaponDef->m_meleeArcDegrees = attributes.GetValue("meleeArc", 0.f);
		weaponDef->m_meleeDamage = attributes.GetValue("meleeDamage", FloatRange(0.f, 0.f));
		weaponDef->m_meleeImpulse = attributes.GetValue("meleeImpulse", 0.f);

		// Ammo Management
		weaponDef->m_magSize = attributes.GetValue("magSize", -1);
		weaponDef->m_reloadTime = attributes.GetValue("reloadTime", -1.f);
		weaponDef->m_heatPerShot = attributes.GetValue("heatPerShot", -1.f);
		weaponDef->m_maxHeat = attributes.GetValue("maxHeat", -1.f);
		weaponDef->m_cooldownTime = attributes.GetValue("cooldownTime", -1.f);
		weaponDef->m_isEnergyBased = attributes.GetValue("isEnergyBased", false);

		// Special
		weaponDef->m_chargeShotTime = attributes.GetValue("chargeShotTime", -1.f);
		weaponDef->m_isExplosive = attributes.GetValue("isExplosive", false);
		weaponDef->m_blastRadius = attributes.GetValue("blastRadius", -1.f);
		weaponDef->m_blastDamage = attributes.GetValue("blastDamage", 0.f);
		weaponDef->m_blastImpulse = attributes.GetValue("blastImpulse", 0.f);

		// Visuals + HUD
		XmlElement* childHUDElement = weaponElement->FirstChildElement();
		NamedStrings hudAttributes;
		hudAttributes.PopulateFromXmlElementAttributes(*childHUDElement);
		std::string shaderName = hudAttributes.GetValue("shader", "Default");
		weaponDef->m_HUDElement.m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str());
		std::string textureName = hudAttributes.GetValue("baseTexture", "Data/Images/Test_StbiFlippedAndOpenGL.png");
		weaponDef->m_HUDElement.m_baseTexture = g_theRenderer->CreateOrGetTextureFromFile(textureName.c_str());
		textureName = hudAttributes.GetValue("reticleTexture", "Data/Images/Test_StbiFlippedAndOpenGL.png");
		weaponDef->m_HUDElement.m_reticleTexture = g_theRenderer->CreateOrGetTextureFromFile(textureName.c_str());
		weaponDef->m_HUDElement.m_reticleSize = hudAttributes.GetValue("reticleSize", Vec2(1.f, 1.f));
		weaponDef->m_HUDElement.m_spriteSize = hudAttributes.GetValue("spriteSize", Vec2(1.f, 1.f));
		weaponDef->m_HUDElement.m_spritePivot = hudAttributes.GetValue("spritePivot", Vec2(0.5f, 0.5f));

		// HUD Animations
		XmlElement* childAnimElement = childHUDElement->FirstChildElement();
		while (childAnimElement)
		{
			NamedStrings animAttributes;
			animAttributes.PopulateFromXmlElementAttributes(*childAnimElement);

			Texture* sheetTexture = g_theRenderer->CreateOrGetTextureFromFile((animAttributes.GetValue("spriteSheet", "Data/Images/Test_StbiFlippedAndOpenGL.png")).c_str());
			SpriteSheet* spriteSheet = new SpriteSheet(*sheetTexture, animAttributes.GetValue("cellCount", IntVec2(1, 1)));
			int startFrame = animAttributes.GetValue("startFrame", 0);
			int endFrame = animAttributes.GetValue("endFrame", 0);
			float framesPerSecond = 1.f / animAttributes.GetValue("secondsPerFrame", 1.f);

			std::string playbackMode = animAttributes.GetValue("playbackMode", "Once");
			SpriteAnimPlaybackType type = SpriteAnimPlaybackType::ONCE;
			if (playbackMode == "Loop") type = SpriteAnimPlaybackType::LOOP;
			else if (playbackMode == "Pingpong") type = SpriteAnimPlaybackType::PINGPONG;

			SpriteAnimDefinition* animationDef = new SpriteAnimDefinition(*spriteSheet, startFrame, endFrame, framesPerSecond, type);
			animationDef->LoadFromXmlElement(*childAnimElement);
			weaponDef->m_HUDElement.m_animationDefs.push_back(animationDef);

			childAnimElement = childAnimElement->NextSiblingElement();
		}

		// Sounds
		XmlElement* childSoundsElement = childHUDElement->NextSiblingElement("Sounds");
		if (childSoundsElement)
		{
			XmlElement* childSoundElement = childSoundsElement->FirstChildElement("Sound");
			while (childSoundElement)
			{
				NamedStrings soundAttributes;
				soundAttributes.PopulateFromXmlElementAttributes(*childSoundElement);

				SoundDefinition soundDef;
				soundDef.m_soundName = soundAttributes.GetValue("sound", "");
				soundDef.m_soundFilePath = soundAttributes.GetValue("name", "");

				weaponDef->m_sounds.push_back(soundDef);
				childSoundElement = childSoundElement->NextSiblingElement("Sound");
			}
		}

		// Loop sound
		weaponDef->m_loopSoundOnHold = attributes.GetValue("loopSound", false);

		m_weaponDefs.push_back(weaponDef);
		weaponElement = weaponElement->NextSiblingElement();
	}
}

void Game::InitializeActor()
{
	XmlDocument actorDefinitions;
	std::string defPath = "Data/Definitions/ActorDefinitions.xml";
	XmlResult eResult = actorDefinitions.LoadFile(defPath.c_str());
	if (eResult != 0)
	{
		ERROR_AND_DIE("ActorDef xml file not found or loaded incorrectly!");
	}

	XmlElement* actorElement = actorDefinitions.RootElement()->FirstChildElement();
	while (actorElement)
	{
		NamedStrings actorAttributes;
		actorAttributes.PopulateFromXmlElementAttributes(*actorElement);

		ActorDefinition* newActorDef = new ActorDefinition();

		// Base attributes
		newActorDef->m_name = actorAttributes.GetValue("name", "uninitialized");
		newActorDef->m_visible = actorAttributes.GetValue("visible", false);
		newActorDef->m_health = actorAttributes.GetValue("health", 1.f);
		newActorDef->m_corpseLifetime = actorAttributes.GetValue("corpseLifetime", 0.f);
		newActorDef->m_canBePossessed = actorAttributes.GetValue("canBePossessed", false);
		newActorDef->m_dieOnSpawn = actorAttributes.GetValue("dieOnSpawn", false);
		newActorDef->m_color = actorAttributes.GetValue("color", Rgba8::WHITE);
		newActorDef->m_isPickup = actorAttributes.GetValue("isPickup", false);
		newActorDef->m_spawnCost = actorAttributes.GetValue("cost", 1);

		std::string factionStr = actorAttributes.GetValue("faction", "NEUTRAL");
		if (factionStr == "GOOD" || factionStr == "Marine")
		{
			newActorDef->m_faction = Faction::GOOD;
		}
		else if (factionStr == "EVIL" || factionStr == "Demon")
		{
			newActorDef->m_faction = Faction::EVIL;
		}
		else
		{
			newActorDef->m_faction = Faction::NEUTRAL;
		}

		// Child elements: Collision, Physics, Camera, AI, Weapon
		XmlElement* childElem = actorElement->FirstChildElement();
		while (childElem)
		{
			std::string tag = childElem->Name();
			NamedStrings childAttributes;
			childAttributes.PopulateFromXmlElementAttributes(*childElem);

			if (tag == "Collision")
			{
				newActorDef->m_collisionElement.m_physicsRadius = childAttributes.GetValue("radius", 0.f);
				newActorDef->m_collisionElement.m_physicsHeight = childAttributes.GetValue("height", 0.f);
				newActorDef->m_collisionElement.m_collidesWithWorld = childAttributes.GetValue("collidesWithWorld", false);
				newActorDef->m_collisionElement.m_collidesWithActors = childAttributes.GetValue("collidesWithActors", false);
				newActorDef->m_collisionElement.m_dieOnCollision = childAttributes.GetValue("dieOnCollide", false);
				newActorDef->m_collisionElement.m_impulseOnCollide = childAttributes.GetValue("impulseOnCollide", 0.f);

				// Optional range parsing
				float minDamage = childAttributes.GetValue("damageMin", 0.f);
				float maxDamage = childAttributes.GetValue("damageMax", 0.f);
				newActorDef->m_collisionElement.m_damageOnCollide = FloatRange(minDamage, maxDamage);
			}
			else if (tag == "Physics")
			{
				newActorDef->m_physicsElement.m_simulated = childAttributes.GetValue("simulated", false);
				newActorDef->m_physicsElement.m_flying = childAttributes.GetValue("flying", false);
				newActorDef->m_physicsElement.m_walkSpeed = childAttributes.GetValue("walkSpeed", 0.f);
				newActorDef->m_physicsElement.m_runSpeed = childAttributes.GetValue("runSpeed", 0.f);
				newActorDef->m_physicsElement.m_drag = childAttributes.GetValue("drag", 0.f);
				newActorDef->m_physicsElement.turnSpeed = childAttributes.GetValue("turnSpeed", 0.f);
			}
			else if (tag == "Camera")
			{
				newActorDef->m_cameraElement.m_eyeHeight = childAttributes.GetValue("eyeHeight", 0.f);
				newActorDef->m_cameraElement.m_cameraFOVDegrees = childAttributes.GetValue("cameraFOV", 60.f);
			}
			else if (tag == "AI")
			{
				newActorDef->m_AIElement.m_aiEnabled = childAttributes.GetValue("aiEnabled", false);
				newActorDef->m_AIElement.m_sightRadius = childAttributes.GetValue("sightRadius", 0.f);
				newActorDef->m_AIElement.m_sightAngle = childAttributes.GetValue("sightAngle", 0.f);
			}
			else if (tag == "Inventory")
			{
				XmlElement* weaponElem = childElem->FirstChildElement("Weapon");
				while (weaponElem)
				{
					NamedStrings weaponAttributes;
					weaponAttributes.PopulateFromXmlElementAttributes(*weaponElem);
					std::string weaponName = weaponAttributes.GetValue("name", "unarmed");
					newActorDef->m_weaponElement.m_weaponNames.push_back(weaponName);

					weaponElem = weaponElem->NextSiblingElement("Weapon");
				}
			}
			else if (tag == "Visuals")
			{
				newActorDef->m_VisualElement.m_spriteWorldSize = childAttributes.GetValue("size", Vec2(1.f,1.f));
				newActorDef->m_VisualElement.m_pivot = childAttributes.GetValue("pivot", Vec2(.5f, .5f));
				std::string billboardString = childAttributes.GetValue("billboardType", "None");
				if (billboardString == "WorldUpFacing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::WORLD_UP_FACING;
				}
				else if (billboardString == "WorldUpOpposing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::WORLD_UP_OPPOSING;
				}
				else if (billboardString == "FullFacing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::FULL_FACING;
				}
				else if (billboardString == "FullOpposing")
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::FULL_OPPOSING;
				}
				else
				{
					newActorDef->m_VisualElement.m_billBoardType = BillBoardType::NONE;
				}

				newActorDef->m_VisualElement.m_renderLit = childAttributes.GetValue("renderLit", false);
				newActorDef->m_VisualElement.m_renderRounded = childAttributes.GetValue("renderRounded", false);
				std::string fullPath = childAttributes.GetValue("shader", "Data/Shaders/Default");
				std::string prefix = "Data/Shaders/";

				std::string shaderName = fullPath.substr(prefix.length());
				newActorDef->m_VisualElement.m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), VertexType::PCUTBN);
				Texture* sheetTexture = g_theRenderer->CreateOrGetTextureFromFile((childAttributes.GetValue("spriteSheet", "Data/Images/Test_StbiFlippedAndOpenGL.png")).c_str());
				SpriteSheet* spriteSheet = new SpriteSheet(*sheetTexture, childAttributes.GetValue("cellCount", IntVec2(1,1)));
				newActorDef->m_VisualElement.m_sheet = spriteSheet;

				XmlElement* animGroup = childElem->FirstChildElement("AnimationGroup");
				while (animGroup)
				{
					newActorDef->AddGroupDefinition(spriteSheet, sheetTexture, animGroup);
					animGroup = animGroup->NextSiblingElement();
				}
			}
			else if (tag == "Sounds")
			{
				XmlElement* soundElem = childElem->FirstChildElement("Sound");
				while (soundElem)
				{
					NamedStrings soundAttributes;
					soundAttributes.PopulateFromXmlElementAttributes(*soundElem);

					SoundDefinition soundDef;
					soundDef.m_soundName = soundAttributes.GetValue("sound", "");
					soundDef.m_soundFilePath = soundAttributes.GetValue("name", "");

					newActorDef->m_sounds.push_back(soundDef);

					soundElem = soundElem->NextSiblingElement("Sound");
				}
			}

			childElem = childElem->NextSiblingElement();
		}

		m_actorDefs.push_back(newActorDef);

		actorElement = actorElement->NextSiblingElement();
	}
}

void Game::CreateAllSounds()
{
	g_theAudioSystem->CreateOrGetSound("Data/Audio/Music/E1M1_AtDoomsGate.mp2", 2);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/Music/MainMenu_InTheDark.mp2", 2);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/BulletBounce.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/BulletRicochet.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/BulletRicochet2.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/Click.mp3", 2);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/DemonActive.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/DemonAttack.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/DemonDeath.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/DemonHurt.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/DemonSight.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/EnemyDied.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/EnemyHit.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/EnemyShoot.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/ExitMap.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/NextMap.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PickupItem.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PickupWeapon.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PistolFire.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlasmaFire.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlasmaHit.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlayerDeath1.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlayerDeath2.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlayerGibbed.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlayerHit.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/PlayerHurt.wav", 3);
	g_theAudioSystem->CreateOrGetSound("Data/Audio/Teleporter.wav", 3);
}
