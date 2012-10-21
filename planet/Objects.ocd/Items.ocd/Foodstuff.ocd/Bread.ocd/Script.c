/*-- Bread --*/

protected func Hit()
{
	Sound("GeneralHit?");
}

/* Eating */

protected func ControlUse(object clonk, int iX, int iY)
{
	clonk->Eat(this);
}

public func NutritionalValue() { return 50; }

public func IsKitchenProduct() { return true; }
public func GetLiquidNeed() { return ["Water", 50]; }
public func GetFuelNeed() { return 50; }

local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local Collectible = 1;
local Rebuy = true;