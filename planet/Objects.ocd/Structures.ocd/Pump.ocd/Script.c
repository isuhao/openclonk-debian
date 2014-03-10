/*--
	Pump
	Author: Maikel, ST-DDT, Sven2, Newton
	
	Pumps liquids using drain and source pipes. Features include:
	+ switch on and off
	+ consume/produce a variable amount of power depending on the height of
	  source and drain
	
--*/

#include Library_Structure
#include Library_Ownable
#include Library_PowerConsumer
#include Library_PowerProducer

local Name = "$Name$";
local Description = "$Description$";
local BlastIncinerate = 50;
local HitPoints = 70;

/*
	States
	"Wait":				turned off or source pipe not connected
	"WaitForPower":		turned on but no power (does consume power)
	"WaitForLiquid":	turned on but no liquid (does not consume power)
	"Pump":				currently working and consuming/producing power

*/
local ActMap = {
	Pump = {
		Prototype = Action,
		Name = "Pump",
		Length = 30,
		Delay = 3,
		Sound = "Pumpjack",
		NextAction = "Pump",
		StartCall = "CheckState",
		PhaseCall = "Pumping"
	},
	Wait = {
		Prototype = Action,
		Name = "Wait",
		Delay = 90,
		NextAction = "Wait",
		EndCall = "CheckState"
	},
	WaitForPower = {
		Prototype = Action,
		Name = "WaitForPower",
		Delay = 30,
		NextAction = "WaitForPower",
		EndCall = "CheckState"
	},
	WaitForLiquid = {
		Prototype = Action,
		Name = "WaitForLiquid",
		Delay = 30,
		NextAction = "WaitForLiquid",
		EndCall = "CheckState"
	}
};

local animation; // animation handle

local switched_on; // controlled by Interaction. Indicates whether the user wants to pump or not

local powered; // whether the pump has enough power as a consumer, always true if producing
local power_used; // the amount of power currently consumed or (if negative) produced

local stored_material_index; //contained liquid
local stored_material_amount;

local source_pipe;
local drain_pipe;

/** This object is a liquid pump, thus pipes can be connected. */
public func IsLiquidPump() { return true; }

func Definition(def) 
{
	// for title image
	SetProperty("PictureTransformation",Trans_Rotate(50,0,1,0),def);
	// for building preview
	SetProperty("MeshTransformation",Trans_Rotate(50,0,1,0),def);
}

func Construction()
{
	// Rotate at a 45 degree angle towards viewer and add a litte bit of Random
	this.MeshTransformation = Trans_Rotate(50 + RandomX(-10,10),0,1,0);
}

func Initialize()
{
	switched_on = true;
	var start = 0;
	var end = GetAnimationLength("pump");
	animation = PlayAnimation("pump", 5, Anim_Linear(GetAnimationPosition(animation), start, end, 35, ANIM_Loop), Anim_Const(1000));
	SetState("Wait");
}


/*-- Interaction --*/

public func IsInteractable() { return GetCon() >= 100; }

public func GetInteractionMetaInfo(object clonk)
{
	if (switched_on)
		return { Description = "$MsgTurnOff$", IconName = nil, IconID = Icon_Stop };
	else
		return { Description = "$MsgTurnOn$", IconName = nil, IconID = Icon_Play };
}

/** Turn on or off. */
public func Interact(object clonk)
{
	switched_on = !switched_on;
	CheckState();
	
	return true;
}

/*-- Pipe connection --*/

public func GetSource() { return source_pipe; }
public func SetDrain(object pipe) { drain_pipe = pipe; }
public func GetDrain() { return drain_pipe; }

public func SetSource(object pipe)
{
	source_pipe = pipe;
	CheckState();
}

/*-- Power stuff --*/

func QueryWaivePowerRequest()
{
	// has less priority than other objects, but not too low
	return 10;
}

func OnNotEnoughPower()
{
	_inherited(...);
	powered = false;
	CheckState();
}

func OnEnoughPower()
{
	_inherited(...);
	powered = true;
	CheckState();
}

