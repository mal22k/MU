// CustomExtMasterTree.cpp: Client-side Extended Master Skill Tree implementation
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomExtMasterTree.h"
#include "Import.h"
#include "Interface.h"
#include "Offset.h"
#include "Util.h"
#include "PacketManager.h"

CCustomExtMasterTreeClient gExtMasterTreeClient;

CCustomExtMasterTreeClient::CCustomExtMasterTreeClient()
{
	this->Init();
}

CCustomExtMasterTreeClient::~CCustomExtMasterTreeClient()
{
}

void CCustomExtMasterTreeClient::Init()
{
	this->m_IsOpen = false;
	this->m_TexturesLoaded = false;
	this->m_ActiveGroup = 0;
	this->m_TotalPoints = 0;
	this->m_UsedPoints = 0;
	this->m_Slots.clear();
	this->m_HoveredSlot = -1;
}

void CCustomExtMasterTreeClient::LoadTextures()
{
	if (this->m_TexturesLoaded) return;
	// Load custom textures (optional)
	this->m_TexturesLoaded = true;
}

// ====================================================================
// Drawing
// ====================================================================

void CCustomExtMasterTreeClient::Draw()
{
	if (!this->m_IsOpen) return;

	float baseX = (MAX_WIN_WIDTH - 500.0f) / 2.0f;
	float baseY = (MAX_WIN_HEIGHT - 500.0f) / 2.0f;

	this->DrawBackground(baseX, baseY);
	this->DrawTabs(baseX, baseY);
	this->DrawSkillSlots(baseX, baseY);
}

void CCustomExtMasterTreeClient::DrawBackground(float baseX, float baseY)
{
	float width = 500.0f;
	float height = 500.0f;

	pSetBlend(1);
	pDrawBarForm(baseX, baseY, width, height, 0.8f, 0);
	pSetBlend(0);

	// Title
	pSetTextColor(pTextThis(), 100, 200, 255, 255);
	pDrawText(pTextThis(), (int)(baseX + 150.0f), (int)(baseY + 10.0f), "EXTENDED SKILL TREE", -1, 0, NULL, 0);

	// Close
	pSetTextColor(pTextThis(), 255, 50, 50, 255);
	pDrawText(pTextThis(), (int)(baseX + width - 30.0f), (int)(baseY + 10.0f), "[X]", -1, 0, NULL, 0);

	// Points info
	char ptsBuf[64];
	pSetTextColor(pTextThis(), 255, 255, 200, 255);
	_snprintf_s(ptsBuf, sizeof(ptsBuf), _TRUNCATE, "Puntos: %d / %d",
		(this->m_TotalPoints - this->m_UsedPoints), this->m_TotalPoints);
	pDrawText(pTextThis(), (int)(baseX + 10.0f), (int)(baseY + 10.0f), ptsBuf, -1, 0, NULL, 0);
}

void CCustomExtMasterTreeClient::DrawTabs(float baseX, float baseY)
{
	float tabY = baseY + 35.0f;
	float tabW = 115.0f;
	float tabH = 22.0f;
	const char* tabNames[4] = { "Ataque", "Defensa", "Soporte", "Especial" };

	for (int t = 0; t < MAX_EXT_TREE_GROUPS; t++)
	{
		float tabX = baseX + 15.0f + (t * (tabW + 5.0f));

		if (t == this->m_ActiveGroup)
		{
			pSetBlend(1);
			pDrawBarForm(tabX, tabY, tabW, tabH, 0.5f, 0);
			pSetBlend(0);
			pSetTextColor(pTextThis(), 255, 215, 0, 255);
		}
		else
		{
			pSetBlend(1);
			pDrawBarForm(tabX, tabY, tabW, tabH, 0.2f, 0);
			pSetBlend(0);
			pSetTextColor(pTextThis(), 180, 180, 180, 255);
		}

		pDrawText(pTextThis(), (int)(tabX + 20.0f), (int)(tabY + 4.0f), tabNames[t], -1, 0, NULL, 0);
	}
}

