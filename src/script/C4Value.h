/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2001, 2004  Sven Eberhardt
 * Copyright (c) 2001-2002, 2006  Peter Wortmann
 * Copyright (c) 2006-2008  Günther Brammer
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
#ifndef INC_C4Value
#define INC_C4Value

#include "C4Id.h"

// class declarations
class C4Value;
class C4Object;
class C4PropList;
class C4String;
class C4ValueArray;

// C4Value type
enum C4V_Type
{
	C4V_Any=0,         // nil
	C4V_Int=1,
	C4V_Bool=2,
	C4V_PropList=3,
	C4V_C4Object=4,
	C4V_String=5,
	C4V_Array=6,

	C4V_C4ObjectEnum=9, // enumerated object
	C4V_C4DefEnum=10 // enumerated object
};

#define C4V_Last (int) C4V_Array

const char* GetC4VName(const C4V_Type Type);
char GetC4VID(const C4V_Type Type);
C4V_Type GetC4VFromID(char C4VID);

union C4V_Data
{
	intptr_t Int;
	C4Object * Obj;
	C4PropList * PropList;
	C4String * Str;
	C4ValueArray * Array;
	// cheat a little - assume that all members have the same length
	operator void * () { return Obj; }
	operator const void * () const { return Obj; }
	C4V_Data &operator = (void *p) { Obj = reinterpret_cast<C4Object *>(p); return *this; }
};

// converter function, used in converter table
struct C4VCnvFn
{
	enum { CnvOK, CnvOK0, CnvError, CnvObject } Function;
	bool Warn;
};

template <typename T> struct C4ValueConv;

class C4Value
{
public:

	C4Value() : Type(C4V_Any), NextRef(NULL) { Data.Obj = 0; }

	C4Value(const C4Value &nValue) : Data(nValue.Data), Type(nValue.Type), NextRef(NULL)
	{ AddDataRef(); }

	explicit C4Value(bool data): Type(C4V_Bool), NextRef(NULL)
	{ Data.Int = data; }
	explicit C4Value(int32_t data): Type(C4V_Int), NextRef(NULL)
	{ Data.Int = data; }
	explicit C4Value(C4Object *pObj): Type(pObj ? C4V_C4Object : C4V_Any), NextRef(NULL)
	{ Data.Obj = pObj; AddDataRef(); }
	explicit C4Value(C4String *pStr): Type(pStr ? C4V_String : C4V_Any), NextRef(NULL)
	{ Data.Str = pStr; AddDataRef(); }
	explicit C4Value(C4ValueArray *pArray): Type(pArray ? C4V_Array : C4V_Any), NextRef(NULL)
	{ Data.Array = pArray; AddDataRef(); }
	explicit C4Value(C4PropList *p): Type(p ? C4V_PropList : C4V_Any), NextRef(NULL)
	{ Data.PropList = p; AddDataRef(); }

	C4Value& operator = (const C4Value& nValue) { Set(nValue); return *this; }

	~C4Value() { DelDataRef(Data, Type, NextRef); }

	// Checked getters
	int32_t getInt() const { return ConvertTo(C4V_Int) ? Data.Int : 0; }
	bool getBool() const { return ConvertTo(C4V_Bool) ? !! Data : 0; }
	C4ID getC4ID() const;
	C4Object * getObj() const { return ConvertTo(C4V_C4Object) ? Data.Obj : NULL; }
	C4PropList * getPropList() const { return ConvertTo(C4V_PropList) ? Data.PropList : NULL; }
	C4String * getStr() const { return ConvertTo(C4V_String) ? Data.Str : NULL; }
	C4ValueArray * getArray() const { return ConvertTo(C4V_Array) ? Data.Array : NULL; }

	// Unchecked getters
	int32_t _getInt() const { return Data.Int; }
	bool _getBool() const { return !! Data.Int; }
	C4Object *_getObj() const { return Data.Obj; }
	C4String *_getStr() const { return Data.Str; }
	C4ValueArray *_getArray() const { return Data.Array; }
	C4PropList *_getPropList() const { return Data.PropList; }

	// Template versions
	template <typename T> inline T Get() { return C4ValueConv<T>::FromC4V(*this); }
	template <typename T> inline T _Get() { return C4ValueConv<T>::_FromC4V(*this); }

	bool operator ! () const { return !GetData(); }
	inline operator const void* () const { return GetData()?this:0; }  // To allow use of C4Value in conditions

