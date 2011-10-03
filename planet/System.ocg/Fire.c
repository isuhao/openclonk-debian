/*--
		Fire.c
		Authors: Zapper
		
		The fire effect ported from the engine.
		
		Functions to be used as documented in the docs.
--*/

// fire drawing modes
static const C4Fx_FireMode_Default=0; // determine mode by category
static const C4Fx_FireMode_LivingVeg   = 2 ;// C4D_Living and C4D_StaticBack
static const C4Fx_FireMode_StructVeh   = 1; // C4D_Structure and C4D_Vehicle
static const C4Fx_FireMode_Object      = 3; // other (C4D_Object and no bit set (magic))
static const C4Fx_FireMode_Last        = 3; // largest valid fire mode



global func OnFire()
{
	if(!this) return false;
	var effect;
	if(!(effect=GetEffect("Fire", this))) return false;
	return effect.strength;
}

global func Extinguish(strength)
{
	if(!this) return false;
	if(strength == nil) strength=100;
	else if(!strength) return false;
	
	var effect=GetEffect("Fire", this);
	if(!effect) return false;
	effect.strength = BoundBy(effect.strength - strength, 0, 100);
	if(effect.strength == 0) RemoveEffect(nil, this, effect);
	return true;
}

global func Incinerate(strength, int caused_by, blasted, incinerating_object)
{
	if(!this) return false;
	if(strength == nil) strength=100;
	else if(!strength) return false;
	
	var effect=GetEffect("Fire", this);
	if(effect) {effect.strength = BoundBy(effect.strength + strength, 0, 100); return true;}
	else AddEffect("Fire", this, 100, 2, this, nil, caused_by, !!blasted, incinerating_object, strength);
	return true;
}

// called when an object is hit by an explosion (and can burn)
global func OnBlastIncinerationDamage(int level, int player)
{
	return this->Incinerate(level, player);
}

// called if the object is for example in lava
global func OnInIncendiaryMaterial()
{
	return this->Incinerate(5, NO_OWNER);
}

global func FxFireStart(object target, effect, bool temp, int caused_by, bool blasted, object incinerating_object, strength)
{
	// safety
	if (!target) return -1;
	// temp readd
	if (temp) { return 1; }
	// fail if already on fire
	//if (target->OnFire()) return -1;
	
	// structures must eject contents now, because DoCon is not guaranteed to be executed!
	// In extinguishing material
	var fire_caused=true;
	var burn_to=target->GetDefCoreVal("BurnTo", "DefCore");
	var mat;
	if (MaterialName(mat=GetMaterial(target->GetX(),target->GetY())))
		if(GetMaterialVal("Extinguisher", "[Material]", mat, 0))
		{
			// blasts should changedef in water, too!
			if (blasted) if(burn_to != nil) target->ChangeDef(burn_to);
			// no fire caused
			fire_caused = false;
		}
	// BurnTurnTo
	if (fire_caused) if(burn_to != nil) target->ChangeDef(burn_to);
	
	// eject contents
	var obj;
	if (!target->GetDefCoreVal("IncompleteActivity", "DefCore") && !target->GetDefCoreVal("NoBurnDecay", "DefCore"))
	{
		for(var i=target->ContentsCount(); obj=target->Contents(--i);)
		{
			if(target->Contained()) obj->Enter(target->Contained());
			else obj->Exit();
		}
		
		// Detach attached objects
		for(obj in FindObjects(Find_ActionTarget(target)))
		{
			if(obj->GetProcedure() == "ATTACH")
				obj->SetAction(0);
		}
	}
	
	// fire caused?
	if (!fire_caused)
	{
		// if object was blasted but not incinerated (i.e., inside extinguisher)
		// do a script callback
		if (blasted) target->~IncinerationEx(caused_by);
		return -1;
	}
	// determine fire appearance
	var fire_mode;
	if (!(fire_mode=target->~GetFireMode()))
	{
		// set default fire modes
		var cat = target->GetCategory();
		if (cat & (C4D_Living | C4D_StaticBack)) // Tiere, B�ume
			fire_mode = C4Fx_FireMode_LivingVeg;
		else if (cat & (C4D_Structure | C4D_Vehicle)) // Geb�ude und Fahrzeuge sind unten meist kantig
			fire_mode = C4Fx_FireMode_StructVeh;
		else
			fire_mode = C4Fx_FireMode_Object;
	}
	else if (!Inside(fire_mode, 1, C4Fx_FireMode_Last))
	{
		DebugLog("Warning: FireMode %d of object %s (%i) is invalid!", fire_mode, target->GetName(), target->GetID());
		fire_mode = C4Fx_FireMode_Object;
	}
	// store causes in effect vars
	effect.strength = strength;
	effect.mode = fire_mode;
	effect.caused_by = caused_by; // used in C4Object::GetFireCause and timer! <- fixme?
	effect.blasted = blasted;
	effect.incinerating_obj = incinerating_object;
	effect.no_burn_decay = target->GetDefCoreVal("NoBurnDecay", "DefCore");

	// Set values
	//target->FirePhase=Random(MaxFirePhase);
	if((target->GetDefCoreVal("Width", "DefCore") * target->GetDefCoreVal("Height", "DefCore")) > 500) target->Sound("Inflame", false, 100);
	if(target->GetMass() >= 100) target->Sound("Fire", false, 100, 0, true);
	
	// callback
	target->~Incineration(effect.caused_by);
	
	// Done, success
	return FX_OK;
}

