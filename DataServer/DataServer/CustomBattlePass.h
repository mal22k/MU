// CustomBattlePass.h: interface for the CCustomBattlePass class in DataServer
//////////////////////////////////////////////////////////////////////

#pragma once

#include "protocol.h"

#define MAX_BATTLEPASS_LEVEL 100

// DataServer <-> GameServer packets
struct SDHP_BATTLEPASS_LOAD_SEND
{
	PSBMSG_HEAD header; // C1:50:01
	WORD index;
	char account[11];
	char name[11];
};

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

class CCustomBattlePassDS
{
public:
	CCustomBattlePassDS();
	virtual ~CCustomBattlePassDS();
	
	void GDBattlePassLoadRecv(SDHP_BATTLEPASS_LOAD_SEND* lpMsg, int index);
	void GDBattlePassSaveRecv(SDHP_BATTLEPASS_SAVE_SEND* lpMsg);
	
private:
	void LoadBattlePassData(char* account, char* name, WORD index);
	void SaveBattlePassData(char* account, char* name, BYTE level, DWORD exp, BYTE seasonId, 
						   BYTE* claimedFree, BYTE* claimedVip1, BYTE* claimedVip2, BYTE* claimedVip3);
};

extern CCustomBattlePassDS gCustomBattlePassDS;
