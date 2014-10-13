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

/* NewGfx interfaces */
#include "C4Include.h"
#include <C4Draw.h>

#include "C4App.h"
#include <C4FontLoader.h>
#include <C4Window.h>
#include <C4DrawGL.h>
#include <C4DrawT.h>
#include <C4Markup.h>
#include "C4Rect.h"
#include <C4Config.h>
#include "StdMesh.h"

#include <stdio.h>
#include <limits.h>

// Global access pointer
C4Draw *pDraw=NULL;

// Transformation matrix to convert meshes from Ogre to Clonk coordinate system
const StdMeshMatrix C4Draw::OgreToClonk = StdMeshMatrix::Scale(-1.0f, 1.0f, 1.0f) * StdMeshMatrix::Rotate(float(M_PI)/2.0f, 1.0f, 0.0f, 0.0f) * StdMeshMatrix::Rotate(float(M_PI)/2.0f, 0.0f, 0.0f, 1.0f);

inline DWORD GetTextShadowClr(DWORD dwTxtClr)
{
	return RGBA(((dwTxtClr >>  0) % 256) / 3, ((dwTxtClr >>  8) % 256) / 3, ((dwTxtClr >> 16) % 256) / 3, (dwTxtClr >> 24) % 256);
}

void C4BltTransform::SetRotate(float iAngle, float fOffX, float fOffY) // set by angle and rotation offset
{
	// iAngle is in degrees (cycling from 0 to 360)
	// determine sine and cos of reversed angle in radians
	// fAngle = -iAngle * pi/180 = iAngle * -pi/180
	float fAngle = iAngle * -0.0174532925f;
	float fsin = sinf(fAngle); float fcos = cosf(fAngle);
	// set matrix values
	mat[0] = +fcos; mat[1] = +fsin; mat[2] = (1-fcos)*fOffX - fsin*fOffY;
	mat[3] = -fsin; mat[4] = +fcos; mat[5] = (1-fcos)*fOffY + fsin*fOffX;
	mat[6] = 0; mat[7] = 0; mat[8] = 1;
	/*    calculation of rotation matrix:
	  x2 = fcos*(x1-fOffX) + fsin*(y1-fOffY) + fOffX
	     = fcos*x1 - fcos*fOffX + fsin*y1 - fsin*fOffY + fOffX
	     = x1*fcos + y1*fsin + (1-fcos)*fOffX - fsin*fOffY

	  y2 = -fsin*(x1-fOffX) + fcos*(y1-fOffY) + fOffY
	     = x1*-fsin + fsin*fOffX + y1*fcos - fcos*fOffY + fOffY
	     = x1*-fsin + y1*fcos + fsin*fOffX + (1-fcos)*fOffY */
}

bool C4BltTransform::SetAsInv(C4BltTransform &r)
{
	// calc inverse of matrix
	float det = r.mat[0]*r.mat[4]*r.mat[8] + r.mat[1]*r.mat[5]*r.mat[6]
	            + r.mat[2]*r.mat[3]*r.mat[7] - r.mat[2]*r.mat[4]*r.mat[6]
	            - r.mat[0]*r.mat[5]*r.mat[7] - r.mat[1]*r.mat[3]*r.mat[8];
	if (!det) { Set(1,0,0,0,1,0,0,0,1); return false; }
	mat[0] = (r.mat[4] * r.mat[8] - r.mat[5] * r.mat[7]) / det;
	mat[1] = (r.mat[2] * r.mat[7] - r.mat[1] * r.mat[8]) / det;
	mat[2] = (r.mat[1] * r.mat[5] - r.mat[2] * r.mat[4]) / det;
	mat[3] = (r.mat[5] * r.mat[6] - r.mat[3] * r.mat[8]) / det;
	mat[4] = (r.mat[0] * r.mat[8] - r.mat[2] * r.mat[6]) / det;
	mat[5] = (r.mat[2] * r.mat[3] - r.mat[0] * r.mat[5]) / det;
	mat[6] = (r.mat[3] * r.mat[7] - r.mat[4] * r.mat[6]) / det;
	mat[7] = (r.mat[1] * r.mat[6] - r.mat[0] * r.mat[7]) / det;
	mat[8] = (r.mat[0] * r.mat[4] - r.mat[1] * r.mat[3]) / det;
	return true;
}

void C4BltTransform::TransformPoint(float &rX, float &rY) const
{
	// apply matrix
	float fW = mat[6] * rX + mat[7] * rY + mat[8];
	// store in temp, so original rX is used for calculation of rY
	float fX = (mat[0] * rX + mat[1] * rY + mat[2]) / fW;
	rY = (mat[3] * rX + mat[4] * rY + mat[5]) / fW;
	rX = fX; // apply temp
}

C4Pattern& C4Pattern::operator=(const C4Pattern& nPattern)
{
	sfcPattern32 = nPattern.sfcPattern32;
	if (sfcPattern32) sfcPattern32->Lock();
	delete [] CachedPattern;
	if (nPattern.CachedPattern)
	{
		CachedPattern = new uint32_t[sfcPattern32->Wdt * sfcPattern32->Hgt];
		memcpy(CachedPattern, nPattern.CachedPattern, sfcPattern32->Wdt * sfcPattern32->Hgt * 4);
	}
	else
	{
		CachedPattern = 0;
	}
	Wdt = nPattern.Wdt;
	Hgt = nPattern.Hgt;
	Zoom = nPattern.Zoom;
	return *this;
}

bool C4Pattern::Set(C4Surface * sfcSource, int iZoom)
{
	// Safety
	if (!sfcSource) return false;
	// Clear existing pattern
	Clear();
	// new style: simply store pattern for modulation or shifting, which will be decided upon use
	sfcPattern32=sfcSource;
	sfcPattern32->Lock();
	Wdt = sfcPattern32->Wdt;
	Hgt = sfcPattern32->Hgt;
	// set zoom
	Zoom=iZoom;
	// set flags
	CachedPattern = new uint32_t[Wdt * Hgt];
	if (!CachedPattern) return false;
	for (int y = 0; y < Hgt; ++y)
		for (int x = 0; x < Wdt; ++x)
		{
			CachedPattern[y * Wdt + x] = sfcPattern32->GetPixDw(x, y, false);
		}
	return true;
}

C4Pattern::C4Pattern()
{
	// disable
	sfcPattern32=NULL;
	CachedPattern = 0;
	Zoom=0;
}

void C4Pattern::Clear()
{
	// pattern assigned
	if (sfcPattern32)
	{
		// unlock it
		sfcPattern32->Unlock();
		// clear field
		sfcPattern32=NULL;
	}
	delete[] CachedPattern; CachedPattern = 0;
}

DWORD C4Pattern::PatternClr(unsigned int iX, unsigned int iY) const
{
	if (!CachedPattern) return 0;
	// wrap position
	iX %= Wdt; iY %= Hgt;
	return CachedPattern[iY * Wdt + iX];
}

