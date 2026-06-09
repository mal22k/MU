// CustomLuaEngine.cpp: Client-side Lua scripting engine implementation
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomLuaEngine.h"
#include "Import.h"
#include "Interface.h"
#include "Offset.h"
#include "Util.h"

CCustomLuaEngine gClientLuaEngine;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomLuaEngine::CCustomLuaEngine()
{
	this->L = NULL;
	this->m_Initialized = false;
}

CCustomLuaEngine::~CCustomLuaEngine()
{
	this->Shutdown();
}

void CCustomLuaEngine::Init()
{
	if (this->m_Initialized) return;

	this->L = luaL_newstate();
	if (this->L == NULL)
	{
		// Failed to create Lua state
		return;
	}

	luaL_openlibs(this->L);
	this->RegisterClientAPI();
	this->m_Initialized = true;
}

void CCustomLuaEngine::Shutdown()
{
	if (this->L != NULL)
	{
		lua_close(this->L);
		this->L = NULL;
	}
	this->m_Initialized = false;
	this->m_Scripts.clear();
}

// ====================================================================
// Script Loading
// ====================================================================

void CCustomLuaEngine::LoadScripts(const char* folderPath)
{
	if (!this->m_Initialized) return;

	this->m_Scripts.clear();

	// Load the main init script first
	char initPath[512];
	_snprintf_s(initPath, sizeof(initPath), _TRUNCATE, "%s\\init.lua", folderPath);
	this->ExecuteFile(initPath);

	// Scan folder for .lua files
	char searchPath[512];
	_snprintf_s(searchPath, sizeof(searchPath), _TRUNCATE, "%s\\*.lua", folderPath);

	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA(searchPath, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		int idx = 0;
		do
		{
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (_stricmp(fd.cFileName, "init.lua") == 0) continue;

				CLIENT_LUA_SCRIPT_INFO info;
				memset(&info, 0, sizeof(info));
				info.Index = idx++;
				_snprintf_s(info.FileName, sizeof(info.FileName), _TRUNCATE, "%s\\%s", folderPath, fd.cFileName);
				info.Loaded = false;
				info.Enabled = true;

				if (this->ExecuteFile(info.FileName))
				{
					info.Loaded = true;
				}

				this->m_Scripts.push_back(info);
			}
		} while (FindNextFileA(hFind, &fd));

		FindClose(hFind);
	}
}

void CCustomLuaEngine::ReloadScripts()
{
	if (!this->m_Initialized) return;

	// Close and reinitialize
	lua_close(this->L);
	this->L = luaL_newstate();
	luaL_openlibs(this->L);
	this->RegisterClientAPI();

	// Reload all
	this->LoadScripts("Data\\LuaScripts");
}

// ====================================================================
// Execution
// ====================================================================

bool CCustomLuaEngine::ExecuteFile(const char* filePath)
{
	if (!this->m_Initialized || this->L == NULL) return false;

	int result = luaL_dofile(this->L, filePath);
	if (result != 0)
	{
		const char* error = lua_tostring(this->L, -1);
		// Log error (could use OutputDebugStringA or file logging)
		char buf[512];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "[LuaEngine] Error loading %s: %s\n", filePath, error ? error : "unknown");
		OutputDebugStringA(buf);
		lua_pop(this->L, 1);
		return false;
	}

	return true;
}

bool CCustomLuaEngine::ExecuteString(const char* code)
{
	if (!this->m_Initialized || this->L == NULL) return false;

	int result = luaL_dostring(this->L, code);
	if (result != 0)
	{
		const char* error = lua_tostring(this->L, -1);
		char buf[512];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "[LuaEngine] Error: %s\n", error ? error : "unknown");
		OutputDebugStringA(buf);
		lua_pop(this->L, 1);
		return false;
	}

	return true;
}

