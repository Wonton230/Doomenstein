#pragma once
#include "GameCommon.hpp"
#include "Engine/Core/EventSystem.hpp"

class Game;
class Clock;

class App
{
public:
	App();
	~App();
	void Startup();
	void Shutdown();
	void RunFrame();
	Game* GetGame();

	bool IsQuitting() const { return m_isQuitting;  }
	bool HandleQuitRequested();

	static bool Event_Quit(EventArgs& args);

	bool	m_drawDebug = false;
	bool	m_isPaused = false;
	bool	m_isSlowMo = false;
	bool	m_isQuitting = false;

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	float	m_timeSinceLastFrameStart = 0;
	float	m_deltaSeconds = 0;
	Game*	m_Game;
};