void C4GammaControl::SetClrChannel(WORD *pBuf, BYTE c1, BYTE c2, int c3)
{
	// Using this minimum value, gamma ramp errors on some cards can be avoided
	int MinGamma = 0x100;
	// adjust clr3-value
	++c3;
	// get rises
	int r1=c2-c1,r2=c3-c2,r=(c3-c1)/2;
	// calc beginning and end rise
	r1=2*r1-r; r2=2*r2-r;
	// calc ramp
	WORD *pBuf2=pBuf+128;
	for (int i=0; i<128; ++i)
	{
		int i2=128-i;
		// interpolate linear ramps with the rises r1 and r
		*pBuf ++=BoundBy(((c1+r1*i/128) *i2  +  (c2-r*i2/128) *i) <<1, MinGamma, 0xffff);
		// interpolate linear ramps with the rises r and r2
		*pBuf2++=BoundBy(((c2+r*i/128) *i2  +  (c3-r2*i2/128) *i) <<1, MinGamma, 0xffff);
	}
}

void C4GammaControl::Set(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3)
{
	// set red, green and blue channel
	SetClrChannel(ramp.red  , GetRedValue(dwClr1), GetRedValue(dwClr2), GetRedValue(dwClr3));
	SetClrChannel(ramp.green, GetGreenValue(dwClr1), GetGreenValue(dwClr2), GetGreenValue(dwClr3));
	SetClrChannel(ramp.blue , GetBlueValue(dwClr1), GetBlueValue(dwClr2), GetBlueValue(dwClr3));
}

DWORD C4GammaControl::ApplyTo(DWORD dwClr)
{
	// apply to red, green and blue color component
	return RGBA(ramp.red[GetRedValue(dwClr)]>>8, ramp.green[GetGreenValue(dwClr)]>>8, ramp.blue[GetBlueValue(dwClr)]>>8, dwClr>>24);
}


//--------------------------------------------------------------------

C4FogOfWar::~C4FogOfWar()
{
	delete[]pMap; delete pSurface;
}

void C4FogOfWar::Reset(int ResX, int ResY, int WdtPx, int HgtPx, int OffX, int OffY, unsigned char StartVis, int x0, int y0, uint32_t dwBackClr, class C4Surface *backsfc)
{
	// set values
	ResolutionX = ResX; ResolutionY = ResY;
	this->dwBackClr = dwBackClr;
	this->OffX = -((OffX) % ResolutionX);
	this->OffY = -((OffY) % ResolutionY);
	// calc w/h required for map
	Wdt = (WdtPx - this->OffX + ResolutionX-1) / ResolutionX + 1;
	Hgt = (HgtPx - this->OffY + ResolutionY-1) / ResolutionY + 1;
	this->OffX += x0;
	this->OffY += y0;
	size_t NewMapSize = Wdt * Hgt;
	if (NewMapSize > MapSize || NewMapSize < MapSize / 2)
	{
		delete [] pMap;
		pMap = new unsigned char[MapSize = NewMapSize];
	}
	if (!pSurface || pSurface->Wdt<Wdt || pSurface->Hgt<Hgt)
	{
		delete pSurface;
		pSurface = new C4Surface(Max(Wdt, Hgt), Max(Wdt, Hgt)); // force larger texture size by making it squared!
	}
	// is a background color desired?
	if (dwBackClr && backsfc)
	{
		// then draw a background now and fade against transparent later
		// FIXME: don't do this if shaders are used
		pDraw->DrawBoxDw(backsfc, x0,y0, x0+WdtPx-1, y0+HgtPx-1, dwBackClr);
		FadeTransparent = true;
	}
	else
		FadeTransparent = false;
	// reset all of map to given values
	memset(pMap, StartVis, MapSize);
	pSurface->Lock();
	pSurface->ClearBoxDw(0, 0, Wdt, Hgt);
}

C4Surface *C4FogOfWar::GetSurface()
{
	if (pSurface->IsLocked())
	{
		for (int x = 0; x < Wdt; ++x)
			for (int y = 0; y < Hgt; ++y)
				pSurface->SetPixDw(x, y, pMap[y * Wdt + x] << 24 | (dwBackClr & 0xFFFFFF));
		//pSurface->SetPixDw(x, y, 0x7f000000 + (((0xff*x)/iWdt) << 24) + (((0xff*y)/iHgt) << 16));
		pSurface->Unlock();
	}
	return pSurface;
}

void C4FogOfWar::ReduceModulation(int cx, int cy, int Radius, int (*VisProc)(int, int, int, int, int))
{
	// landscape coordinates: cx, cy, VisProc
	// display coordinates: zx, zy, x, y
	float zx = float(cx);
	float zy = float(cy);
	pDraw->ApplyZoom(zx, zy);
	Radius = int(pDraw->Zoom * Radius);
	// reveal all within iRadius1; fade off squared until iRadius2
	int x = OffX, y = OffY, xe = Wdt*ResolutionX+OffX;
	int RadiusSq = Radius*Radius;
	for (unsigned int i = 0; i < MapSize; i++)
	{
		if ((x-zx)*(x-zx)+(y-zy)*(y-zy) < RadiusSq)
		{
			float lx = float(x);
			float ly = float(y);
			pDraw->RemoveZoom(lx, ly);
			pMap[i] = Max<int>(pMap[i], VisProc(255, int(lx), int(ly), int(cx), int(cy)));
		}
		// next pos
		x += ResolutionX;
		if (x >= xe) { x = OffX; y += ResolutionY; }
	}
}

void C4FogOfWar::AddModulation(int cx, int cy, int Radius, uint8_t Transparency)
{
	{
		float x=float(cx); float y=float(cy);
		pDraw->ApplyZoom(x,y);
		cx=int(x); cy=int(y);
	}
	Radius = int(pDraw->Zoom * Radius);
	// hide all within iRadius1; fade off squared until iRadius2
	int x = OffX, y = OffY, xe = Wdt*ResolutionX+OffX;
	int RadiusSq = Radius*Radius;
	for (unsigned int i = 0; i < MapSize; i++)
	{
		int d = (x-cx)*(x-cx)+(y-cy)*(y-cy);
		if (d < RadiusSq)
			pMap[i] = Min<uint8_t>(Transparency, pMap[i]);
		// next pos
		x += ResolutionX;
		if (x >= xe) { x = OffX; y += ResolutionY; }
	}
}

uint32_t C4FogOfWar::GetModAt(int x, int y) const
{
#if 0
	// fast but inaccurate method
	x = BoundBy((x - iOffX + iResolutionX/2) / iResolutionX, 0, iWdt-1);
	y = BoundBy((y - iOffY + iResolutionY/2) / iResolutionY, 0, iHgt-1);
	return pMap[y * iWdt + x]->dwModClr;
#else
	// slower, more accurate method: Interpolate between 4 neighboured modulations
	x -= OffX;
	y -= OffY;
	int tx = BoundBy(x / ResolutionX, 0, Wdt-1);
	int ty = BoundBy(y / ResolutionY, 0, Hgt-1);
	int tx2 = Min(tx + 1, Wdt-1);
	int ty2 = Min(ty + 1, Hgt-1);

	// TODO: Alphafixed. Correct?
	unsigned char Vis = pMap[ty*Wdt+tx];
	uint32_t c1 = FadeTransparent ? 0xffffff | (Vis << 24) : C4RGB(Vis, Vis, Vis);
	Vis = pMap[ty*Wdt+tx2];
	uint32_t c2 = FadeTransparent ? 0xffffff | (Vis << 24) : C4RGB(Vis, Vis, Vis);
	Vis = pMap[ty2*Wdt+tx];
	uint32_t c3 = FadeTransparent ? 0xffffff | (Vis << 24) : C4RGB(Vis, Vis, Vis);
	Vis = pMap[ty2*Wdt+tx2];
	uint32_t c4 = FadeTransparent ? 0xffffff | (Vis << 24) : C4RGB(Vis, Vis, Vis);
	CColorFadeMatrix clrs(tx*ResolutionX, ty*ResolutionY, ResolutionX, ResolutionY, c1, c2, c3, c4);
	return clrs.GetColorAt(x, y);
#endif
}

