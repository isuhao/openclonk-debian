/**
	Sequence
	
	Cutscene to be watched by all players.
	Start calling global func StartSequence, stop using StopSequence
*/

local seq_name;
local seq_progress;
local started;


/* Start and stop */

func Start(string name, int progress, ...)
{
	if (started) Stop();
	SetPosition(0,0); // force global coordinates
	this.seq_name = name;
	this.seq_progress = progress;
	// call init function of this scene - difference to start function is that it is called before any player joins
	var fn_init = Format("~%s_Init", seq_name);
	if (!Call(fn_init, ...))
		GameCall(fn_init, this, ...);
	// Disable crew of all players
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		JoinPlayer(plr);
	}
	started = true;
	// effect
	Sound("Ding", true);
	// call start function of this scene
	var fn_start = Format("%s_Start", seq_name);
	if (!Call(fn_start, ...))
		GameCall(fn_start, this, ...);
	return true;
}

public func InitializePlayer(int plr)
{
	JoinPlayer(plr);
	return true;
}

public func RemovePlayer(int plr)
{
	// call by sequence if it ends and by engine if player leaves
	var fn_remove = Format("~%s_RemovePlayer", seq_name);
	if (!Call(fn_remove, plr))
		GameCall(fn_remove, this, plr);
	return true;
}

public func JoinPlayer(int plr)
{
	var j=0, crew;
	while (crew = GetCrew(plr, j++))
	{
		//if (crew == GetCursor(plr)) crew.Sequence_was_cursor = true; else crew.Sequence_was_cursor = nil;
		crew->SetCrewEnabled(false);
		crew->CancelUse();
		if(crew->GetMenu()) if(!crew->GetMenu()->~Uncloseable()) crew->CancelMenu();
		crew->MakeInvincible();
		crew->SetCommand("None");
		crew->SetComDir(COMD_Stop);
	}
	// per-player sequence callback
	var fn_join = Format("~%s_JoinPlayer", seq_name);
	if (!Call(fn_join, plr))
		GameCall(fn_join, this, plr);
	return true;
}

public func Stop(bool no_remove)
{
	if (started)
	{
		SetViewTarget(nil);
		// Reenable crew and reset cursor
		for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			var j=0, crew;
			while (crew = GetCrew(plr, j++))
			{
				crew->SetCrewEnabled(true);
				crew->ClearInvincible();
				//if (crew.Sequence_was_cursor) SetCursor(plr, crew);
			}
			// ensure proper cursor
			if (!GetCursor(plr)) SetCursor(plr, GetCrew(plr));
			if (crew = GetCursor(plr)) SetPlrView(plr, crew);
			// per-player sequence callback
			RemovePlayer(plr);
		}
		Sound("Ding", true);
		started = false;
		// call stop function of this scene
		var fn_init = Format("~%s_Stop", seq_name);
		if (!Call(fn_init))
			GameCall(fn_init, this);
	}
	if (!no_remove) RemoveObject();
	return true;
}

func Destruction()
{
	Stop(true);
}


/* Sequence callbacks */

func ScheduleNext(int delay, next_idx)
{
	return ScheduleCall(this, this.CallNext, delay, 1, next_idx);
}

func ScheduleSame(int delay) { return ScheduleNext(delay, seq_progress); }

func CallNext(next_idx)
{
	// Start conversation context.
	// Update dialogue progress first.
	if (GetType(next_idx)) seq_progress = next_idx; else ++seq_progress;
	// Then call relevant functions.
	var fn_progress = Format("%s_%d", seq_name, seq_progress);
	if (!Call(fn_progress))
		GameCall(fn_progress, this);
	return true;
}



/* Force view on target */

// Force all player views on given target
public func SetViewTarget(object view_target)
{
	ClearScheduleCall(this, this.UpdateViewTarget);
	if (view_target)
	{
		UpdateViewTarget(view_target);
		ScheduleCall(this, this.UpdateViewTarget, 30, 999999999, view_target);
	}
	else
	{
		for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
		{
			var plr = GetPlayerByIndex(i, C4PT_User);
			SetPlrView(plr, GetCursor(plr));
		}
	}
	return true;
}

private func UpdateViewTarget(object view_target)
{
	// Force view of all players on target
	if (!view_target) return;
	for (var i=0; i<GetPlayerCount(C4PT_User); ++i)
	{
		var plr = GetPlayerByIndex(i, C4PT_User);
		SetPlrView(plr, view_target);
	}
}



/* Status */

// No scenario saving
func SaveScenarioObject(props) { return false; }


/* Message function forwards */

public func MessageBoxAll(string message, object talker, bool as_message, ...)
{
	return Dialogue->MessageBoxAll(message, talker, as_message, ...);
}

private func MessageBox(string message, object clonk, object talker, int for_player, bool as_message, ...)
{
	return Dialogue->MessageBox(message, clonk, talker, for_player, as_message, ...);
}


// Helper function to get a speaker in sequences
func GetHero(object nearest_obj)
{
	// prefer object stored as hero - if not assigned, find someone close to specified object
	if (!this.hero)
		if (nearest_obj)
			this.hero = nearest_obj->FindObject(Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER)), nearest_obj->Sort_Distance());
		else
			this.hero = FindObject(Find_ID(Clonk), Find_Not(Find_Owner(NO_OWNER)));
	// if there is still no hero, take any clonk. let the NPCs do the serquence among themselves
	// (to prevent null pointer exceptions if all players left during the sequence)
	if (!this.hero) this.hero = FindObject(Find_ID(Clonk));
	// might return zero if all players are gone and there are no NPCs. well, there was noone to listen anyway.
	return this.hero;
}


/* Scenario section helper functions */

// Scenario section overload: Automatically transfers all player clonks
func LoadScenarioSection(name, ...)
{
	// Store objects: All clonks and sequence object
	this.save_objs = [];
	AddSectSaveObj(this);
	var iplr,plr;
	for (iplr=0; iplr<GetPlayerCount(C4PT_User); ++iplr)
	{
		plr = GetPlayerByIndex(iplr, C4PT_User);
		for (var icrew=0,crew; icrew<GetCrewCount(iplr); ++icrew)
			if (crew = GetCrew(plr, icrew))
				AddSectSaveObj(crew);
	}
	var save_objs = this.save_objs;
	// Load new section
	var result = inherited(name, ...);
	// Restore objects
	for (var obj in save_objs) if (obj) obj->SetObjectStatus(C4OS_NORMAL);
	if (this) this.save_objs = nil;
	// Recover HUD
	for (iplr=0; iplr<GetPlayerCount(C4PT_User); ++iplr)
	{
		plr = GetPlayerByIndex(iplr, C4PT_User);
		var HUDcontroller = FindObject(Find_ID(GUI_Controller), Find_Owner(plr));
		if (!HUDcontroller) HUDcontroller = CreateObject(GUI_Controller, 10, 10, plr);
	}
	return result;
}

// flag obj and any contained stuff for scenario saving
func AddSectSaveObj(obj)
{
	if (!obj) return false;
	this.save_objs[GetLength(this.save_objs)] = obj;
	var cont,i=0;
	while (cont = obj->Contents(i++)) AddSectSaveObj(cont);
	return obj->SetObjectStatus(C4OS_INACTIVE);
}




/* Global helper functions */

global func StartSequence(string name, int progress, ...)
{
	var seq = CreateObject(Sequence, 0,0, NO_OWNER);
	seq->Start(name, progress, ...);
	return seq;
}

global func StopSequence()
{
	var seq = FindObject(Find_ID(Sequence));
	if (!seq) return false;
	return seq->Stop();
}
