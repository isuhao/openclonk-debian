/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2002, 2005  Peter Wortmann
 * Copyright (c) 2006, 2009  Günther Brammer
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */
/* string table: holds all strings used by script engine */

#include <C4Include.h>
#include <C4StringTable.h>

#include <C4Group.h>
#include <C4Components.h>
//#include <C4Aul.h>

// *** C4Set
template<> template<>
unsigned int C4Set<C4String *>::Hash<const char *>(const char * s)
{
	// Fowler/Noll/Vo hash
	unsigned int h = 2166136261u;
	while (*s)
		h = (h ^ *(s++)) * 16777619;
	return h;
}

template<> template<>
bool C4Set<C4String *>::Equals<const char *>(C4String * a, const char * b)
{
	return a->GetData() == b;
}

// *** C4String

C4String::C4String(StdStrBuf strString)
		: iRefCnt(0)
{
	// take string
	Data.Take(std::move(strString));
	Hash = C4Set<C4String*>::Hash(Data.getData());
	// reg
	Strings.Set.Add(this);
}

C4String::~C4String()
{
	// unreg
	iRefCnt = 1;
	Strings.Set.Remove(this);
}

void C4String::IncRef()
{
	++iRefCnt;
}

void C4String::DecRef()
{
	--iRefCnt;
	if (iRefCnt <= 0)
		delete this;
}

// *** C4StringTable

C4StringTable::C4StringTable()
{
	P[P_Prototype] = RegString("Prototype");
	P[P_Name] = RegString("Name");
	P[P_Collectible] = RegString("Collectible");
	P[P_ActMap] = RegString("ActMap");
	P[P_Procedure] = RegString("Procedure");
	P[P_Attach] = RegString("Attach");
	P[P_Directions] = RegString("Directions");
	P[P_FlipDir] = RegString("FlipDir");
	P[P_Length] = RegString("Length");
	P[P_Delay] = RegString("Delay");
	P[P_X] = RegString("X");
	P[P_Y] = RegString("Y");
	P[P_Wdt] = RegString("Wdt");
	P[P_Hgt] = RegString("Hgt");
	P[P_OffX] = RegString("OffX");
	P[P_OffY] = RegString("OffY");
	P[P_FacetBase] = RegString("FacetBase");
	P[P_FacetTopFace] = RegString("FacetTopFace");
	P[P_FacetTargetStretch] = RegString("FacetTargetStretch");
	P[P_NextAction] = RegString("NextAction");
	P[P_Hold] = RegString("Hold");
	P[P_Idle] = RegString("Idle");
	P[P_NoOtherAction] = RegString("NoOtherAction");
	P[P_StartCall] = RegString("StartCall");
	P[P_EndCall] = RegString("EndCall");
	P[P_AbortCall] = RegString("AbortCall");
	P[P_PhaseCall] = RegString("PhaseCall");
	P[P_Sound] = RegString("Sound");
	P[P_ObjectDisabled] = RegString("ObjectDisabled");
	P[P_DigFree] = RegString("DigFree");
	P[P_EnergyUsage] = RegString("EnergyUsage");
	P[P_InLiquidAction] = RegString("InLiquidAction");
	P[P_TurnAction] = RegString("TurnAction");
	P[P_Reverse] = RegString("Reverse");
	P[P_Step] = RegString("Step");
	P[P_Animation] = RegString("Animation");
	P[P_Action] = RegString("Action");
	P[P_Visibility] = RegString("Visibility");
	P[P_Parallaxity] = RegString("Parallaxity");
	P[P_LineColors] = RegString("LineColors");
	P[P_LineAttach] = RegString("LineAttach");
	P[P_MouseDragImage] = RegString("MouseDragImage");
	P[P_PictureTransformation] = RegString("PictureTransformation");
	P[P_MeshTransformation] = RegString("MeshTransformation");
	for (unsigned int i = 0; i < P_LAST; ++i) P[i]->IncRef();
}

C4StringTable::~C4StringTable()
{
	for (unsigned int i = 0; i < P_LAST; ++i) P[i]->DecRef();
	assert(!Set.GetSize());
}

C4String *C4StringTable::RegString(StdStrBuf String)
{
	C4String * s = FindString(String.getData());
	if (s)
		return s;
	else
		return new C4String(String);
}

C4String *C4StringTable::FindString(const char *strString)
{
	return Set.Get(strString);
}

C4String *C4StringTable::FindString(C4String *pString)
{
	for (C4String * const * i = Set.First(); i; i = Set.Next(i))
		if (*i == pString)
			return pString;
	return NULL;
}

C4StringTable Strings;
