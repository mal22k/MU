// CustomExtMasterTree.cpp: Extended Master Skill Tree implementation
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomExtMasterTree.h"
#include "Util.h"
#include "Notice.h"
#include "DSProtocol.h"
#include "ObjectManager.h"
#include "ServerInfo.h"
#include "Message.h"
#include "pugiconfig.hpp"
#include "pugixml.hpp"

CCustomExtMasterTree gCustomExtMasterTree;

CCustomExtMasterTree::CCustomExtMasterTree()
{
	this->Init();
}

CCustomExtMasterTree::~CCustomExtMasterTree()
{
}

void CCustomExtMasterTree::Init()
{
	this->Enable = 0;
	this->PointsPerMasterLevel = 1;
	this->PointsPerReset = 0;
	this->m_BonusInfo.clear();

	for (int i = 0; i < MAX_OBJECT_USER; i++)
	{
		memset(&this->m_UserData[i], 0, sizeof(EXT_MASTER_USER_DATA));
		this->m_UserData[i].Loaded = false;
	}
}

// ====================================================================
// XML Loader
// ====================================================================
// Example: Data/Custom/CustomExtMasterTree.xml
//
// <ExtMasterTree Enable="1" PointsPerMasterLevel="1" PointsPerReset="5">
//   <Skills>
//     <Skill Index="0" Group="0" Row="0" Col="0" MinLevel="1" MaxLevel="20"
//            Name="Fuerza Suprema" Desc="+5 Strength por nivel"
//            ValuePerLevel="5.0" BonusType="12"
//            Require1="-1" Require2="-1"
//            DW="1" DK="1" ELF="1" MG="1" DL="1" SUM="1" RF="1"/>
//   </Skills>
// </ExtMasterTree>

void CCustomExtMasterTree::LoadXML(char* FilePath)
{
	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(FilePath);

	if (res.status != pugi::status_ok)
	{
		ErrorMessageBox("[ExtMasterTree] File %s load fail. Error: %s", FilePath, res.description());
		return;
	}

	pugi::xml_node root = file.child("ExtMasterTree");
	this->Enable = root.attribute("Enable").as_int();
	this->PointsPerMasterLevel = root.attribute("PointsPerMasterLevel").as_int(1);
	this->PointsPerReset = root.attribute("PointsPerReset").as_int(0);

	if (this->Enable == 0)
	{
		LogAdd(LOG_BLACK, "[ExtMasterTree] System disabled");
		return;
	}

	this->m_BonusInfo.clear();

	pugi::xml_node skills = root.child("Skills");
	for (pugi::xml_node sk = skills.child("Skill"); sk; sk = sk.next_sibling("Skill"))
	{
		EXT_MASTER_BONUS_INFO info;
		memset(&info, 0, sizeof(info));

		info.Index = sk.attribute("Index").as_int();
		info.Group = sk.attribute("Group").as_int();
		info.Row = sk.attribute("Row").as_int();
		info.Column = sk.attribute("Col").as_int();
		info.MinLevel = sk.attribute("MinLevel").as_int(1);
		info.MaxLevel = sk.attribute("MaxLevel").as_int(20);
		strncpy_s(info.Name, sk.attribute("Name").as_string(), sizeof(info.Name));
		strncpy_s(info.Description, sk.attribute("Desc").as_string(), sizeof(info.Description));
		info.ValuePerLevel = sk.attribute("ValuePerLevel").as_float(1.0f);
		info.BonusType = sk.attribute("BonusType").as_int(0);
		info.RequireSkill[0] = sk.attribute("Require1").as_int(-1);
		info.RequireSkill[1] = sk.attribute("Require2").as_int(-1);
		info.RequireClass[0] = sk.attribute("DW").as_int(0);
		info.RequireClass[1] = sk.attribute("DK").as_int(0);
		info.RequireClass[2] = sk.attribute("ELF").as_int(0);
		info.RequireClass[3] = sk.attribute("MG").as_int(0);
		info.RequireClass[4] = sk.attribute("DL").as_int(0);
		info.RequireClass[5] = sk.attribute("SUM").as_int(0);
		info.RequireClass[6] = sk.attribute("RF").as_int(0);

		if (info.Index >= 0 && info.Index < MAX_EXT_SKILL_TREE_SLOTS)
		{
			this->m_BonusInfo.insert(std::pair<int, EXT_MASTER_BONUS_INFO>(info.Index, info));
		}
	}

	LogAdd(LOG_BLACK, "[ExtMasterTree] Loaded %d bonus skills, PtsPerML=%d, PtsPerReset=%d",
		(int)this->m_BonusInfo.size(), this->PointsPerMasterLevel, this->PointsPerReset);
}