void CCustomExtMasterTreeClient::DrawSkillSlots(float baseX, float baseY)
{
	float startX = baseX + 20.0f;
	float startY = baseY + 65.0f;
	float slotW = 110.0f;
	float slotH = 45.0f;
	float gapX = 5.0f;
	float gapY = 5.0f;

	this->m_HoveredSlot = -1;

	for (size_t i = 0; i < this->m_Slots.size(); i++)
	{
		EXT_TREE_CLIENT_SLOT* slot = &this->m_Slots[i];
		if (slot->Group != this->m_ActiveGroup) continue;

		float posX = startX + (slot->Column * (slotW + gapX));
		float posY = startY + (slot->Row * (slotH + gapY));

		bool isMaxed = (slot->CurrentLevel >= slot->MaxLevel);
		bool hasPoints = (this->m_TotalPoints - this->m_UsedPoints) > 0;
		bool isActive = (slot->CurrentLevel > 0);

		// Background
		pSetBlend(1);
		if (isMaxed)
			pDrawBarForm(posX, posY, slotW, slotH, 0.4f, 0);
		else if (isActive)
			pDrawBarForm(posX, posY, slotW, slotH, 0.3f, 0);
		else
			pDrawBarForm(posX, posY, slotW, slotH, 0.15f, 0);
		pSetBlend(0);

		// Name
		if (isMaxed)
			pSetTextColor(pTextThis(), 0, 255, 100, 255);
		else if (isActive)
			pSetTextColor(pTextThis(), 255, 215, 0, 255);
		else
			pSetTextColor(pTextThis(), 180, 180, 180, 255);

		pDrawText(pTextThis(), (int)(posX + 3.0f), (int)(posY + 3.0f), slot->Name, -1, 0, NULL, 0);

		// Level indicator
		char lvlBuf[32];
		_snprintf_s(lvlBuf, sizeof(lvlBuf), _TRUNCATE, "%d/%d", slot->CurrentLevel, slot->MaxLevel);
		pSetTextColor(pTextThis(), 255, 255, 255, 255);
		pDrawText(pTextThis(), (int)(posX + 3.0f), (int)(posY + 18.0f), lvlBuf, -1, 0, NULL, 0);

		// Bonus value
		char valBuf[64];
		float totalVal = slot->ValuePerLevel * slot->CurrentLevel;
		_snprintf_s(valBuf, sizeof(valBuf), _TRUNCATE, "+%.1f", totalVal);
		pSetTextColor(pTextThis(), 100, 255, 100, 255);
		pDrawText(pTextThis(), (int)(posX + 50.0f), (int)(posY + 18.0f), valBuf, -1, 0, NULL, 0);

		// "+" button
		if (!isMaxed && hasPoints)
		{
			pSetTextColor(pTextThis(), 255, 215, 0, 255);
			pDrawText(pTextThis(), (int)(posX + slotW - 20.0f), (int)(posY + 15.0f), "[+]", -1, 0, NULL, 0);
		}

		// Check hover for tooltip
		if (CheckMouseIn((int)posX, (int)posY, (int)slotW, (int)slotH))
		{
			this->m_HoveredSlot = slot->Index;
			this->DrawSlotTooltip(slot, posX + slotW + 5.0f, posY);
		}
	}
}

void CCustomExtMasterTreeClient::DrawSlotTooltip(EXT_TREE_CLIENT_SLOT* slot, float posX, float posY)
{
	float tipW = 180.0f;
	float tipH = 80.0f;

	pSetBlend(1);
	pDrawBarForm(posX, posY, tipW, tipH, 0.85f, 0);
	pSetBlend(0);

	// Name
	pSetTextColor(pTextThis(), 255, 215, 0, 255);
	pDrawText(pTextThis(), (int)(posX + 5.0f), (int)(posY + 5.0f), slot->Name, -1, 0, NULL, 0);

	// Description
	pSetTextColor(pTextThis(), 200, 200, 200, 255);
	pDrawText(pTextThis(), (int)(posX + 5.0f), (int)(posY + 22.0f), slot->Description, -1, 0, NULL, 0);

	// Current / Next value
	char valBuf[128];
	float curVal = slot->ValuePerLevel * slot->CurrentLevel;
	float nextVal = slot->ValuePerLevel * (slot->CurrentLevel + 1);
	_snprintf_s(valBuf, sizeof(valBuf), _TRUNCATE, "Actual: +%.1f -> Sig: +%.1f", curVal, nextVal);
	pSetTextColor(pTextThis(), 100, 255, 100, 255);
	pDrawText(pTextThis(), (int)(posX + 5.0f), (int)(posY + 40.0f), valBuf, -1, 0, NULL, 0);

	// Level
	char lvlBuf[32];
	_snprintf_s(lvlBuf, sizeof(lvlBuf), _TRUNCATE, "Nivel: %d / %d", slot->CurrentLevel, slot->MaxLevel);
	pSetTextColor(pTextThis(), 255, 255, 255, 255);
	pDrawText(pTextThis(), (int)(posX + 5.0f), (int)(posY + 57.0f), lvlBuf, -1, 0, NULL, 0);
}

// ====================================================================
// Input
// ====================================================================

