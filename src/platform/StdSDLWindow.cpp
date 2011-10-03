/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2010  Peter Wortmann
 * Copyright (c) 2010  Benjamin Herr
 * Copyright (c) 2010  Armin Burgmeier
 * Copyright (c) 2011  Günther Brammer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
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

/* A wrapper class to OS dependent event and window interfaces, SDL version */

#include <C4Include.h>
#include <StdWindow.h>
#include <StdGL.h>
#include <StdDDraw2.h>
#include <StdFile.h>
#include <StdBuf.h>

#include "C4Version.h"
#include <C4Rect.h>
#include <C4Config.h>

/* CStdWindow */

CStdWindow::CStdWindow ():
		Active(false), pSurface(0)
{
}

CStdWindow::~CStdWindow ()
{
	Clear();
}

CStdWindow * CStdWindow::Init(WindowKind windowKind, CStdApp * pApp, const char * Title, CStdWindow * pParent, bool HideCursor)
{
	Active = true;
	// SDL doesn't support multiple monitors.
	if (!SDL_SetVideoMode(Config.Graphics.ResX, Config.Graphics.ResY, Config.Graphics.BitDepth, SDL_OPENGL | (Config.Graphics.Windowed ? 0 : SDL_FULLSCREEN)))
	{
		Log(SDL_GetError());
		return 0;
	}
	SDL_ShowCursor(HideCursor ? SDL_DISABLE : SDL_ENABLE);
	SetSize(Config.Graphics.ResX, Config.Graphics.ResY);
	SetTitle(Title);
	return this;
}

bool CStdWindow::ReInit(CStdApp* pApp)
{
	// TODO: How do we enable multisampling with SDL?
	// Maybe re-call SDL_SetVideoMode?
	return false;
}

void CStdWindow::Clear() {}

void CStdWindow::EnumerateMultiSamples(std::vector<int>& samples) const
{
	// TODO: Enumerate multi samples
}

bool CStdWindow::StorePosition(const char *, const char *, bool) { return true; }

bool CStdWindow::RestorePosition(const char *, const char *, bool) { return true; }

// Window size is automatically managed by CStdApp's display mode management.
// Just remember the size for others to query.

bool CStdWindow::GetSize(C4Rect * pRect)
{
	pRect->x = pRect->y = 0;
	pRect->Wdt = width, pRect->Hgt = height;
	return true;
}

void CStdWindow::SetSize(unsigned int X, unsigned int Y)
{
	width = X, height = Y;
}

void CStdWindow::SetTitle(const char * Title)
{
	SDL_WM_SetCaption(Title, 0);
}

// For Max OS X, the implementation resides in StdMacApp.mm
#ifndef __APPLE__

void CStdWindow::FlashWindow()
{
}

#endif
