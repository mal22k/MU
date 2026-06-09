// CustomExtMasterTree.h: Client-side Extended Master Skill Tree
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"
#include "Import.h"
#include "Protocol.h"
#include "Interface.h"

#define MAX_EXT_SKILL_TREE_SLOTS 50
#define MAX_EXT_TREE_GROUPS 4

// ====================================================================
// Packet Structures (matching GameServer)
// ====================================================================

struct PMSG_EXT_MASTER_INFO_RECV
{
	PSWMSG_HEAD header; // C2:F3:B0
	WORD TotalPoints;
	WORD UsedPoints;
	BYTE count;
};

struct PMSG_EXT_MASTER_SLOT_INFO_RECV
{
	BYTE Index;
	BYTE Group;
	BYTE Row;
	BYTE Column;
	BYTE MinLevel;
	BYTE MaxLevel;
	BYTE CurrentLevel;
	BYTE BonusType;
	float ValuePerLevel;
	char Name[64];
	char Description[128];
	BYTE RequireSlot1;
	BYTE RequireSlot2;
};

struct PMSG_EXT_MASTER_INVEST_RESULT_RECV
{
	PSBMSG_HEAD header; // C1:F3:B1
	BYTE Result;
	BYTE SlotIndex;
	BYTE NewLevel;
	WORD RemainingPoints;
};

struct PMSG_EXT_MASTER_INFO_REQ_SEND
{
	PSBMSG_HEAD header; // C1:F3:B0
};

struct PMSG_EXT_MASTER_INVEST_REQ_SEND
{
	PSBMSG_HEAD header; // C1:F3:B1
	BYTE SlotIndex;
};

// ====================================================================
// Client slot data
// ====================================================================

struct EXT_TREE_CLIENT_SLOT
{
	int Index;
	int Group;
	int Row;
	int Column;
	int MinLevel;
	int MaxLevel;
	int CurrentLevel;
	int BonusType;
	float ValuePerLevel;
	char Name[64];
	char Description[128];
	int RequireSlot1;
	int RequireSlot2;
};

// ====================================================================
// Main Client Class
// ====================================================================

class CCustomExtMasterTreeClient
{
public:
	CCustomExtMasterTreeClient();
	~CCustomExtMasterTreeClient();

	void Init();
	void LoadTextures();

	// Drawing
	void Draw();
	void DrawBackground(float baseX, float baseY);
	void DrawTabs(float baseX, float baseY);
	void DrawSkillSlots(float baseX, float baseY);
	void DrawSlotTooltip(EXT_TREE_CLIENT_SLOT* slot, float posX, float posY);

	// Input
	bool HandleMouse();
	void OnKeyPress(int key);

	// Window
	void Open();
	void Close();
	void Toggle();
	bool IsOpen() { return this->m_IsOpen; }

	// Packet handlers
	void GCExtMasterInfoRecv(LPBYTE lpMsg);
	void GCExtMasterInvestResultRecv(PMSG_EXT_MASTER_INVEST_RESULT_RECV* lpMsg);

	// Send
	void CGRequestInfo();
	void CGInvestPoint(BYTE slotIndex);

private:
	bool m_IsOpen;
	bool m_TexturesLoaded;
	int m_ActiveGroup; // 0-3 tab selection

	WORD m_TotalPoints;
	WORD m_UsedPoints;

	std::vector<EXT_TREE_CLIENT_SLOT> m_Slots;
	int m_HoveredSlot; // -1 = none
};

extern CCustomExtMasterTreeClient gExtMasterTreeClient;