bool CCustomLuaEngine::CallFunction(const char* funcName)
{
	if (!this->m_Initialized || this->L == NULL) return false;

	lua_getglobal(this->L, funcName);
	if (!lua_isfunction(this->L, -1))
	{
		lua_pop(this->L, 1);
		return false;
	}

	if (lua_pcall(this->L, 0, 0, 0) != 0)
	{
		const char* error = lua_tostring(this->L, -1);
		char buf[512];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "[LuaEngine] Error calling %s: %s\n", funcName, error ? error : "unknown");
		OutputDebugStringA(buf);
		lua_pop(this->L, 1);
		return false;
	}

	return true;
}

bool CCustomLuaEngine::CallFunctionInt(const char* funcName, int arg1)
{
	if (!this->m_Initialized || this->L == NULL) return false;

	lua_getglobal(this->L, funcName);
	if (!lua_isfunction(this->L, -1))
	{
		lua_pop(this->L, 1);
		return false;
	}

	lua_pushinteger(this->L, arg1);

	if (lua_pcall(this->L, 1, 0, 0) != 0)
	{
		lua_pop(this->L, 1);
		return false;
	}

	return true;
}

bool CCustomLuaEngine::CallFunctionStr(const char* funcName, const char* arg1)
{
	if (!this->m_Initialized || this->L == NULL) return false;

	lua_getglobal(this->L, funcName);
	if (!lua_isfunction(this->L, -1))
	{
		lua_pop(this->L, 1);
		return false;
	}

	lua_pushstring(this->L, arg1);

	if (lua_pcall(this->L, 1, 0, 0) != 0)
	{
		lua_pop(this->L, 1);
		return false;
	}

	return true;
}

// ====================================================================
// Event Triggers
// ====================================================================

void CCustomLuaEngine::OnDraw()
{
	this->CallFunction("OnDraw");
}

void CCustomLuaEngine::OnDrawAfterUI()
{
	this->CallFunction("OnDrawAfterUI");
}

void CCustomLuaEngine::OnKeyPress(int key)
{
	this->CallFunctionInt("OnKeyPress", key);
}

void CCustomLuaEngine::OnMouseClick(int x, int y, int button)
{
	if (!this->m_Initialized || this->L == NULL) return;

	lua_getglobal(this->L, "OnMouseClick");
	if (!lua_isfunction(this->L, -1)) { lua_pop(this->L, 1); return; }

	lua_pushinteger(this->L, x);
	lua_pushinteger(this->L, y);
	lua_pushinteger(this->L, button);

	lua_pcall(this->L, 3, 0, 0);
}

void CCustomLuaEngine::OnLogin(const char* account, const char* character)
{
	if (!this->m_Initialized || this->L == NULL) return;

	lua_getglobal(this->L, "OnLogin");
	if (!lua_isfunction(this->L, -1)) { lua_pop(this->L, 1); return; }

	lua_pushstring(this->L, account);
	lua_pushstring(this->L, character);

	lua_pcall(this->L, 2, 0, 0);
}

void CCustomLuaEngine::OnLogout()
{
	this->CallFunction("OnLogout");
}

void CCustomLuaEngine::OnPacketRecv(BYTE headcode, BYTE subcode, LPBYTE data, int size)
{
	if (!this->m_Initialized || this->L == NULL) return;

	lua_getglobal(this->L, "OnPacketRecv");
	if (!lua_isfunction(this->L, -1)) { lua_pop(this->L, 1); return; }

	lua_pushinteger(this->L, headcode);
	lua_pushinteger(this->L, subcode);
	// Pass data as a string blob
	lua_pushlstring(this->L, (const char*)data, size);
	lua_pushinteger(this->L, size);

	lua_pcall(this->L, 4, 0, 0);
}

void CCustomLuaEngine::OnChat(const char* sender, const char* message)
{
	if (!this->m_Initialized || this->L == NULL) return;

	lua_getglobal(this->L, "OnChat");
	if (!lua_isfunction(this->L, -1)) { lua_pop(this->L, 1); return; }

	lua_pushstring(this->L, sender);
	lua_pushstring(this->L, message);

	lua_pcall(this->L, 2, 0, 0);
}

