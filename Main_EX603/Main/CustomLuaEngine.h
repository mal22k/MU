// CustomLuaEngine.h: Client-side Lua scripting engine
//
// The GameServer already has a full Lua integration (LuaFunction.h/cpp + LuaStack.hpp)
// with 180+ registered functions. This client-side engine adds Lua scripting
// to the client DLL for UI customization, event handling, and client-side logic.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"

// Lua headers - add LuaJIT library to the client project
extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#define MAX_LUA_SCRIPTS 100

// ====================================================================
// Script entry
// ====================================================================

struct CLIENT_LUA_SCRIPT_INFO
{
	int Index;
	char FileName[256];
	char EventTrigger[64];  // "OnDraw", "OnKeyPress", "OnLogin", "OnMonsterKill", etc.
	bool Loaded;
	bool Enabled;
};

// ====================================================================
// Main Client Lua Engine
// ====================================================================

class CCustomLuaEngine
{
public:
	CCustomLuaEngine();
	~CCustomLuaEngine();

	// Lifecycle
	void Init();
	void Shutdown();
	void LoadScripts(const char* folderPath);
	void ReloadScripts();

	// Execute
	bool ExecuteFile(const char* filePath);
	bool ExecuteString(const char* code);
	bool CallFunction(const char* funcName);
	bool CallFunctionInt(const char* funcName, int arg1);
	bool CallFunctionStr(const char* funcName, const char* arg1);

	// Event triggers (called from hooks)
	void OnDraw();
	void OnDrawAfterUI();
	void OnKeyPress(int key);
	void OnMouseClick(int x, int y, int button);
	void OnLogin(const char* account, const char* character);
	void OnLogout();
	void OnPacketRecv(BYTE headcode, BYTE subcode, LPBYTE data, int size);
	void OnChat(const char* sender, const char* message);
	void OnMapChange(int mapID);
	void OnLevelUp(int newLevel);

	// API registration
	void RegisterClientAPI();

	// Utility
	lua_State* GetState() { return this->L; }
	bool IsInitialized() { return this->m_Initialized; }

private:
	lua_State* L;
	bool m_Initialized;
	std::vector<CLIENT_LUA_SCRIPT_INFO> m_Scripts;
};

extern CCustomLuaEngine gClientLuaEngine;

// ====================================================================
// Lua API functions exposed to scripts
// ====================================================================

// Drawing
int Lua_DrawText(lua_State* L);
int Lua_DrawRect(lua_State* L);
int Lua_DrawImage(lua_State* L);
int Lua_SetTextColor(lua_State* L);
int Lua_LoadTexture(lua_State* L);

// Game state
int Lua_GetPlayerName(lua_State* L);
int Lua_GetPlayerLevel(lua_State* L);
int Lua_GetPlayerMap(lua_State* L);
int Lua_GetPlayerHP(lua_State* L);
int Lua_GetPlayerMaxHP(lua_State* L);
int Lua_GetPlayerMP(lua_State* L);
int Lua_GetPlayerMaxMP(lua_State* L);
int Lua_GetPlayerX(lua_State* L);
int Lua_GetPlayerY(lua_State* L);
int Lua_GetCursorX(lua_State* L);
int Lua_GetCursorY(lua_State* L);
int Lua_GetScreenWidth(lua_State* L);
int Lua_GetScreenHeight(lua_State* L);

// UI
int Lua_OpenWindow(lua_State* L);
int Lua_CloseWindow(lua_State* L);
int Lua_IsWindowOpen(lua_State* L);

// Sound
int Lua_PlaySound(lua_State* L);

// Network
int Lua_SendPacket(lua_State* L);
int Lua_SendChat(lua_State* L);

// Utility
int Lua_GetTickCount(lua_State* L);
int Lua_LogMessage(lua_State* L);
