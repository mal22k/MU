// CustomKillStreak.cpp: Client-side Kill Streak effects implementation
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CustomKillStreak.h"
#include "Import.h"
#include "Interface.h"
#include "Offset.h"
#include "Util.h"

CCustomKillStreakClient gKillStreakClient;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCustomKillStreakClient::CCustomKillStreakClient()
{
	this->Init();
}

CCustomKillStreakClient::~CCustomKillStreakClient()
{
}

void CCustomKillStreakClient::Init()
{
	for (int i = 0; i < MAX_KILLSTREAK_DISPLAY; i++)
	{
		memset(&this->m_Displays[i], 0, sizeof(KILLSTREAK_DISPLAY_INFO));
	}

	memset(&this->m_Shake, 0, sizeof(KILLSTREAK_SHAKE_INFO));
	this->m_NextDisplaySlot = 0;
	this->m_NextTexID = KILLSTREAK_TEX_BASE;
}

// ====================================================================
// Drawing
// ====================================================================

void CCustomKillStreakClient::Draw()
{
	DWORD now = GetTickCount();

	// Draw active streak notifications
	float posY = MAX_WIN_HEIGHT * 0.25f;
	for (int i = 0; i < MAX_KILLSTREAK_DISPLAY; i++)
	{
		if (this->m_Displays[i].Active)
		{
			if ((now - this->m_Displays[i].StartTime) > KILLSTREAK_DISPLAY_TIME)
			{
				this->m_Displays[i].Active = false;
				continue;
			}

			this->DrawStreakNotification(&this->m_Displays[i], posY);
			posY += 50.0f;
		}
	}

	// Apply screen shake
	this->ApplyScreenShake();
}

void CCustomKillStreakClient::DrawStreakNotification(KILLSTREAK_DISPLAY_INFO* info, float posY)
{
	DWORD now = GetTickCount();
	DWORD elapsed = now - info->StartTime;

	// Calculate animation
	float alpha = 1.0f;
	float scale = 1.0f;
	float offsetX = 0.0f;

	// Entry animation (first 300ms) - slide in from right with scale up
	if (elapsed < 300)
	{
		float t = (float)elapsed / 300.0f;
		scale = 0.5f + (t * 0.5f);
		offsetX = (1.0f - t) * 200.0f;
	}
	// Stay (300ms - 2500ms)
	else if (elapsed < 2500)
	{
		scale = 1.0f;
	}
	// Fade out (2500ms - 3000ms)
	else
	{
		float t = (float)(elapsed - 2500) / 500.0f;
		alpha = 1.0f - t;
	}

	float centerX = MAX_WIN_WIDTH / 2.0f + offsetX;

	// Draw streak image if loaded
	if (info->HasCustomImage && info->TexID > 0)
	{
		float imgW = 256.0f * scale;
		float imgH = 64.0f * scale;
		pSetBlend(1);
		pDrawImage(info->TexID, centerX - imgW / 2.0f, posY - imgH / 2.0f,
			imgW, imgH, 0.0f, 0.0f, 1.0f, 1.0f, true, true, true);
		pSetBlend(0);
	}

	// Draw streak text on top of image
	BYTE textAlpha = (BYTE)(255.0f * alpha);

	// Streak name - large, gold
	pSetTextColor(pTextThis(), 255, 215, 0, textAlpha);

	char nameBuf[128];
	_snprintf_s(nameBuf, sizeof(nameBuf), _TRUNCATE, "%s", info->StreakName);

	// Center the text
	int textWidth = (int)(strlen(nameBuf) * 7.0f * scale);
	pDrawText(pTextThis(), (int)(centerX - textWidth / 2.0f), (int)(posY), nameBuf, -1, 0, NULL, 0);

	// Killer name below
	pSetTextColor(pTextThis(), 255, 255, 255, textAlpha);
	char killerBuf[64];
	_snprintf_s(killerBuf, sizeof(killerBuf), _TRUNCATE, "%s (%d kills)", info->KillerName, info->RequiredKills);
	int killerWidth = (int)(strlen(killerBuf) * 6.0f);
	pDrawText(pTextThis(), (int)(centerX - killerWidth / 2.0f), (int)(posY + 18.0f), killerBuf, -1, 0, NULL, 0);
}

