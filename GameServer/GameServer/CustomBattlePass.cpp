// CustomBattlePass.cpp: implementation of the CCustomBattlePass class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomBattlePass.h"
#include "Util.h"
#include "Notice.h"
#include "DSProtocol.h"
#include "ItemManager.h"
#include "ObjectManager.h"
#include "ServerInfo.h"
#include "CashShop.h"
#include "Message.h"
#include "pugiconfig.hpp"
#include "pugixml.hpp"

CCustomBattlePass gCustomBattlePass;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomBattlePass::CCustomBattlePass()
{
	this->Init();
}

CCustomBattlePass::~CCustomBattlePass()
{
}

void CCustomBattlePass::Init()
{
	this->Enable = 0;
	memset(&this->m_SeasonConfig, 0, sizeof(this->m_SeasonConfig));
	this->m_LevelConfigs.clear();
	this->m_ExpSources.clear();

	for (int i = 0; i < MAX_OBJECT_USER; i++)
	{
		memset(&this->m_UserData[i], 0, sizeof(BATTLEPASS_USER_DATA));
		this->m_UserData[i].Loaded = false;
	}
}

// ====================================================================
// XML Configuration Loader
// ====================================================================
// Example XML: Data/Custom/CustomBattlePass.xml
//
// <BattlePass Enable="1">
//   <Season Id="1" Name="Season 1" MaxLevel="100"/>
//   <ExpSources>
//     <Source Type="0" MonsterID="-1" MinLevel="0" Map="-1" Exp="10"/>   <!-- MonsterKill -->
//     <Source Type="1" MonsterID="-1" MinLevel="0" Map="-1" Exp="500"/>  <!-- Reset -->
//     <Source Type="2" MonsterID="43" MinLevel="0" Map="-1" Exp="100"/>  <!-- BossKill -->
//   </ExpSources>
//   <Levels>
//     <Level Num="1" RequiredExp="100">
//       <Free WC="0" WP="0" GP="0" Ruud="0" Zen="100000">
//         <Item Type="12" Index="15" Level="0" Dur="0" Skill="0" Luck="0" Opt="0" Exc="0" Anc="0" JOH="0" Sk1="255" Sk2="255" Sk3="255" Sk4="255" Sk5="255" SkBonus="255" Duration="0"/>
//       </Free>
//       <Vip1 WC="100" WP="0" GP="0" Ruud="50" Zen="500000">
//         <Item Type="0" Index="19" Level="11" Dur="255" Skill="1" Luck="1" Opt="4" Exc="63" Anc="0" JOH="0" Sk1="255" Sk2="255" Sk3="255" Sk4="255" Sk5="255" SkBonus="255" Duration="0"/>
//       </Vip1>
//       <Vip2 WC="200" WP="100" GP="0" Ruud="100" Zen="1000000">
//       </Vip2>
//       <Vip3 WC="500" WP="200" GP="100" Ruud="200" Zen="2000000">
//       </Vip3>
//     </Level>
//   </Levels>
// </BattlePass>

