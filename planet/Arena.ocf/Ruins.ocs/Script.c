/*-- 
	Ruins
	Author: Mimmo_O
	
	An arena like last man standing round for up to 12 players.
--*/

static const RUINS_RAIN_PERIOD_TIME=3200;

protected func Initialize()
{
	// Goal.
	CreateObject(Goal_LastManStanding, 0, 0, NO_OWNER);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	
	// Mood.
	SetSkyAdjust(RGBa(255, 255, 255, 127), RGB(255, 200, 150));
	SetGamma(RGB(40, 35, 30), RGB(140, 135, 130), RGB(255, 250, 245));
	
	// Chests with weapons.
	CreateObject(Chest, 230, 224, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 500, 64, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 124, 128, NO_OWNER)->MakeInvincible();
	CreateObject(Chest, 340, 440, NO_OWNER)->MakeInvincible();
	AddEffect("IntFillChests", nil, 100, 2 * 36);
	
	// Ropeladders to get to the upper part.

	CreateObject(Ropeladder, 380, 112, NO_OWNER)->Unroll(-1,0,19);
	CreateObject(Ropeladder, 135, 135, NO_OWNER)->Unroll(1,0,16);
	
	// Objects fade after 5 seconds.
	CreateObject(Rule_ObjectFade)->DoFadeTime(5 * 36);

	// Smooth brick edges.
	var x=[188, 205, 261, 244, 308, 325];
	var y=[124, 124, 132, 132, 108, 108];
	var d=[3, 2, 2, 3, 3, 2];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i], NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	
	AddEffect("DryTime",nil,100,2);
	return;
}

// Gamecall from LastManStanding goal, on respawning.
protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	var relaunch = CreateObject(RelaunchContainer, LandscapeWidth() / 2, LandscapeHeight() / 2, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	return;
}


global func FxRainTimer(object pTarget, effect, int timer)
{
	if(timer<400)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
		return 1;
	} 
		for(var i=0; i<(6+Random(3)); i++)
	{
		InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
	}
	if(timer>(RUINS_RAIN_PERIOD_TIME+Random(800))) 
	{
	AddEffect("DryTime",nil,100,2);
	return -1;	
	}
	
	return 1;
}
global func FxDryTimeTimer(object pTarget, effect, int timer)
{
	if(timer<(380+Random(300))){
	InsertMaterial(Material("Water"),Random(LandscapeWidth()-60)+30,1,Random(7)-3,100+Random(100));
		return 1;
	}
	ExtractLiquidAmount(310+Random(50),430+Random(10),6+Random(4));
	
	if(!GBackLiquid(335,430))
	{
		AddEffect("Rain",nil,100,2);
		return -1;
	}	
}



// Refill/fill chests.
global func FxIntFillChestsStart(object target, effect, int temporary)
{
	if(temporary) return 1;
	var chests = FindObjects(Find_ID(Chest));
	var w_list = [Bow, Musket, Shield, Sword, Club, GrenadeLauncher, Bow, Musket, Shield, Sword, Club, GrenadeLauncher, DynamiteBox];
	
	for(var chest in chests)
		for(var i=0; i<4; ++i)
			chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func FxIntFillChestsTimer()
{
	SetTemperature(100);
	var chest = FindObjects(Find_ID(Chest), Sort_Random())[0];
	var w_list = [Boompack, IronBomb, IronBomb, Firestone, Bow, Musket, Sword, Javelin];
	
	if (chest->ContentsCount() < 5)
		chest->CreateChestContents(w_list[Random(GetLength(w_list))]);
	return 1;
}

global func CreateChestContents(id obj_id)
{
	if (!this)
		return;
	var obj = CreateObject(obj_id);
	if (obj_id == Bow)
		obj->CreateContents(Arrow);
	if (obj_id == Musket)
		obj->CreateContents(LeadShot);
	obj->Enter(this);
	return;
}

// GameCall from RelaunchContainer.
func OnClonkLeftRelaunch(object clonk)
{
	clonk->SetPosition(RandomX(200, LandscapeWidth() - 200), -20);
}

func KillsToRelaunch() { return 0; }
func RelaunchWeaponList() { return [Bow, Shield, Sword, Club, Javelin, Musket]; }
