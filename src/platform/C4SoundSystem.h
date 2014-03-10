/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, Matthes Bender
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de/
 * Copyright (c) 2009-2013, The OpenClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

/* Handles the sound bank and plays effects using FMOD */

#ifndef INC_C4SoundSystem
#define INC_C4SoundSystem

#include <C4Group.h>

const int32_t
	C4MaxSoundName=100,
	C4MaxSoundInstances=20,
	C4NearSoundRadius=50,
	C4AudibilityRadius=700;

class C4Object;
class C4SoundInstance;

#if defined(HAVE_FMOD)
#include <fmod.h>
typedef FSOUND_SAMPLE* C4SoundHandle;
#elif defined(HAVE_LIBSDL_MIXER)
typedef struct Mix_Chunk* C4SoundHandle;
#elif defined(USE_OPEN_AL)
#ifdef __APPLE__
#import <OpenAL/al.h>
#else
#include <AL/al.h>
#endif
typedef ALuint C4SoundHandle;
#else
typedef void* C4SoundHandle;
#endif

class C4SoundEffect
{
	friend class C4SoundInstance;
public:
	C4SoundEffect();
	~C4SoundEffect();
public:
	char Name[C4MaxSoundName+1];
	int32_t Instances;
	int32_t SampleRate, Length;
	C4SoundHandle pSample;
	C4SoundInstance *FirstInst;
	C4SoundEffect *Next;
public:
	void Clear();
	bool Load(const char *szFileName, C4Group &hGroup);
	bool Load(BYTE *pData, size_t iDataLen, bool fRaw=false); // load directly from memory
	void Execute();
	C4SoundInstance *New(bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = NULL, int32_t iCustomFalloffDistance = 0);
	C4SoundInstance *GetInstance(C4Object *pObj);
	void ClearPointers(C4Object *pObj);
	int32_t GetStartedInstanceCount(int32_t iX, int32_t iY, int32_t iRad); // local
	int32_t GetStartedInstanceCount(); // global
protected:
	void AddInst(C4SoundInstance *pInst);
	void RemoveInst(C4SoundInstance *pInst);
};

class C4SoundInstance
{
	friend class C4SoundEffect;
protected:
	C4SoundInstance();
public:
	~C4SoundInstance();
protected:
	C4SoundEffect *pEffect;
	int32_t iVolume, iPan, iChannel;
	C4TimeMilliseconds tStarted;
	int32_t iNearInstanceMax;
	bool fLooping;
	C4Object *pObj;
	int32_t iFalloffDistance;
	C4SoundInstance *pNext;
public:
	C4Object *getObj() const { return pObj; }
	bool isStarted() const { return iChannel != -1; }
	void Clear();
	bool Create(C4SoundEffect *pEffect, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = NULL, int32_t iNearInstanceMax = 0, int32_t iFalloffDistance = 0);
	bool CheckStart();
	bool Start();
	bool Stop();
	bool Playing();
	void Execute();
	void SetVolume(int32_t inVolume) { iVolume = inVolume; }
	void SetPan(int32_t inPan) { iPan = inPan; }
	void SetVolumeByPos(int32_t x, int32_t y);
	void SetObj(C4Object *pnObj) { pObj = pnObj; }
	void ClearPointers(C4Object *pObj);
	bool Inside(int32_t iX, int32_t iY, int32_t iRad);
};

const int32_t SoundUnloadTime=60, SoundMaxUnloadSize=100000;

class C4SoundSystem
{
public:
	C4SoundSystem();
	~C4SoundSystem();
	void Clear();
	void Execute();
	int32_t LoadEffects(C4Group &hGroup);
	C4SoundInstance *NewEffect(const char *szSound, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj = NULL, int32_t iCustomFalloffDistance = 0);
	C4SoundInstance *FindInstance(const char *szSound, C4Object *pObj);
	bool Init();
	void ClearPointers(C4Object *pObj);
protected:
	C4Group SoundFile;
	C4SoundEffect *FirstSound;
	void ClearEffects();
	C4SoundEffect* GetEffect(const char *szSound);
	int32_t RemoveEffect(const char *szFilename);
};

class C4SoundInstance *StartSoundEffect(const char *szSndName, bool fLoop = false, int32_t iVolume = 100, C4Object *pObj=NULL, int32_t iCustomFalloffDistance=0);
class C4SoundInstance *StartSoundEffectAt(const char *szSndName, int32_t iX, int32_t iY, int32_t iVolume = 100, int32_t iCustomFallofDistance=0);
class C4SoundInstance *GetSoundInstance(const char *szSndName, C4Object *pObj);
void StopSoundEffect(const char *szSndName, C4Object *pObj);
void SoundLevel(const char *szSndName, C4Object *pObj, int32_t iLevel);
void SoundPan(const char *szSndName, C4Object *pObj, int32_t iPan);


#endif