void CCustomBattlePass::LoadXML(char* FilePath)
{
	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(FilePath);

	if (res.status != pugi::status_ok)
	{
		ErrorMessageBox("[BattlePass] File %s load fail. Error: %s", FilePath, res.description());
		return;
	}

	pugi::xml_node root = file.child("BattlePass");
	this->Enable = root.attribute("Enable").as_int();

	if (this->Enable == 0)
	{
		LogAdd(LOG_BLACK, "[BattlePass] System disabled");
		return;
	}

	// Season Config
	pugi::xml_node season = root.child("Season");
	this->m_SeasonConfig.SeasonId = season.attribute("Id").as_int(1);
	this->m_SeasonConfig.MaxLevel = season.attribute("MaxLevel").as_int(MAX_BATTLEPASS_LEVEL);
	this->m_SeasonConfig.Enable = 1;
	strncpy_s(this->m_SeasonConfig.SeasonName, season.attribute("Name").as_string("Season 1"), sizeof(this->m_SeasonConfig.SeasonName));

	// EXP Sources
	this->m_ExpSources.clear();
	pugi::xml_node expSources = root.child("ExpSources");
	for (pugi::xml_node src = expSources.child("Source"); src; src = src.next_sibling("Source"))
	{
		BATTLEPASS_EXP_SOURCE info;
		info.Type = src.attribute("Type").as_int();
		info.MonsterID = src.attribute("MonsterID").as_int(-1);
		info.MinLevel = src.attribute("MinLevel").as_int(0);
		info.MapNumber = src.attribute("Map").as_int(-1);
		info.ExpAmount = src.attribute("Exp").as_uint(10);
		this->m_ExpSources.push_back(info);
	}

	// Level Configs
	this->m_LevelConfigs.clear();
	pugi::xml_node levels = root.child("Levels");
	for (pugi::xml_node lvl = levels.child("Level"); lvl; lvl = lvl.next_sibling("Level"))
	{
		BATTLEPASS_LEVEL_CONFIG config;
		memset(&config, 0, sizeof(config));
		config.Level = lvl.attribute("Num").as_int();
		config.RequiredExp = lvl.attribute("RequiredExp").as_uint(100);

		// Parse rewards for each tier
		const char* tierNames[4] = { "Free", "Vip1", "Vip2", "Vip3" };
		BATTLEPASS_REWARD* rewards[4] = { &config.RewardFree, &config.RewardVip1, &config.RewardVip2, &config.RewardVip3 };

		for (int t = 0; t < 4; t++)
		{
			pugi::xml_node tierNode = lvl.child(tierNames[t]);
			if (tierNode)
			{
				rewards[t]->WCoin = tierNode.attribute("WC").as_int(0);
				rewards[t]->WPoint = tierNode.attribute("WP").as_int(0);
				rewards[t]->GoblinPoint = tierNode.attribute("GP").as_int(0);
				rewards[t]->Ruud = tierNode.attribute("Ruud").as_int(0);
				rewards[t]->Zen = tierNode.attribute("Zen").as_int(0);

				for (pugi::xml_node item = tierNode.child("Item"); item; item = item.next_sibling("Item"))
				{
					BATTLEPASS_REWARD_ITEM ri;
					memset(&ri, 0, sizeof(ri));
					ri.ItemIndex = SafeGetItem(GET_ITEM(item.attribute("Type").as_int(), item.attribute("Index").as_int()));
					ri.ItemLevel = item.attribute("Level").as_int(0);
					ri.ItemDurability = item.attribute("Dur").as_int(0);
					ri.ItemOption1 = item.attribute("Skill").as_int(0);
					ri.ItemOption2 = item.attribute("Luck").as_int(0);
					ri.ItemOption3 = item.attribute("Opt").as_int(0);
					ri.ItemNewOption = item.attribute("Exc").as_int(0);
					ri.SetOption = item.attribute("Anc").as_int(0);
					ri.JewelOfHarmony = item.attribute("JOH").as_int(0);
					ri.SocketOption[0] = item.attribute("Sk1").as_int(0xFF);
					ri.SocketOption[1] = item.attribute("Sk2").as_int(0xFF);
					ri.SocketOption[2] = item.attribute("Sk3").as_int(0xFF);
					ri.SocketOption[3] = item.attribute("Sk4").as_int(0xFF);
					ri.SocketOption[4] = item.attribute("Sk5").as_int(0xFF);
					ri.SocketOptionBonus = item.attribute("SkBonus").as_int(0xFF);
					ri.Duration = item.attribute("Duration").as_int(0);
					rewards[t]->Items.push_back(ri);
				}
			}
		}

		this->m_LevelConfigs.insert(std::pair<int, BATTLEPASS_LEVEL_CONFIG>(config.Level, config));
	}

	LogAdd(LOG_BLACK, "[BattlePass] Loaded %d levels, %d exp sources, Season: %s",
		(int)this->m_LevelConfigs.size(), (int)this->m_ExpSources.size(), this->m_SeasonConfig.SeasonName);
}

