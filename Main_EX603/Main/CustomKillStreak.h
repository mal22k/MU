// CustomKillStreak.h: Client-side Kill Streak effects
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Import.h"
#include "Protocol.h"

#define MAX_KILLSTREAK_DISPLAY 8
#define KILLSTREAK_DISPLAY_TIME 3000 // ms to show the streak notification

// ====================================================================
// Texture IDs
// ====================================================================
#define KILLSTREAK_TEX_BASE 0x7F200

// ====================================================================
// Packet Structures (matching GameServer)
// ====================================================================

struct PMSG_KILLSTREAK_NOTIFY_RECV
{
	PSBMSG_HEAD header; // C1:F3:A5
	BYTE StreakIndex;
	BYTE RequiredKills;
	BYTE ScreenShake;
	BYTE ShakeIntensity;
	WORD ShakeDuration;
	char KillerName[11];
	char VictimName[11];
	char StreakName[64];
	char SoundFile[128];
	char ImageFile[128];
};

struct PMSG_KILLSTREAK_RESET_RECV
{
	PSBMSG_HEAD header; // C1:F3:A6
	BYTE TotalKills;
	BYTE MaxStreakIndex;
};

// ====================================================================
// Display entry
// ====================================================================

struct KILLSTREAK_DISPLAY_INFO
{
	bool Active;
	DWORD StartTime;
	BYTE StreakIndex;
	BYTE RequiredKills;
	char KillerName[11];
	char VictimName[11];
	char StreakName[64];
	int TexID;
	bool HasCustomImage;
};

// ====================================================================
// Screen shake data
// ====================================================================

struct KILLSTREAK_SHAKE_INFO
{
	bool Active;
	DWORD StartTime;
	DWORD Duration;
	int Intensity;
};

// ====================================================================
// Main Class
// ====================================================================

class CCustomKillStreakClient
{
public:
	CCustomKillStreakClient();
	~CCustomKillStreakClient();

	void Init();

	// Drawing
	void Draw();
	void DrawStreakNotification(KILLSTREAK_DISPLAY_INFO* info, float posY);
	void ApplyScreenShake();

	// Packet handlers
	void GCKillStreakNotifyRecv(PMSG_KILLSTREAK_NOTIFY_RECV* lpMsg);
	void GCKillStreakResetRecv(PMSG_KILLSTREAK_RESET_RECV* lpMsg);

private:
	void PlayStreakSound(const char* soundFile);
	void LoadStreakImage(int texID, const char* imageFile);
	void StartScreenShake(int intensity, DWORD duration);

	KILLSTREAK_DISPLAY_INFO m_Displays[MAX_KILLSTREAK_DISPLAY];
	KILLSTREAK_SHAKE_INFO m_Shake;
	int m_NextDisplaySlot;

	// Loaded texture cache
	std::map<std::string, int> m_LoadedTextures;
	int m_NextTexID;
};

extern CCustomKillStreakClient gKillStreakClient;