// ====================================================================
// Core Logic
// ====================================================================

bool CCustomExtMasterTree::InvestPoint(int aIndex, int slotIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return false;
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return false;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return false;

	LPOBJ lpObj = &gObj[aIndex];

	// Check slot exists
	EXT_MASTER_BONUS_INFO* bonus = this->GetBonusInfo(slotIndex);
	if (bonus == NULL)
	{
		this->GCExtMasterInvestResult(aIndex, 0, (BYTE)slotIndex, 0, (WORD)(data->TotalPoints - data->UsedPoints));
		return false;
	}

	// Check class
	if (!this->CheckClass(lpObj, slotIndex))
	{
		this->GCExtMasterInvestResult(aIndex, 5, (BYTE)slotIndex, 0, (WORD)(data->TotalPoints - data->UsedPoints));
		return false;
	}

	// Check points available
	if (data->UsedPoints >= data->TotalPoints)
	{
		this->GCExtMasterInvestResult(aIndex, 2, (BYTE)slotIndex, 0, 0);
		return false;
	}

	// Check max level
	if (data->SkillLevels[slotIndex] >= bonus->MaxLevel)
	{
		this->GCExtMasterInvestResult(aIndex, 3, (BYTE)slotIndex, (BYTE)data->SkillLevels[slotIndex], (WORD)(data->TotalPoints - data->UsedPoints));
		return false;
	}

	// Check prerequisites
	if (!this->CheckPrerequisites(aIndex, slotIndex))
	{
		this->GCExtMasterInvestResult(aIndex, 4, (BYTE)slotIndex, 0, (WORD)(data->TotalPoints - data->UsedPoints));
		return false;
	}

	// Invest
	data->SkillLevels[slotIndex]++;
	data->UsedPoints++;

	// Recalculate bonuses
	this->CalcExtMasterOptions(lpObj, true);

	// Save
	this->GDExtMasterSave(aIndex);

	// Send result
	this->GCExtMasterInvestResult(aIndex, 1, (BYTE)slotIndex, (BYTE)data->SkillLevels[slotIndex], (WORD)(data->TotalPoints - data->UsedPoints));

	return true;
}

void CCustomExtMasterTree::CalcExtMasterOptions(LPOBJ lpObj, bool flag)
{
	if (!OBJECT_USER_RANGE(lpObj->Index)) return;
	int rIndex = lpObj->Index - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return;

	for (auto it = this->m_BonusInfo.begin(); it != this->m_BonusInfo.end(); it++)
	{
		int idx = it->first;
		if (idx < 0 || idx >= MAX_EXT_SKILL_TREE_SLOTS) continue;
		if (data->SkillLevels[idx] <= 0) continue;

		this->ApplyBonus(lpObj, &it->second, data->SkillLevels[idx], flag);
	}
}

void CCustomExtMasterTree::ApplyBonus(LPOBJ lpObj, EXT_MASTER_BONUS_INFO* bonus, int level, bool flag)
{
	int value = (int)(bonus->ValuePerLevel * level);

	if (!flag) value = -value; // Remove bonus

	switch (bonus->BonusType)
	{
	case 0: // AddDamage
		lpObj->EffectOption.AddPhysiDamage += value;
		break;
	case 1: // AddDefense
		lpObj->EffectOption.AddDefense += value;
		break;
	case 2: // AddHP
		lpObj->MaxLife += (float)value;
		break;
	case 3: // AddMP
		lpObj->MaxMana += (float)value;
		break;
	case 4: // AddSD
		lpObj->MaxShield += value;
		break;
	case 5: // CritRate
		lpObj->CriticalDamageRate += value;
		break;
	case 6: // CritDmg
		lpObj->EffectOption.AddCriticalDamage += value;
		break;
	case 7: // ExcDmgRate
		lpObj->EffectOption.AddExcellentDamageRate += value;
		break;
	case 8: // IgnoreDefRate
		lpObj->IgnoreDefenseRate += value;
		break;
	case 9: // DoubleDmgRate
		lpObj->DoubleDamageRate += value;
		break;
	case 10: // TripleDmgRate
		lpObj->TripleDamageRate += value;
		break;
	case 11: // ReflectDmg
		lpObj->EffectOption.AddDamageReflect += value;
		break;
	case 12: // AddStr
		lpObj->Strength += value;
		break;
	case 13: // AddDex
		lpObj->Dexterity += value;
		break;
	case 14: // AddVit
		lpObj->Vitality += value;
		break;
	case 15: // AddEne
		lpObj->Energy += value;
		break;
	case 16: // AddLead
		lpObj->Leadership += value;
		break;
	case 17: // AddExpRate
		// Custom field - would need to be added to OBJECTSTRUCT
		break;
	case 18: // AddDropRate
		// Custom field
		break;
	case 19: // Custom - extensible
		break;
	}
}