/** Returns object to which the liquid is pumped */
private func GetDrainObject()
{
	if (drain_pipe) return drain_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** Re	turns object to which the liquid is pumped */
private func GetSourceObject()
{
	if (source_pipe) return source_pipe->GetConnectedObject(this) ?? this;
	return this;
}

/** Returns amount of pixels to pump per 30 frames */
public func GetPumpSpeed()
{
	return 50;
}

/** PhaseCall of Pump: Pump the liquid from the source to the drain pipe */
protected func Pumping()
{
	// at this point we can assert that we have power
	
	// something went wrong in the meantime?
	// let the central function handle that on next check
	if (!source_pipe) return;
	
	var pump_ok = true;
	
	// is empty? -> try to get liquid
	if (!stored_material_amount)
	{

		// get new materials
		var aMat = GetSourceObject()->ExtractLiquidAmount(0,0, GetPumpSpeed()/10);
	
		// no material to pump?
		if (aMat)
		{
		stored_material_index = aMat[0];
		stored_material_amount = aMat[1];
		}
		else
		{
			pump_ok = false;
		}
	}
	if (pump_ok)
	{
		var i = stored_material_amount;
		while (i > 0)
		{
			if (GetDrainObject()->InsertMaterial(stored_material_index))
			{
				i--;
			}
			// Drain is stuck.
			else
			{
				pump_ok = false;
				break;
			}
		}
		
		stored_material_amount = i;
		if (stored_material_amount <= 0)
			stored_material_index = nil;
	}
	
	if(!pump_ok)
	{
		SetState("WaitForLiquid");
	}
}

/** Re check state and change the state if needed */
func CheckState()
{
	var is_fullcon = GetCon() >= 100;
	var can_pump = source_pipe && is_fullcon && switched_on;
	
	// can't pump at all -> wait
	if (!can_pump)
	{
		SetState("Wait");
	}
	else
	{
		// can pump but has no liquid -> wait for liquid
		if (!HasLiquidToPump())
		{
			SetState("WaitForLiquid");
		}
		else
		{
			// can pump, has liquid but has no power -> wait for power
			if (!powered)
			{
				SetState("WaitForPower");
			}
			// otherwise, pump! :-)
			else
			{
				SetState("Pump");
			}
			
			// regularly update the power usage while pumping or waiting for power
			UpdatePowerUsage();
		}
	}
}

/** Get current height the pump has to push liquids upwards (input.y - output.y) */
private func GetPumpHeight()
{
	// compare each the surfaces of the bodies of liquid pumped
	
	// find Y position of surface of liquid that is pumped to target
	var source_y = 0;
	if (GetSourceObject()->GBackLiquid())
	{
		var src_mat = GetSourceObject()->GetMaterial();
		while (src_mat == GetSourceObject()->GetMaterial(0, source_y-1))
			--source_y;
	}
	// same for target (use same function as if inserting)
	var target_pos = {X=0, Y=0};
	GetDrainObject()->CanInsertMaterial(Material("Water"),0,0,target_pos);
	return (GetSourceObject()->GetY() + source_y) - target_pos.Y;
}

/** Recheck power usage/production for current pump height
	and make the pump a producer / consumer for the power system */
private func UpdatePowerUsage()
{
	var new_power;
	if(IsUsingPower())
		new_power = PumpHeight2Power(GetPumpHeight());
	else
		new_power = 0;
	
	// do nothing if not necessary
	if (new_power == power_used)
		return;
	
	// and update energy system
	if (new_power > 0)
	{
		if (power_used < 0)
		{
			powered = false; // needed since the flag was set manually
			UnmakePowerProducer();
		}
		MakePowerConsumer(new_power);
		
	}
	else if (new_power < 0)
	{
		if (power_used > 0)
			UnmakePowerConsumer();
		MakePowerProducer(-new_power);
		powered = true; // when producing, we always have power
	}
	else // new_power == 0
	{
		if (power_used < 0) UnmakePowerProducer();
		else if (power_used > 0) UnmakePowerConsumer();
		powered = true;
	}
	
	power_used = new_power;
}

/** Return whether the pump should be using power in the current state */
private func IsUsingPower()
{
	return switched_on && (GetAction() == "Pump" || GetAction() == "WaitForPower");
}

/** Transform pump height (input.y - output.y) to required power */
private func PumpHeight2Power(int pump_height)
{
	// pumping downwards will only produce energy after an offset
	var power_offset = 40;
	// max power consumed/produced
	var max_power = 150;
	
	return BoundBy((pump_height + power_offset)/15*5, -max_power,max_power+power_offset);
}

/** Returns whether there is liquid at the source pipe to pump */
private func HasLiquidToPump()
{
	if (!source_pipe)
		return false;
		
	// source
	if(!GetSourceObject()->GBackLiquid())
		return false;
	
	// target (test with the very popular liquid "water")
	if(!GetDrainObject()->CanInsertMaterial(Material("Water"),0,0))
		return false;
	
	return true;
}

/** Set the state of the pump, retaining the animation position and updating the power usage */
func SetState(string act)
{
	if(act == GetAction()) return;

	var start = 0;
	var end = GetAnimationLength("pump");
	var anim_pos = GetAnimationPosition(animation);
	if (act == "Pump")
	{
		SetAnimationPosition(animation, Anim_Linear(anim_pos, start, end, 35, ANIM_Loop));
	}
	else if(act == "WaitForLiquid")
	{
		SetAnimationPosition(animation, Anim_Linear(anim_pos, start, end, 350, ANIM_Loop));
	}
	else
	{
		SetAnimationPosition(animation, Anim_Const(anim_pos));
	}
	
	// deactivate power usage when not pumping
	if (powered && (act == "Wait" || act == "WaitForLiquid"))
	{
		if (power_used < 0) UnmakePowerProducer();
		else if(power_used > 0) UnmakePowerConsumer();
		power_used = 0;
		powered = false;
	}
	// finally, set the action
	SetAction(act);
}