/*-- Dynamite box --*/

static const DYNA_MaxLength = 500;
static const DYNA_MaxCount  = 5;

public func Initialize()
{
	iCount = DYNA_MaxCount;
	aDynamites = CreateArray(iCount);
	aWires = CreateArray(iCount);
	for(var i = 0; i < iCount; i++)
	{
		aDynamites[i] = nil;
		aWires[i] = nil;
	}

	this.PictureTransformation = Trans_Scale(); // Hide it TODO: Remove if the mesh isn't shown if there is a picture set
	UpdatePicture();
}

private func Hit()
{
	Sound("DullWoodHit?");
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 450; }

local iCount;
local aDynamites;
local aWires;

local pWire;

public func ControlUse(object clonk, int x, int y)
{
	var pDyna = aDynamites[iCount-1] = CreateContents(Dynamite);
	if(!pDyna->ControlUse(clonk, x, y, 1))
	{
		pDyna->RemoveObject();
		return true;
	}
	if(pWire)
		pWire->Connect(aDynamites[iCount], pDyna);
	// First? then add Timer
/*	else
		AddEffect("IntLength", this, 1, 10, this);*/

	pWire = CreateObject(Fuse);
	pWire->Connect(pDyna, this);
	Sound("Connect");
	aWires[iCount-1] = pWire;
	
	iCount--;

	UpdatePicture();
	
//	Message("%d left", iCount);

	if(iCount == 0)
	{
		var pos = clonk->GetItemPos(this);
		ChangeDef(Igniter);
		SetGraphics("0", Fuse, 1, GFXOV_MODE_Picture);
		clonk->UpdateAttach();
		clonk->OnSlotFull(pos);
	}

	return true;
}

private func UpdatePicture()
{
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;

	SetGraphics(Format("%d", iCount), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);

	SetGraphics(Format("%d", iCount), Fuse, 1, GFXOV_MODE_Picture);
}

local fWarning;
local fWarningColor;

func FxIntLengthTimer(pTarget, effect, iTime)
{
	var iLength = 0;
	var i = GetLength(aWires)-1;
	while(i >= 0 && aWires[i])
	{
		iLength += aWires[i]->GetLineLength();
		i--;
	}
	if(iLength > DYNA_MaxLength*4/5)
	{
		fWarning = 1;
//		Message("Line too long! %d%%", iLength*100/DYNA_MaxLength);
		if( (iTime % 5) == 0)
		{
			var fOn = 1;
			if( fWarningColor == 1) fOn = 0;
			fWarningColor = fOn;
//			for(var i = 0; i < GetLength(aWires); i++)
//				if(aWires[i]) aWires[i]->SetColorWarning(fOn);
		}
	}
	else if(fWarning)
	{
		fWarning = 0;
//		for(var i = 0; i < GetLength(aWires); i++)
//			if(aWires[i]) aWires[i]->SetColorWarning(0);
	}
	if(iLength > DYNA_MaxLength)
	{
		var iMax = GetLength(aWires)-1;
		aWires[iMax]->RemoveObject();
		aDynamites[iMax]->Reset();
		SetLength(aWires, iMax);
		SetLength(aDynamites, iMax);
//		Message("Line too long,|lost dynamite!|%d left.", iMax);
		if(iMax == 0)
		{
			Contained()->Message("Line broken.");
			RemoveObject();
		}
	}
}

public func OnFuseFinished()
{
	DoExplode();
}

public func DoExplode()
{
	// Activate all fuses
	for(var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
		obj->~StartFusing(this);
	// Explode, calc the radius out of the area of a explosion of a single dynamite times the amount of dynamite
	// This results to 18, 25, 31, 36, and 40
	Explode(Sqrt(18**2*iCount));
}

protected func Incineration() { DoExplode(); }

protected func Damage() { DoExplode(); }

func FxIntLengthStop(pTarget, effect, iReason, fTmp)
{
	for(var i = 0; i < GetLength(aWires); i++)
			if(aWires[i]) aWires[i]->SetColorWarning(0);
}

public func IsTool() { return true; }
public func IsChemicalProduct() { return true; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(150, 1, 0, 0), Trans_Rotate(140, 0, 1, 0)), def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Rebuy = true;