global func FxFireTimer(object target, effect, int time)
{
	// safety
	if (!target) return FX_Execute_Kill;
	
	// get cause
	//if(!GetPlayerName(effect.caused_by)) effect.caused_by=NO_OWNER;;
		
	// strength changes over time
	if(time % 4 == 0)
	{
		if(effect.strength < 50){ if(time % 8 == 0) effect.strength=Max(effect.strength-1, 0); if(effect.strength <= 0) return FX_Execute_Kill;}
		else effect.strength=Min(effect.strength+1, 100);
	}
	
	var width=target->GetDefCoreVal("Width", "DefCore")/2;
	var height=target->GetDefCoreVal("Height", "DefCore")/2;
	
	// target is in liquid?
	if(time % 24 == 0)
	{
		
		var mat;
		if(mat = GetMaterial())
			if(GetMaterialVal("Extinguisher", "Material", mat))
				return FX_Execute_Kill;
				
		// check spreading of fire
		if(effect.strength > 10)
		for(var obj in FindObjects(Find_AtRect(-width/2, -height/2, width, height), Find_Exclude(this)))
		{
			if(obj->GetCategory() & C4D_Living) if(!obj->GetAlive()) continue;
			
			var inf = obj->GetDefCoreVal("ContactIncinerate", "DefCore");
			if(!inf) continue;
			var amount = (effect.strength/3) / Max(1, inf);

			if(!amount) continue;
			obj->Incinerate(Max(10, amount), effect.caused_by, false, effect.incinerating_obj);
		}
	}
	
	if(time % 20 == 0)target->Message(Format("<c ee0000>%d</c>", effect.strength));
	// causes on object
	//target->ExecFire(effect_number, caused_by);
	if(target->GetAlive())
	{
		target->DoEnergy(-effect.strength*2, true, FX_Call_EngFire, effect.caused_by); 
	}
	else 
	{
		if((time*10) % 100 <= effect.strength)
		{
			target->DoDamage(2, true, FX_Call_DmgFire, effect.caused_by);
			if(target) if(!Random(2)) if(!effect.no_burn_decay) target->DoCon(-1);
		} 
	}
	if(!target) return FX_Execute_Kill;
	//if(!(time % 2 == 0)) return FX_OK;
	
	//if(!target->OnFire()) {return FX_Execute_Kill;}
	if(target->Contained()) return FX_OK;
	
	// particles
	//var smoke;//, sparks, bright;
	//smoke=BoundBy(effect.strength, 0, 100);
	//sparks=BoundBy((effect.strength*3)/4, 0, 100);
	//bright=BoundBy((effect.strength-50), 0, 100);
	if(time % 4 == 0)
	{
		var size = BoundBy(10*Max(width, height), 50, 800);
		if(size > 300) if(time % 8 == 0) return;
		
		var wind=BoundBy(GetWind(target->GetX(), target->GetY()), -5, 5);
		var smoke_color; if(effect.strength < 50) smoke_color=RGBa(255,255,255, 50); else smoke_color=RGBa(100, 100, 100, 50);
		
		CreateParticle("ExploSmoke", RandomX(-width, width), RandomX(-height, height), wind, -effect.strength/8, size, smoke_color);
	}
	
	/*for(var i=0;i<Max(1, sparks/20);++i)
	{
		CreateParticle("MagicSpark", RandomX(-width, width), RandomX(-height, height), wind/2, -10, 100, RGBa(255,255,255, 200), target, Random(2));
	}*/
	
	/*for(var i=0;i<bright/10;++i)
	{
		CreateParticle("Flash", RandomX(-width, width), RandomX(-height, height), wind, -10, 5*Max(width, height)*Max(1, bright/15), RGBa(255,200,100, 50), target, Random(2));
	}*/
	
	return FX_OK;
}

global func FxFireStop(object target, int effect_number, int reason, bool temp)
{
	// safety
	if (!target) return false;
	// only if real removal is done
	if (temp)
	{
		return true;
	}
	// stop sound
	if (target->GetMass() >=100) target->Sound("Fire", false,  1, 0, false);
	// done, success
	return true;
}

global func FxFireInfo(object target,  effect)
{
	return Format("Is on %d%% fire.", effect.strength); // todo: <--
}