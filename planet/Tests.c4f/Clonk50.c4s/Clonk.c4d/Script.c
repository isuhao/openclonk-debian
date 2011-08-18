/*-- Der Clonk --*/

#include Clonk

/* Act Map */

func Definition(def) {
	inherited();
	SetProperty("ActMap", {
	Walk = {
		Prototype = Action,
		Name = "Walk",
		Procedure = DFA_WALK,
		Directions = 2,
		FlipDir = 0,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 0,
		Wdt = 4,
		Hgt = 10,
		StartCall = "StartWalk",
		AbortCall = "StopWalk",
		InLiquidAction = "Swim",
	},
	Scale = {
		Prototype = Action,
		Name = "Scale",
		Procedure = DFA_SCALE,
		Attach = CNAT_MultiAttach,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 20,
		Wdt = 4,
		Hgt = 10,
		OffX = 0,
		OffY = 0,
		StartCall = "StartScale",
	},
	Tumble = {
		Prototype = Action,
		Name = "Tumble",
		Procedure = DFA_FLIGHT,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 40,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Tumble",
		ObjectDisabled = 1,
		InLiquidAction = "Swim",
		StartCall = "StartTumble",
		EndCall = "CheckStuck",
	},
	Dig = {
		Prototype = Action,
		Name = "Dig",
		Procedure = DFA_DIG,
		Directions = 2,
		Length = 16,
		Delay = 15*3*0,
		X = 0,
		Y = 60,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Dig",
		StartCall = "StartDigging",
		AbortCall = "StopDigging",
		DigFree = 6,
		InLiquidAction = "Swim",
		Attach = CNAT_Left | CNAT_Right | CNAT_Bottom,
	},
	Bridge = {
		Prototype = Action,
		Name = "Bridge",
		Procedure = DFA_THROW,
		Directions = 2,
		Length = 16,
		Delay = 1,
		X = 0,
		Y = 60,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Bridge",
		StartCall = "Digging",
		InLiquidAction = "Swim",
	},
	Swim = {
		Prototype = Action,
		Name = "Swim",
		Procedure = DFA_SWIM,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 80,
		Wdt = 4,
		Hgt = 10,
		OffX = 0,
		OffY = 2,
		StartCall = "StartSwim",
		AbortCall = "StopSwim",
	},
	Hangle = {
		Prototype = Action,
		Name = "Hangle",
		Procedure = DFA_HANGLE,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 100,
		Wdt = 4,
		Hgt = 10,
		OffX = 0,
		OffY = 0,
		StartCall = "StartHangle",
		AbortCall = "StopHangle",
		InLiquidAction = "Swim",
	},
	Jump = {
		Prototype = Action,
		Name = "Jump",
		Procedure = DFA_FLIGHT,
		Directions = 2,
		Length = 1,
		Delay = 0,
		X = 0,
		Y = 120,
		Wdt = 4,
		Hgt = 10,
		InLiquidAction = "Swim",
		PhaseCall = "CheckStuck",
	//	Animation = "Jump",
		StartCall = "StartJump",
	},
	Dive = {
		Prototype = Action,
		Name = "Dive",
		Procedure = DFA_FLIGHT,
		Directions = 2,
		Length = 8,
		Delay = 4,
		X = 0,
		Y = 160,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Hold",
		ObjectDisabled = 1,
		InLiquidAction = "Swim",
		PhaseCall = "CheckStuck",
	},
	Dead = {
		Prototype = Action,
		Name = "Dead",
		Directions = 2,
		X = 0,
		Y = 240,
		Wdt = 4,
		Hgt = 10,
		Length = 1,
		Delay = 0,
		NextAction = "Hold",
		StartCall = "StartDead",
		NoOtherAction = 1,
		ObjectDisabled = 1,
	},
	Ride = {
		Prototype = Action,
		Name = "Ride",
		Procedure = DFA_ATTACH,
		Directions = 2,
		FlipDir = 1,
		Length = 4,
		Delay = 3,
		X = 128,
		Y = 120,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Ride",
		StartCall = "Riding",
		InLiquidAction = "Swim",
	},
	RideStill = {
		Prototype = Action,
		Name = "RideStill",
		Procedure = DFA_ATTACH,
		Directions = 2,
		FlipDir = 1,
		Length = 1,
		Delay = 10,
		X = 128,
		Y = 120,
		Wdt = 4,
		Hgt = 10,
		NextAction = "RideStill",
		StartCall = "Riding",
		InLiquidAction = "Swim",
	},
	Push = {
		Prototype = Action,
		Name = "Push",
		Procedure = DFA_PUSH,
		Directions = 2,
		FlipDir = 1,
		Length = 8,
		Delay = 15,
		X = 128,
		Y = 140,
		Wdt = 4,
		Hgt = 10,
		NextAction = "Push",
		InLiquidAction = "Swim",
	},
	}, def);
}
