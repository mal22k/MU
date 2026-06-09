// CustomExtMasterTree.h: Extended Master Skill Tree system for DataServer
//////////////////////////////////////////////////////////////////////

#pragma once

#include "protocol.h"

#define MAX_EXT_SKILL_TREE_SLOTS 50

// DataServer <-> GameServer packets
struct SDHP_EXT_MASTER_LOAD_SEND
{
	PSBMSG_HEAD header; // C1:52:01
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

class CCustomExtMasterTreeDS
{
public:
	CCustomExtMasterTreeDS();
	virtual ~CCustomExtMasterTreeDS();
	
	void GDExtMasterLoadRecv(SDHP_EXT_MASTER_LOAD_SEND* lpMsg, int index);
	void GDExtMasterSaveRecv(SDHP_EXT_MASTER_SAVE_SEND* lpMsg);
	
private:
	void LoadExtMasterData(char* account, char* name, WORD index);
	void SaveExtMasterData(char* name, WORD totalPoints, WORD usedPoints, BYTE* skillLevels);
};

extern CCustomExtMasterTreeDS gCustomExtMasterTreeDS;