// ====================================================================
// EXP Sources - hooks called from other systems
// ====================================================================

void CCustomBattlePass::OnMonsterKill(LPOBJ lpObj, LPOBJ lpMonster)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpObj->Index)) return;

	for (size_t i = 0; i < this->m_ExpSources.size(); i++)
	{
		BATTLEPASS_EXP_SOURCE* src = &this->m_ExpSources[i];
		if (src->Type != 0) continue; // 0 = MonsterKill

		if (src->MonsterID != -1 && src->MonsterID != lpMonster->Class)
			continue;

		if (src->MinLevel > 0 && lpMonster->Level < src->MinLevel)
			continue;

		if (src->MapNumber != -1 && src->MapNumber != lpObj->Map)
			continue;

		this->AddExp(lpObj->Index, src->ExpAmount, 0);
	}
}

void CCustomBattlePass::OnPlayerKill(LPOBJ lpKiller, LPOBJ lpVictim)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpKiller->Index)) return;

	for (size_t i = 0; i < this->m_ExpSources.size(); i++)
	{
		BATTLEPASS_EXP_SOURCE* src = &this->m_ExpSources[i];
		if (src->Type != 3) continue; // 3 = PlayerKill (reuse EventComplete slot)

		this->AddExp(lpKiller->Index, src->ExpAmount, 3);
	}
}

void CCustomBattlePass::OnReset(LPOBJ lpObj)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpObj->Index)) return;

	for (size_t i = 0; i < this->m_ExpSources.size(); i++)
	{
		BATTLEPASS_EXP_SOURCE* src = &this->m_ExpSources[i];
		if (src->Type != 1) continue; // 1 = Reset

		this->AddExp(lpObj->Index, src->ExpAmount, 1);
	}
}

void CCustomBattlePass::OnBossKill(LPOBJ lpObj, int MonsterClass)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpObj->Index)) return;

	for (size_t i = 0; i < this->m_ExpSources.size(); i++)
	{
		BATTLEPASS_EXP_SOURCE* src = &this->m_ExpSources[i];
		if (src->Type != 2) continue; // 2 = BossKill

		if (src->MonsterID != -1 && src->MonsterID != MonsterClass)
			continue;

		this->AddExp(lpObj->Index, src->ExpAmount, 2);
	}
}

// ====================================================================
// Core Logic
// ====================================================================

void CCustomBattlePass::AddExp(int aIndex, DWORD amount, int sourceType)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;

	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return;

	if (data->Level >= this->m_SeasonConfig.MaxLevel)
		return;

	data->Exp += amount;
	this->CheckLevelUp(aIndex);
	this->GCBattlePassExpUpdate(aIndex, amount);
}

void CCustomBattlePass::CheckLevelUp(int aIndex)
{
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];

	while (data->Level < this->m_SeasonConfig.MaxLevel)
	{
		DWORD reqExp = this->GetRequiredExp(data->Level + 1);
		if (reqExp == 0 || data->Exp < reqExp)
			break;

		data->Exp -= reqExp;
		data->Level++;

		LPOBJ lpObj = &gObj[aIndex];
		LogAdd(LOG_BLACK, "[BattlePass] [%s][%s] Level Up -> %d",
			lpObj->Account, lpObj->Name, data->Level);
	}

	this->GDBattlePassSave(aIndex);
}