	void Set(const C4Value &nValue) { if (this != &nValue) Set(nValue.Data, nValue.Type); }

	void SetInt(int i) { C4V_Data d; d.Int = i; Set(d, C4V_Int); }
	void SetBool(bool b) { C4V_Data d; d.Int = b; Set(d, C4V_Bool); }
	void SetObject(C4Object * Obj) { C4V_Data d; d.Obj = Obj; Set(d, C4V_C4Object); }
	void SetString(C4String * Str) { C4V_Data d; d.Str = Str; Set(d, C4V_String); }
	void SetArray(C4ValueArray * Array) { C4V_Data d; d.Array = Array; Set(d, C4V_Array); }
	void SetPropList(C4PropList * PropList) { C4V_Data d; d.PropList = PropList; Set(d, C4V_PropList); }
	void Set0();

	bool operator == (const C4Value& Value2) const;
	bool operator != (const C4Value& Value2) const;

	// Change and set Type to int in case it was any before (avoids GuessType())
	C4Value & operator += (int32_t by) { Data.Int += by; Type=C4V_Int; return *this; }
	C4Value & operator -= (int32_t by) { Data.Int -= by; Type=C4V_Int; return *this; }
	C4Value & operator *= (int32_t by) { Data.Int *= by; Type=C4V_Int; return *this; }
	C4Value & operator /= (int32_t by) { Data.Int /= by; Type=C4V_Int; return *this; }
	C4Value & operator %= (int32_t by) { Data.Int %= by; Type=C4V_Int; return *this; }
	C4Value & operator &= (int32_t by) { Data.Int &= by; Type=C4V_Int; return *this; }
	C4Value & operator ^= (int32_t by) { Data.Int ^= by; Type=C4V_Int; return *this; }
	C4Value & operator |= (int32_t by) { Data.Int |= by; Type=C4V_Int; return *this; }
	C4Value & operator ++ ()           { Data.Int++;     Type=C4V_Int; return *this; }
	C4Value operator ++ (int)          { C4Value old = *this; ++(*this); return old; }
	C4Value & operator -- ()           { Data.Int--;     Type=C4V_Int; return *this; }
	C4Value operator -- (int)          { C4Value old = *this; --(*this); return old; }

	// getters
	C4V_Data GetData()    const { return Data; }
	C4V_Type GetType()    const { return Type; }

	const char *GetTypeName() const { return GetC4VName(GetType()); }
	const char *GetTypeInfo();

	void DenumeratePointer();

	StdStrBuf GetDataString() const;

	inline bool ConvertTo(C4V_Type vtToType) const // convert to dest type
	{
		switch (C4ScriptCnvMap[Type][vtToType].Function)
		{
		case C4VCnvFn::CnvOK: return true;
		case C4VCnvFn::CnvOK0: return !*this;
		case C4VCnvFn::CnvError: return false;
		case C4VCnvFn::CnvObject: return FnCnvObject();
		}
		assert(!"C4Value::ConvertTo: Invalid conversion function specified");
		return false;
	}
	inline static bool WarnAboutConversion(C4V_Type vtFromType, C4V_Type vtToType)
	{
		return C4ScriptCnvMap[vtFromType][vtToType].Warn;
	}

	// Compilation
	void CompileFunc(StdCompiler *pComp);

	static inline bool IsNullableType(C4V_Type Type)
	{ return Type == C4V_Int || Type == C4V_Bool; }

protected:
	// data
	C4V_Data Data;

	// data type
	C4V_Type Type;

	// proplist reference list
	C4Value * NextRef;

	C4Value(C4V_Data nData, C4V_Type nType): Data(nData), NextRef(NULL)
	{ Type = (nData || IsNullableType(nType) ? nType : C4V_Any); AddDataRef(); }

	void Set(C4V_Data nData, C4V_Type nType);

	void AddDataRef();
	void DelDataRef(C4V_Data Data, C4V_Type Type, C4Value *pNextRef);

	static C4VCnvFn C4ScriptCnvMap[C4V_Last+1][C4V_Last+1];
	bool FnCnvObject() const;

	friend class C4PropList;
	friend class C4AulDefFunc;
	friend class C4AulExec;
	friend C4Value C4VInt(int32_t iVal);
	friend C4Value C4VBool(bool fVal);
};

