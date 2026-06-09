// CustomBattlePass.h: interface for the CCustomBattlePass class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Protocol.h"
#include "User.h"

#define MAX_BATTLEPASS_LEVEL 100
#define MAX_BATTLEPASS_TIER 4 // 0=Free, 1=VIP1, 2=VIP2, 3=VIP3

// ====================================================================
// Packet Structures (Client <-> GameServer)
// ====================================================================

// Client -> Server: Request battle pass info
struct PMSG_BATTLEPASS_INFO_REQ
{
	PSBMSG_HEAD header; // C1:F3:A0
};

// Client -> Server: Claim reward at level
struct PMSG_BATTLEPASS_CLAIM_REQ
{
	PSBMSG_HEAD header; // C1:F3:A1
	BYTE Level;
	BYTE Tier; // 0=Free, 1=VIP1, 2=VIP2, 3=VIP3
};

// Server -> Client: Full battle pass info
struct PMSG_BATTLEPASS_INFO_SEND
{
	PSWMSG_HEAD header; // C2:F3:A0
	BYTE CurrentLevel;
	DWORD CurrentExp;
	DWORD NextLevelExp;
	DWORD TotalExp;
	BYTE VipLevel; // 0=Free, 1-3=VIP
	BYTE SeasonId;
	BYTE count; // number of level entries following
};

// Sub-struct for each level reward info
struct PMSG_BATTLEPASS_LEVEL_INFO
{
	BYTE Level;
	DWORD RequiredExp;
	BYTE ClaimedFree;   // 0=not claimed, 1=claimed
	BYTE ClaimedVip1;
	BYTE ClaimedVip2;
	BYTE ClaimedVip3;
};

// Server -> Client: Claim result
struct PMSG_BATTLEPASS_CLAIM_RESULT
{
	PSBMSG_HEAD header; // C1:F3:A1
	BYTE Result; // 0=fail, 1=success, 2=already claimed, 3=level not reached, 4=vip required
	BYTE Level;
	BYTE Tier;
};

// Server -> Client: EXP update notification
struct PMSG_BATTLEPASS_EXP_UPDATE
{
	PSBMSG_HEAD header; // C1:F3:A2
	BYTE CurrentLevel;
	DWORD CurrentExp;
	DWORD NextLevelExp;
	DWORD AddedExp;
};

// ====================================================================
// DataServer Packets (GameServer <-> DataServer)
// ====================================================================

// GameServer -> DataServer: Request load battle pass data
struct SDHP_BATTLEPASS_LOAD_SEND
{
	PSBMSG_HEAD header; // C1:50
	WORD index;
	char account[11];
	char name[11];
};

// DataServer -> GameServer: Battle pass data response
struct SDHP_BATTLEPASS_LOAD_RECV
{
	PWMSG_HEAD header; // C2:50
	WORD index;
	char account[11];
	char name[11];
	BYTE result;
	BYTE BPLevel;
	DWORD BPExp;
	BYTE SeasonId;
	BYTE ClaimedFree[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip1[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip2[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip3[MAX_BATTLEPASS_LEVEL];
};

// GameServer -> DataServer: Save battle pass data
struct SDHP_BATTLEPASS_SAVE_SEND
{
	PWMSG_HEAD header; // C2:51
	WORD index;
	char account[11];
	char name[11];
	BYTE BPLevel;
	DWORD BPExp;
	BYTE SeasonId;
	BYTE ClaimedFree[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip1[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip2[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip3[MAX_BATTLEPASS_LEVEL];
};

// ====================================================================
// Configuration Structures (from XML)
// ====================================================================

struct BATTLEPASS_EXP_SOURCE
{
	int Type;       // 0=MonsterKill, 1=Reset, 2=BossKill, 3=EventComplete
	int MonsterID;  // -1 = any monster, or specific monster ID
	int MinLevel;   // Minimum monster/player level to count
	int MapNumber;  // -1 = any map
	DWORD ExpAmount;
};

struct BATTLEPASS_REWARD_ITEM
{
	int ItemIndex;
	int ItemLevel;
	int ItemDurability;
	int ItemOption1; // Skill
	int ItemOption2; // Luck
	int ItemOption3; // Additional (+x)
	int ItemNewOption; // Excellent
	int SetOption;     // Ancient
	int JewelOfHarmony;
	int SocketOption[5];
	int SocketOptionBonus;
	int Duration;      // 0=permanent
};

struct BATTLEPASS_REWARD
{
	int WCoin;
	int WPoint;
	int GoblinPoint;
	int Ruud;
	int Zen;
	std::vector<BATTLEPASS_REWARD_ITEM> Items;
};

struct BATTLEPASS_LEVEL_CONFIG
{
	int Level;
	DWORD RequiredExp;
	BATTLEPASS_REWARD RewardFree;
	BATTLEPASS_REWARD RewardVip1;
	BATTLEPASS_REWARD RewardVip2;
	BATTLEPASS_REWARD RewardVip3;
};

struct BATTLEPASS_SEASON_CONFIG
{
	int SeasonId;
	int Enable;
	int MaxLevel;
	char SeasonName[64];
};

// ====================================================================
// Per-user runtime data
// ====================================================================

struct BATTLEPASS_USER_DATA
{
	int aIndex;
	BYTE Level;
	DWORD Exp;
	BYTE SeasonId;
	BYTE ClaimedFree[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip1[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip2[MAX_BATTLEPASS_LEVEL];
	BYTE ClaimedVip3[MAX_BATTLEPASS_LEVEL];
	bool Loaded;
};

// ====================================================================
// Main Class
// ====================================================================

class CCustomBattlePass
{
public:
	CCustomBattlePass();
	virtual ~CCustomBattlePass();
	void Init();
	void LoadXML(char* FilePath);

	// Packet handlers
	void CGBattlePassInfoReq(int aIndex);
	void CGBattlePassClaimReq(PMSG_BATTLEPASS_CLAIM_REQ* lpMsg, int aIndex);

	// EXP Sources
	void OnMonsterKill(LPOBJ lpObj, LPOBJ lpMonster);
	void OnPlayerKill(LPOBJ lpKiller, LPOBJ lpVictim);
	void OnReset(LPOBJ lpObj);
	void OnBossKill(LPOBJ lpObj, int MonsterClass);

	// Core
	void AddExp(int aIndex, DWORD amount, int sourceType);
	void CheckLevelUp(int aIndex);
	bool ClaimReward(int aIndex, int level, int tier);
	void GiveReward(int aIndex, BATTLEPASS_REWARD* reward);

	// DataServer communication
	void DGBattlePassLoadRecv(LPBYTE lpMsg);
	void GDBattlePassSave(int aIndex);
	void GDBattlePassLoad(int aIndex);

	// Send packets to client
	void GCBattlePassInfoSend(int aIndex);
	void GCBattlePassClaimResult(int aIndex, BYTE result, BYTE level, BYTE tier);
	void GCBattlePassExpUpdate(int aIndex, DWORD addedExp);

	// Utility
	DWORD GetRequiredExp(int level);
	BATTLEPASS_LEVEL_CONFIG* GetLevelConfig(int level);

private:
	int Enable;
	BATTLEPASS_SEASON_CONFIG m_SeasonConfig;
	std::map<int, BATTLEPASS_LEVEL_CONFIG> m_LevelConfigs;
	std::vector<BATTLEPASS_EXP_SOURCE> m_ExpSources;
	BATTLEPASS_USER_DATA m_UserData[MAX_OBJECT_USER];
};

extern CCustomBattlePass gCustomBattlePass;
