#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <math.h>
#include <cassert>
#include <crtdbg.h>

#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Window/Window.hpp"

extern App*				g_theApp;
extern Renderer*		g_theRenderer;
extern InputSystem*		g_theInputSystem;
extern Window*			g_theWindow;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED( commandLineString );
	UNUSED( applicationInstanceHandle );

	g_theApp = new App();
	g_theApp->Startup();

	while( !g_theApp->IsQuitting() )
	{
		g_theApp->RunFrame();
	}

	g_theApp->Shutdown();

	return 0;
}