void CCustomLuaEngine::OnMapChange(int mapID)
{
	this->CallFunctionInt("OnMapChange", mapID);
}

void CCustomLuaEngine::OnLevelUp(int newLevel)
{
	this->CallFunctionInt("OnLevelUp", newLevel);
}

// ====================================================================
// API Registration
// ====================================================================

void CCustomLuaEngine::RegisterClientAPI()
{
	if (!this->L) return;

	// Drawing API
	lua_register(this->L, "DrawText", Lua_DrawText);
	lua_register(this->L, "DrawRect", Lua_DrawRect);
	lua_register(this->L, "DrawImage", Lua_DrawImage);
	lua_register(this->L, "SetTextColor", Lua_SetTextColor);
	lua_register(this->L, "LoadTexture", Lua_LoadTexture);

	// Game state API
	lua_register(this->L, "GetPlayerName", Lua_GetPlayerName);
	lua_register(this->L, "GetPlayerLevel", Lua_GetPlayerLevel);
	lua_register(this->L, "GetPlayerMap", Lua_GetPlayerMap);
	lua_register(this->L, "GetPlayerHP", Lua_GetPlayerHP);
	lua_register(this->L, "GetPlayerMaxHP", Lua_GetPlayerMaxHP);
	lua_register(this->L, "GetPlayerMP", Lua_GetPlayerMP);
	lua_register(this->L, "GetPlayerMaxMP", Lua_GetPlayerMaxMP);
	lua_register(this->L, "GetPlayerX", Lua_GetPlayerX);
	lua_register(this->L, "GetPlayerY", Lua_GetPlayerY);
	lua_register(this->L, "GetCursorX", Lua_GetCursorX);
	lua_register(this->L, "GetCursorY", Lua_GetCursorY);
	lua_register(this->L, "GetScreenWidth", Lua_GetScreenWidth);
	lua_register(this->L, "GetScreenHeight", Lua_GetScreenHeight);

	// UI API
	lua_register(this->L, "OpenWindow", Lua_OpenWindow);
	lua_register(this->L, "CloseWindow", Lua_CloseWindow);
	lua_register(this->L, "IsWindowOpen", Lua_IsWindowOpen);

	// Sound
	lua_register(this->L, "PlaySound", Lua_PlaySound);

	// Network
	lua_register(this->L, "SendPacket", Lua_SendPacket);
	lua_register(this->L, "SendChat", Lua_SendChat);

	// Utility
	lua_register(this->L, "GetTickCount", Lua_GetTickCount);
	lua_register(this->L, "LogMessage", Lua_LogMessage);
}

// ====================================================================
// Lua API Implementations
// ====================================================================

int Lua_DrawText(lua_State* L)
{
	// DrawText(x, y, text)
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	const char* text = luaL_checkstring(L, 3);

	pDrawText(pTextThis(), x, y, text, -1, 0, NULL, 0);
	return 0;
}

int Lua_DrawRect(lua_State* L)
{
	// DrawRect(x, y, w, h, alpha)
	float x = (float)luaL_checknumber(L, 1);
	float y = (float)luaL_checknumber(L, 2);
	float w = (float)luaL_checknumber(L, 3);
	float h = (float)luaL_checknumber(L, 4);
	float a = (float)luaL_optnumber(L, 5, 0.5);

	pSetBlend(1);
	pDrawBarForm(x, y, w, h, a, 0);
	pSetBlend(0);
	return 0;
}

int Lua_DrawImage(lua_State* L)
{
	// DrawImage(texID, x, y, w, h)
	int texID = (int)luaL_checkinteger(L, 1);
	float x = (float)luaL_checknumber(L, 2);
	float y = (float)luaL_checknumber(L, 3);
	float w = (float)luaL_checknumber(L, 4);
	float h = (float)luaL_checknumber(L, 5);

	pSetBlend(1);
	pDrawImage(texID, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, true, true, true);
	pSetBlend(0);
	return 0;
}

