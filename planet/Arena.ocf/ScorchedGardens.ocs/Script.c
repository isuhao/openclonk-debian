/*-- 
	Scorches Gardens
	Author: Mimmo_O
	
	A melee in a fiery setting.
--*/



protected func Initialize()
{
	SetMatAdjust(RGB(255,150,128));

	// Goal.
	CreateObject(Goal_DeathMatch, 0, 0, NO_OWNER);
	CreateObject(Rule_KillLogs);
	CreateObject(Rule_Gravestones);
	
	//Enviroment.
	CreateObject(Rule_ObjectFade)->DoFadeTime(10 * 36);
	SetSkyAdjust(RGB(255,128,0));
	SetSkyParallax(1, 20,20, 0,0, nil, nil);
	CreateObject(Column,160,304)->SetClrModulation(RGB(255,100,80));
	CreateObject(Column,448,272)->SetClrModulation(RGB(255,100,80));
	
	AddEffect("RandomMeteor", nil, 100, 36-Min(GetPlayerCount()*3,20));
	AddEffect("DangerousLava", nil, 100, 1);
	// Smooth brick edges.
	PlaceEdges();
	PlaceGras();
	return;
}

global func FxDangerousLavaTimer()
{
	//uber effect abuse
	
	for(var burning in FindObjects(Find_ID(Clonk),Find_OCF(OCF_OnFire)))
	{
		burning->DoEnergy(-3); //lava hurts a lot
	}

}

global func FxRandomMeteorTimer()
{
	if(!Random(10)) return ;
	LaunchMeteor(50+Random(LandscapeWidth()-100),-10, 40 + Random(40), RandomX(-20,20), 0);
}

private func PlaceEdges()
{
	var x=[69, 69, 76, 84, 484, 76, 565, 565, 532, 68, 68, 476, 468, 235, 665, 675, 685, 775, 685, 775, 532, 765, 695, 765, 765, 365, 525, 505];
	var y=[357, 349, 364, 364, 260, 300, 305, 313, 313, 300, 308, 260, 268, 495, 375, 365, 355, 355, 275, 415, 305, 345, 345, 255, 285, 445, 445, 435];
	var d=[3, 1, 3, 2, 0, 0, 1, 3, 2, 1, 3, 1, 1, 3, 1, 1, 1, 0, 3, 2, 0, 0, 1, 1, 3, 3, 2, 3];
	for (var i = 0; i < GetLength(x); i++)
	{
		var edge=CreateObject(BrickEdge, x[i], y[i] + 4, NO_OWNER);
		edge->Initialize();
		edge->SetP(d[i]);
		edge->SetPosition(x[i],y[i]);
		edge->PermaEdge();
	}
	return;
}

global func PlaceGras()
{
	var x=[502,468,530,525,548,560,555,551,461,483,354,425,348,343,338,420,412,405,300,315,310,305,290,193,198,169,181,176,127,137,142,133,122,147,35,45,41,30,122];
	var y=[225,221,201,206,191,178,181,185,228,220,190,234,190,188,188,231,226,221,229,218,221,228,229,262,260,261,261,259,227,227,230,228,237,240,221,221,219,222,224];
	var r=[45,-45,-45,-45,-45,-45,-45,-45,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-45,45,45,0,-45,0,-45,45,0,-45,90];
	for (var i = 0; i < GetLength(x); i++)
	{
		while(GBackSolid(x[i],y[i])) y[i]--;
		var edge=CreateObject(Grass, x[i], y[i] + 5, NO_OWNER);
		edge->SetCategory(C4D_StaticBack);
		edge->SetR(r[i]); 
		edge->Initialize();
		edge->SetClrModulation(RGB(200+Random(50),100+Random(60),100+Random(60)));
		
	}
	return 1;
}

protected func OnPlayerRelaunch(int plr)
{
	var clonk = GetCrew(plr);
	clonk->DoEnergy(100000);
	var x = RandomX(75,500);
	var y=100;
	while(!GBackSolid(x,y)) y+=1;
	y-=30;
	var relaunch = CreateObject(RelaunchContainer, x, y, clonk->GetOwner());
	relaunch->StartRelaunch(clonk);
	relaunch->SetRelaunchTime(3);
	clonk->CreateContents(TeleGlove);
	return;
}

func OnClonkLeftRelaunch(object clonk)
{
	clonk->CreateParticle("Fire", 0, 0, PV_Random(-20, 20), PV_Random(-40, 5), PV_Random(20, 90), Particles_Glimmer(), 30);
	clonk->SetYDir(-5);
	return;
}



func WinKillCount() { return 5; }
