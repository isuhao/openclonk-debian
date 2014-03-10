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

/* A facet that can hold its own surface and also target coordinates */

#ifndef INC_C4FacetEx
#define INC_C4FacetEx

#include <C4Facet.h>
#include <C4Surface.h>

const int C4FCT_Full   = -1,
          C4FCT_Height = -2,
          C4FCT_Width  = -3;

class C4TargetFacet: public C4Facet
{
public:
	C4TargetFacet() { Default(); }
	~C4TargetFacet() { }
public:
	float TargetX,TargetY,Zoom;
public:
	void Default() { TargetX=TargetY=0; Zoom=1; C4Facet::Default(); }
	void Clear() { Surface=NULL; }

	void Set(const C4Facet &cpy) { TargetX=TargetY=0; Zoom=1; C4Facet::Set(cpy); }
	void Set(const C4TargetFacet &cpy) { *this = cpy; }
	void Set(class C4Surface *nsfc, float nx, float ny, float nwdt, float nhgt, float ntx=0, float nty=0, float Zoom=1);
	void Set(class C4Surface *nsfc, const C4Rect & r, float ntx=0, float nty=0, float Zoom=1);

	void DrawLineDw(int iX1, int iY1, int iX2, int iY2, uint32_t col1, uint32_t col2); // uses Target and position
public:
	C4TargetFacet &operator = (const C4Facet& rhs)
	{
		Set(rhs.Surface,rhs.X,rhs.Y,rhs.Wdt,rhs.Hgt);
		return *this;
	}
	void SetRect(C4TargetRect &rSrc);
};

// facet that can hold its own surface
class C4FacetSurface : public C4Facet
{
private:
	C4Surface Face;

private:
	C4FacetSurface(const C4FacetSurface &rCpy); // NO copy!
	C4FacetSurface &operator = (const C4FacetSurface &rCpy); // NO copy assignment!
public:
	C4FacetSurface() { Default(); }
	~C4FacetSurface() { Clear(); }

	void Default() { Face.Default(); C4Facet::Default(); }
	void Clear() { Face.Clear(); }

	void Set(const C4Facet &cpy) { Clear(); C4Facet::Set(cpy); }
	void Set(C4Surface * nsfc, int nx, int ny, int nwdt, int nhgt)
	{ C4Facet::Set(nsfc, nx,ny,nwdt,nhgt); }

	void Grayscale(int32_t iOffset = 0);
	bool Create(int iWdt, int iHgt, int iWdt2=C4FCT_Full, int iHgt2=C4FCT_Full);
	C4Surface &GetFace() { return Face; } // get internal face
	bool CreateClrByOwner(C4Surface *pBySurface);
	bool EnsureSize(int iMinWdt, int iMinHgt);
	bool EnsureOwnSurface();
	bool Load(C4Group &hGroup, const char *szName, int iWdt=C4FCT_Full, int iHgt=C4FCT_Full, bool fOwnPal=false, bool fNoErrIfNotFound=false);
	bool Load(BYTE *bpBitmap, int iWdt=C4FCT_Full, int iHgt=C4FCT_Full);
	bool Save(C4Group &hGroup, const char *szName);
	void GrabFrom(C4FacetSurface &rSource)
	{
		Clear(); Default();
		Face.MoveFrom(&rSource.Face);
		Set(rSource.Surface == &rSource.Face ? &Face : rSource.Surface, rSource.X, rSource.Y, rSource.Wdt, rSource.Hgt);
		rSource.Default();
	}
	bool CopyFromSfcMaxSize(C4Surface &srcSfc, int32_t iMaxSize, uint32_t dwColor=0u);
};

// facet with source group ID; used to avoid doubled loading from same group
class C4FacetID : public C4FacetSurface
{
public:
	int32_t idSourceGroup;

	C4FacetID() : C4FacetSurface(), idSourceGroup(0) { } // ctor

	void Default() { C4FacetSurface::Default(); idSourceGroup = 0; } // default to std values
	void Clear() { C4FacetSurface::Clear(); idSourceGroup = 0; } // clear all data in class
};

#endif
