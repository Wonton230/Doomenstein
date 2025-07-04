#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/XboxController.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "App.hpp"
#include "Game.hpp"

App*					g_theApp = nullptr;
Renderer*				g_theRenderer	 = nullptr;
RandomNumberGenerator*	g_rng			 = nullptr;
extern InputSystem*		g_theInputSystem;
AudioSystem*			g_theAudioSystem = nullptr;
Window*					g_theWindow		 = nullptr;
BitmapFont*				g_testFont	= nullptr;	
extern NamedStrings*	g_gameConfigBlackboard;
extern DevConsole*		g_theDevConsole;
extern Clock*			g_theSystemClock;

App::App()
{
	m_Game = nullptr;
}

App::~App()
{
}

void App::Startup()
{
	XmlDocument gameConfig;
	XmlResult eResult = gameConfig.LoadFile("Data/GameConfig.xml");
	if (eResult != 0)
	{
		ERROR_AND_DIE("GameConfig.xml not found or loaded incorrectly!");
	}
	XmlElement* rootElement = gameConfig.RootElement();
	g_gameConfigBlackboard->PopulateFromXmlElementAttributes(*rootElement);

	InputConfig inputConfig;
	g_theInputSystem = new InputSystem(inputConfig);

	WindowConfig windowConfig;
	windowConfig.m_inputSystem = g_theInputSystem;
	windowConfig.m_aspectRatio = g_gameConfigBlackboard->GetValue("windowAspect", 2.f);
	windowConfig.m_windowTitle = "DOOMENSTEIN";
	g_theWindow = new Window(windowConfig);
	g_theWindow->Startup();
	IntVec2 clientDims = g_theWindow->GetClientDimensions();
	SCREEN_SIZE_X = (float)clientDims.x;
	SCREEN_SIZE_Y = (float)clientDims.y - WINDOWS_BAR_HEIGHT;
	SCREEN_CENTER_X = SCREEN_SIZE_X * .5f;
	SCREEN_CENTER_Y = SCREEN_SIZE_Y * .5f;

	RenderConfig renderConfig;
	renderConfig.m_window = g_theWindow;
	g_theRenderer = new Renderer(renderConfig);
	g_theRenderer->Startup();

	DevConsoleConfig consoleConfig;
	consoleConfig.m_renderer = g_theRenderer;
	consoleConfig.m_camera = nullptr;
	g_testFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/OctoFont.png");
	consoleConfig.font = g_testFont;
	g_theDevConsole = new DevConsole(consoleConfig);

	//Input starts up after devconsole to block inputs 
	g_theDevConsole->Startup();
	g_theInputSystem->Startup();

	g_theEventSystem->SubscribeEventCallbackFunction("quit", App::Event_Quit);
	g_theDevConsole->AddText(g_theDevConsole->INFO_MAJOR, 
		"Controls:\n\
		Menu:\n\
		W \t\t- Scroll Up\n\
		S \t\t- Scroll Down\n\
		Space \t\t- Select\n\n\
		Keyboard Controls:\n\
		W\t\t- Move Forward\n\
		A\t\t- Move Left\n\
		S\t\t- Move Backward\n\
		D\t\t- Move Right\n\
		LMB\t\t- Fire\n\
		RMB\t\t- Secondary Fire (unused)\n\
		R\t\t- Reload\n\
		SHIFT\t\t- Run\n\
		MOUSEWHEEL \t- Change Weapons\n\n\
		XBOX Controls:\n\
		LEFTSTICK\t- Move\n\
		RIGHTSTICK\t- Aim\n\
		B\t\t- Toggle Run/Walk\n\
		X\t\t- Reload\n\
		RIGHTBUMPER\t- Change Weapon\n\
		LEFTBUMPER\t- Change Weapon\n\
		RIGHTTRIGGER\t- Fire\n\n\
		P\t\t- Pause Game\n\
		T\t\t- Slow Mo\n\
		O\t\t- Advance Frame\n\
		`\t\t- Dev Console"
	);

	AudioConfig audioConfig;
	g_theAudioSystem = new AudioSystem(audioConfig);
	g_theAudioSystem->Startup();

	g_rng = new RandomNumberGenerator();

	DebugRenderConfig debugRenderConfig;
	debugRenderConfig.m_renderer = g_theRenderer;
	DebugRenderSystemStartup(debugRenderConfig);

	m_Game = new Game();
	m_Game->Startup();
}

void App::Shutdown()
{
	m_isQuitting = true;

	m_Game->Shutdown();
	delete m_Game;
	m_Game = nullptr;

	DebugRenderSystemShutdown();

	g_theAudioSystem->Shutdown();
	delete g_theAudioSystem;
	g_theAudioSystem = nullptr;

	g_theRenderer->Shutdown();
	delete g_theRenderer;
	g_theRenderer = nullptr;

	g_theWindow->Shutdown();
	delete g_theWindow;
	g_theWindow = nullptr;

	g_theInputSystem->Shutdown();
	delete g_theInputSystem;
	g_theInputSystem = nullptr;
	
	delete g_gameConfigBlackboard;
}

void App::RunFrame()
{
	BeginFrame();
	Update();
 	Render();
	EndFrame();
}

Game* App::GetGame()
{
	return m_Game;
}

void App::BeginFrame()
{
	g_theInputSystem->BeginFrame();
	g_theAudioSystem->BeginFrame();
	g_theWindow->BeginFrame();
	g_theRenderer->BeginFrame();
	g_theDevConsole->BeginFrame();
	g_theEventSystem->BeginFrame();
	DebugRenderBeginFrame();

	g_theSystemClock->TickSystemClock();
}

void App::Update()
{	
	m_Game->Update();

	if (g_theInputSystem->WasKeyJustPressed(KEYCODE_TILDE))
	{
		g_theDevConsole->ToggleMode(DevConsoleMode::FULL);
	}

	if (g_theDevConsole->IsOpen() || m_Game->m_gameState == GameState::ATTRACT || !g_theWindow->IsActiveWindow())
	{
		g_theInputSystem->SetCursorMode(CursorMode::POINTER);
	}
	else
	{
		g_theInputSystem->SetCursorMode(CursorMode::FPS);
	}

	if (g_theDevConsole->IsOpen())
	{
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	}
	else
	{
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	}
}

void App::Render() const
{
	m_Game->Render();

	m_Game->m_screenCamera.SetViewPort(AABB2(0.f, 0.f, SCREEN_SIZE_X, SCREEN_SIZE_Y));
	g_theRenderer->BeginCamera(m_Game->m_screenCamera);
	g_theRenderer->SetStatesIfChanged();
	g_theDevConsole->Render(AABB2(m_Game->m_screenCamera.GetOrthographicBottomLeft(), m_Game->m_screenCamera.GetOrthographicTopRight()), g_theRenderer);
	g_theRenderer->EndCamera(m_Game->m_screenCamera);

	DebugRenderScreen(m_Game->m_screenCamera);
}

void App::EndFrame()
{
	g_theInputSystem->EndFrame();
	g_theAudioSystem->EndFrame();
	g_theWindow->EndFrame();
	g_theRenderer->EndFrame();
	DebugRenderEndFrame();
}

bool App::HandleQuitRequested()
{
	m_isQuitting = true;
	return m_isQuitting;
}

bool App::Event_Quit(EventArgs& args)
{
	UNUSED(args);
	g_theApp->HandleQuitRequested();
	return true;
}