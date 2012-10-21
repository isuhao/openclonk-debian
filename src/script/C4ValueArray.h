/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004, 2006, 2010  Peter Wortmann
 * Copyright (c) 2001  Sven Eberhardt
 * Copyright (c) 2006, 2010  Günther Brammer
 * Copyright (c) 2009  Nicolas Hake
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

#include "C4Value.h"

#ifndef INC_C4ValueList
#define INC_C4ValueList

// reference counted array of C4Values
class C4ValueArray
{
public:
	enum { MaxSize = 1000000 }; // ye shalt not create arrays larger than that!

	C4ValueArray();
	C4ValueArray(int32_t inSize);
	C4ValueArray(const C4ValueArray &);

	~C4ValueArray();

	C4ValueArray &operator =(const C4ValueArray&);

	int32_t GetSize() const { return iSize; }

	const C4Value &GetItem(int32_t iElem) const
	{
		if (-iSize <= iElem && iElem < 0)
			return pData[iSize + iElem];
		else if (0 <= iElem && iElem < iSize)
			return pData[iElem];
		else
			return C4VNull;
	}

	C4Value operator[](int32_t iElem) const { return GetItem(iElem); }
	C4Value &operator[](int32_t iElem); // interface for the engine, asserts that 0 <= index < MaxSize

	void Reset();
	void SetItem(int32_t iElemNr, const C4Value &Value); // interface for script
	void SetSize(int32_t inSize); // (enlarge only!)

	void Denumerate(C4ValueNumbers *);

	// comparison
	bool operator==(const C4ValueArray&) const;

	// Compilation
	void CompileFunc(class StdCompiler *pComp, C4ValueNumbers *);


	// Add/Remove Reference
	void IncRef() { iRefCnt++; }
	void DecRef() { if (!--iRefCnt) delete this;  }

	// Return sub-array [startIndex, endIndex). Throws C4AulExecError.
	C4ValueArray * GetSlice(int32_t startIndex, int32_t endIndex);
	// Sets sub-array [startIndex, endIndex). Might resize the array.
	void SetSlice(int32_t startIndex, int32_t endIndex, const C4Value &Val);

	void Sort(class C4SortObject &rSort);
	void SortStrings();

private:
	// Reference counter
	unsigned int iRefCnt;
	int32_t iSize, iCapacity;
	C4Value* pData;
};

#endif

