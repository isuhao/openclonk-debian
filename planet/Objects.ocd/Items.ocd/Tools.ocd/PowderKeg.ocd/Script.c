/*--
	Powder Keg
	Author: Ringwaul

	A barrel filled with black powder.
--*/

#include Library_CarryHeavy

local count;
local oldcount;

public func GetCarryTransform(clonk)
{
	if(GetCarrySpecial(clonk))
		return Trans_Translate(0, 1000, -6500);
		
	return Trans_Mul(Trans_Translate(-1500,1500,0),Trans_Rotate(180,0,1,0));
}
public func GetCarryPhase() { return 900; }

protected func Initialize()
{
	UpdatePicture();
}

protected func Construction()
{
	oldcount = count;
	count = 12;
	AddEffect("Update",this,1,1,this);
}

protected func MaxContentsCount() {	return 12;	}

func PowderCount()
{
	return count;
}

func SetPowderCount(int newcount)
{
	count = newcount;
}

public func FxUpdateTimer(object target, effect, int timer)
{
	if(count != oldcount)
		UpdatePicture();
	if(count == 0)
	{
		ChangeDef(Barrel);
		return -1;
	}
	oldcount = count;
	return 1;
}

private func UpdatePicture()
{
	//modified script from Stackable.ocd
	var one = count % 10;
	var ten = (count / 10) % 10;
	
	var s = 400;
	var yoffs = 14000;
	var xoffs = 22000;
	var spacing = 14000;
	
	if (ten > 0)
	{
		SetGraphics(Format("%d", ten), Icon_Number, 11, GFXOV_MODE_Picture);
		SetObjDrawTransform(s, 0, xoffs - spacing, 0, s, yoffs, 11);
	}
	else
		SetGraphics(nil, nil, 11);
		
	SetGraphics(Format("%d", one), Icon_Number, 12, GFXOV_MODE_Picture);
	SetObjDrawTransform(s, 0, xoffs, 0, s, yoffs, 12);
}

public func Incineration()
{
	AddEffect("Fuse",this,1,1,this);
}

public func FxFuseTimer(object target, effect, int timer)
{
	CreateParticle("Fire", 0, 0, PV_Random(-10, 10), PV_Random(-20, 10), PV_Random(10, 40), Particles_Glimmer(), 6);
	if(timer > 90)
	{
		//17-32 explosion radius
		var radius = Sqrt(64 * (4 + count));
		Explode(radius);
	}
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func Damage()
{
	Incinerate();
}

public func OnProjectileHit()
{
	Incinerate();
}

func Hit()
{
	Sound("DullWoodHit?");
}

public func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	var v = PowderCount();
	if (v != 12) props->AddCall("Powder", this, "SetPowderCount", v);
	return true;
}

func IsChemicalProduct() { return true; }
func AlchemyProcessTime() { return 100; }

local Collectible = false;
local Touchable = 2;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
local BlastIncinerate = 1;
local NoBurnDecay = 1;
local ContactIncinerate = 2;