// -------------------------------------------------------------------

CColorFadeMatrix::CColorFadeMatrix(int iX, int iY, int iWdt, int iHgt, uint32_t dwClr1, uint32_t dwClr2, uint32_t dwClr3, uint32_t dwClr4)
		: ox(iX), oy(iY), w(iWdt), h(iHgt)
{
	uint32_t dwMask = 0xff;
	for (int iChan = 0; iChan < 4; (++iChan),(dwMask<<=8))
	{
		int c0 = (dwClr1 & dwMask) >> (iChan*8);
		int cx = (dwClr2 & dwMask) >> (iChan*8);
		int cy = (dwClr3 & dwMask) >> (iChan*8);
		int ce = (dwClr4 & dwMask) >> (iChan*8);
		clrs[iChan].c0 = c0;
		clrs[iChan].cx = cx - c0;
		clrs[iChan].cy = cy - c0;
		clrs[iChan].ce = ce;
	}
}

uint32_t CColorFadeMatrix::GetColorAt(int iX, int iY)
{
	iX -= ox; iY -= oy;
	uint32_t dwResult = 0x00;
	for (int iChan = 0; iChan < 4; ++iChan)
	{
		int clr = clrs[iChan].c0 + clrs[iChan].cx * iX / w + clrs[iChan].cy * iY / h;
		clr += iX*iY * (clrs[iChan].ce - clr) / (w*h);
		dwResult |= (BoundBy(clr, 0, 255) << (iChan*8));
	}
	return dwResult;
}

// -------------------------------------------------------------------

void C4Draw::Default()
{
	Editor=true;
	RenderTarget=NULL;
	ClipAll=false;
	Active=false;
	BlitModulated=false;
	dwBlitMode = 0;
	Gamma.Default();
	DefRamp.Default();
	// pClrModMap = NULL; - invalid if !fUseClrModMap anyway
	fUseClrModMap = false;
	ZoomX = 0; ZoomY = 0; Zoom = 1;
	MeshTransform = NULL;
	fUsePerspective = false;
	for (int32_t iRamp=0; iRamp<3*C4MaxGammaRamps; iRamp+=3)
		{ dwGamma[iRamp+0]=0; dwGamma[iRamp+1]=0x808080; dwGamma[iRamp+2]=0xffffff; }
	fSetGamma=false;
}

void C4Draw::Clear()
{
	ResetGamma();
	DisableGamma();
	Active=BlitModulated=fUseClrModMap=false;
	dwBlitMode = 0;
}

bool C4Draw::GetSurfaceSize(C4Surface * sfcSurface, int &iWdt, int &iHgt)
{
	return sfcSurface->GetSurfaceSize(iWdt, iHgt);
}

bool C4Draw::SubPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// Set sub primary clipper
	SetPrimaryClipper(Max(iX1,iClipX1),Max(iY1,iClipY1),Min(iX2,iClipX2),Min(iY2,iClipY2));
	return true;
}

bool C4Draw::StorePrimaryClipper()
{
	// Store current primary clipper
	fStClipX1=fClipX1; fStClipY1=fClipY1; fStClipX2=fClipX2; fStClipY2=fClipY2;
	return true;
}

bool C4Draw::RestorePrimaryClipper()
{
	// Restore primary clipper
	SetPrimaryClipper(fStClipX1, fStClipY1, fStClipX2, fStClipY2);
	return true;
}

bool C4Draw::SetPrimaryClipper(int iX1, int iY1, int iX2, int iY2)
{
	// set clipper
	fClipX1=iX1; fClipY1=iY1; fClipX2=iX2; fClipY2=iY2;
	iClipX1=iX1; iClipY1=iY1; iClipX2=iX2; iClipY2=iY2;
	UpdateClipper();
	// Done
	return true;
}

bool C4Draw::ApplyPrimaryClipper(C4Surface * sfcSurface)
{
	//sfcSurface->SetClipper(lpClipper);
	return true;
}

bool C4Draw::DetachPrimaryClipper(C4Surface * sfcSurface)
{
	//sfcSurface->SetClipper(NULL);
	return true;
}

bool C4Draw::NoPrimaryClipper()
{
	// apply maximum clipper
	SetPrimaryClipper(0,0,439832,439832);
	// Done
	return true;
}

void C4Draw::BlitLandscape(C4Surface * sfcSource, float fx, float fy,
                              C4Surface * sfcTarget, float tx, float ty, float wdt, float hgt, const C4Surface * textures[])
{
	Blit(sfcSource, fx, fy, wdt, hgt, sfcTarget, tx, ty, wdt, hgt, false);
}

void C4Draw::Blit8Fast(CSurface8 * sfcSource, int fx, int fy,
                          C4Surface * sfcTarget, int tx, int ty, int wdt, int hgt)
{
	// blit 8bit-sfc
	// lock surfaces
	assert(sfcTarget->fPrimary);
	bool fRender = sfcTarget->IsRenderTarget();
	if (!fRender) if (!sfcTarget->Lock())
			{ return; }

	float tfx = tx, tfy = ty, twdt = wdt, thgt = hgt;
	if (Zoom != 1.0)
	{
		ApplyZoom(tfx, tfy);
		twdt *= Zoom;
		thgt *= Zoom;
	}

	// blit 8 bit pix
	for (int ycnt=0; ycnt<thgt; ++ycnt)
		for (int xcnt=0; xcnt<twdt; ++xcnt)
		{
			BYTE byPix = sfcSource->GetPix(fx+wdt*xcnt/twdt, fy+hgt*ycnt/thgt);
			if (byPix) PerformPix(sfcTarget,(float)(tfx+xcnt), (float)(tfy+ycnt), sfcSource->pPal->GetClr(byPix));
		}
	// unlock
	if (!fRender) sfcTarget->Unlock();
}

bool C4Draw::Blit(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
                     C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
                     bool fSrcColKey, const C4BltTransform *pTransform)
{
	return BlitUnscaled(sfcSource, fx * sfcSource->Scale, fy * sfcSource->Scale, fwdt * sfcSource->Scale, fhgt * sfcSource->Scale,
	                    sfcTarget, tx, ty, twdt, thgt, fSrcColKey, pTransform);
}