bool CCustomBattlePass::ClaimReward(int aIndex, int level, int tier)
{
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return false;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return false;

	LPOBJ lpObj = &gObj[aIndex];

	// Check level reached
	if (data->Level < level)
	{
		this->GCBattlePassClaimResult(aIndex, 3, (BYTE)level, (BYTE)tier); // level not reached
		return false;
	}

	// Check VIP requirement
	if (tier > 0 && lpObj->Vip < tier)
	{
		this->GCBattlePassClaimResult(aIndex, 4, (BYTE)level, (BYTE)tier); // vip required
		return false;
	}

	// Check already claimed
	BYTE* claimArray = NULL;
	switch (tier)
	{
	case 0: claimArray = data->ClaimedFree; break;
	case 1: claimArray = data->ClaimedVip1; break;
	case 2: claimArray = data->ClaimedVip2; break;
	case 3: claimArray = data->ClaimedVip3; break;
	default: return false;
	}

	if (level < 1 || level > MAX_BATTLEPASS_LEVEL) return false;

	if (claimArray[level - 1] == 1)
	{
		this->GCBattlePassClaimResult(aIndex, 2, (BYTE)level, (BYTE)tier); // already claimed
		return false;
	}

	// Get reward config
	BATTLEPASS_LEVEL_CONFIG* config = this->GetLevelConfig(level);
	if (config == NULL)
	{
		this->GCBattlePassClaimResult(aIndex, 0, (BYTE)level, (BYTE)tier);
		return false;
	}

	BATTLEPASS_REWARD* reward = NULL;
	switch (tier)
	{
	case 0: reward = &config->RewardFree; break;
	case 1: reward = &config->RewardVip1; break;
	case 2: reward = &config->RewardVip2; break;
	case 3: reward = &config->RewardVip3; break;
	}

	if (reward == NULL)
	{
		this->GCBattlePassClaimResult(aIndex, 0, (BYTE)level, (BYTE)tier);
		return false;
	}

	// Give reward
	this->GiveReward(aIndex, reward);

	// Mark claimed
	claimArray[level - 1] = 1;

	// Save to DataServer
	this->GDBattlePassSave(aIndex);

	this->GCBattlePassClaimResult(aIndex, 1, (BYTE)level, (BYTE)tier); // success

	LogAdd(LOG_BLACK, "[BattlePass] [%s][%s] Claimed Level %d Tier %d",
		lpObj->Account, lpObj->Name, level, tier);

	return true;
}

void CCustomBattlePass::GiveReward(int aIndex, BATTLEPASS_REWARD* reward)
{
	LPOBJ lpObj = &gObj[aIndex];

	// Currency rewards
	if (reward->Zen > 0)
	{
		lpObj->Money += reward->Zen;
		GCMoneySend(aIndex, lpObj->Money);
	}

	if (reward->WCoin > 0)
	{
		gCashShop.GDCashShopAddPointSaveSend(lpObj->Index, 0, reward->WCoin, 0, 0, 0);
	}

	if (reward->WPoint > 0)
	{
		gCashShop.GDCashShopAddPointSaveSend(lpObj->Index, 0, 0, reward->WPoint, 0, 0);
	}

	if (reward->GoblinPoint > 0)
	{
		gCashShop.GDCashShopAddPointSaveSend(lpObj->Index, 0, 0, 0, reward->GoblinPoint, 0);
	}

	if (reward->Ruud > 0)
	{
		gCashShop.GDCashShopAddPointSaveSend(lpObj->Index, 0, 0, 0, 0, reward->Ruud);
	}

	// Item rewards
	for (size_t i = 0; i < reward->Items.size(); i++)
	{
		BATTLEPASS_REWARD_ITEM* ri = &reward->Items[i];

		BYTE SocketOption[MAX_SOCKET_OPTION];
		SocketOption[0] = (BYTE)ri->SocketOption[0];
		SocketOption[1] = (BYTE)ri->SocketOption[1];
		SocketOption[2] = (BYTE)ri->SocketOption[2];
		SocketOption[3] = (BYTE)ri->SocketOption[3];
		SocketOption[4] = (BYTE)ri->SocketOption[4];

		GDCreateItemSend(aIndex, 0xEB, (BYTE)lpObj->X, (BYTE)lpObj->Y, ri->ItemIndex,
			(BYTE)ri->ItemLevel, (BYTE)ri->ItemDurability,
			(BYTE)ri->ItemOption1, (BYTE)ri->ItemOption2, (BYTE)ri->ItemOption3,
			aIndex, (BYTE)ri->ItemNewOption, (BYTE)ri->SetOption,
			(BYTE)ri->JewelOfHarmony, 0,
			SocketOption, (BYTE)ri->SocketOptionBonus, ri->Duration);
	}
}

