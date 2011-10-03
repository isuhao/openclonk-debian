/*-- Dynamite Igniter --*/

#include DynamiteBox

private func Hit()
{
	Sound("WoodHit"); //TODO Some metal sond
}

public func HoldingEnabled() { return true; }

public func GetCarryMode() { return CARRY_BothHands; }
public func GetCarryPhase() { return 250; }

public func GetCarrySpecial(clonk) { if(fIgnite) return "pos_hand2"; }

local fIgnite;
local aDynamites;
local aWires;

public func ControlUse(object clonk, int x, int y)
{
	if(clonk->GetAction() != "Walk") return true;
	
	fIgnite = 1;
	
	// The clonk has to stand
	clonk->SetAction("Stand");
	clonk->SetXDir(0);

	var iIgniteTime = 35*2;
	clonk->PlayAnimation("DoIgnite", 10, Anim_Linear(0, 0, clonk->GetAnimationLength("DoIgnite"), iIgniteTime, ANIM_Hold), Anim_Const(1000));
	PlayAnimation("Ignite", 1, Anim_Linear(0, 0, GetAnimationLength("Ignite"), iIgniteTime, ANIM_Hold), Anim_Const(1000));

	ScheduleCall(this, "Ignite", iIgniteTime, 1, clonk);

	RemoveEffect("IntLength", this);
	
	return true;
}
local iIgniteNumber;
local pIgniteClonk;

public func Ignite(clonk)
{
	iIgniteNumber = 0;

	pIgniteClonk = clonk;
	if (aWires[iIgniteNumber])
		aWires[iIgniteNumber]->StartFusing(this);
	else
		for (var obj in FindObjects(Find_Category(C4D_StaticBack), Find_Func("IsFuse"), Find_ActionTargets(this)))
			obj->~StartFusing(this);

	ScheduleCall(this, "ResetClonk", 35, 1, clonk);
}

public func OnFuseFinished()
{
	if(Contained() != nil) ResetClonk(Contained());
	else RemoveObject();
}

public func ResetClonk(clonk)
{
	StopAnimation(GetRootAnimation(1));
	
	// Reset animation
	clonk->StopAnimation(clonk->GetRootAnimation(10));
	clonk->SetAction("Walk");
	clonk->DetachObject(this);

	RemoveObject();
}

func Definition(def) {
	SetProperty("PictureTransformation",Trans_Scale(), def); // Hide it TODO: Remove if the mesh isn't shown if there is a picture set
}
local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";