bool C4Draw::BlitUnscaled(C4Surface * sfcSource, float fx, float fy, float fwdt, float fhgt,
                     C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt,
                     bool fSrcColKey, const C4BltTransform *pTransform)
{
	// safety
	if (!sfcSource || !sfcTarget || !twdt || !thgt || !fwdt || !fhgt) return false;
	// Apply Zoom
	C4BltTransform t;
	if (pTransform && Zoom != 1.0)
	{
		//tx = tx * Zoom - ZoomX * Zoom + ZoomX;
		//ty = ty * Zoom - ZoomY * Zoom + ZoomY;
		// The transformation is not location-independant, thus has to be zoomed, too.
		t.Set(pTransform->mat[0]*Zoom, pTransform->mat[1]*Zoom, pTransform->mat[2]*Zoom + ZoomX*(1-Zoom),
		      pTransform->mat[3]*Zoom, pTransform->mat[4]*Zoom, pTransform->mat[5]*Zoom + ZoomY*(1-Zoom),
		      pTransform->mat[6], pTransform->mat[7], pTransform->mat[8]);
		pTransform = &t;
	}
	else if (Zoom != 1.0)
	{
		ApplyZoom(tx, ty);
		twdt *= Zoom;
		thgt *= Zoom;
	}
	// emulated blit?
	if (!sfcTarget->IsRenderTarget())
		return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, int(tx), int(ty), int(twdt), int(thgt), fSrcColKey, pTransform);
	// calc stretch
	float scaleX = twdt/fwdt;
	float scaleY = thgt/fhgt;
	// bound
	if (ClipAll) return true;
	// check exact
	bool fExact = !pTransform && fwdt==twdt && fhgt==thgt;
	// manual clipping? (primary surface only)
	if (Config.Graphics.ClipManuallyE && !pTransform && sfcTarget->fPrimary)
	{
		float iOver;
		// Left
		iOver=tx-iClipX1;
		if (iOver<0)
		{
			twdt+=iOver;
			fwdt+=iOver/scaleX;
			fx-=iOver/scaleX;
			tx=float(iClipX1);
		}
		// Top
		iOver=ty-iClipY1;
		if (iOver<0)
		{
			thgt+=iOver;
			fhgt+=iOver/scaleY;
			fy-=iOver/scaleY;
			ty=float(iClipY1);
		}
		// Right
		iOver=iClipX2+1-(tx+twdt);
		if (iOver<0)
		{
			fwdt+=iOver/scaleX;
			twdt+=iOver;
		}
		// Bottom
		iOver=iClipY2+1-(ty+thgt);
		if (iOver<0)
		{
			fhgt+=iOver/scaleY;
			thgt+=iOver;
		}
	}
	// inside screen?
	if (twdt<=0 || thgt<=0) return false;
	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// texture present?
	if (!sfcSource->ppTex)
	{
		// primary surface?
		if (sfcSource->fPrimary)
		{
			// blit emulated
			return Blit8(sfcSource, int(fx), int(fy), int(fwdt), int(fhgt), sfcTarget, int(tx), int(ty), int(twdt), int(thgt), fSrcColKey);
		}
		return false;
	}
	// create blitting struct
	C4BltData BltData;
	// pass down pTransform
	BltData.pTransform=pTransform;
	// blit with basesfc?
	bool fBaseSfc=false;
	if (sfcSource->pMainSfc) if (sfcSource->pMainSfc->ppTex) fBaseSfc=true;
	// set blitting state - done by PerformBlt
	// get involved texture offsets
	int iTexSizeX=sfcSource->iTexSize;
	int iTexSizeY=sfcSource->iTexSize;
	int iTexX=Max(int(fx/iTexSizeX), 0);
	int iTexY=Max(int(fy/iTexSizeY), 0);
	int iTexX2=Min((int)(fx+fwdt-1)/iTexSizeX +1, sfcSource->iTexX);
	int iTexY2=Min((int)(fy+fhgt-1)/iTexSizeY +1, sfcSource->iTexY);
	// calc stretch regarding texture size and indent
/*	float scaleX2 = scaleX * iTexSizeX;
	float scaleY2 = scaleY * iTexSizeY;*/
	// Enable textures
	SetTexture();
	// blit from all these textures
	for (int iY=iTexY; iY<iTexY2; ++iY)
	{
		for (int iX=iTexX; iX<iTexX2; ++iX)
		{
			C4TexRef *pTex = *(sfcSource->ppTex + iY * sfcSource->iTexX + iX);
			// get current blitting offset in texture
			int iBlitX=sfcSource->iTexSize*iX;
			int iBlitY=sfcSource->iTexSize*iY;
			// size changed? recalc dependant, relevant (!) values
			if (iTexSizeX != pTex->iSizeX)
			{
				iTexSizeX = pTex->iSizeX;
				/*scaleX2 = scaleX * iTexSizeX;*/
			}
			if (iTexSizeY != pTex->iSizeY)
			{
				iTexSizeY = pTex->iSizeY;
				/*scaleY2 = scaleY * iTexSizeY;*/
			}

			// get new texture source bounds
			FLOAT_RECT fTexBlt;
			fTexBlt.left  = Max<float>(fx - iBlitX, 0);
			fTexBlt.top   = Max<float>(fy - iBlitY, 0);
			fTexBlt.right = Min<float>(fx + fwdt - (float)iBlitX, (float)iTexSizeX);
			fTexBlt.bottom= Min<float>(fy + fhgt - (float)iBlitY, (float)iTexSizeY);
			// get new dest bounds
			FLOAT_RECT tTexBlt;
			tTexBlt.left  = (fTexBlt.left  + iBlitX - fx) * scaleX + tx;
			tTexBlt.top   = (fTexBlt.top   + iBlitY - fy) * scaleY + ty;
			tTexBlt.right = (fTexBlt.right + iBlitX - fx) * scaleX + tx;
			tTexBlt.bottom= (fTexBlt.bottom+ iBlitY - fy) * scaleY + ty;
			// prepare blit data texture matrix
			// translate back to texture 0/0 regarding indent and blit offset
			/*BltData.TexPos.SetMoveScale(-tTexBlt.left, -tTexBlt.top, 1, 1);
			// apply back scaling and texture-indent - simply scale matrix down
			int i;
			for (i=0; i<3; ++i) BltData.TexPos.mat[i] /= scaleX2;
			for (i=3; i<6; ++i) BltData.TexPos.mat[i] /= scaleY2;
			// now, finally, move in texture - this must be done last, so no stupid zoom is applied...
			BltData.TexPos.MoveScale(((float) fTexBlt.left) / iTexSize,
			  ((float) fTexBlt.top) / iTexSize, 1, 1);*/
			// Set resulting matrix directly
			/*BltData.TexPos.SetMoveScale(
			  fTexBlt.left / iTexSizeX - tTexBlt.left / scaleX2,
			  fTexBlt.top / iTexSizeY - tTexBlt.top / scaleY2,
			  1 / scaleX2,
			  1 / scaleY2);*/

			// get tex bounds
			// The code below is commented out since the problem
			// in question is currently fixed by using non-power-of-two
			// and non-square textures.
#if 0
			// Size of this texture actually containing image data
			const int iImgSizeX = (iX == sfcSource->iTexX-1) ? ((sfcSource->Wdt - 1) % iTexSizeX + 1) : (iTexSizeX);
			const int iImgSizeY = (iY == sfcSource->iTexY-1) ? ((sfcSource->Hgt - 1) % iTexSizeY + 1) : (iTexSizeY);			
			// Make sure we don't access border pixels. Normally this is prevented
			// by GL_CLAMP_TO_EDGE anyway but for the bottom and rightmost textures
			// this does not work as the textures might only be partially filled.
			// This is the case if iImgSizeX != iTexSize or iImgSizeY != iTexSize.
			// See bug #396.
			fTexBlt.left  = Max<float>(fTexBlt.left, 0.5);
			fTexBlt.top   = Max<float>(fTexBlt.top, 0.5);
			fTexBlt.right = Min<float>(fTexBlt.right, iImgSizeX - 0.5);
			fTexBlt.bottom= Min<float>(fTexBlt.bottom, iImgSizeY - 0.5);
#endif

			// set up blit data as rect
			BltData.byNumVertices = 4;
			BltData.vtVtx[0].ftx = tTexBlt.left;  BltData.vtVtx[0].fty = tTexBlt.top;
			BltData.vtVtx[1].ftx = tTexBlt.right; BltData.vtVtx[1].fty = tTexBlt.top;
			BltData.vtVtx[2].ftx = tTexBlt.right; BltData.vtVtx[2].fty = tTexBlt.bottom;
			BltData.vtVtx[3].ftx = tTexBlt.left;  BltData.vtVtx[3].fty = tTexBlt.bottom;
			BltData.vtVtx[0].tx = fTexBlt.left / iTexSizeX; BltData.vtVtx[0].ty = fTexBlt.top / iTexSizeY;
			BltData.vtVtx[1].tx = fTexBlt.right / iTexSizeX; BltData.vtVtx[1].ty = fTexBlt.top / iTexSizeY;
			BltData.vtVtx[2].tx = fTexBlt.right / iTexSizeX; BltData.vtVtx[2].ty = fTexBlt.bottom / iTexSizeY;
			BltData.vtVtx[3].tx = fTexBlt.left / iTexSizeX; BltData.vtVtx[3].ty = fTexBlt.bottom / iTexSizeY;

			C4TexRef * pBaseTex = pTex;
			// is there a base-surface to be blitted first?
			if (fBaseSfc)
			{
				// then get this surface as same offset as from other surface
				// assuming this is only valid as long as there's no texture management,
				// organizing partially used textures together!
				pBaseTex = *(sfcSource->pMainSfc->ppTex + iY * sfcSource->iTexX + iX);
			}
			// base blit
			PerformBlt(BltData, pBaseTex, BlitModulated ? BlitModulateClr : 0xffffffff, !!(dwBlitMode & C4GFXBLIT_MOD2), fExact);
			// overlay
			if (fBaseSfc)
			{
				DWORD dwModClr = sfcSource->ClrByOwnerClr;
				// apply global modulation to overlay surfaces only if desired
				if (BlitModulated && !(dwBlitMode & C4GFXBLIT_CLRSFC_OWNCLR))
					ModulateClr(dwModClr, BlitModulateClr);
				PerformBlt(BltData, pTex, dwModClr, !!(dwBlitMode & C4GFXBLIT_CLRSFC_MOD2), fExact);
			}
		}
	}
	// reset texture
	ResetTexture();
	// success
	return true;
}