bool CCustomExtMasterTreeClient::HandleMouse()
{
	if (!this->m_IsOpen) return false;

	float baseX = (MAX_WIN_WIDTH - 500.0f) / 2.0f;
	float baseY = (MAX_WIN_HEIGHT - 500.0f) / 2.0f;

	// Close button
	if (CheckButtonPressed((int)(baseX + 470.0f), (int)(baseY + 5.0f), 25, 20))
	{
		this->Close();
		return true;
	}

	// Tab buttons
	float tabY = baseY + 35.0f;
	float tabW = 115.0f;
	float tabH = 22.0f;

	for (int t = 0; t < MAX_EXT_TREE_GROUPS; t++)
	{
		float tabX = baseX + 15.0f + (t * (tabW + 5.0f));
		if (CheckButtonPressed((int)tabX, (int)tabY, (int)tabW, (int)tabH))
		{
			this->m_ActiveGroup = t;
			return true;
		}
	}

	// Skill slot "+" buttons
	float startX = baseX + 20.0f;
	float startY = baseY + 65.0f;
	float slotW = 110.0f;
	float slotH = 45.0f;
	float gapX = 5.0f;
	float gapY = 5.0f;

	for (size_t i = 0; i < this->m_Slots.size(); i++)
	{
		EXT_TREE_CLIENT_SLOT* slot = &this->m_Slots[i];
		if (slot->Group != this->m_ActiveGroup) continue;

		float posX = startX + (slot->Column * (slotW + gapX));
		float posY = startY + (slot->Row * (slotH + gapY));

		bool isMaxed = (slot->CurrentLevel >= slot->MaxLevel);
		bool hasPoints = (this->m_TotalPoints - this->m_UsedPoints) > 0;

		if (!isMaxed && hasPoints)
		{
			if (CheckButtonPressed((int)(posX + slotW - 25.0f), (int)(posY + 12.0f), 22, 18))
			{
				this->CGInvestPoint((BYTE)slot->Index);
				return true;
			}
		}
	}

	return false;
}

void CCustomExtMasterTreeClient::OnKeyPress(int key)
{
	// Toggle with 'U' key
	if (key == 0x55) // VK_U
	{
		this->Toggle();
	}
}

// ====================================================================
// Window
// ====================================================================

void CCustomExtMasterTreeClient::Open()
{
	this->m_IsOpen = true;
	this->CGRequestInfo();
}

void CCustomExtMasterTreeClient::Close()
{
	this->m_IsOpen = false;
}

void CCustomExtMasterTreeClient::Toggle()
{
	if (this->m_IsOpen)
		this->Close();
	else
		this->Open();
}

// ====================================================================
// Packet Handlers
// ====================================================================

void CCustomExtMasterTreeClient::GCExtMasterInfoRecv(LPBYTE lpMsg)
{
	PMSG_EXT_MASTER_INFO_RECV* pMsg = (PMSG_EXT_MASTER_INFO_RECV*)lpMsg;

	this->m_TotalPoints = pMsg->TotalPoints;
	this->m_UsedPoints = pMsg->UsedPoints;
	this->m_Slots.clear();

	for (int i = 0; i < pMsg->count; i++)
	{
		PMSG_EXT_MASTER_SLOT_INFO_RECV* info =
			(PMSG_EXT_MASTER_SLOT_INFO_RECV*)(lpMsg + sizeof(PMSG_EXT_MASTER_INFO_RECV) + (sizeof(PMSG_EXT_MASTER_SLOT_INFO_RECV) * i));

		EXT_TREE_CLIENT_SLOT slot;
		memset(&slot, 0, sizeof(slot));
		slot.Index = info->Index;
		slot.Group = info->Group;
		slot.Row = info->Row;
		slot.Column = info->Column;
		slot.MinLevel = info->MinLevel;
		slot.MaxLevel = info->MaxLevel;
		slot.CurrentLevel = info->CurrentLevel;
		slot.BonusType = info->BonusType;
		slot.ValuePerLevel = info->ValuePerLevel;
		strncpy_s(slot.Name, info->Name, sizeof(slot.Name));
		strncpy_s(slot.Description, info->Description, sizeof(slot.Description));
		slot.RequireSlot1 = (info->RequireSlot1 == 0xFF) ? -1 : info->RequireSlot1;
		slot.RequireSlot2 = (info->RequireSlot2 == 0xFF) ? -1 : info->RequireSlot2;

		this->m_Slots.push_back(slot);
	}
}

void CCustomExtMasterTreeClient::GCExtMasterInvestResultRecv(PMSG_EXT_MASTER_INVEST_RESULT_RECV* lpMsg)
{
	if (lpMsg->Result == 1) // success
	{
		for (size_t i = 0; i < this->m_Slots.size(); i++)
		{
			if (this->m_Slots[i].Index == lpMsg->SlotIndex)
			{
				this->m_Slots[i].CurrentLevel = lpMsg->NewLevel;
				break;
			}
		}
		this->m_UsedPoints = this->m_TotalPoints - lpMsg->RemainingPoints;
	}
}

// ====================================================================
// Send
// ====================================================================

void CCustomExtMasterTreeClient::CGRequestInfo()
{
	PMSG_EXT_MASTER_INFO_REQ_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xB0, sizeof(pMsg));
	DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void CCustomExtMasterTreeClient::CGInvestPoint(BYTE slotIndex)
{
	PMSG_EXT_MASTER_INVEST_REQ_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xB1, sizeof(pMsg));
	pMsg.SlotIndex = slotIndex;
	DataSend((BYTE*)&pMsg, pMsg.header.size);
}
