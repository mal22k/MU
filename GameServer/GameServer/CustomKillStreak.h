// CustomKillStreak.h: interface for the CCustomKillStreak class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"
#include "User.h"

#define MAX_KILLSTREAK_TYPES 20
#define KILLSTREAK_TIMEOUT 10000 // 10 seconds between kills to keep streak

// ====================================================================
// Kill streak tier config (loaded from XML)
// ====================================================================

struct KILLSTREAK_TIER_INFO
{
	int Index;
	int RequiredKills;
	char Name[64];         // "DoubleKill", "TripleKill", etc.
	char SoundFile[128];   // "Custom\\Sounds\\doublekill.wav"
	char ImageFile[128];   // "Custom\\Images\\doublekill.tga"
	int ScreenShake;       // 0=no, 1=yes
	int ShakeIntensity;    // 1-10
	int ShakeDuration;     // milliseconds
	int NoticeType;        // 0=personal, 1=map, 2=server
	char NoticeMsg[256];   // "%s got a Double Kill!"
};

// ====================================================================
// Packet Structures
// ====================================================================

// Server -> Client: Kill streak notification
struct PMSG_KILLSTREAK_NOTIFY
{
	PSBMSG_HEAD header; // C1:F3:A5
	BYTE StreakIndex;       // tier index from config
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

// Server -> Client: Kill streak reset
struct PMSG_KILLSTREAK_RESET
{
	PSBMSG_HEAD header; // C1:F3:A6
	BYTE TotalKills;
	BYTE MaxStreakIndex;
};

// ====================================================================
// Per-user runtime data
// ====================================================================

struct KILLSTREAK_USER_DATA
{
	int CurrentKills;
	int MaxStreakThisSession;
	DWORD LastKillTime;     // GetTickCount() of last kill
	int LastStreakNotified;  // last tier index notified
};

// ====================================================================
// Main Class
// ====================================================================

class CCustomKillStreak
{
public:
	CCustomKillStreak();
	virtual ~CCustomKillStreak();
	void Init();
	void LoadXML(char* FilePath);

	// Called when a player kills another player
	void OnPlayerKill(LPOBJ lpKiller, LPOBJ lpVictim);

	// Called on timer tick to check streak timeouts
	void OnSecondProc(int aIndex);

	// Reset streak on death
	void OnPlayerDeath(LPOBJ lpObj);

private:
	void ResetStreak(int aIndex);
	void NotifyStreak(int aIndex, int streakIndex, LPOBJ lpVictim);
	KILLSTREAK_TIER_INFO* GetStreakByKills(int kills);
	KILLSTREAK_TIER_INFO* GetStreakByIndex(int index);

	int Enable;
	DWORD StreakTimeout; // ms between kills
	std::vector<KILLSTREAK_TIER_INFO> m_StreakTiers;
	KILLSTREAK_USER_DATA m_UserData[MAX_OBJECT_USER];
};

extern CCustomKillStreak gCustomKillStreak;
