/* shrapnel */

public func ProjectileDamage() { return 3; }
public func FlightTime() { return 4; }

protected func Initialize()
{
	SetAction("Flight");
	AddEffect("Fade", this, 1, 1, this);
}

public func Launch(int shooter)
{
	SetController(shooter);
	AddEffect("HitCheck", this, 1,1, nil, nil);
}

protected func FxFadeTimer(object target, int num, int timer)
{
	if(timer > FlightTime()) RemoveObject();
}

protected func Hit()
{
	ShakeFree(6);
	RemoveEffect("HitCheck",this);
	Sound("BulletHitGround?");
	CreateParticle("StarSpark", 0, 0, PV_Random(-20, 20), PV_Random(-20, 20), PV_Random(10, 20), Particles_Glimmer(), 3);
	
	RemoveObject();
}

public func HitObject(object obj)
{
	ProjectileHit(obj,ProjectileDamage(),ProjectileHit_tumble);
	Sound("ProjectileHitLiving?");
	RemoveObject();
}

public func TrailColor(int time)
{
	return RGBa(100,100,100,240*Max(0,FlightTime()-time)/FlightTime());
}

local ActMap = {
	Fly = {
		Prototype = Action,
		Name = "Fly",
		Procedure = DFA_FLIGHT,
		NextAction = "Fly",
		Delay = 1,
		Length = 1,
	},
};