void CCustomKillStreakClient::ApplyScreenShake()
{
	if (!this->m_Shake.Active) return;

	DWORD now = GetTickCount();
	DWORD elapsed = now - this->m_Shake.StartTime;

	if (elapsed > this->m_Shake.Duration)
	{
		this->m_Shake.Active = false;
		return;
	}

	// Decay over time
	float decay = 1.0f - ((float)elapsed / (float)this->m_Shake.Duration);
	float intensity = (float)this->m_Shake.Intensity * decay;

	// Generate pseudo-random shake offset
	float shakeX = ((float)(rand() % 200 - 100) / 100.0f) * intensity;
	float shakeY = ((float)(rand() % 200 - 100) / 100.0f) * intensity;

	// Apply to camera/viewport via OpenGL translate
	// This modifies the current modelview matrix temporarily
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(shakeX, shakeY, 0.0f);

	// The matrix will be popped at end of frame by the main render loop
	// If that doesn't work, hook into the render pipeline to pop it
	glPopMatrix();

	// Alternative: Directly modify the camera position offsets
	// *(float*)(oCam_Zoom + 0x10) += shakeX;  // If camera struct allows
	// *(float*)(oCam_Zoom + 0x14) += shakeY;
}

// ====================================================================
// Packet Handlers
// ====================================================================

void CCustomKillStreakClient::GCKillStreakNotifyRecv(PMSG_KILLSTREAK_NOTIFY_RECV* lpMsg)
{
	// Find a free display slot
	int slot = -1;
	for (int i = 0; i < MAX_KILLSTREAK_DISPLAY; i++)
	{
		if (!this->m_Displays[i].Active)
		{
			slot = i;
			break;
		}
	}

	if (slot == -1)
	{
		// Overwrite oldest
		slot = this->m_NextDisplaySlot;
		this->m_NextDisplaySlot = (this->m_NextDisplaySlot + 1) % MAX_KILLSTREAK_DISPLAY;
	}

	KILLSTREAK_DISPLAY_INFO* info = &this->m_Displays[slot];
	memset(info, 0, sizeof(KILLSTREAK_DISPLAY_INFO));
	info->Active = true;
	info->StartTime = GetTickCount();
	info->StreakIndex = lpMsg->StreakIndex;
	info->RequiredKills = lpMsg->RequiredKills;
	memcpy(info->KillerName, lpMsg->KillerName, sizeof(info->KillerName));
	memcpy(info->VictimName, lpMsg->VictimName, sizeof(info->VictimName));
	strncpy_s(info->StreakName, lpMsg->StreakName, sizeof(info->StreakName));

	// Load custom image if specified
	if (strlen(lpMsg->ImageFile) > 0)
	{
		std::string key(lpMsg->ImageFile);
		auto it = this->m_LoadedTextures.find(key);
		if (it != this->m_LoadedTextures.end())
		{
			info->TexID = it->second;
		}
		else
		{
			info->TexID = this->m_NextTexID++;
			this->LoadStreakImage(info->TexID, lpMsg->ImageFile);
			this->m_LoadedTextures[key] = info->TexID;
		}
		info->HasCustomImage = true;
	}

	// Play sound
	if (strlen(lpMsg->SoundFile) > 0)
	{
		this->PlayStreakSound(lpMsg->SoundFile);
	}

	// Screen shake
	if (lpMsg->ScreenShake)
	{
		this->StartScreenShake(lpMsg->ShakeIntensity, lpMsg->ShakeDuration);
	}
}

void CCustomKillStreakClient::GCKillStreakResetRecv(PMSG_KILLSTREAK_RESET_RECV* lpMsg)
{
	// Streak ended - can show "Streak Over" message if desired
}

// ====================================================================
// Sound & Image helpers
// ====================================================================

void CCustomKillStreakClient::PlayStreakSound(const char* soundFile)
{
	// Use Windows PlaySound API (included via Mmsystem.h / Winmm.lib)
	PlaySoundA(soundFile, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void CCustomKillStreakClient::LoadStreakImage(int texID, const char* imageFile)
{
	// Extract folder and filename from path
	// e.g. "Custom\\Sounds\\doublekill.tga" -> folder="Custom\\Sounds\\", file is loaded by pLoadTexture
	char folder[256];
	memset(folder, 0, sizeof(folder));
	strncpy_s(folder, imageFile, sizeof(folder));

	// pLoadTexture expects a folder path; the actual .tga filename is part of the texture system
	// For simplicity, load directly:
	pLoadTexture(texID, folder, GL_REPEAT, GL_NEAREST, GL_TRUE);
}

void CCustomKillStreakClient::StartScreenShake(int intensity, DWORD duration)
{
	this->m_Shake.Active = true;
	this->m_Shake.StartTime = GetTickCount();
	this->m_Shake.Duration = duration;
	this->m_Shake.Intensity = intensity;
}
