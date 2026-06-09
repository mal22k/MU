// CustomBattlePass.cpp: Client-side Battle Pass Interface implementation
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomBattlePass.h"
#include "Import.h"
#include "Interface.h"
#include "Offset.h"
#include "Protocol.h"
#include "Util.h"
#include "PacketManager.h"

CCustomBattlePassClient gBattlePassClient;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomBattlePassClient::CCustomBattlePassClient()
{
	this->Init();
}

CCustomBattlePassClient::~CCustomBattlePassClient()
{
}

void CCustomBattlePassClient::Init()
{
	this->m_IsOpen = false;
	this->m_TexturesLoaded = false;
	this->m_ScrollOffset = 0;
	this->m_MaxVisibleLevels = 10;
	this->m_CurrentLevel = 0;
	this->m_CurrentExp = 0;
	this->m_NextLevelExp = 0;
	this->m_TotalExp = 0;
	this->m_VipLevel = 0;
	this->m_SeasonId = 0;
	this->m_Levels.clear();
	this->m_ShowExpNotify = false;
	this->m_ExpNotifyTime = 0;
	this->m_ExpNotifyAmount = 0;
	this->m_ExpNotifyNewLevel = 0;
}

void CCustomBattlePassClient::LoadTextures()
{
	if (this->m_TexturesLoaded) return;

	// Load custom textures for Battle Pass UI
	// These need .tga files placed in Data/Custom/BattlePass/
	pLoadTexture(BATTLEPASS_TEX_BG,    "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_BAR,   "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_BARFILL,"Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_ICON,  "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_LOCK,  "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_CHECK, "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_STAR,  "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_BTN,   "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);
	pLoadTexture(BATTLEPASS_TEX_CLOSE, "Custom\\BattlePass\\", GL_REPEAT, GL_NEAREST, GL_TRUE);

	this->m_TexturesLoaded = true;
}

// ====================================================================
// Drawing
// ====================================================================

void CCustomBattlePassClient::Draw()
{
	if (!this->m_IsOpen) return;

	float baseX = (MAX_WIN_WIDTH - 600.0f) / 2.0f;
	float baseY = (MAX_WIN_HEIGHT - 450.0f) / 2.0f;

	this->DrawBackground(baseX, baseY);
	this->DrawProgressBar(baseX, baseY);
	this->DrawLevelRewards(baseX, baseY);
	this->DrawExpNotification();
}

void CCustomBattlePassClient::DrawBackground(float baseX, float baseY)
{
	float width = 600.0f;
	float height = 450.0f;

	// Dark background panel
	pSetBlend(1);
	pDrawBarForm(baseX, baseY, width, height, 0.7f, 0);
	pSetBlend(0);

	// Title
	pSetTextColor(pTextThis(), 255, 215, 0, 255); // Gold
	char titleBuf[128];
	_snprintf_s(titleBuf, sizeof(titleBuf), _TRUNCATE, "BATTLE PASS - Season %d", this->m_SeasonId);
	pDrawText(pTextThis(), (int)(baseX + 200.0f), (int)(baseY + 10.0f), titleBuf, -1, 0, NULL, 0);

	// Close button area
	pSetTextColor(pTextThis(), 255, 50, 50, 255);
	pDrawText(pTextThis(), (int)(baseX + width - 30.0f), (int)(baseY + 10.0f), "[X]", -1, 0, NULL, 0);

	// VIP Status
	pSetTextColor(pTextThis(), 200, 200, 200, 255);
	char vipBuf[64];
	if (this->m_VipLevel == 0)
		_snprintf_s(vipBuf, sizeof(vipBuf), _TRUNCATE, "FREE");
	else
		_snprintf_s(vipBuf, sizeof(vipBuf), _TRUNCATE, "VIP %d", this->m_VipLevel);
	pDrawText(pTextThis(), (int)(baseX + 10.0f), (int)(baseY + 10.0f), vipBuf, -1, 0, NULL, 0);
}

