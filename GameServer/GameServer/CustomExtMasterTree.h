// CustomExtMasterTree.h: Extended Master Skill Tree system
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"
#include "User.h"

#define MAX_EXT_SKILL_TREE_SLOTS 50
#define MAX_EXT_SKILL_TREE_LEVEL 20
#define MAX_EXT_SKILL_TREE_GROUPS 4

// ====================================================================
// Configuration structures
// ====================================================================

struct EXT_MASTER_BONUS_INFO
{
	int Index;          // Unique bonus ID
	int Group;          // Tab/group (0-3)
	int Row;            // Row in the tree (0-8)
	int Column;         // Column (0-3)
	int MinLevel;       // min points to invest
	int MaxLevel;       // max points
	char Name[64];
	char Description[128];
	float ValuePerLevel; // Bonus value per level
	int BonusType;       // 0=AddDamage, 1=AddDefense, 2=AddHP, 3=AddMP, 4=AddSD,
	                     // 5=CritRate, 6=CritDmg, 7=ExcDmgRate, 8=IgnoreDefRate,
	                     // 9=DoubleDmgRate, 10=TripleDmgRate, 11=ReflectDmg,
	                     // 12=AddStr, 13=AddDex, 14=AddVit, 15=AddEne, 16=AddLead,
	                     // 17=AddExpRate, 18=AddDropRate, 19=Custom
	int RequireSkill[2]; // prerequisite bonus indexes (-1 = none)
	int RequireClass[7]; // 1=allowed, 0=not
};

// ====================================================================
// Per-user data
// ====================================================================

struct EXT_MASTER_USER_DATA
{
	int aIndex;
	int TotalPoints;     // Points available to spend
	int UsedPoints;
	int SkillLevels[MAX_EXT_SKILL_TREE_SLOTS]; // points invested in each slot
	bool Loaded;
};

// ====================================================================
// Packet structures
// ====================================================================

// Client -> Server: Request info
struct PMSG_EXT_MASTER_INFO_REQ
{
	PSBMSG_HEAD header; // C1:F3:B0
};

// Client -> Server: Invest point
struct PMSG_EXT_MASTER_INVEST_REQ
{
	PSBMSG_HEAD header; // C1:F3:B1
	BYTE SlotIndex;
};

// Server -> Client: Full tree info
struct PMSG_EXT_MASTER_INFO_SEND
{
	PSWMSG_HEAD header; // C2:F3:B0
	WORD TotalPoints;
	WORD UsedPoints;
	BYTE count;
};

struct PMSG_EXT_MASTER_SLOT_INFO
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

// Server -> Client: Invest result
struct PMSG_EXT_MASTER_INVEST_RESULT
{
	PSBMSG_HEAD header; // C1:F3:B1
	BYTE Result;     // 0=fail, 1=success, 2=no points, 3=max level, 4=prereq not met, 5=class not allowed
	BYTE SlotIndex;
	BYTE NewLevel;
	WORD RemainingPoints;
};

// DataServer packets
struct SDHP_EXT_MASTER_LOAD_SEND
{
	PSBMSG_HEAD header; // C1:52
	WORD index;
	char account[11];
	char name[11];
};

struct SDHP_EXT_MASTER_LOAD_RECV
{
	PWMSG_HEAD header; // C2:52
	WORD index;
	char name[11];
	BYTE result;
	WORD TotalPoints;
	WORD UsedPoints;
	BYTE SkillLevels[MAX_EXT_SKILL_TREE_SLOTS];
};

struct SDHP_EXT_MASTER_SAVE_SEND
{
	PWMSG_HEAD header; // C2:53
	WORD index;
	char name[11];
	WORD TotalPoints;
	WORD UsedPoints;
	BYTE SkillLevels[MAX_EXT_SKILL_TREE_SLOTS];
};

// ====================================================================
// Main Class
// ====================================================================

class CCustomExtMasterTree
{
public:
	CCustomExtMasterTree();
	virtual ~CCustomExtMasterTree();
	void Init();
	void LoadXML(char* FilePath);

	// Packet handlers
	void CGExtMasterInfoReq(int aIndex);
	void CGExtMasterInvestReq(PMSG_EXT_MASTER_INVEST_REQ* lpMsg, int aIndex);

	// Core
	bool InvestPoint(int aIndex, int slotIndex);
	void CalcExtMasterOptions(LPOBJ lpObj, bool flag);

	// Bonus application
	void ApplyBonus(LPOBJ lpObj, EXT_MASTER_BONUS_INFO* bonus, int level, bool flag);

	// Points management
	void AddPoints(int aIndex, int amount);
	void OnMasterLevelUp(LPOBJ lpObj);
	void OnReset(LPOBJ lpObj, int pointsPerReset);

	// DataServer
	void DGExtMasterLoadRecv(LPBYTE lpMsg);
	void GDExtMasterSave(int aIndex);
	void GDExtMasterLoad(int aIndex);

	// Send to client
	void GCExtMasterInfoSend(int aIndex);
	void GCExtMasterInvestResult(int aIndex, BYTE result, BYTE slotIndex, BYTE newLevel, WORD remaining);

	// Utility
	EXT_MASTER_BONUS_INFO* GetBonusInfo(int index);
	bool CheckPrerequisites(int aIndex, int slotIndex);
	bool CheckClass(LPOBJ lpObj, int slotIndex);

private:
	int Enable;
	int PointsPerMasterLevel;  // how many ext points per master level
	int PointsPerReset;        // bonus points per reset
	std::map<int, EXT_MASTER_BONUS_INFO> m_BonusInfo;
	EXT_MASTER_USER_DATA m_UserData[MAX_OBJECT_USER];
};

extern CCustomExtMasterTree gCustomExtMasterTree;
