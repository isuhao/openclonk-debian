/*
	Grapple Hook
	Author: Randrian
	
	The hook can be shot with the grappling bow.
	On impact the hook will stick to the ground
	The hook also controls the swinging controls for the clonk
*/

local rope; // The rope is the connection between the hook
local clonk;
local pull;
local grappler;
local fx_hook;

public func ArrowStrength() { return 10; }

public func GetRope() { return rope; }

public func New(object new_clonk, object new_rope)
{
	SetObjDrawTransform(0, 1, 0, 0, 0, 0, 0); // Hide
	clonk = new_clonk;
	rope = new_rope;
}

public func Launch(int angle, int str, object shooter, object bow)
{
	SetObjDrawTransform(0, 1, 0, 0, 0, 0, 0); // Hide
	Exit();

	pull = 0;
		
	// Create rope
	rope = CreateObject(GrappleRope, 0, 0, NO_OWNER);

	rope->Connect(this, bow);
	rope->ConnectLoose();
	clonk = shooter;
	grappler = bow;

	var xdir = Sin(angle,str);
	var ydir = Cos(angle,-str);
	SetXDir(xdir);
	SetYDir(ydir);
	SetR(angle);
	Sound("ArrowShoot?");
	
	AddEffect("InFlight", this, 1, 1, this);
}

public func Destruction()
{
	if(rope)
		rope->HookRemoved();
}

private func Stick()
{
	if (GetEffect("InFlight",this))
	{
		Sound("ArrowHitGround");
	
		RemoveEffect("InFlight",this);
	
		SetXDir(0);
		SetYDir(0);
		SetRDir(0);

		// Stick in landscape (vertex 3-7)
		SetVertex(2, VTX_X,  0, 2);
		SetVertex(2, VTX_Y, -6, 2);
		SetVertex(3, VTX_X, -3, 2);
		SetVertex(3, VTX_Y, -4, 2);
		SetVertex(4, VTX_X,  3, 2);
		SetVertex(4, VTX_Y, -4, 2);
		SetVertex(5, VTX_X,  4, 2);
		SetVertex(5, VTX_Y, -1, 2);
		SetVertex(6, VTX_X, -4, 2);
		SetVertex(6, VTX_Y, -1, 2);

		rope->HockAnchored();
		for(var obj in FindObjects(Find_ID(GrappleBow), Find_Container(clonk)))
			if(obj != grappler)
				obj->DrawRopeIn();
//		StartPull();
		ScheduleCall(this, "StartPull", 5); // TODO
	}
}

public func StartPull()
{
	pull = 1;

	fx_hook = AddEffect("IntGrappleControl", clonk, 1, 1, this);
	if(clonk->GetAction() == "Jump")
	{
		rope->AdjustClonkMovement();
		rope->ConnectPull();
		fx_hook.var5 = 1;
		fx_hook.var6 = 10;
	}
}

public func Hit(x, y)
{
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
		effect.countdown++;
		if(effect.countdown >= 10)
			return Hit();
	}
	else
		effect.countdown = 0;

	// rotate arrow according to speed
	var anglediff = Normalize(Angle(oldx,oldy,newx,newy)-GetR(),-180);
	SetRDir(anglediff/2);
	effect.x = newx;
	effect.y = newy;
	return;
}

public func Entrance(object container)
{
	if(container->GetID() == GrappleBow) return;
	if (rope)
		rope->BreakRope();
	RemoveObject();
	return;
}

public func OnRopeBreak()
{
	RemoveEffect(nil, clonk, fx_hook);
	RemoveObject();
	return;
}

local Name = "$Name$";


/*-- Grapple rope controls --*/

public func FxIntGrappleControlControl(object target, fxnum, ctrl, x,y,strength, repeat, release)
{
	// Cancel this effect if clonk is now attached to something
	if (target->GetProcedure() == "ATTACH") {
		RemoveEffect(nil,target,fxnum);
		return false;
	}

	if(ctrl != CON_Up && ctrl != CON_Down && ctrl != CON_Right && ctrl != CON_Left) return false;

	if(ctrl == CON_Right)
	{
		fxnum.mv_right = !release;
		if(release)
		{
		  if(fxnum.lastkey == CON_Right)
		  {
		    target->SetDir(0);
		    target->UpdateTurnRotation();
		  }
		  fxnum.lastkey = CON_Right;
		  fxnum.keyTimer = 10;
		}
	}
	if(ctrl == CON_Left)
	{
		fxnum.mv_left = !release;
		if(release)
		{
		  if(fxnum.lastkey == CON_Left)
		  {
		    target->SetDir(1);
		    target->UpdateTurnRotation();
		  }
		  fxnum.lastkey = CON_Left;
		  fxnum.keyTimer = 10;
		}
	}
	if(ctrl == CON_Up)
	{
		fxnum.mv_up = !release;
		if(target->GetAction() == "Jump" && !release && pull)
			rope->ConnectPull();
	}
	if(ctrl == CON_Down)
	{
		fxnum.mv_down = !release;
	}
	
	// never swallow the control
	return false;
}

local iSwingAnimation;

