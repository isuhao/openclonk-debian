/** 
	Flame
	Spreads fire.
	
	@author Maikel
*/


protected func Initialize()
{
	Incinerate(100, GetController());
	AddTimer("Burning");
	return;
}

public func Burning()
{
	// TODO: Consume inflammable material
	// Split the flame if it is large enough.
	if (GetCon() > 50 && !Random(3))
	{
		var x = Random(15);
		var new_flame = CreateObjectAbove(GetID());
		new_flame->SetSpeed(x, -7);
		new_flame->SetCon(GetCon() / 2);
		SetSpeed(-x, -7);
		SetCon(GetCon() / 2);	
	}
	return;
}

// Don't incinerate twice in saved scenarios.
func SaveScenarioObject(props)
{
	if (!inherited(props, ...)) return false;
	props->Remove("Fire");
	return true;
}


/*-- Properties --*/

local Name = "$Name$";
local Description = "$Description$";
local Plane = 500;