/*-- Target Balloon --*/

local ysin;

protected func Initialize()
{
	ysin = 0;
	SetAction("Float");
	SetComDir(COMD_None);
	AddEffect("Float",this,1,1,this);
}

func FxFloatTimer(object target, effect, int time)
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
	CreateParticle("Air", 0, -10, PV_Random(-30, 30), PV_Random(-30,30), PV_Random(30, 120), Particles_Air(), 10);
	Sound("BalloonPop");
	RemoveObject();
}

func FxFlyOffTimer(target, effect, time)
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
	SetProperty("Name", "$Name$", def);
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