void CCustomBattlePassClient::DrawProgressBar(float baseX, float baseY)
{
	float barX = baseX + 20.0f;
	float barY = baseY + 40.0f;
	float barW = 560.0f;
	float barH = 20.0f;

	// Background bar
	pSetBlend(1);
	pDrawBarForm(barX, barY, barW, barH, 0.3f, 0);

	// Fill bar
	float fillPct = 0.0f;
	if (this->m_NextLevelExp > 0)
	{
		fillPct = (float)this->m_CurrentExp / (float)this->m_NextLevelExp;
		if (fillPct > 1.0f) fillPct = 1.0f;
	}
	if (fillPct > 0.0f)
	{
		pDrawBarForm(barX, barY, barW * fillPct, barH, 0.8f, 0);
	}
	pSetBlend(0);

	// Level text
	pSetTextColor(pTextThis(), 255, 255, 255, 255);
	char levelBuf[128];
	_snprintf_s(levelBuf, sizeof(levelBuf), _TRUNCATE, "Nivel %d  |  EXP: %d / %d",
		this->m_CurrentLevel, this->m_CurrentExp, this->m_NextLevelExp);
	pDrawText(pTextThis(), (int)(barX + 5.0f), (int)(barY + 3.0f), levelBuf, -1, 0, NULL, 0);

	// Scroll buttons
	pSetTextColor(pTextThis(), 200, 200, 200, 255);
	pDrawText(pTextThis(), (int)(barX), (int)(barY + 25.0f), "< Prev", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(barX + barW - 50.0f), (int)(barY + 25.0f), "Next >", -1, 0, NULL, 0);
}

void CCustomBattlePassClient::DrawLevelRewards(float baseX, float baseY)
{
	float startY = baseY + 90.0f;
	float rowH = 35.0f;
	int maxVisible = this->m_MaxVisibleLevels;

	// Column headers
	pSetTextColor(pTextThis(), 255, 215, 0, 255);
	pDrawText(pTextThis(), (int)(baseX + 20.0f),  (int)(startY - 20.0f), "Nivel", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(baseX + 80.0f),  (int)(startY - 20.0f), "EXP Req", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(baseX + 170.0f), (int)(startY - 20.0f), "FREE", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(baseX + 280.0f), (int)(startY - 20.0f), "VIP1", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(baseX + 390.0f), (int)(startY - 20.0f), "VIP2", -1, 0, NULL, 0);
	pDrawText(pTextThis(), (int)(baseX + 500.0f), (int)(startY - 20.0f), "VIP3", -1, 0, NULL, 0);

	for (int i = 0; i < maxVisible && (i + this->m_ScrollOffset) < (int)this->m_Levels.size(); i++)
	{
		BATTLEPASS_CLIENT_LEVEL* lvl = &this->m_Levels[i + this->m_ScrollOffset];
		float rowY = startY + (float)(i * rowH);
		bool levelReached = (this->m_CurrentLevel >= lvl->Level);

		// Alternate row background
		if (i % 2 == 0)
		{
			pSetBlend(1);
			pDrawBarForm(baseX + 15.0f, rowY - 2.0f, 570.0f, rowH - 2.0f, 0.15f, 0);
			pSetBlend(0);
		}

		// Highlight current level
		if (lvl->Level == this->m_CurrentLevel)
		{
			pSetBlend(1);
			pDrawBarForm(baseX + 15.0f, rowY - 2.0f, 570.0f, rowH - 2.0f, 0.25f, 0);
			pSetBlend(0);
		}

		// Level number
		char buf[32];
		pSetTextColor(pTextThis(), levelReached ? 255 : 128, levelReached ? 255 : 128, levelReached ? 255 : 128, 255);
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", lvl->Level);
		pDrawText(pTextThis(), (int)(baseX + 30.0f), (int)(rowY + 5.0f), buf, -1, 0, NULL, 0);

		// Required EXP
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%d", lvl->RequiredExp);
		pDrawText(pTextThis(), (int)(baseX + 80.0f), (int)(rowY + 5.0f), buf, -1, 0, NULL, 0);

		// Tier columns (Free, VIP1, VIP2, VIP3)
		float tierX[4] = { baseX + 170.0f, baseX + 280.0f, baseX + 390.0f, baseX + 500.0f };
		BYTE claimed[4] = { lvl->ClaimedFree, lvl->ClaimedVip1, lvl->ClaimedVip2, lvl->ClaimedVip3 };

		for (int t = 0; t < 4; t++)
		{
			if (claimed[t] == 1)
			{
				pSetTextColor(pTextThis(), 0, 255, 0, 255); // Green = claimed
				pDrawText(pTextThis(), (int)(tierX[t]), (int)(rowY + 5.0f), "OK", -1, 0, NULL, 0);
			}
			else if (levelReached && (t == 0 || this->m_VipLevel >= t))
			{
				// Clickable claim button
				pSetTextColor(pTextThis(), 255, 215, 0, 255); // Gold = available
				pDrawText(pTextThis(), (int)(tierX[t]), (int)(rowY + 5.0f), "[Claim]", -1, 0, NULL, 0);
			}
			else if (t > 0 && this->m_VipLevel < t)
			{
				pSetTextColor(pTextThis(), 100, 100, 100, 255); // Gray = VIP required
				pDrawText(pTextThis(), (int)(tierX[t]), (int)(rowY + 5.0f), "VIP", -1, 0, NULL, 0);
			}
			else
			{
				pSetTextColor(pTextThis(), 100, 100, 100, 255); // Gray = locked
				pDrawText(pTextThis(), (int)(tierX[t]), (int)(rowY + 5.0f), "---", -1, 0, NULL, 0);
			}
		}
	}
}

