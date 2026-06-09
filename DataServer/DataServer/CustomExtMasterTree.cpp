// CustomExtMasterTree.cpp: implementation for DataServer
#include "stdafx.h"
#include "CustomExtMasterTree.h"
#include "QueryManager.h"
#include "SocketManager.h"
#include "Util.h"

CCustomExtMasterTreeDS gCustomExtMasterTreeDS;

CCustomExtMasterTreeDS::CCustomExtMasterTreeDS()
{
}

CCustomExtMasterTreeDS::~CCustomExtMasterTreeDS()
{
}

void CCustomExtMasterTreeDS::GDExtMasterLoadRecv(SDHP_EXT_MASTER_LOAD_SEND* lpMsg, int index)
{
	this->LoadExtMasterData(lpMsg->account, lpMsg->name, lpMsg->index);
}

void CCustomExtMasterTreeDS::GDExtMasterSaveRecv(SDHP_EXT_MASTER_SAVE_SEND* lpMsg)
{
	this->SaveExtMasterData(lpMsg->name, lpMsg->TotalPoints, lpMsg->UsedPoints, lpMsg->SkillLevels);
}

void CCustomExtMasterTreeDS::LoadExtMasterData(char* account, char* name, WORD index)
{
	SDHP_EXT_MASTER_LOAD_RECV pMsg;
	
	pMsg.header.set(0x52, sizeof(pMsg));
	pMsg.index = index;
	memcpy(pMsg.name, name, sizeof(pMsg.name));
	pMsg.result = 0;
	pMsg.TotalPoints = 0;
	pMsg.UsedPoints = 0;
	memset(pMsg.SkillLevels, 0, sizeof(pMsg.SkillLevels));
	
	if(gQueryManager.ExecQuery("SELECT * FROM CustomExtMasterTree WHERE CharName='%s'", name) == 0 || gQueryManager.Fetch() == SQL_NO_DATA)
	{
		gQueryManager.Close();
		// Insert new record
		gQueryManager.ExecQuery("INSERT INTO CustomExtMasterTree (CharName, TotalPoints, UsedPoints, SkillLevels) VALUES ('%s', 0, 0, 0x00)", name);
		gQueryManager.Close();
		pMsg.result = 1; // New character
	}
	else
	{
		pMsg.result = 1;
		pMsg.TotalPoints = gQueryManager.GetAsInteger("TotalPoints");
		pMsg.UsedPoints = gQueryManager.GetAsInteger("UsedPoints");
		
		BYTE* skillLevels = (BYTE*)gQueryManager.GetAsBinary("SkillLevels");
		if(skillLevels) memcpy(pMsg.SkillLevels, skillLevels, sizeof(pMsg.SkillLevels));
		
		gQueryManager.Close();
	}
	
	gSocketManager.DataSend((int)index, (BYTE*)&pMsg, pMsg.header.size);
}

void CCustomExtMasterTreeDS::SaveExtMasterData(char* name, WORD totalPoints, WORD usedPoints, BYTE* skillLevels)
{
	char szSkillLevels[MAX_EXT_SKILL_TREE_SLOTS * 2 + 1] = {0};
	
	gQueryManager.ConvertBinaryToString(skillLevels, MAX_EXT_SKILL_TREE_SLOTS, szSkillLevels, sizeof(szSkillLevels));
	
	gQueryManager.ExecQuery("UPDATE CustomExtMasterTree SET TotalPoints=%d, UsedPoints=%d, SkillLevels=0x%s WHERE CharName='%s'",
							totalPoints, usedPoints, szSkillLevels, name);
	gQueryManager.Close();
}
