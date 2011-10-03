/*-- Steam engine --*/

#include Library_PowerGenerator

/*-- Power system --*/

public func GetCapacity() { return 500; }
public func GetGeneratorPriority() { return 128; }

local iFuelAmount;

func Construction()
{
	iFuelAmount = 0;
	return _inherited(...);
}

func ContentsCheck()
{
	// Still active?
	if(!ActIdle())
		return true;

	// No need for power?
	if(GetPower() >= GetCapacity())
		return true;

	// Still has some fuel?
	if(iFuelAmount) return SetAction("Work");
	
	var pFuel;
	// Search for new fuel
	if(pFuel = FindObject(Find_Container(this), Find_Func("IsFuel")))
	{
		iFuelAmount += pFuel->~GetFuelAmount();
		pFuel->RemoveObject();
		SetAction("Work");
		return 1;
	}

	//Ejects non fuel items immediately
	if(pFuel = FindObject(Find_Container(this), !Find_Func("IsFuel"))) {
	pFuel->Exit(-53,21, -20, -1, -1, -30);
	Sound("Chuff"); //I think a 'chuff' or 'metal clonk' sound could be good here -Ringwaul
	}

	return 0;
}

func ConsumeFuel()
{
	// Are we full? Stop
	if(GetPower() >= GetCapacity())
		return SetAction("Idle");
	
	// Use up fuel and create power
	DoPower(50);
	if(iFuelAmount)
		iFuelAmount--;

	// All used up?
	if(!iFuelAmount)
	{
		SetAction("Idle");
		ContentsCheck();
	}
}

local ActMap = {
Work = {
	Prototype = Action,
	Name = "Work",
	Procedure = DFA_NONE,
	Directions = 2,
	FlipDir = 1,
	Length = 20,
	Delay = 2,
	X = 0,
	Y = 0,
	Wdt = 110,
	Hgt = 80,
	NextAction = "Work",
	Animation = "Work",
	EndCall = "ConsumeFuel",
},
};
local Name = "$Name$";

func Definition(def) {
	SetProperty("MeshTransformation", Trans_Mul(Trans_Rotate(25,0,1,0), Trans_Scale(625)), def);
}