// converter
inline C4Value C4VInt(int32_t iVal) { C4V_Data d; d.Int = iVal; return C4Value(d, C4V_Int); }
inline C4Value C4VBool(bool fVal) { C4V_Data d; d.Int = fVal; return C4Value(d, C4V_Bool); }
C4Value C4VID(C4ID iVal);
inline C4Value C4VObj(C4Object *pObj) { return C4Value(pObj); }
inline C4Value C4VPropList(C4PropList * p) { return C4Value(p); }
inline C4Value C4VString(C4String *pStr) { return C4Value(pStr); }
inline C4Value C4VArray(C4ValueArray *pArray) { return C4Value(pArray); }

C4Value C4VString(StdStrBuf strString);
C4Value C4VString(const char *strString);

// converter templates
template <> struct C4ValueConv<int32_t>
{
	inline static C4V_Type Type() { return C4V_Int; }
	inline static int32_t FromC4V(C4Value &v) { return v.getInt(); }
	inline static int32_t _FromC4V(C4Value &v) { return v._getInt(); }
	inline static C4Value ToC4V(int32_t v) { return C4VInt(v); }
};
template <> struct C4ValueConv<bool>
{
	inline static C4V_Type Type() { return C4V_Bool; }
	inline static bool FromC4V(C4Value &v) { return v.getBool(); }
	inline static bool _FromC4V(C4Value &v) { return v._getBool(); }
	inline static C4Value ToC4V(bool v) { return C4VBool(v); }
};
template <> struct C4ValueConv<C4ID>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4ID FromC4V(C4Value &v) { return v.getC4ID(); }
	inline static C4ID _FromC4V(C4Value &v) { return FromC4V(v); }
	inline static C4Value ToC4V(C4ID v) { return C4VID(v); }
};
template <> struct C4ValueConv<C4Object *>
{
	inline static C4V_Type Type() { return C4V_C4Object; }
	inline static C4Object *FromC4V(C4Value &v) { return v.getObj(); }
	inline static C4Object *_FromC4V(C4Value &v) { return v._getObj(); }
	inline static C4Value ToC4V(C4Object *v) { return C4VObj(v); }
};
template <> struct C4ValueConv<C4String *>
{
	inline static C4V_Type Type() { return C4V_String; }
	inline static C4String *FromC4V(C4Value &v) { return v.getStr(); }
	inline static C4String *_FromC4V(C4Value &v) { return v._getStr(); }
	inline static C4Value ToC4V(C4String *v) { return C4VString(v); }
};
template <> struct C4ValueConv<C4ValueArray *>
{
	inline static C4V_Type Type() { return C4V_Array; }
	inline static C4ValueArray *FromC4V(C4Value &v) { return v.getArray(); }
	inline static C4ValueArray *_FromC4V(C4Value &v) { return v._getArray(); }
	inline static C4Value ToC4V(C4ValueArray *v) { return C4VArray(v); }
};
template <> struct C4ValueConv<C4PropList *>
{
	inline static C4V_Type Type() { return C4V_PropList; }
	inline static C4PropList *FromC4V(C4Value &v) { return v.getPropList(); }
	inline static C4PropList *_FromC4V(C4Value &v) { return v._getPropList(); }
	inline static C4Value ToC4V(C4PropList *v) { return C4VPropList(v); }
};
template <> struct C4ValueConv<const C4Value &>
{
	inline static C4V_Type Type() { return C4V_Any; }
	inline static const C4Value &FromC4V(C4Value &v) { return v; }
	inline static const C4Value &_FromC4V(C4Value &v) { return v; }
	inline static C4Value ToC4V(const C4Value &v) { return v; }
};
template <> struct C4ValueConv<C4Value>
{
	inline static C4V_Type Type() { return C4V_Any; }
	inline static C4Value FromC4V(C4Value &v) { return v; }
	inline static C4Value _FromC4V(C4Value &v) { return v; }
	inline static C4Value ToC4V(C4Value v) { return v; }
};

// aliases
template <> struct C4ValueConv<long> : public C4ValueConv<int32_t> { };
#if defined(_MSC_VER) && _MSC_VER <= 1100
template <> struct C4ValueConv<int> : public C4ValueConv<int32_t> { };
#endif

extern const C4Value C4VFalse, C4VTrue;

// type tag to allow other code to recognize C4VNull at compile time
class C4NullValue : public C4Value {};
extern const C4NullValue C4VNull;

#endif