// ====================================================================
// DataServer Communication
// ====================================================================

void CCustomBattlePass::GDBattlePassLoad(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;

	LPOBJ lpObj = &gObj[aIndex];

	SDHP_BATTLEPASS_LOAD_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0x50, 0x01, sizeof(pMsg));
	pMsg.index = (WORD)aIndex;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	memcpy(pMsg.name, lpObj->Name, sizeof(pMsg.name));

	// Send to DataServer via gDataServerConnection
	// DataSend(DataServerSocket, (BYTE*)&pMsg, sizeof(pMsg));
}

void CCustomBattlePass::DGBattlePassLoadRecv(LPBYTE lpMsg)
{
	SDHP_BATTLEPASS_LOAD_RECV* pMsg = (SDHP_BATTLEPASS_LOAD_RECV*)lpMsg;

	if (!OBJECT_USER_RANGE(pMsg->index)) return;

	int rIndex = pMsg->index - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];
	data->aIndex = pMsg->index;
	data->Level = pMsg->BPLevel;
	data->Exp = pMsg->BPExp;
	data->SeasonId = pMsg->SeasonId;
	memcpy(data->ClaimedFree, pMsg->ClaimedFree, MAX_BATTLEPASS_LEVEL);
	memcpy(data->ClaimedVip1, pMsg->ClaimedVip1, MAX_BATTLEPASS_LEVEL);
	memcpy(data->ClaimedVip2, pMsg->ClaimedVip2, MAX_BATTLEPASS_LEVEL);
	memcpy(data->ClaimedVip3, pMsg->ClaimedVip3, MAX_BATTLEPASS_LEVEL);
	data->Loaded = true;

	// If season changed, reset progress
	if (data->SeasonId != this->m_SeasonConfig.SeasonId)
	{
		data->Level = 0;
		data->Exp = 0;
		data->SeasonId = (BYTE)this->m_SeasonConfig.SeasonId;
		memset(data->ClaimedFree, 0, MAX_BATTLEPASS_LEVEL);
		memset(data->ClaimedVip1, 0, MAX_BATTLEPASS_LEVEL);
		memset(data->ClaimedVip2, 0, MAX_BATTLEPASS_LEVEL);
		memset(data->ClaimedVip3, 0, MAX_BATTLEPASS_LEVEL);
		this->GDBattlePassSave(pMsg->index);
	}

	// Send current info to client
	this->GCBattlePassInfoSend(pMsg->index);
}

void CCustomBattlePass::GDBattlePassSave(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;

	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return;

	LPOBJ lpObj = &gObj[aIndex];

	SDHP_BATTLEPASS_SAVE_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0x51, sizeof(pMsg));
	pMsg.index = (WORD)aIndex;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	memcpy(pMsg.name, lpObj->Name, sizeof(pMsg.name));
	pMsg.BPLevel = data->Level;
	pMsg.BPExp = data->Exp;
	pMsg.SeasonId = data->SeasonId;
	memcpy(pMsg.ClaimedFree, data->ClaimedFree, MAX_BATTLEPASS_LEVEL);
	memcpy(pMsg.ClaimedVip1, data->ClaimedVip1, MAX_BATTLEPASS_LEVEL);
	memcpy(pMsg.ClaimedVip2, data->ClaimedVip2, MAX_BATTLEPASS_LEVEL);
	memcpy(pMsg.ClaimedVip3, data->ClaimedVip3, MAX_BATTLEPASS_LEVEL);

	// Send to DataServer
	// DataSend(DataServerSocket, (BYTE*)&pMsg, sizeof(pMsg));
}

// ====================================================================
// Client Packet Handlers
// ====================================================================

void CCustomBattlePass::CGBattlePassInfoReq(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	this->GCBattlePassInfoSend(aIndex);
}