int Lua_SetTextColor(lua_State* L)
{
	// SetTextColor(r, g, b, a)
	int r = (int)luaL_checkinteger(L, 1);
	int g = (int)luaL_checkinteger(L, 2);
	int b = (int)luaL_checkinteger(L, 3);
	int a = (int)luaL_optinteger(L, 4, 255);

	pSetTextColor(pTextThis(), (BYTE)r, (BYTE)g, (BYTE)b, (BYTE)a);
	return 0;
}

int Lua_LoadTexture(lua_State* L)
{
	// LoadTexture(texID, folderPath)
	int texID = (int)luaL_checkinteger(L, 1);
	const char* folder = luaL_checkstring(L, 2);

	pLoadTexture(texID, (char*)folder, GL_REPEAT, GL_NEAREST, GL_TRUE);
	return 0;
}

int Lua_GetPlayerName(lua_State* L)
{
	// Read from client memory - character name offset
	char* name = (char*)oCharName;
	lua_pushstring(L, name ? name : "");
	return 1;
}

int Lua_GetPlayerLevel(lua_State* L)
{
	lua_pushinteger(L, *(WORD*)oCharLevel);
	return 1;
}

int Lua_GetPlayerMap(lua_State* L)
{
	lua_pushinteger(L, pMapNumber);
	return 1;
}

int Lua_GetPlayerHP(lua_State* L)
{
	lua_pushinteger(L, *(DWORD*)oCharLife);
	return 1;
}

int Lua_GetPlayerMaxHP(lua_State* L)
{
	lua_pushinteger(L, *(DWORD*)oCharMaxLife);
	return 1;
}

int Lua_GetPlayerMP(lua_State* L)
{
	lua_pushinteger(L, *(DWORD*)oCharMana);
	return 1;
}

int Lua_GetPlayerMaxMP(lua_State* L)
{
	lua_pushinteger(L, *(DWORD*)oCharMaxMana);
	return 1;
}

int Lua_GetPlayerX(lua_State* L)
{
	lua_pushinteger(L, *(BYTE*)oCharX);
	return 1;
}

int Lua_GetPlayerY(lua_State* L)
{
	lua_pushinteger(L, *(BYTE*)oCharY);
	return 1;
}

int Lua_GetCursorX(lua_State* L)
{
	lua_pushinteger(L, pCursorX);
	return 1;
}

int Lua_GetCursorY(lua_State* L)
{
	lua_pushinteger(L, pCursorY);
	return 1;
}

int Lua_GetScreenWidth(lua_State* L)
{
	lua_pushinteger(L, pWinWidth);
	return 1;
}

int Lua_GetScreenHeight(lua_State* L)
{
	lua_pushinteger(L, pWinHeight);
	return 1;
}

int Lua_OpenWindow(lua_State* L)
{
	int code = (int)luaL_checkinteger(L, 1);
	pOpenWindow(pWindowThis(), code);
	return 0;
}

int Lua_CloseWindow(lua_State* L)
{
	int code = (int)luaL_checkinteger(L, 1);
	pCloseWindow(pWindowThis(), code);
	return 0;
}

int Lua_IsWindowOpen(lua_State* L)
{
	int code = (int)luaL_checkinteger(L, 1);
	bool open = pCheckWindow(pWindowThis(), code);
	lua_pushboolean(L, open);
	return 1;
}

int Lua_PlaySound(lua_State* L)
{
	const char* file = luaL_checkstring(L, 1);
	PlaySoundA(file, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	return 0;
}

int Lua_SendPacket(lua_State* L)
{
	// SendPacket(bytestring)
	size_t len = 0;
	const char* data = luaL_checklstring(L, 1, &len);

	if (data != NULL && len > 0)
	{
		DataSend((BYTE*)data, (DWORD)len);
	}

	return 0;
}

int Lua_SendChat(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	pChatSend(text);
	return 0;
}

int Lua_GetTickCount(lua_State* L)
{
	lua_pushinteger(L, (int)GetTickCount());
	return 1;
}

int Lua_LogMessage(lua_State* L)
{
	const char* msg = luaL_checkstring(L, 1);
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
	return 0;
}
