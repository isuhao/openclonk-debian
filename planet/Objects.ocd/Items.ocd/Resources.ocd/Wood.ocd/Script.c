/*--- The Log ---*/

protected func Hit()
{
	Sound("WoodHit*");
	return 1;
}

func Incineration()
{
	SetClrModulation (RGB(48, 32, 32));
}

public func IsFuel() { return 1; }
public func GetFuelAmount() { return 30; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local Rebuy = true;