bool C4Draw::RenderMesh(StdMeshInstance &instance, C4Surface * sfcTarget, float tx, float ty, float twdt, float thgt, DWORD dwPlayerColor, C4BltTransform* pTransform)
{
	// TODO: Emulate rendering
	if (!sfcTarget->IsRenderTarget()) return false;

	// TODO: Clip

	// prepare rendering to surface
	if (!PrepareRendering(sfcTarget)) return false;
	// Update bone matrices and vertex data (note this also updates attach transforms and child transforms)
	instance.UpdateBoneTransforms();
	// Order faces according to MeshTransformation (note pTransform does not affect Z coordinate, so does not need to be taken into account for correct ordering)
	StdMeshMatrix mat = OgreToClonk;
	if(MeshTransform) mat = *MeshTransform * mat;
	instance.ReorderFaces(&mat);
	// Render mesh
	PerformMesh(instance, tx, ty, twdt, thgt, dwPlayerColor, pTransform);
	// success
	return true;
}

bool C4Draw::Blit8(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
                      C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
                      bool fSrcColKey, const C4BltTransform *pTransform)
{
	if (!pTransform) return BlitSimple(sfcSource, fx, fy, fwdt, fhgt, sfcTarget, tx, ty, twdt, thgt, fSrcColKey!=false);
	// safety
	if (!fwdt || !fhgt) return true;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
		{ sfcSource->Unlock(); return false; }
	// transformed, emulated blit
	// Calculate transform target rect
	C4BltTransform Transform;
	Transform.SetMoveScale(tx-(float)fx*twdt/fwdt, ty-(float)fy*thgt/fhgt, (float) twdt/fwdt, (float) thgt/fhgt);
	Transform *=* pTransform;
	C4BltTransform TransformBack;
	TransformBack.SetAsInv(Transform);
	float ttx0=(float)tx, tty0=(float)ty, ttx1=(float)(tx+twdt), tty1=(float)(ty+thgt);
	float ttx2=(float)ttx0, tty2=(float)tty1, ttx3=(float)ttx1, tty3=(float)tty0;
	pTransform->TransformPoint(ttx0, tty0);
	pTransform->TransformPoint(ttx1, tty1);
	pTransform->TransformPoint(ttx2, tty2);
	pTransform->TransformPoint(ttx3, tty3);
	int ttxMin = Max<int>((int)floor(Min(Min(ttx0, ttx1), Min(ttx2, ttx3))), 0);
	int ttxMax = Min<int>((int)ceil(Max(Max(ttx0, ttx1), Max(ttx2, ttx3))), sfcTarget->Wdt);
	int ttyMin = Max<int>((int)floor(Min(Min(tty0, tty1), Min(tty2, tty3))), 0);
	int ttyMax = Min<int>((int)ceil(Max(Max(tty0, tty1), Max(tty2, tty3))), sfcTarget->Hgt);
	// blit within target rect
	for (int y = ttyMin; y < ttyMax; ++y)
		for (int x = ttxMin; x < ttxMax; ++x)
		{
			float ffx=(float)x, ffy=(float)y;
			TransformBack.TransformPoint(ffx, ffy);
			int ifx=static_cast<int>(ffx), ify=static_cast<int>(ffy);
			if (ifx<fx || ify<fy || ifx>=fx+fwdt || ify>=fy+fhgt) continue;
			sfcTarget->BltPix(x,y, sfcSource, ifx,ify, !!fSrcColKey);
		}
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}

