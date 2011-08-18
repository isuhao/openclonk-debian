/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000  Matthes Bender
 * Copyright (c) 2001, 2005  Sven Eberhardt
 * Copyright (c) 2002  Peter Wortmann
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

/* Fullscreen startup log and chat type-in */

#ifndef INC_C4MessageBoard
#define INC_C4MessageBoard

const int C4MSGB_MaxMsgFading   = 6;

#include <C4Facet.h>
#include <C4LogBuf.h>

class C4Player;

class C4MessageBoard
{
public:
	C4MessageBoard();
	~C4MessageBoard();

	C4Facet Output;
	
	bool Active;
protected:
	float ScreenFader;
	bool Startup;
	// mode 0:
	int Delay;  // how long the curr msg will stay
	int Fader;  // =0: hold curr msg until Delay == 0
	// >0: fade curr msg in
	int Speed;  // fade/delay speed
	int iBackScroll; // how many lines scrolled back?
	int iLineHgt; // line height

	C4LogBuffer LogBuffer; // backbuffer for log
public:
	void Default();
	void Clear();
	void Init(C4Facet &cgo, bool fStartup);
	void Execute();
	void DrawLoader(C4Facet &cgo);
	void Draw(C4Facet &cgo);
	void AddLog(const char *szMessage);
	void ClearLog();
	void LogNotify();
	void EnsureLastMessage();
	bool ControlScrollUp();
	bool ControlScrollDown();
	C4Player* GetMessagePlayer(const char *szMessage);

	friend class C4MessageInput;
};


#endif