void CCustomExtMasterTree::AddPoints(int aIndex, int amount)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return;

	data->TotalPoints += amount;
	this->GDExtMasterSave(aIndex);
}

void CCustomExtMasterTree::OnMasterLevelUp(LPOBJ lpObj)
{
	if (this->Enable == 0) return;
	if (this->PointsPerMasterLevel > 0)
	{
		this->AddPoints(lpObj->Index, this->PointsPerMasterLevel);
	}
}

void CCustomExtMasterTree::OnReset(LPOBJ lpObj, int pointsPerReset)
{
	if (this->Enable == 0) return;
	int pts = (pointsPerReset > 0) ? pointsPerReset : this->PointsPerReset;
	if (pts > 0)
	{
		this->AddPoints(lpObj->Index, pts);
	}
}

// ====================================================================
// Packet Handlers
// ====================================================================

void CCustomExtMasterTree::CGExtMasterInfoReq(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	this->GCExtMasterInfoSend(aIndex);
}

void CCustomExtMasterTree::CGExtMasterInvestReq(PMSG_EXT_MASTER_INVEST_REQ* lpMsg, int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	this->InvestPoint(aIndex, lpMsg->SlotIndex);
}

// ====================================================================
// Send to Client
// ====================================================================

void CCustomExtMasterTree::GCExtMasterInfoSend(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];

	int slotCount = (int)this->m_BonusInfo.size();

	BYTE buffer[16384];
	memset(buffer, 0, sizeof(buffer));

	PMSG_EXT_MASTER_INFO_SEND* pMsg = (PMSG_EXT_MASTER_INFO_SEND*)buffer;
	pMsg->TotalPoints = (WORD)data->TotalPoints;
	pMsg->UsedPoints = (WORD)data->UsedPoints;
	pMsg->count = (BYTE)slotCount;

	int offset = sizeof(PMSG_EXT_MASTER_INFO_SEND);

	for (auto it = this->m_BonusInfo.begin(); it != this->m_BonusInfo.end(); it++)
	{
		PMSG_EXT_MASTER_SLOT_INFO info;
		memset(&info, 0, sizeof(info));
		info.Index = (BYTE)it->second.Index;
		info.Group = (BYTE)it->second.Group;
		info.Row = (BYTE)it->second.Row;
		info.Column = (BYTE)it->second.Column;
		info.MinLevel = (BYTE)it->second.MinLevel;
		info.MaxLevel = (BYTE)it->second.MaxLevel;
		info.BonusType = (BYTE)it->second.BonusType;
		info.ValuePerLevel = it->second.ValuePerLevel;
		strncpy_s(info.Name, it->second.Name, sizeof(info.Name));
		strncpy_s(info.Description, it->second.Description, sizeof(info.Description));
		info.RequireSlot1 = (BYTE)((it->second.RequireSkill[0] >= 0) ? it->second.RequireSkill[0] : 0xFF);
		info.RequireSlot2 = (BYTE)((it->second.RequireSkill[1] >= 0) ? it->second.RequireSkill[1] : 0xFF);

		if (it->first >= 0 && it->first < MAX_EXT_SKILL_TREE_SLOTS)
			info.CurrentLevel = (BYTE)data->SkillLevels[it->first];

		memcpy(&buffer[offset], &info, sizeof(info));
		offset += sizeof(PMSG_EXT_MASTER_SLOT_INFO);
	}

	pMsg->header.set(0xF3, 0xB0, (WORD)offset);
	DataSend(aIndex, buffer, offset);
}

