/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 1998-2000, 2007  Matthes Bender
 * Copyright (c) 2005  Tobias Zwick
 * Copyright (c) 2005, 2008, 2010  Sven Eberhardt
 * Copyright (c) 2005-2006, 2010  Günther Brammer
 * Copyright (c) 2010  Nicolas Hake
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

/* Main header to include all others */

#ifndef INC_C4Include
#define INC_C4Include

#include "PlatformAbstraction.h"

// boost headers - after PlatformAbstraction to prevent redefines of stdint
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "Standard.h"
#include "C4Prototypes.h"
#include "C4Real.h"
#include "StdBuf.h"
#include "StdFile.h"
#include "StdResStr2.h"
#include "C4Log.h"
#include "C4Reloc.h"
#include "C4Config.h"

#include "C4Game.h"

#ifdef DEBUGREC
#define DEBUGREC_SCRIPT
#define DEBUGREC_START_FRAME 0
#define DEBUGREC_PXS
#define DEBUGREC_OBJCOM
#define DEBUGREC_MATSCAN
//#define DEBUGREC_RECRUITMENT
#define DEBUGREC_MENU
#define DEBUGREC_OCF
#endif

// solidmask debugging
//#define SOLIDMASK_DEBUG

// debug memory management - must come after boost headers,
// because boost uses placement new
#ifndef NODEBUGMEM
#if defined(_DEBUG) && defined(_MSC_VER)
#if _MSC_VER <= 1200
#include <new>
#include <memory>
#include <crtdbg.h>
#include <malloc.h>
#define malloc(size) ::_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
inline void *operator new(size_t s, const char *szFile, long iLine)
{ return ::operator new(s, _NORMAL_BLOCK, szFile, iLine); }
inline void operator delete(void *p, const char *, long)
{ ::operator delete(p); }
#define new_orig new
#define new new(__FILE__, __LINE__)
#endif
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <climits>

#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#endif // INC_C4Include