void CCustomBattlePassClient::DrawExpNotification()
{
	if (!this->m_ShowExpNotify) return;

	DWORD now = GetTickCount();
	if ((now - this->m_ExpNotifyTime) > 3000)
	{
		this->m_ShowExpNotify = false;
		return;
	}

	// Fade effect
	float alpha = 1.0f - ((float)(now - this->m_ExpNotifyTime) / 3000.0f);
	float posY = MAX_WIN_HEIGHT * 0.3f - ((float)(now - this->m_ExpNotifyTime) * 0.02f);

	char buf[128];
	_snprintf_s(buf, sizeof(buf), _TRUNCATE, "Battle Pass +%d EXP", this->m_ExpNotifyAmount);

	pSetTextColor(pTextThis(), 255, 215, 0, (BYTE)(255 * alpha));
	pDrawText(pTextThis(), (int)(MAX_WIN_WIDTH / 2.0f - 80.0f), (int)posY, buf, -1, 0, NULL, 0);
}

// ====================================================================
// Input
// ====================================================================

bool CCustomBattlePassClient::HandleMouse()
{
	if (!this->m_IsOpen) return false;

	float baseX = (MAX_WIN_WIDTH - 600.0f) / 2.0f;
	float baseY = (MAX_WIN_HEIGHT - 450.0f) / 2.0f;

	// Close button
	if (CheckButtonPressed((int)(baseX + 570.0f), (int)(baseY + 5.0f), 25, 20))
	{
		this->Close();
		return true;
	}

	// Scroll prev
	if (CheckButtonPressed((int)(baseX + 20.0f), (int)(baseY + 65.0f), 50, 15))
	{
		if (this->m_ScrollOffset > 0) this->m_ScrollOffset--;
		return true;
	}

	// Scroll next
	if (CheckButtonPressed((int)(baseX + 530.0f), (int)(baseY + 65.0f), 50, 15))
	{
		if (this->m_ScrollOffset + this->m_MaxVisibleLevels < (int)this->m_Levels.size())
			this->m_ScrollOffset++;
		return true;
	}

	// Claim buttons
	float startY = baseY + 90.0f;
	float rowH = 35.0f;
	float tierX[4] = { baseX + 170.0f, baseX + 280.0f, baseX + 390.0f, baseX + 500.0f };

	for (int i = 0; i < this->m_MaxVisibleLevels && (i + this->m_ScrollOffset) < (int)this->m_Levels.size(); i++)
	{
		BATTLEPASS_CLIENT_LEVEL* lvl = &this->m_Levels[i + this->m_ScrollOffset];
		float rowY = startY + (float)(i * rowH);
		bool levelReached = (this->m_CurrentLevel >= lvl->Level);

		BYTE claimed[4] = { lvl->ClaimedFree, lvl->ClaimedVip1, lvl->ClaimedVip2, lvl->ClaimedVip3 };

		for (int t = 0; t < 4; t++)
		{
			if (claimed[t] == 0 && levelReached && (t == 0 || this->m_VipLevel >= t))
			{
				if (CheckButtonPressed((int)tierX[t], (int)(rowY + 2.0f), 50, 18))
				{
					this->CGClaimReward((BYTE)lvl->Level, (BYTE)t);
					return true;
				}
			}
		}
	}

	return false;
}

void CCustomBattlePassClient::OnKeyPress(int key)
{
	// Toggle with a hotkey (e.g., 'B' key = 0x42)
	if (key == 0x42) // VK_B
	{
		this->Toggle();
	}
}

// ====================================================================
// Window Management
// ====================================================================

void CCustomBattlePassClient::Open()
{
	this->m_IsOpen = true;
	this->CGRequestInfo();
}

