// CustomKillStreak.cpp: implementation of the CCustomKillStreak class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomKillStreak.h"
#include "Util.h"
#include "Notice.h"
#include "Message.h"
#include "pugiconfig.hpp"
#include "pugixml.hpp"

CCustomKillStreak gCustomKillStreak;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomKillStreak::CCustomKillStreak()
{
	this->Init();
}

CCustomKillStreak::~CCustomKillStreak()
{
}

void CCustomKillStreak::Init()
{
	this->Enable = 0;
	this->StreakTimeout = KILLSTREAK_TIMEOUT;
	this->m_StreakTiers.clear();

	for (int i = 0; i < MAX_OBJECT_USER; i++)
	{
		memset(&this->m_UserData[i], 0, sizeof(KILLSTREAK_USER_DATA));
	}
}

// ====================================================================
// XML Configuration Loader
// ====================================================================
// Example XML: Data/Custom/CustomKillStreak.xml
//
// <KillStreak Enable="1" Timeout="10000">
//   <Streak Index="1" Kills="2" Name="Double Kill" Sound="Custom\\Sounds\\doublekill.wav" Image="Custom\\Images\\doublekill.tga" Shake="1" ShakeIntensity="3" ShakeDuration="300" Notice="2" NoticeMsg="%s consiguio Double Kill!"/>
//   <Streak Index="2" Kills="3" Name="Triple Kill" Sound="Custom\\Sounds\\triplekill.wav" Image="Custom\\Images\\triplekill.tga" Shake="1" ShakeIntensity="5" ShakeDuration="500" Notice="2" NoticeMsg="%s consiguio Triple Kill!"/>
//   <Streak Index="3" Kills="4" Name="Multi Kill" Sound="Custom\\Sounds\\multikill.wav" Image="Custom\\Images\\multikill.tga" Shake="1" ShakeIntensity="6" ShakeDuration="600" Notice="2" NoticeMsg="%s consiguio Multi Kill!"/>
//   <Streak Index="4" Kills="5" Name="Insano" Sound="Custom\\Sounds\\insano.wav" Image="Custom\\Images\\insano.tga" Shake="1" ShakeIntensity="8" ShakeDuration="800" Notice="2" NoticeMsg="%s consiguio Insano!"/>
//   <Streak Index="5" Kills="7" Name="Puto Amo" Sound="Custom\\Sounds\\putoamo.wav" Image="Custom\\Images\\putoamo.tga" Shake="1" ShakeIntensity="10" ShakeDuration="1000" Notice="2" NoticeMsg="%s es el Puto Amo!"/>
// </KillStreak>

void CCustomKillStreak::LoadXML(char* FilePath)
{
	pugi::xml_document file;
	pugi::xml_parse_result res = file.load_file(FilePath);

	if (res.status != pugi::status_ok)
	{
		ErrorMessageBox("[KillStreak] File %s load fail. Error: %s", FilePath, res.description());
		return;
	}

	pugi::xml_node root = file.child("KillStreak");
	this->Enable = root.attribute("Enable").as_int();
	this->StreakTimeout = root.attribute("Timeout").as_uint(KILLSTREAK_TIMEOUT);

	if (this->Enable == 0)
	{
		LogAdd(LOG_BLACK, "[KillStreak] System disabled");
		return;
	}

	this->m_StreakTiers.clear();

	for (pugi::xml_node streak = root.child("Streak"); streak; streak = streak.next_sibling("Streak"))
	{
		KILLSTREAK_TIER_INFO info;
		memset(&info, 0, sizeof(info));

		info.Index = streak.attribute("Index").as_int();
		info.RequiredKills = streak.attribute("Kills").as_int();
		strncpy_s(info.Name, streak.attribute("Name").as_string(), sizeof(info.Name));
		strncpy_s(info.SoundFile, streak.attribute("Sound").as_string(), sizeof(info.SoundFile));
		strncpy_s(info.ImageFile, streak.attribute("Image").as_string(), sizeof(info.ImageFile));
		info.ScreenShake = streak.attribute("Shake").as_int(0);
		info.ShakeIntensity = streak.attribute("ShakeIntensity").as_int(5);
		info.ShakeDuration = streak.attribute("ShakeDuration").as_int(500);
		info.NoticeType = streak.attribute("Notice").as_int(0);
		strncpy_s(info.NoticeMsg, streak.attribute("NoticeMsg").as_string(), sizeof(info.NoticeMsg));

		this->m_StreakTiers.push_back(info);
	}

	// Sort by kills ascending
	std::sort(this->m_StreakTiers.begin(), this->m_StreakTiers.end(),
		[](const KILLSTREAK_TIER_INFO& a, const KILLSTREAK_TIER_INFO& b) {
			return a.RequiredKills < b.RequiredKills;
		});

	LogAdd(LOG_BLACK, "[KillStreak] Loaded %d streak tiers, Timeout: %dms",
		(int)this->m_StreakTiers.size(), this->StreakTimeout);
}

// ====================================================================
// Kill Tracking
// ====================================================================

void CCustomKillStreak::OnPlayerKill(LPOBJ lpKiller, LPOBJ lpVictim)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpKiller->Index)) return;
	if (!OBJECT_USER_RANGE(lpVictim->Index)) return;

	int rIndex = lpKiller->Index - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	KILLSTREAK_USER_DATA* data = &this->m_UserData[rIndex];
	DWORD now = GetTickCount();

	// Check timeout
	if (data->LastKillTime > 0 && (now - data->LastKillTime) > this->StreakTimeout)
	{
		this->ResetStreak(lpKiller->Index);
	}

	data->CurrentKills++;
	data->LastKillTime = now;

	if (data->CurrentKills > data->MaxStreakThisSession)
	{
		data->MaxStreakThisSession = data->CurrentKills;
	}

	// Check if this kill count matches a streak tier
	KILLSTREAK_TIER_INFO* tier = this->GetStreakByKills(data->CurrentKills);
	if (tier != NULL)
	{
		this->NotifyStreak(lpKiller->Index, tier->Index, lpVictim);
		data->LastStreakNotified = tier->Index;
	}
}

