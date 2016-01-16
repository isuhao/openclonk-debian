/*--- The Log ---*/

protected func Hit()
{
	Sound("Hits::Materials::Wood::WoodHit?");
	return 1;
}

func Incineration()
{
	SetClrModulation(RGB(48, 32, 32));
}

public func IsFuel() { return true; }
public func GetFuelAmount(bool get_partial) 
{ 
	if (get_partial)
		return GetCon() / 2;
	return 50;
}
public func IsSawmillProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 5;
local ContactIncinerate = 1;
local Plane = 470;