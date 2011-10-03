/*-- Restart --*/

public func Activate(int plr)
{
	// Notify scenario.
	if (GameCall("OnPlayerRestart", plr))
		return;
	// Remove the player's clonk, including contents.
	var clonk = GetCrew(plr);
	if (clonk)
		clonk->RemoveObject();
}

local Name = "$Name$";
local Description = "$Description$";
