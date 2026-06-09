// CustomBattlePass.h: Client-side Battle Pass Interface
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Import.h"
#include "Protocol.h"
#include "Interface.h"

#define MAX_BATTLEPASS_LEVEL 100
#define MAX_BATTLEPASS_TIER 4

// ====================================================================
// Texture IDs for Battle Pass UI
// ====================================================================
#define BATTLEPASS_TEX_BG       0x7F100  // Background
#define BATTLEPASS_TEX_BAR      0x7F101  // Progress bar
#define BATTLEPASS_TEX_BARFILL  0x7F102  // Progress bar fill
#define BATTLEPASS_TEX_ICON     0x7F103  // Level icon
#define BATTLEPASS_TEX_LOCK     0x7F104  // Locked reward icon
#define BATTLEPASS_TEX_CHECK    0x7F105  // Claimed checkmark
#define BATTLEPASS_TEX_STAR     0x7F106  // VIP star
#define BATTLEPASS_TEX_BTN      0x7F107  // Claim button
#define BATTLEPASS_TEX_CLOSE    0x7F108  // Close button
#define BATTLEPASS_TEX_SCROLL_L 0x7F109  // Scroll left
#define BATTLEPASS_TEX_SCROLL_R 0x7F10A  // Scroll right

// ====================================================================
// Packet Structures (matching GameServer)
// ====================================================================

struct PMSG_BATTLEPASS_INFO_RECV
{
	PSWMSG_HEAD header; // C2:F3:A0
	BYTE CurrentLevel;
	DWORD CurrentExp;
	DWORD NextLevelExp;
	DWORD TotalExp;
	BYTE VipLevel;
	BYTE SeasonId;
	BYTE count;
};

struct PMSG_BATTLEPASS_LEVEL_INFO_RECV
{
	BYTE Level;
	DWORD RequiredExp;
	BYTE ClaimedFree;
	BYTE ClaimedVip1;
	BYTE ClaimedVip2;
	BYTE ClaimedVip3;
};

struct PMSG_BATTLEPASS_CLAIM_RESULT_RECV
{
	PSBMSG_HEAD header; // C1:F3:A1
	BYTE Result;
	BYTE Level;
	BYTE Tier;
};

struct PMSG_BATTLEPASS_EXP_UPDATE_RECV
{
	PSBMSG_HEAD header; // C1:F3:A2
	BYTE CurrentLevel;
	DWORD CurrentExp;
	DWORD NextLevelExp;
	DWORD AddedExp;
};

// Client -> Server
struct PMSG_BATTLEPASS_INFO_REQ_SEND
{
	PSBMSG_HEAD header; // C1:F3:A0
};

struct PMSG_BATTLEPASS_CLAIM_REQ_SEND
{
	PSBMSG_HEAD header; // C1:F3:A1
	BYTE Level;
	BYTE Tier;
};

// ====================================================================
// Client-side level data
// ====================================================================

struct BATTLEPASS_CLIENT_LEVEL
{
	BYTE Level;
	DWORD RequiredExp;
	BYTE ClaimedFree;
	BYTE ClaimedVip1;
	BYTE ClaimedVip2;
	BYTE ClaimedVip3;
};

// ====================================================================
// Main Client Class
// ====================================================================

class CCustomBattlePassClient
{
public:
	CCustomBattlePassClient();
	~CCustomBattlePassClient();

	void Init();
	void LoadTextures();

	// Drawing
	void Draw();
	void DrawBackground(float baseX, float baseY);
	void DrawProgressBar(float baseX, float baseY);
	void DrawLevelRewards(float baseX, float baseY);
	void DrawClaimButton(float x, float y, int level, int tier);
	void DrawExpNotification();

	// Input
	bool HandleMouse();
	void OnKeyPress(int key);

	// Window management
	void Open();
	void Close();
	void Toggle();
	bool IsOpen() { return this->m_IsOpen; }

	// Packet handlers
	void GCBattlePassInfoRecv(LPBYTE lpMsg);
	void GCBattlePassClaimResultRecv(PMSG_BATTLEPASS_CLAIM_RESULT_RECV* lpMsg);
	void GCBattlePassExpUpdateRecv(PMSG_BATTLEPASS_EXP_UPDATE_RECV* lpMsg);

	// Send requests
	void CGRequestInfo();
	void CGClaimReward(BYTE level, BYTE tier);

private:
	bool m_IsOpen;
	bool m_TexturesLoaded;
	int m_ScrollOffset;    // which level to start displaying from
	int m_MaxVisibleLevels; // how many levels fit on screen

	// Current data
	BYTE m_CurrentLevel;
	DWORD m_CurrentExp;
	DWORD m_NextLevelExp;
	DWORD m_TotalExp;
	BYTE m_VipLevel;
	BYTE m_SeasonId;

	// Level data
	std::vector<BATTLEPASS_CLIENT_LEVEL> m_Levels;

	// EXP notification
	bool m_ShowExpNotify;
	DWORD m_ExpNotifyTime;
	DWORD m_ExpNotifyAmount;
	BYTE m_ExpNotifyNewLevel;
};

extern CCustomBattlePassClient gBattlePassClient;