bool C4Draw::BlitSimple(C4Surface * sfcSource, int fx, int fy, int fwdt, int fhgt,
                           C4Surface * sfcTarget, int tx, int ty, int twdt, int thgt,
                           bool fTransparency)
{
	// rendertarget?
	if (sfcTarget->IsRenderTarget())
	{
		return Blit(sfcSource, float(fx), float(fy), float(fwdt), float(fhgt), sfcTarget, float(tx), float(ty), float(twdt), float(thgt), true);
	}
	// Object is first stretched to dest rect
	int xcnt,ycnt,tcx,tcy,cpcx,cpcy;
	if (!fwdt || !fhgt || !twdt || !thgt) return false;
	// Lock the surfaces
	if (!sfcSource->Lock())
		return false;
	if (!sfcTarget->Lock())
		{ sfcSource->Unlock(); return false; }
	// Rectangle centers
	tcx=twdt/2; tcy=thgt/2;
	for (ycnt=0; ycnt<thgt; ycnt++)
		if (Inside(cpcy=ty+tcy-thgt/2+ycnt,0,sfcTarget->Hgt-1))
			for (xcnt=0; xcnt<twdt; xcnt++)
				if (Inside(cpcx=tx+tcx-twdt/2+xcnt,0,sfcTarget->Wdt-1))
					sfcTarget->BltPix(cpcx, cpcy, sfcSource, xcnt*fwdt/twdt+fx, ycnt*fhgt/thgt+fy, fTransparency);
	// Unlock the surfaces
	sfcSource->Unlock();
	sfcTarget->Unlock();
	return true;
}


bool C4Draw::Error(const char *szMsg)
{
	if (pApp) pApp->Error(szMsg);
	Log(szMsg); return false;
}


bool C4Draw::CreatePrimaryClipper(unsigned int iXRes, unsigned int iYRes)
{
	// simply setup primary viewport
	// assume no zoom has been set yet
	assert(Zoom==1.0f);
	SetPrimaryClipper(0, 0, iXRes - 1, iYRes - 1);
	StorePrimaryClipper();
	return true;
}

bool C4Draw::BlitSurface(C4Surface * sfcSurface, C4Surface * sfcTarget, int tx, int ty, bool fBlitBase)
{
	if (fBlitBase)
	{
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, float(tx), float(ty), float(sfcSurface->Wdt), float(sfcSurface->Hgt), false);
		return true;
	}
	else
	{
		if (!sfcSurface) return false;
		C4Surface *pSfcBase = sfcSurface->pMainSfc;
		sfcSurface->pMainSfc = NULL;
		Blit(sfcSurface, 0.0f, 0.0f, (float)sfcSurface->Wdt, (float)sfcSurface->Hgt, sfcTarget, float(tx), float(ty), float(sfcSurface->Wdt), float(sfcSurface->Hgt), false);
		sfcSurface->pMainSfc = pSfcBase;
		return true;
	}
}

bool C4Draw::BlitSurfaceTile(C4Surface * sfcSurface, C4Surface * sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, bool fSrcColKey)
{
	int iSourceWdt,iSourceHgt,iX,iY,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// Get source surface size
	if (!GetSurfaceSize(sfcSurface,iSourceWdt,iSourceHgt)) return false;
	// reduce offset to needed size
	iOffsetX %= iSourceWdt;
	iOffsetY %= iSourceHgt;
	// Vertical blits
	for (iY=iToY+iOffsetY; iY<iToY+iToHgt; iY+=iSourceHgt)
	{
		// Vertical blit size
		iBlitY=Max(iToY-iY,0); iBlitHgt=Min(iSourceHgt,iToY+iToHgt-iY)-iBlitY;
		// Horizontal blits
		for (iX=iToX+iOffsetX; iX<iToX+iToWdt; iX+=iSourceWdt)
		{
			// Horizontal blit size
			iBlitX=Max(iToX-iX,0); iBlitWdt=Min(iSourceWdt,iToX+iToWdt-iX)-iBlitX;
			// Blit
			if (!Blit(sfcSurface,float(iBlitX),float(iBlitY),float(iBlitWdt),float(iBlitHgt),sfcTarget,float(iX+iBlitX),float(iY+iBlitY),float(iBlitWdt),float(iBlitHgt),fSrcColKey)) return false;
		}
	}
	return true;
}

bool C4Draw::BlitSurfaceTile2(C4Surface * sfcSurface, C4Surface * sfcTarget, int iToX, int iToY, int iToWdt, int iToHgt, int iOffsetX, int iOffsetY, bool fSrcColKey)
{
	// if it's a render target, simply blit with repeating texture
	// repeating textures, however, aren't currently supported
	/*if (sfcTarget->IsRenderTarget())
	  return Blit(sfcSurface, iOffsetX, iOffsetY, iToWdt, iToHgt, sfcTarget, iToX, iToY, iToWdt, iToHgt, false);*/
	int tx,ty,iBlitX,iBlitY,iBlitWdt,iBlitHgt;
	// get tile size
	int iTileWdt=sfcSurface->Wdt;
	int iTileHgt=sfcSurface->Hgt;
	// adjust size of offsets
	iOffsetX%=iTileWdt;
	iOffsetY%=iTileHgt;
	if (iOffsetX<0) iOffsetX+=iTileWdt;
	if (iOffsetY<0) iOffsetY+=iTileHgt;
	// get start pos for blitting
	int iStartX=iToX-iOffsetX;
	int iStartY=iToY-iOffsetY;
	ty=0;
	// blit vertical
	for (int iY=iStartY; ty<iToHgt; iY+=iTileHgt)
	{
		// get vertical blit bounds
		iBlitY=0; iBlitHgt=iTileHgt;
		if (iY<iToY) { iBlitY=iToY-iY; iBlitHgt+=iY-iToY; }
		int iOver=ty+iBlitHgt-iToHgt; if (iOver>0) iBlitHgt-=iOver;
		// blit horizontal
		tx=0;
		for (int iX=iStartX; tx<iToWdt; iX+=iTileWdt)
		{
			// get horizontal blit bounds
			iBlitX=0; iBlitWdt=iTileWdt;
			if (iX<iToX) { iBlitX=iToX-iX; iBlitWdt+=iX-iToX; }
			iOver=tx+iBlitWdt-iToWdt; if (iOver>0) iBlitWdt-=iOver;
			// blit
			if (!Blit(sfcSurface,float(iBlitX),float(iBlitY),float(iBlitWdt),float(iBlitHgt),sfcTarget,float(tx+iToX),float(ty+iToY),float(iBlitWdt),float(iBlitHgt),fSrcColKey))
			{
				// Ignore blit errors. This is usually due to blit border lying outside surface and shouldn't cause remaining blits to fail.
			}
			// next col
			tx+=iBlitWdt;
		}
		// next line
		ty+=iBlitHgt;
	}
	// success
	return true;
}

bool C4Draw::TextOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
{
	C4Markup Markup(true);
	static char szLinebuf[2500+1];
	for (int cnt=0; SCopySegmentEx(szText,cnt,szLinebuf,fDoMarkup ? '|' : '\n','\n',2500); cnt++,iTy+=int(fZoom*rFont.GetLineHeight()))
		if (!StringOut(szLinebuf,sfcDest,iTx,iTy,dwFCol,byForm,fDoMarkup,Markup,&rFont,fZoom)) return false;
	return true;
}

bool C4Draw::StringOut(const char *szText, CStdFont &rFont, float fZoom, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup)
{
	// init markup
	C4Markup Markup(true);
	// output string
	return StringOut(szText, sfcDest, iTx, iTy, dwFCol, byForm, fDoMarkup, Markup, &rFont, fZoom);
}

