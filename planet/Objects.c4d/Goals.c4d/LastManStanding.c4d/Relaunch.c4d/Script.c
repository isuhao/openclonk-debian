/*--
	Relaunch container
	Author: Maikel

	This container holds the clonk after relaunches.
	* The time the clonk is held can be specified with SetRelaunchTime(int time);
	* After that time the clonk is released and OnClonkLeftRelaunch(object clonk) is called in the scenario script.
	* Optionally the clonk can choose a weapon if GetRelaunchWeaponList in the scenario script returns a valid id-array.
--*/

local time;
local menu;
local hold;
local has_selected;

protected func Initialize()
{
	time = 36 * 10;
	return;
}

// Sets the time, in seconds, the clonk is held in the container.
public func SetRelaunchTime(int to_time, bool to_hold)
{
	time = to_time * 36;
	hold = to_hold;
	return;
}

// Returns the time, in seconds the clonk is held.
public func GetRelaunchTime() { return time / 36; }

// Retrieve weapon list from scenario.
private func WeaponList() { return GameCall("RelaunchWeaponList"); }

public func StartRelaunch(object clonk)
{
	if (!clonk)
		return;
	clonk->Enter(this);
	ScheduleCall(this, "OpenWeaponMenu", 36, 0, clonk);
	AddEffect("IntTimeLimit", this, 100, 36, this);
	return true;
}

private func OpenWeaponMenu(object clonk)
{
	if (!menu)
	{
		var weapons = WeaponList();
		if(weapons)
		{
			menu = clonk->CreateRingMenu(Clonk, this);
			for (var weapon in weapons)
				menu->AddItem(weapon);
			menu->Show();
		}
	}
}

func FxIntTimeLimitTimer(object target, int num, int fxtime)
{
	var clonk = Contents();
	if (!clonk)
	{
		RemoveObject();
		return -1;
	}
	if (fxtime >= time)
	{
		if (!has_selected)
			GiveWeapon(WeaponList()[Random(GetLength(WeaponList()))]);
		RelaunchClonk();
		return -1;
	}
	if (menu)
		PlayerMessage(clonk->GetOwner(), Format("$MsgWeapon$", (time - fxtime) / 36));
	else
		PlayerMessage(clonk->GetOwner(), Format("$MsgRelaunch$", (time - fxtime) / 36));
	return 1;
}

public func Selected(object menu, object selector, bool alt)
{
	if (!selector)
		return false;
	
	for (var i = 0; i < selector->GetAmount(); i++)
		GiveWeapon(selector->GetSymbol(), alt);
	
	has_selected = true;

	if (!hold)
		RelaunchClonk();
	return true;
}

private func RelaunchClonk()
{
	var clonk = Contents();
	clonk->Exit();
	GameCall("OnClonkLeftRelaunch", clonk);
	if (menu)
		menu->Close();
	PlayerMessage(clonk->GetOwner(), "");
	RemoveObject();
	return;
}

private func GiveWeapon(id weapon_id, bool alt)
{
	var newobj = CreateObject(weapon_id);
	if (weapon_id == Bow)
		newobj->CreateContents(Arrow);
	if (weapon_id == Musket)
		newobj->CreateContents(LeadShot);
	Contents()->Collect(newobj, nil, alt);
	return;
}

local Name = "$Name$";