void CCustomBattlePass::CGBattlePassClaimReq(PMSG_BATTLEPASS_CLAIM_REQ* lpMsg, int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	this->ClaimReward(aIndex, lpMsg->Level, lpMsg->Tier);
}

// ====================================================================
// Send Packets to Client
// ====================================================================

void CCustomBattlePass::GCBattlePassInfoSend(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;

	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];
	LPOBJ lpObj = &gObj[aIndex];

	int levelCount = (int)this->m_LevelConfigs.size();

	BYTE buffer[8192];
	memset(buffer, 0, sizeof(buffer));

	PMSG_BATTLEPASS_INFO_SEND* pMsg = (PMSG_BATTLEPASS_INFO_SEND*)buffer;
	pMsg->CurrentLevel = data->Level;
	pMsg->CurrentExp = data->Exp;
	pMsg->NextLevelExp = this->GetRequiredExp(data->Level + 1);
	pMsg->TotalExp = data->Exp; // cumulative
	pMsg->VipLevel = lpObj->Vip;
	pMsg->SeasonId = data->SeasonId;
	pMsg->count = (BYTE)levelCount;

	int offset = sizeof(PMSG_BATTLEPASS_INFO_SEND);

	for (auto it = this->m_LevelConfigs.begin(); it != this->m_LevelConfigs.end(); it++)
	{
		PMSG_BATTLEPASS_LEVEL_INFO info;
		info.Level = (BYTE)it->second.Level;
		info.RequiredExp = it->second.RequiredExp;
		info.ClaimedFree = (it->second.Level <= MAX_BATTLEPASS_LEVEL) ? data->ClaimedFree[it->second.Level - 1] : 0;
		info.ClaimedVip1 = (it->second.Level <= MAX_BATTLEPASS_LEVEL) ? data->ClaimedVip1[it->second.Level - 1] : 0;
		info.ClaimedVip2 = (it->second.Level <= MAX_BATTLEPASS_LEVEL) ? data->ClaimedVip2[it->second.Level - 1] : 0;
		info.ClaimedVip3 = (it->second.Level <= MAX_BATTLEPASS_LEVEL) ? data->ClaimedVip3[it->second.Level - 1] : 0;

		memcpy(&buffer[offset], &info, sizeof(info));
		offset += sizeof(PMSG_BATTLEPASS_LEVEL_INFO);
	}

	pMsg->header.set(0xF3, 0xA0, (WORD)offset);
	DataSend(aIndex, buffer, offset);
}

void CCustomBattlePass::GCBattlePassClaimResult(int aIndex, BYTE result, BYTE level, BYTE tier)
{
	PMSG_BATTLEPASS_CLAIM_RESULT pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xA1, sizeof(pMsg));
	pMsg.Result = result;
	pMsg.Level = level;
	pMsg.Tier = tier;
	DataSend(aIndex, (BYTE*)&pMsg, sizeof(pMsg));
}

void CCustomBattlePass::GCBattlePassExpUpdate(int aIndex, DWORD addedExp)
{
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	BATTLEPASS_USER_DATA* data = &this->m_UserData[rIndex];

	PMSG_BATTLEPASS_EXP_UPDATE pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xA2, sizeof(pMsg));
	pMsg.CurrentLevel = data->Level;
	pMsg.CurrentExp = data->Exp;
	pMsg.NextLevelExp = this->GetRequiredExp(data->Level + 1);
	pMsg.AddedExp = addedExp;
	DataSend(aIndex, (BYTE*)&pMsg, sizeof(pMsg));
}

// ====================================================================
// Utility
// ====================================================================

DWORD CCustomBattlePass::GetRequiredExp(int level)
{
	auto it = this->m_LevelConfigs.find(level);
	if (it != this->m_LevelConfigs.end())
	{
		return it->second.RequiredExp;
	}
	return 0;
}

BATTLEPASS_LEVEL_CONFIG* CCustomBattlePass::GetLevelConfig(int level)
{
	auto it = this->m_LevelConfigs.find(level);
	if (it != this->m_LevelConfigs.end())
	{
		return &it->second;
	}
	return NULL;
}