bool C4Draw::StringOut(const char *szText, C4Surface * sfcDest, float iTx, float iTy, DWORD dwFCol, BYTE byForm, bool fDoMarkup, C4Markup &Markup, CStdFont *pFont, float fZoom)
{
	// clip
	if (ClipAll) return true;
	// safety
	if (!PrepareRendering(sfcDest)) return false;
	// convert align
	int iFlags=0;
	switch (byForm)
	{
	case ACenter: iFlags |= STDFONT_CENTERED; break;
	case ARight: iFlags |= STDFONT_RIGHTALGN; break;
	}
	if (!fDoMarkup) iFlags|=STDFONT_NOMARKUP;
	// draw text
	pFont->DrawText(sfcDest, iTx  , iTy  , dwFCol, szText, iFlags, Markup, fZoom);
	// done, success
	return true;
}

void C4Draw::DrawPix(C4Surface * sfcDest, float tx, float ty, DWORD dwClr)
{
	ApplyZoom(tx, ty);
	// FIXME: zoom to a box
	// manual clipping?
	if (Config.Graphics.ClipManuallyE)
	{
		if (tx < iClipX1) { return; }
		if (ty < iClipY1) { return; }
		if (iClipX2 < tx) { return; }
		if (iClipY2 < ty) { return; }
	}
	// apply global modulation
	ClrByCurrentBlitMod(dwClr);
	// apply modulation map
	if (fUseClrModMap)
		ModulateClr(dwClr, pClrModMap->GetModAt((int)tx, (int)ty));
	// Draw
	PerformPix(sfcDest, tx, ty, dwClr);
}

void C4Draw::DrawLineDw(C4Surface * sfcTarget, float x1, float y1, float x2, float y2, DWORD dwClr, float width)
{
	ApplyZoom(x1, y1);
	ApplyZoom(x2, y2);
	// manual clipping?
	if (Config.Graphics.ClipManuallyE)
	{
		float i;
		// sort left/right
		if (x1>x2) { i=x1; x1=x2; x2=i; i=y1; y1=y2; y2=i; }
		// clip horizontally
		if (x1 < iClipX1)
			if (x2 < iClipX1)
				return; // left out
			else
				{ y1+=(y2-y1)*((float)iClipX1-x1)/(x2-x1); x1=(float)iClipX1; } // clip left
		else if (x2 > iClipX2)
		{
			if (x1 > iClipX2)
				return; // right out
			else
				{ y2-=(y2-y1)*(x2-(float)iClipX2)/(x2-x1); x2=(float)iClipX2; } // clip right
		}
		// sort top/bottom
		if (y1>y2) { i=x1; x1=x2; x2=i; i=y1; y1=y2; y2=i; }
		// clip vertically
		if (y1 < iClipY1)
		{
			if (y2 < iClipY1)
				return; // top out
			else
				{ x1+=(x2-x1)*((float)iClipY1-y1)/(y2-y1); y1=(float)iClipY1; } // clip top
		}
		else if (y2 > iClipY2)
		{
			if (y1 > iClipY2)
				return; // bottom out
			else
				{ x2-=(x2-x1)*(y2-(float)iClipY2)/(y2-y1); y2=(float)iClipY2; } // clip bottom
		}
	}
	// apply color modulation
	ClrByCurrentBlitMod(dwClr);

	PerformLine(sfcTarget, x1, y1, x2, y2, dwClr, width);
}

void C4Draw::DrawFrameDw(C4Surface * sfcDest, int x1, int y1, int x2, int y2, DWORD dwClr) // make these parameters float...?
{
	DrawLineDw(sfcDest,(float)x1,(float)y1,(float)x2,(float)y1, dwClr);
	DrawLineDw(sfcDest,(float)x2,(float)y1,(float)x2,(float)y2, dwClr);
	DrawLineDw(sfcDest,(float)x2,(float)y2,(float)x1,(float)y2, dwClr);
	DrawLineDw(sfcDest,(float)x1,(float)y2,(float)x1,(float)y1, dwClr);
}

// Globally locked surface variables - for DrawLine callback crap

C4Surface *GLSBuffer=NULL;

bool LockSurfaceGlobal(C4Surface * sfcTarget)
{
	if (GLSBuffer) return false;
	GLSBuffer=sfcTarget;
	return !!sfcTarget->Lock();
}

bool UnLockSurfaceGlobal(C4Surface * sfcTarget)
{
	if (!GLSBuffer) return false;
	sfcTarget->Unlock();
	GLSBuffer=NULL;
	return true;
}

bool DLineSPixDw(int32_t x, int32_t y, int32_t dwClr)
{
	if (!GLSBuffer) return false;
	GLSBuffer->SetPixDw(x,y,(DWORD) dwClr);
	return true;
}

void C4Draw::DrawPatternedCircle(C4Surface * sfcDest, int x, int y, int r, BYTE col, C4Pattern & Pattern, CStdPalette &rPal)
{
	if (!sfcDest->Lock()) return;
	for (int ycnt = -r; ycnt < r; ycnt++)
	{
		int lwdt = (int) sqrt(float(r * r - ycnt * ycnt));
		// Set line
		for (int xcnt = x - lwdt; xcnt < x + lwdt; ++xcnt)
		{
			sfcDest->SetPixDw(xcnt, y + ycnt, Pattern.PatternClr(xcnt, y + ycnt));
		}
	}
	sfcDest->Unlock();
}

void C4Draw::Grayscale(C4Surface * sfcSfc, int32_t iOffset)
{
	// safety
	if (!sfcSfc) return;
	// change colors
	int xcnt,ycnt,wdt=sfcSfc->Wdt,hgt=sfcSfc->Hgt;
	// Lock surface
	if (!sfcSfc->Lock()) return;
	for (ycnt=0; ycnt<hgt; ycnt++)
	{
		for (xcnt=0; xcnt<wdt; xcnt++)
		{
			DWORD dwColor = sfcSfc->GetPixDw(xcnt,ycnt,false);
			uint32_t r = GetRedValue(dwColor), g = GetGreenValue(dwColor), b = GetBlueValue(dwColor), a = dwColor >> 24;
			int32_t gray = BoundBy<int32_t>((r + g + b) / 3 + iOffset, 0, 255);
			sfcSfc->SetPixDw(xcnt, ycnt, RGBA(gray, gray, gray, a));
		}
	}
	sfcSfc->Unlock();
}

bool C4Draw::GetPrimaryClipper(int &rX1, int &rY1, int &rX2, int &rY2)
{
	// Store drawing clip values
	rX1=fClipX1; rY1=fClipY1; rX2=fClipX2; rY2=fClipY2;
	// Done
	return true;
}

void C4Draw::SetGamma(DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, int32_t iRampIndex)
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (iRampIndex < 0 || iRampIndex >= C4MaxGammaRamps) return;
	// turn ramp index into array offset
	iRampIndex*=3;
	// set array members
	dwGamma[iRampIndex+0]=dwClr1;
	dwGamma[iRampIndex+1]=dwClr2;
	dwGamma[iRampIndex+2]=dwClr3;
	// mark gamma ramp to be recalculated
	fSetGamma=true;
}