void CCustomBattlePassClient::Close()
{
	this->m_IsOpen = false;
}

void CCustomBattlePassClient::Toggle()
{
	if (this->m_IsOpen)
		this->Close();
	else
		this->Open();
}

// ====================================================================
// Packet Handlers (Server -> Client)
// ====================================================================

void CCustomBattlePassClient::GCBattlePassInfoRecv(LPBYTE lpMsg)
{
	PMSG_BATTLEPASS_INFO_RECV* pMsg = (PMSG_BATTLEPASS_INFO_RECV*)lpMsg;

	this->m_CurrentLevel = pMsg->CurrentLevel;
	this->m_CurrentExp = pMsg->CurrentExp;
	this->m_NextLevelExp = pMsg->NextLevelExp;
	this->m_TotalExp = pMsg->TotalExp;
	this->m_VipLevel = pMsg->VipLevel;
	this->m_SeasonId = pMsg->SeasonId;

	this->m_Levels.clear();

	for (int i = 0; i < pMsg->count; i++)
	{
		PMSG_BATTLEPASS_LEVEL_INFO_RECV* info =
			(PMSG_BATTLEPASS_LEVEL_INFO_RECV*)(lpMsg + sizeof(PMSG_BATTLEPASS_INFO_RECV) + (sizeof(PMSG_BATTLEPASS_LEVEL_INFO_RECV) * i));

		BATTLEPASS_CLIENT_LEVEL level;
		level.Level = info->Level;
		level.RequiredExp = info->RequiredExp;
		level.ClaimedFree = info->ClaimedFree;
		level.ClaimedVip1 = info->ClaimedVip1;
		level.ClaimedVip2 = info->ClaimedVip2;
		level.ClaimedVip3 = info->ClaimedVip3;

		this->m_Levels.push_back(level);
	}

	// Scroll to current level
	if (this->m_CurrentLevel > this->m_MaxVisibleLevels / 2)
	{
		this->m_ScrollOffset = this->m_CurrentLevel - this->m_MaxVisibleLevels / 2;
		if (this->m_ScrollOffset + this->m_MaxVisibleLevels > (int)this->m_Levels.size())
			this->m_ScrollOffset = (int)this->m_Levels.size() - this->m_MaxVisibleLevels;
		if (this->m_ScrollOffset < 0) this->m_ScrollOffset = 0;
	}
}

void CCustomBattlePassClient::GCBattlePassClaimResultRecv(PMSG_BATTLEPASS_CLAIM_RESULT_RECV* lpMsg)
{
	if (lpMsg->Result == 1)
	{
		// Success - update local state
		for (size_t i = 0; i < this->m_Levels.size(); i++)
		{
			if (this->m_Levels[i].Level == lpMsg->Level)
			{
				switch (lpMsg->Tier)
				{
				case 0: this->m_Levels[i].ClaimedFree = 1; break;
				case 1: this->m_Levels[i].ClaimedVip1 = 1; break;
				case 2: this->m_Levels[i].ClaimedVip2 = 1; break;
				case 3: this->m_Levels[i].ClaimedVip3 = 1; break;
				}
				break;
			}
		}
	}
}

void CCustomBattlePassClient::GCBattlePassExpUpdateRecv(PMSG_BATTLEPASS_EXP_UPDATE_RECV* lpMsg)
{
	this->m_CurrentLevel = lpMsg->CurrentLevel;
	this->m_CurrentExp = lpMsg->CurrentExp;
	this->m_NextLevelExp = lpMsg->NextLevelExp;

	// Show EXP notification
	this->m_ShowExpNotify = true;
	this->m_ExpNotifyTime = GetTickCount();
	this->m_ExpNotifyAmount = lpMsg->AddedExp;
	this->m_ExpNotifyNewLevel = lpMsg->CurrentLevel;
}

// ====================================================================
// Send Packets (Client -> Server)
// ====================================================================

void CCustomBattlePassClient::CGRequestInfo()
{
	PMSG_BATTLEPASS_INFO_REQ_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xA0, sizeof(pMsg));

	DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void CCustomBattlePassClient::CGClaimReward(BYTE level, BYTE tier)
{
	PMSG_BATTLEPASS_CLAIM_REQ_SEND pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xA1, sizeof(pMsg));
	pMsg.Level = level;
	pMsg.Tier = tier;

	DataSend((BYTE*)&pMsg, pMsg.header.size);
}
