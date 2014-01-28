/*
	Arrow
	Author: Newton
	
	The arrow is the standard ammo for the bow and base object for all other
	types of arrows. This object is stackable (up to 15) as it is required
	from the bow.
	The arrow employs the HitCheck effect (in System.ocg/HitChecks.c) originally
	from Hazard to search for targets during flight.
*/

#include Library_Stackable

public func IsArrow() { return true; }
public func MaxStackCount() { return 15; }
public func ArrowStrength() { return 10; }

protected func Construction()
{
	SetR(90);
	return _inherited(...);
}

public func Launch(int angle, int str, object shooter)
{
	//SetGraphics(0, HelpArrow);
	SetShape(-2, -2, 4, 11);
	SetVertex(0, VTX_Y, 3, 1);
	SetVertex(1, VTX_Y, 4, 1);
	SetVertex(2, VTX_Y, -2, 1);
	SetPosition(GetX(), GetY() - 2);
	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("ArrowShoot?");
	// Shooter controls the arrow.
	SetController(shooter->GetController());
	
	AddEffect("HitCheck", this, 1,1, nil,nil, shooter);
	AddEffect("InFlight", this, 1,1, this);
}

private func Stick()
{
	if(GetEffect("InFlight",this))
	{
		RemoveEffect("HitCheck",this);
		RemoveEffect("InFlight",this);
	
		SetXDir(0);
		SetYDir(0);
		SetRDir(0);
	
		var x=Sin(GetR(),+9);
		var y=Cos(GetR(),-9);
		var mat = GetMaterial(x,y);
		if(mat != -1)
		{
			// sticks in any material now
			//if(GetMaterialVal("DigFree","Material",mat))
			//{
			// stick in landscape
			SetVertex(2,VTX_Y,-10,1);
			//}
		}
	}
}

public func HitObject(object obj)
{
	var relx = GetXDir() - obj->GetXDir();
	var rely = GetYDir() - obj->GetYDir();
	var speed = Sqrt(relx*relx+rely*rely);
	var dmg = ArrowStrength()*speed/100;
	ProjectileHit(obj,dmg,ProjectileHit_tumble);
	// Stick does something unwanted to controller.
	if (this) Stick();
}

// called by successful hit of object after from ProjectileHit(...)
public func OnStrike(object obj)
{
	if(obj->GetAlive())
		Sound("ProjectileHitLiving?");
	else
		Sound("ArrowHitGround");
}

public func Hit()
{
	if(GetEffect("InFlight",this))
		Sound("ArrowHitGround");
	Stick();
}

// rotate arrow according to speed
public func FxInFlightStart(object target, effect, int temp)
{
	if(temp) return;
	effect.x = target->GetX();
	effect.y = target->GetY();
}
public func FxInFlightTimer(object target, effect, int time)
{
	var oldx = effect.x;
	var oldy = effect.y;
	var newx = GetX();
	var newy = GetY();
	
	// and additionally, we need one check: If the arrow has no speed
	// anymore but is still in flight, we'll remove the hit check
	if(oldx == newx && oldy == newy)
	{
		// but we give the arrow 5 frames to speed up again
		effect.var2++;
		if(effect.var2 >= 10)
			return Hit();
	}
	else
		effect.var2 = 0;

	// rotate arrow according to speed
	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	SetRDir(anglediff/2);
	effect.x = newx;
	effect.y = newy;

}

func UpdatePicture()
{
	var count = GetStackCount();
	if(count >= 9) SetGraphics(nil);
	else SetGraphics(Format("%d",count));
	_inherited(...);
}

func Entrance()
{
	// reset sticky-vertex
	SetVertex(2,VTX_Y,0,1);
}

func RejectEntrance()
{
	// not while flying
	if(GetEffect("InFlight",this)) return true;

	return _inherited(...);
}

public func IsArmoryProduct() { return true; }

local Name = "$Name$";
local Description = "$Description$";
local Collectible = 1;