void CCustomExtMasterTree::GCExtMasterInvestResult(int aIndex, BYTE result, BYTE slotIndex, BYTE newLevel, WORD remaining)
{
	PMSG_EXT_MASTER_INVEST_RESULT pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xB1, sizeof(pMsg));
	pMsg.Result = result;
	pMsg.SlotIndex = slotIndex;
	pMsg.NewLevel = newLevel;
	pMsg.RemainingPoints = remaining;
	DataSend(aIndex, (BYTE*)&pMsg, sizeof(pMsg));
}

// ====================================================================
// DataServer
// ====================================================================

void CCustomExtMasterTree::GDExtMasterLoad(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	LPOBJ lpObj = &gObj[aIndex];

	SDHP_EXT_MASTER_LOAD_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0x52, 0x01, sizeof(pMsg));
	pMsg.index = (WORD)aIndex;
	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));
	memcpy(pMsg.name, lpObj->Name, sizeof(pMsg.name));
}

void CCustomExtMasterTree::DGExtMasterLoadRecv(LPBYTE lpMsg)
{
	SDHP_EXT_MASTER_LOAD_RECV* pMsg = (SDHP_EXT_MASTER_LOAD_RECV*)lpMsg;

	if (!OBJECT_USER_RANGE(pMsg->index)) return;
	int rIndex = pMsg->index - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];
	data->aIndex = pMsg->index;
	data->TotalPoints = pMsg->TotalPoints;
	data->UsedPoints = pMsg->UsedPoints;
	memcpy(data->SkillLevels, pMsg->SkillLevels, MAX_EXT_SKILL_TREE_SLOTS);
	data->Loaded = true;

	this->GCExtMasterInfoSend(pMsg->index);
}

void CCustomExtMasterTree::GDExtMasterSave(int aIndex)
{
	if (!OBJECT_USER_RANGE(aIndex)) return;
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];
	if (!data->Loaded) return;

	LPOBJ lpObj = &gObj[aIndex];

	SDHP_EXT_MASTER_SAVE_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0x53, sizeof(pMsg));
	pMsg.index = (WORD)aIndex;
	memcpy(pMsg.name, lpObj->Name, sizeof(pMsg.name));
	pMsg.TotalPoints = (WORD)data->TotalPoints;
	pMsg.UsedPoints = (WORD)data->UsedPoints;
	memcpy(pMsg.SkillLevels, data->SkillLevels, MAX_EXT_SKILL_TREE_SLOTS);
}

// ====================================================================
// Utility
// ====================================================================

EXT_MASTER_BONUS_INFO* CCustomExtMasterTree::GetBonusInfo(int index)
{
	auto it = this->m_BonusInfo.find(index);
	if (it != this->m_BonusInfo.end())
		return &it->second;
	return NULL;
}

bool CCustomExtMasterTree::CheckPrerequisites(int aIndex, int slotIndex)
{
	EXT_MASTER_BONUS_INFO* bonus = this->GetBonusInfo(slotIndex);
	if (bonus == NULL) return false;

	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return false;

	EXT_MASTER_USER_DATA* data = &this->m_UserData[rIndex];

	for (int i = 0; i < 2; i++)
	{
		int req = bonus->RequireSkill[i];
		if (req >= 0 && req < MAX_EXT_SKILL_TREE_SLOTS)
		{
			EXT_MASTER_BONUS_INFO* reqBonus = this->GetBonusInfo(req);
			if (reqBonus != NULL && data->SkillLevels[req] < reqBonus->MinLevel)
			{
				return false;
			}
		}
	}

	return true;
}

bool CCustomExtMasterTree::CheckClass(LPOBJ lpObj, int slotIndex)
{
	EXT_MASTER_BONUS_INFO* bonus = this->GetBonusInfo(slotIndex);
	if (bonus == NULL) return false;

	int classIdx = lpObj->Class;
	if (classIdx < 0 || classIdx >= 7) return false;

	return (bonus->RequireClass[classIdx] == 1);
}