void C4Draw::ResetGamma()
{
	pApp->ApplyGammaRamp(DefRamp.ramp, false);
}

void C4Draw::ApplyGamma()
{
	// No gamma effects
	if (Config.Graphics.DisableGamma) return;
	if (!fSetGamma) return;

	//  calculate color channels by adding the difference between the gamma ramps to their normals
	int32_t ChanOff[3];
	DWORD tGamma[3];
	const int32_t DefChanVal[3] = { 0x00, 0x80, 0xff };
	// calc offset for curve points
	for (int32_t iCurve=0; iCurve<3; ++iCurve)
	{
		memset(ChanOff, 0, sizeof(int32_t)*3);
		// ...channels...
		for (int32_t iChan=0; iChan<3; ++iChan)
			// ...ramps...
			for (int32_t iRamp=0; iRamp<C4MaxGammaRamps; ++iRamp)
				// add offset
				ChanOff[iChan]+=(int32_t) BYTE(dwGamma[iRamp*3+iCurve]>>(16-iChan*8)) - DefChanVal[iCurve];
		// calc curve point
		tGamma[iCurve]=C4RGB(BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[0], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[1], 0, 255), BoundBy<int32_t>(DefChanVal[iCurve]+ChanOff[2], 0, 255));
	}
	// calc ramp
	Gamma.Set(tGamma[0], tGamma[1], tGamma[2]);
	// set gamma
	pApp->ApplyGammaRamp(Gamma.ramp, false);
	fSetGamma=false;
}

void C4Draw::DisableGamma()
{
	// set it
	pApp->ApplyGammaRamp(DefRamp.ramp, true);
}

void C4Draw::EnableGamma()
{
	// set it
	pApp->ApplyGammaRamp(Gamma.ramp, false);
}

DWORD C4Draw::ApplyGammaTo(DWORD dwClr)
{
	return Gamma.ApplyTo(dwClr);
}

void C4Draw::SetZoom(int X, int Y, float Zoom)
{
	this->ZoomX = X; this->ZoomY = Y; this->Zoom = Zoom;
}

void C4Draw::ApplyZoom(float & X, float & Y)
{
	X = (X - ZoomX) * Zoom + ZoomX;
	Y = (Y - ZoomY) * Zoom + ZoomY;
}

void C4Draw::RemoveZoom(float & X, float & Y)
{
	X = (X - ZoomX) / Zoom + ZoomX;
	Y = (Y - ZoomY) / Zoom + ZoomY;
}

bool DDrawInit(C4AbstractApp * pApp, bool Editor, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor)
{
	// create engine
    #ifndef USE_CONSOLE
	  pDraw = new CStdGL();
    #else
	  pDraw = new CStdNoGfx();
    #endif
	if (!pDraw) return false;
	// init it
	if (!pDraw->Init(pApp, Editor, fUsePageLock, iXRes, iYRes, iBitDepth, iMonitor))
	{
		delete pDraw;
		return false;
	}
	// done, success
	return true;
}

bool C4Draw::Init(C4AbstractApp * pApp, bool Editor, bool fUsePageLock, unsigned int iXRes, unsigned int iYRes, int iBitDepth, unsigned int iMonitor)
{
	this->pApp = pApp;

	// store default gamma
	if (!pApp->SaveDefaultGammaRamp(DefRamp.ramp))
		DefRamp.Default();

	pApp->pWindow->pSurface = new C4Surface(pApp, pApp->pWindow);

	if (!CreatePrimarySurfaces(Editor, iXRes, iYRes, iBitDepth, iMonitor))
		return false;

	if (!CreatePrimaryClipper(iXRes, iYRes))
		return Error("  Clipper failure.");

	this->Editor = Editor;

	return true;
}

void C4Draw::DrawBoxFade(C4Surface * sfcDest, float iX, float iY, float iWdt, float iHgt, DWORD dwClr1, DWORD dwClr2, DWORD dwClr3, DWORD dwClr4, int iBoxOffX, int iBoxOffY)
{
	ApplyZoom(iX, iY);
	iWdt *= Zoom;
	iHgt *= Zoom;
	// clipping not performed - this fn should be called for clipped rects only
	// apply modulation map: Must sectionize blit
	if (fUseClrModMap)
	{
		int iModResX = pClrModMap ? pClrModMap->GetResolutionX() : C4FogOfWar::DefResolutionX;
		int iModResY = pClrModMap ? pClrModMap->GetResolutionY() : C4FogOfWar::DefResolutionY;
		iBoxOffX %= iModResX;
		iBoxOffY %= iModResY;
		if (iWdt+iBoxOffX > iModResX || iHgt+iBoxOffY > iModResY)
		{
			if (iWdt<=0 || iHgt<=0) return;
			CColorFadeMatrix clrs(int(iX), int(iY), int(iWdt), int(iHgt), dwClr1, dwClr2, dwClr3, dwClr4);
			float iMaxH = float(iModResY - iBoxOffY);
			float w,h;
			for (float y = iY, H = iHgt; H > 0; (y += h), (H -= h), (iMaxH = float(iModResY)))
			{
				h = Min(H, iMaxH);
				float iMaxW = float(iModResX - iBoxOffX);
				for (float x = iX, W = iWdt; W > 0; (x += w), (W -= w), (iMaxW = float(iModResX)))
				{
					w = Min(W, iMaxW);
					//DrawBoxFade(sfcDest, x,y,w,h, clrs.GetColorAt(x,y), clrs.GetColorAt(x+w,y), clrs.GetColorAt(x,y+h), clrs.GetColorAt(x+w,y+h), 0,0);
					float vtx[8];
					vtx[0] = x     ; vtx[1] = y;
					vtx[2] = x     ; vtx[3] = y+h;
					vtx[4] = x+w; vtx[5] = y+h;
					vtx[6] = x+w; vtx[7] = y;
					DrawQuadDw(sfcDest, vtx, clrs.GetColorAt(int(x),int(y)), clrs.GetColorAt(int(x),int(y+h)), clrs.GetColorAt(int(x+w),int(y+h)), clrs.GetColorAt(int(x+w),int(y)));
				}
			}
			return;
		}
	}
	// set vertex buffer data
	// vertex order:
	// 0=upper left   dwClr1
	// 1=lower left   dwClr3
	// 2=lower right  dwClr4
	// 3=upper right  dwClr2
	float vtx[8];
	vtx[0] = iX     ; vtx[1] = iY;
	vtx[2] = iX     ; vtx[3] = iY+iHgt;
	vtx[4] = iX+iWdt; vtx[5] = iY+iHgt;
	vtx[6] = iX+iWdt; vtx[7] = iY;
	DrawQuadDw(sfcDest, vtx, dwClr1, dwClr3, dwClr4, dwClr2);
}

void C4Draw::DrawBoxDw(C4Surface * sfcDest, int iX1, int iY1, int iX2, int iY2, DWORD dwClr)
{
	DrawBoxFade(sfcDest, float(iX1), float(iY1), float(iX2-iX1+1), float(iY2-iY1+1), dwClr, dwClr, dwClr, dwClr, 0,0);
}
