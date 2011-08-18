/*-- Shovel --*/

private func Hit()
{
	Sound("WoodHit");
}

public func GetCarryMode(clonk) { return CARRY_Back; }

public func GetCarrySpecial(clonk) { if(clonk->~GetAction() == "Dig") return "pos_hand1"; }

local fDigging;
local DigAngle;
local DigX, DigY;

public func IsDigging() { return fDigging; }


public func ControlUseStart(object clonk, int x, int y)
{
	AddEffect("ShovelDig",clonk,1,1,this);
//	ControlUseHolding(clonk, x, y);
	return true;
}

public func HoldingEnabled() { return true; }

public func ControlUseHolding(object clonk, int x, int y)
{
	DigX = x;
	DigY = y;
	DigAngle = Angle(0,0,x,y);
	
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	ControlUseStop(clonk, x, y);
}

public func ControlUseStop(object clonk, int x, int y)
{
	fDigging = 0;
	RemoveEffect("ShovelDig",clonk,0);
	if(clonk->GetAction() != "Dig") return true;

//	EffectCall(clonk, GetEffect("IntDig", clonk), "StopDig");
	clonk->SetXDir(0,100);
	clonk->SetYDir(0,100);
//	clonk->SetAction("Walk");
//	clonk->SetComDir(COMD_Stop);

	return true;
}

public func FxShovelDigTimer(object clonk, int num, int time)
{
	var xdir_boost = 0, ydir_boost = 0;
	// Currently not digging?
	if(clonk->GetAction() != "Dig")
	{
		var is_scaling = (clonk->GetProcedure() == "SCALE");
		var can_dig = (clonk->GetAction() == "Walk" || is_scaling || clonk->GetProcedure() == "HANGLE");
		if (can_dig)
		{
			clonk->SetAction("Dig");
			clonk->SetComDir(COMD_None);
			if (is_scaling)
			{
				// speed boost when Clonk started digging from scaling, so we don't drop down
				var clnk_xdir = clonk->GetDir()*2-1;
				if (Abs(DigX) > Abs(DigY) && DigX*clnk_xdir > 0) // only if player actually wants to go sideways in the scaling direction (|x|>|y| and sign(x)==sign(clnk_xdir))
				{
					// not if standing on ground (to prevent speed boost digging) or on ceiling (to make working your way upwards through earth a little harder)
					if (!clonk->GetContact(-1, CNAT_Top|CNAT_Bottom))
					{
						xdir_boost = clnk_xdir*1000;
						ydir_boost = -100;
					}
				}
			}
		}
		else
		{
			if (fDigging)
			{
				fDigging = false;
				RemoveEffect("ShovelDust",clonk,0);
			}
			return true;
		}
	}
	// Dig start procedure
	if(!fDigging && clonk->GetAction() == "Dig")
	{
		AddEffect("ShovelDust",clonk,1,1,this);
		fDigging = true;
	}
	if(fDigging)
	{
		// Adjust speed at current animation position
		var speed = clonk->GetPhysical("Dig")/400;

		var iAnimation = EffectVar(1, clonk, GetEffect("IntDig", clonk));
		var iPosition = clonk->GetAnimationPosition(iAnimation)*180/clonk->GetAnimationLength("Dig");
		speed = speed*(Cos(iPosition-45, 50)**2)/2500;

		// limit angle
		DigAngle = BoundBy(DigAngle,65,300);
		clonk->SetXDir(Sin(DigAngle,+speed)+xdir_boost,100);
		clonk->SetYDir(Cos(DigAngle,-speed)+ydir_boost,100);

		// Dust
		Dust(clonk);
	}
}

public func Dust(object target)
{
	// Only when the clonk moves the shovel
	var iAnimation = EffectVar(1, target, GetEffect("IntDig", target));
	var iPosition = target->GetAnimationPosition(iAnimation)*100/target->GetAnimationLength("Dig");
	if(iPosition > 50)
		return;
	var xdir = target->GetXDir();
	var ydir = target->GetYDir();
	
	// particle effect
	var angle = Angle(0,0,xdir,ydir)+iPosition-25;//RandomX(-25,25);
	var groundx = Sin(angle,15);
	var groundy = -Cos(angle,15);
	var mat = GetMaterial(groundx, groundy);
	var tex = GetTexture(groundx,groundy);
	if(GetMaterialVal("DigFree","Material",mat))
	{
		var clr = GetAverageTextureColor(tex);
		var a = 80;
		CreateParticle("Dust",groundx,groundy,RandomX(-3,3),RandomX(-3,3),RandomX(10,250),DoRGBaValue(clr,-255+a,0));
	}
}

public func IsTool() { return 1; }

public func IsToolProduct() { return 1; }

func Definition(def) {
	SetProperty("PictureTransformation", Trans_Mul(Trans_Rotate(135, 0, 0, 1),Trans_Rotate(30, 0, 1, 0)),def);
}

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
