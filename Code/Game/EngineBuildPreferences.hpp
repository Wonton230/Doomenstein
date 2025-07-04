//-----------------------------------------------------------------------------------------------
// EngineBuildPreferences.hpp
//
// Defines build preferences that the Engine should use when building for this particular game.
//
//	*Each game can now direct the engine via #defines to build differently for that game.
//	*ALL games must now have this Code/Game/EngineBuildPreferences.hpp file.
//

#define ENGINE_DISABLE_AUDIO	// (If uncommented) Disables AudioSystem code and fmod linkage.
#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER		// (If uncommented) Enables Debug Render Mode for DX11
#endif