// Effect for smooth movement.
public func FxIntGrappleControlTimer(object target, fxnum, int time)
{
	// Cancel this effect if clonk is now attached to something
	// this check is also in the timer because on a high control rate
	// (higher than 1 actually), the timer could be called first
	if (target->GetProcedure() == "ATTACH")
		return -1;
	// Also cancel if the clonk is contained
	if (target->Contained())
		return -1;
		
	if(fxnum.keyTimer)
	{
	  fxnum.keyTimer--;
	  if(fxnum.keyTimer == 0)
	    fxnum.lastkey = 0;
	}
	// Movement.
	if (fxnum.mv_up)
		if (rope)
    {
      var iSpeed = 10-Cos(target->GetAnimationPosition(fxnum.Climb)*360*2/target->GetAnimationLength("RopeClimb")-45, 10);
      fxnum.speedCounter += iSpeed;
      if(fxnum.speedCounter > 20)
      {
        rope->DoLength(-1);
        fxnum.speedCounter -= 20;
      }
    }
	if (fxnum.mv_down)
		if (rope)
			rope->DoLength(+1);
	if (fxnum.mv_left)
	{
		rope->DoSpeed(-10);
	}
	if (fxnum.mv_right)
	{
		rope->DoSpeed(+10);
	}

	if(target->GetAction() == "Tumble" && target->GetActTime() > 10)
		target->SetAction("Jump");

	if(target->GetAction() != "Jump")
	{
		if(rope->GetConnectStatus())
			rope->ConnectLoose();
	}
	
	if(target->GetAction() == "Jump" && rope->PullObjects() && !fxnum.var6)
	{
		if(!fxnum.ani_mode)
		{
			target->SetTurnType(1);
			if(!target->GetHandAction())
				target->SetHandAction(-1);
		}
/*		target->SetObjDrawTransform(1000, 0, 3000, 0, 1000);
    if(target->GetDir())
      SetObjDrawTransform(1000, 0, -3000, 0, 1000);*/

		if(fxnum.mv_up)
		{
			if(fxnum.ani_mode != 2)
			{
				fxnum.ani_mode = 2;
				fxnum.Climb = target->PlayAnimation("RopeClimb", 10, Anim_Linear(target->GetAnimationLength("RopeClimb")/2, 0, target->GetAnimationLength("RopeClimb"), 35), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
        fxnum.speedCounter = 0;
			}
		}
		else if(fxnum.mv_down)
		{
			if(fxnum.ani_mode != 3)
			{
				fxnum.ani_mode = 3;
				target->PlayAnimation("RopeDown", 10, Anim_Const(0), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
			}
		}
		else if(fxnum.mv_left || fxnum.mv_right)
		{
			var start = target->GetAnimationLength("RopeSwing")/2;
			var length = target->GetAnimationLength("RopeSwing");
			var dir = 0;
			if( (fxnum.mv_left && !target->GetDir())
				|| (!fxnum.mv_left && target->GetDir())
				) dir = 1;
			if(fxnum.ani_mode != 4+dir)
			{
				iSwingAnimation = target->PlayAnimation("RopeSwing", 10, Anim_Linear(start, length*dir, length*(!dir), 35, ANIM_Hold), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
				fxnum.ani_mode = 4+dir;
			}
		}
		else if(fxnum.ani_mode != 1)
		{
			fxnum.ani_mode = 1;
			target->PlayAnimation("OnRope", 10, Anim_Linear(0, 0, target->GetAnimationLength("OnRope"), 35*2, ANIM_Loop), Anim_Linear(0, 0, 1000, 5, ANIM_Remove));
		}
		var angle = rope->GetClonkAngle();
		var off = rope->GetClonkOff();
//    off = [0,0];
    //var pos = rope->GetClonkPos();
    //target->SetPosition(pos[0], pos[1], nil, Rope_Precision);
    target.MyAngle = angle;
    //angle = 0;
		target->SetMeshTransformation(Trans_Translate(-off[0]*10+3000*(1-2*target->GetDir()),-off[1]*10), 2);
    target->SetMeshTransformation(Trans_RotX(angle,500,11000),3);//-6000,12000), 3);
	}
	else if(fxnum.ani_mode)
	{
		target->SetMeshTransformation(0, 2);
    target->SetMeshTransformation(0, 3);
//		target->SetObjDrawTransform(1000, 0, 0, 0, 1000);
		target->StopAnimation(target->GetRootAnimation(10));
		if(!target->GetHandAction())
				target->SetHandAction(0);
		fxnum.ani_mode = 0;
	}

	if(fxnum.var6) fxnum.var6--;
	
	return FX_OK;
}

global func Trans_RotX(int rotation, int ox, int oy)
{
  return Trans_Mul(Trans_Translate(-ox, -oy), Trans_Rotate(rotation,0,0,1), Trans_Translate(ox, oy));
}

public func FxIntGrappleControlStop(object target, fxnum, int reason, int tmp)
{
	if(tmp) return;
	target->SetTurnType(0);
	target->SetMeshTransformation(0, 2);
  target->SetMeshTransformation(0, 3);
	target->StopAnimation(target->GetRootAnimation(10));
//	target->SetObjDrawTransform();
	if(!target->GetHandAction())
		target->SetHandAction(0);
	
	// grapple hook == this:
	// if the hook is not already drawing in, break the rope
	if(!GetEffect("DrawIn", this->GetRope()))
	{
		this->GetRope()->BreakRope();
	}
}

public func NoWindbagForce() {	return true; }

// Only the grappler is stored.
public func SaveScenarioObject() { return false; }
