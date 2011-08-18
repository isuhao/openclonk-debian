/*-- Target Balloon --*/

local ysin;

func Definition(def) {
	SetProperty("Name", "$Name$", def);
}

protected func Initialize()
{
	ysin = 0;
	SetAction("Float");
	AddEffect("Float",this,1,1,this);
}

func FxFloatTimer(object target, int num, int time)
{
	if(ysin >= 360) ysin = 0;
	if(ysin <= 360)
	{
	++ysin;
	}
	target->SetYDir(Sin(ysin,2));
}

public func IsProjectileTarget(target,shooter)
{
	return 1;
}

public func OnProjectileHit()
{
	CastParticles("Air",20,5,0,-10,170,190,RGB(255,255,255),RGB(255,255,255));
	RemoveObject();
}

func FxFlyOffTimer(target, num, time)
{
	RemoveEffect("Float", this);
	RemoveEffect("HorizontalMoving", this);
	if(GetYDir()>-30)
	{
		SetYDir(GetYDir()-1);
	}

	if(GetY()<0)
	{
		RemoveObject();
		return -1;
	}
}

func Definition(def) {
	SetProperty("ActMap", {

Float = {
	Prototype = Action,
	Name = "Float",
	Procedure = DFA_FLOAT,
	Directions = 1,
	FlipDir = 0,
	Length = 1,
	Delay = 1,
	X = 0,
	Y = 0,
	Wdt = 64,
	Hgt = 64,
	NextAction = "Float",
},
}, def);}