void CCustomKillStreak::OnPlayerDeath(LPOBJ lpObj)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(lpObj->Index)) return;

	this->ResetStreak(lpObj->Index);
}

void CCustomKillStreak::OnSecondProc(int aIndex)
{
	if (this->Enable == 0) return;
	if (!OBJECT_USER_RANGE(aIndex)) return;

	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	KILLSTREAK_USER_DATA* data = &this->m_UserData[rIndex];

	if (data->CurrentKills > 0 && data->LastKillTime > 0)
	{
		DWORD now = GetTickCount();
		if ((now - data->LastKillTime) > this->StreakTimeout)
		{
			this->ResetStreak(aIndex);
		}
	}
}

// ====================================================================
// Internal Functions
// ====================================================================

void CCustomKillStreak::ResetStreak(int aIndex)
{
	int rIndex = aIndex - OBJECT_START_USER;
	if (rIndex < 0 || rIndex >= MAX_OBJECT_USER) return;

	KILLSTREAK_USER_DATA* data = &this->m_UserData[rIndex];

	if (data->CurrentKills > 1)
	{
		PMSG_KILLSTREAK_RESET pMsg;
		memset(&pMsg, 0, sizeof(pMsg));
		pMsg.header.set(0xF3, 0xA6, sizeof(pMsg));
		pMsg.TotalKills = (BYTE)data->CurrentKills;
		pMsg.MaxStreakIndex = (BYTE)data->LastStreakNotified;
		DataSend(aIndex, (BYTE*)&pMsg, sizeof(pMsg));
	}

	data->CurrentKills = 0;
	data->LastKillTime = 0;
	data->LastStreakNotified = 0;
}

void CCustomKillStreak::NotifyStreak(int aIndex, int streakIndex, LPOBJ lpVictim)
{
	KILLSTREAK_TIER_INFO* tier = this->GetStreakByIndex(streakIndex);
	if (tier == NULL) return;

	LPOBJ lpKiller = &gObj[aIndex];

	// Send notification packet to the killer
	PMSG_KILLSTREAK_NOTIFY pMsg;
	memset(&pMsg, 0, sizeof(pMsg));
	pMsg.header.set(0xF3, 0xA5, sizeof(pMsg));
	pMsg.StreakIndex = (BYTE)tier->Index;
	pMsg.RequiredKills = (BYTE)tier->RequiredKills;
	pMsg.ScreenShake = (BYTE)tier->ScreenShake;
	pMsg.ShakeIntensity = (BYTE)tier->ShakeIntensity;
	pMsg.ShakeDuration = (WORD)tier->ShakeDuration;
	memcpy(pMsg.KillerName, lpKiller->Name, sizeof(pMsg.KillerName));
	memcpy(pMsg.VictimName, lpVictim->Name, sizeof(pMsg.VictimName));
	strncpy_s(pMsg.StreakName, tier->Name, sizeof(pMsg.StreakName));
	strncpy_s(pMsg.SoundFile, tier->SoundFile, sizeof(pMsg.SoundFile));
	strncpy_s(pMsg.ImageFile, tier->ImageFile, sizeof(pMsg.ImageFile));

	DataSend(aIndex, (BYTE*)&pMsg, sizeof(pMsg));

	// Server-wide notice
	if (tier->NoticeType >= 1)
	{
		char noticeBuf[256];
		memset(noticeBuf, 0, sizeof(noticeBuf));
		_snprintf_s(noticeBuf, sizeof(noticeBuf), _TRUNCATE, tier->NoticeMsg, lpKiller->Name);

		if (tier->NoticeType == 1)
		{
			// Map notice - send to all users on same map
			for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
			{
				if (gObj[n].Connected == OBJECT_ONLINE && gObj[n].Map == lpKiller->Map)
				{
					// Also send the streak notify to nearby players so they see the image/sound
					DataSend(n, (BYTE*)&pMsg, sizeof(pMsg));
				}
			}
		}
		else if (tier->NoticeType == 2)
		{
			// Server-wide notice
			for (int n = OBJECT_START_USER; n < MAX_OBJECT; n++)
			{
				if (gObj[n].Connected == OBJECT_ONLINE)
				{
					DataSend(n, (BYTE*)&pMsg, sizeof(pMsg));
				}
			}
		}
	}

	LogAdd(LOG_BLACK, "[KillStreak] [%s] %s (%d kills)",
		lpKiller->Name, tier->Name, tier->RequiredKills);
}

KILLSTREAK_TIER_INFO* CCustomKillStreak::GetStreakByKills(int kills)
{
	KILLSTREAK_TIER_INFO* result = NULL;

	for (size_t i = 0; i < this->m_StreakTiers.size(); i++)
	{
		if (this->m_StreakTiers[i].RequiredKills == kills)
		{
			return &this->m_StreakTiers[i];
		}
	}

	return result;
}

KILLSTREAK_TIER_INFO* CCustomKillStreak::GetStreakByIndex(int index)
{
	for (size_t i = 0; i < this->m_StreakTiers.size(); i++)
	{
		if (this->m_StreakTiers[i].Index == index)
		{
			return &this->m_StreakTiers[i];
		}
	}

	return NULL;
}
