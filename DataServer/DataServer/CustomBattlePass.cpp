// CustomBattlePass.cpp: implementation for DataServer
#include "stdafx.h"
#include "CustomBattlePass.h"
#include "QueryManager.h"
#include "SocketManager.h"
#include "Util.h"

CCustomBattlePassDS gCustomBattlePassDS;

CCustomBattlePassDS::CCustomBattlePassDS()
{
}

CCustomBattlePassDS::~CCustomBattlePassDS()
{
}

void CCustomBattlePassDS::GDBattlePassLoadRecv(SDHP_BATTLEPASS_LOAD_SEND* lpMsg, int index)
{
	this->LoadBattlePassData(lpMsg->account, lpMsg->name, lpMsg->index);
}

void CCustomBattlePassDS::GDBattlePassSaveRecv(SDHP_BATTLEPASS_SAVE_SEND* lpMsg)
{
	this->SaveBattlePassData(lpMsg->account, lpMsg->name, lpMsg->BPLevel, lpMsg->BPExp, 
							lpMsg->SeasonId, lpMsg->ClaimedFree, lpMsg->ClaimedVip1, 
							lpMsg->ClaimedVip2, lpMsg->ClaimedVip3);
}

void CCustomBattlePassDS::LoadBattlePassData(char* account, char* name, WORD index)
{
	SDHP_BATTLEPASS_LOAD_RECV pMsg;
	
	pMsg.header.set(0x50, sizeof(pMsg));
	pMsg.index = index;
	memcpy(pMsg.account, account, sizeof(pMsg.account));
	memcpy(pMsg.name, name, sizeof(pMsg.name));
	pMsg.result = 0;
	pMsg.BPLevel = 0;
	pMsg.BPExp = 0;
	pMsg.SeasonId = 1;
	memset(pMsg.ClaimedFree, 0, sizeof(pMsg.ClaimedFree));
	memset(pMsg.ClaimedVip1, 0, sizeof(pMsg.ClaimedVip1));
	memset(pMsg.ClaimedVip2, 0, sizeof(pMsg.ClaimedVip2));
	memset(pMsg.ClaimedVip3, 0, sizeof(pMsg.ClaimedVip3));
	
	if(gQueryManager.ExecQuery("SELECT * FROM CustomBattlePass WHERE AccountID='%s' AND CharName='%s'", account, name) == 0 || gQueryManager.Fetch() == SQL_NO_DATA)
	{
		gQueryManager.Close();
		// Insert new record
		gQueryManager.ExecQuery("INSERT INTO CustomBattlePass (AccountID, CharName, BPLevel, BPExp, SeasonId) VALUES ('%s', '%s', 0, 0, 1)", account, name);
		gQueryManager.Close();
		pMsg.result = 1; // New character
	}
	else
	{
		pMsg.result = 1;
		pMsg.BPLevel = gQueryManager.GetAsInteger("BPLevel");
		pMsg.BPExp = gQueryManager.GetAsInteger("BPExp");
		pMsg.SeasonId = gQueryManager.GetAsInteger("SeasonId");
		
		BYTE* claimedFree = (BYTE*)gQueryManager.GetAsBinary("ClaimedFree");
		BYTE* claimedVip1 = (BYTE*)gQueryManager.GetAsBinary("ClaimedVip1");
		BYTE* claimedVip2 = (BYTE*)gQueryManager.GetAsBinary("ClaimedVip2");
		BYTE* claimedVip3 = (BYTE*)gQueryManager.GetAsBinary("ClaimedVip3");
		
		if(claimedFree) memcpy(pMsg.ClaimedFree, claimedFree, sizeof(pMsg.ClaimedFree));
		if(claimedVip1) memcpy(pMsg.ClaimedVip1, claimedVip1, sizeof(pMsg.ClaimedVip1));
		if(claimedVip2) memcpy(pMsg.ClaimedVip2, claimedVip2, sizeof(pMsg.ClaimedVip2));
		if(claimedVip3) memcpy(pMsg.ClaimedVip3, claimedVip3, sizeof(pMsg.ClaimedVip3));
		
		gQueryManager.Close();
	}
	
	gSocketManager.DataSend((int)index, (BYTE*)&pMsg, pMsg.header.size);
}

void CCustomBattlePassDS::SaveBattlePassData(char* account, char* name, BYTE level, DWORD exp, BYTE seasonId,
											 BYTE* claimedFree, BYTE* claimedVip1, BYTE* claimedVip2, BYTE* claimedVip3)
{
	char szClaimedFree[MAX_BATTLEPASS_LEVEL * 2 + 1] = {0};
	char szClaimedVip1[MAX_BATTLEPASS_LEVEL * 2 + 1] = {0};
	char szClaimedVip2[MAX_BATTLEPASS_LEVEL * 2 + 1] = {0};
	char szClaimedVip3[MAX_BATTLEPASS_LEVEL * 2 + 1] = {0};
	
	gQueryManager.ConvertBinaryToString(claimedFree, MAX_BATTLEPASS_LEVEL, szClaimedFree, sizeof(szClaimedFree));
	gQueryManager.ConvertBinaryToString(claimedVip1, MAX_BATTLEPASS_LEVEL, szClaimedVip1, sizeof(szClaimedVip1));
	gQueryManager.ConvertBinaryToString(claimedVip2, MAX_BATTLEPASS_LEVEL, szClaimedVip2, sizeof(szClaimedVip2));
	gQueryManager.ConvertBinaryToString(claimedVip3, MAX_BATTLEPASS_LEVEL, szClaimedVip3, sizeof(szClaimedVip3));
	
	gQueryManager.ExecQuery("UPDATE CustomBattlePass SET BPLevel=%d, BPExp=%d, SeasonId=%d, ClaimedFree=0x%s, ClaimedVip1=0x%s, ClaimedVip2=0x%s, ClaimedVip3=0x%s WHERE AccountID='%s' AND CharName='%s'",
							level, exp, seasonId,
							szClaimedFree, szClaimedVip1, szClaimedVip2, szClaimedVip3,
							account, name);
	gQueryManager.Close();
}
