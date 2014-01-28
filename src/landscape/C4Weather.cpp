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

/* Controls temperature, wind, and natural disasters */

#include <C4Include.h>
#include <C4Weather.h>

#include <C4Object.h>
#include <C4Random.h>
#include <C4GraphicsSystem.h>
#include <C4Game.h>
#include <C4SoundSystem.h>

C4Weather::C4Weather()
{
	Default();
}

C4Weather::~C4Weather()
{
	Clear();
}

void C4Weather::Init(bool fScenario)
{
	if (fScenario)
	{
		// Season
		Season=Game.C4S.Weather.StartSeason.Evaluate();
		YearSpeed=Game.C4S.Weather.YearSpeed.Evaluate();
		// Temperature
		Climate=100-Game.C4S.Weather.Climate.Evaluate()-50;
		Temperature=Climate;
		// Wind
		Wind=TargetWind=Game.C4S.Weather.Wind.Evaluate();
		// gamma?
		NoGamma=Game.C4S.Weather.NoGamma;
	}
	// set gamma
	SetSeasonGamma();
}

void C4Weather::Execute()
{
	// Season
	if (!::Game.iTick35)
	{
		SeasonDelay+=YearSpeed;
		if (SeasonDelay>=200)
		{
			SeasonDelay=0;
			Season++;
			if (Season>Game.C4S.Weather.StartSeason.Max)
				Season=Game.C4S.Weather.StartSeason.Min;
			SetSeasonGamma();
		}
	}
	// Temperature
	if (!::Game.iTick35)
	{
		int32_t iTemperature=Climate-(int32_t)(TemperatureRange*cos(6.28*(float)Season/100.0));
		if (Temperature<iTemperature) Temperature++;
		else if (Temperature>iTemperature) Temperature--;
	}
	// Wind
	if (!::Game.iTick1000)
		TargetWind=Game.C4S.Weather.Wind.Evaluate();
	if (!::Game.iTick10)
		Wind=BoundBy<int32_t>(Wind+Sign(TargetWind-Wind),
		                      Game.C4S.Weather.Wind.Min,
		                      Game.C4S.Weather.Wind.Max);
	if (!::Game.iTick10)
		SoundLevel("Wind",NULL,Max(Abs(Wind)-30,0)*2);
}

void C4Weather::Clear()
{

}

int32_t C4Weather::GetWind(int32_t x, int32_t y)
{
	if (GBackIFT(x,y)) return 0;
	return Wind;
}

int32_t C4Weather::GetTemperature()
{
	return Temperature;
}

void C4Weather::Default()
{
	Season=0; YearSpeed=0; SeasonDelay=0;
	Wind=TargetWind=0;
	Temperature=Climate=0;
	TemperatureRange=30;
	NoGamma=true;
}

void C4Weather::SetWind(int32_t iWind)
{
	Wind=BoundBy<int32_t>(iWind,-100,+100);
	TargetWind=BoundBy<int32_t>(iWind,-100,+100);
}

void C4Weather::SetTemperature(int32_t iTemperature)
{
	Temperature = BoundBy<int32_t>(iTemperature,-100,100);
	SetSeasonGamma();
}

void C4Weather::SetSeason(int32_t iSeason)
{
	Season = BoundBy<int32_t>(iSeason,0,100);
	SetSeasonGamma();
}

int32_t C4Weather::GetSeason()
{
	return Season;
}

void C4Weather::SetClimate(int32_t iClimate)
{
	Climate = BoundBy<int32_t>(iClimate,-50,+50);
	SetSeasonGamma();
}

int32_t C4Weather::GetClimate()
{
	return Climate;
}

static DWORD SeasonColors[4][3] =
{
	{ 0x000000, 0x7f7f90, 0xefefff }, // winter: slightly blue; blued out by temperature
	{ 0x070f00, 0x90a07f, 0xffffdf }, // spring: green to yellow
	{ 0x000000, 0x808080, 0xffffff }, // summer: regular ramp
	{ 0x0f0700, 0xa08067, 0xffffdf }  // fall:   dark, brown ramp
};

void C4Weather::SetSeasonGamma()
{
	if (NoGamma) return;
	// get season num and offset
	int32_t iSeason1=(Season/25)%4; int32_t iSeason2=(iSeason1+1)%4;
	int32_t iSeasonOff1=BoundBy(Season%25, 5, 19)-5; int32_t iSeasonOff2=15-iSeasonOff1;
	DWORD dwClr[3]; memset(dwClr, 0, sizeof(DWORD)*3);
	// interpolate between season colors
	for (int32_t i=0; i<3; ++i)
		for (int32_t iChan=0; iChan<24; iChan+=8)
		{
			BYTE byC1=BYTE(SeasonColors[iSeason1][i]>>iChan);
			BYTE byC2=BYTE(SeasonColors[iSeason2][i]>>iChan);
			int32_t iChanVal=(byC1*iSeasonOff2 + byC2*iSeasonOff1) / 15;
			// red+green: reduce in winter
			if (Temperature<0)
			{
				if (iChan)
					iChanVal+=Temperature/2;
				else
					// blue channel: emphasize in winter
					iChanVal-=Temperature/2;
			}
			// set channel
			dwClr[i] |= BoundBy<int32_t>(iChanVal,0,255)<<iChan;
		}
	// apply gamma ramp
	pDraw->SetGamma(dwClr[0], dwClr[1], dwClr[2], C4GRI_SEASON);
}

void C4Weather::CompileFunc(StdCompiler *pComp)
{
	pComp->Value(mkNamingAdapt(Season,           "Season",                0));
	pComp->Value(mkNamingAdapt(YearSpeed,        "YearSpeed",             0));
	pComp->Value(mkNamingAdapt(SeasonDelay,      "SeasonDelay",           0));
	pComp->Value(mkNamingAdapt(Wind,             "Wind",                  0));
	pComp->Value(mkNamingAdapt(TargetWind,       "TargetWind",            0));
	pComp->Value(mkNamingAdapt(Temperature,      "Temperature",           0));
	pComp->Value(mkNamingAdapt(TemperatureRange, "TemperatureRange",      30));
	pComp->Value(mkNamingAdapt(Climate,          "Climate",               0));
	pComp->Value(mkNamingAdapt(NoGamma,          "NoGamma",               0));
	uint32_t dwGammaDefaults[C4MaxGammaRamps*3];
	for (int32_t i=0; i<C4MaxGammaRamps; ++i)
	{
		dwGammaDefaults[i*3+0] = 0x000000;
		dwGammaDefaults[i*3+1] = 0x808080;
		dwGammaDefaults[i*3+2] = 0xffffff;
	}
	pComp->Value(mkNamingAdapt(mkArrayAdaptM(pDraw->dwGamma), "Gamma", dwGammaDefaults));
}

C4Weather Weather;
