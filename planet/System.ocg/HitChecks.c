/*--
		HitCheck.c
		Authors: Newton, Boni
	
		Effect for hit checking.
		Facilitates any hit check of a projectile. The Projectile hits anything
		which is either alive or returns for IsProjectileTarget(object projectile,
		object shooter) true. If the projectile hits something, it calls
		HitObject(object target) in the projectile.
--*/

global func FxHitCheckStart(object target, proplist effect, int temp, object by_obj, bool never_shooter)
{
	if (temp)
		return;
	effect.x = target->GetX();
	effect.y = target->GetY();
	if (!by_obj || GetType(by_obj) != C4V_C4Object)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	effect.shooter = by_obj;
	effect.live = false;
	effect.never_shooter = never_shooter;
	
	// C4D_Object has a hitcheck too -> change to vehicle to supress that.
	if (target->GetCategory() & C4D_Object)
		target->SetCategory((target->GetCategory() - C4D_Object) | C4D_Vehicle);
	return;
}

global func FxHitCheckStop(object target, proplist effect, int reason, bool temp)
{
	if (temp)
		return;
	
	target->SetCategory(target->GetID()->GetCategory());
	return;
}

global func FxHitCheckDoCheck(object target, proplist effect)
{
	var obj;
	// rather search in front of the projectile, since a hit might delete the effect,
	// and clonks can effectively hide in front of walls.
	var oldx = target->GetX();
	var oldy = target->GetY();
	var newx = target->GetX() + target->GetXDir() / 10;
	var newy = target->GetY() + target->GetYDir() / 10;
	var dist = Distance(oldx, oldy, newx, newy);
	
	var shooter = effect.shooter;
	var live = effect.live;
	
	if (live)
		shooter = target;
	
	if (dist <= Max(1, Max(Abs(target->GetXDir()), Abs(target->GetYDir()))) * 2)
	{
		// We search for objects along the line on which we moved since the last check
		// and sort by distance (closer first).
		for (obj in FindObjects(Find_OnLine(oldx, oldy, newx, newy),
								Find_NoContainer(),
								Find_Layer(target->GetObjectLayer()),
								Find_PathFree(target),
								Sort_Distance(oldx, oldy)))
		{
			// Excludes
			if (!obj) continue; // hit callback of one object might have removed other objects
			if(obj == target) continue;
			if(obj == shooter) continue;

			// Unlike in hazard, there is no NOFF rule (yet)
			// CheckEnemy
			//if(!CheckEnemy(obj,target)) continue;

			// IsProjectileTarget or Alive will be hit
			if (obj->~IsProjectileTarget(target, shooter) || obj->GetOCF() & OCF_Alive)
			{
				target->~HitObject(obj);
				if (!target)
					return;
			}
		}
	}
	return;
}

global func FxHitCheckEffect(string newname)
{
	if (newname == "HitCheck")
		return -2;
	return;
}

global func FxHitCheckAdd(object target, proplist effect, string neweffectname, int newtimer, by_obj, never_shooter)
{
	effect.x = target->GetX();
	effect.y = target->GetY();
	if (!by_obj)
		by_obj = target;
	if (by_obj->Contained())
		by_obj = by_obj->Contained();
	effect.shooter = by_obj;
	effect.live = false;
	effect.never_shooter = never_shooter;
	return;
}

global func FxHitCheckTimer(object target, proplist effect, int time)
{
	EffectCall(target, effect, "DoCheck");
	// It could be that it hit something and removed itself. thus check if target is still there.
	// The effect will be deleted right after this.
	if (!target)
		return -1;
	
	effect.x = target->GetX();
	effect.y = target->GetY();
	var live = effect.live;
	var never_shooter = effect.never_shooter;
	var shooter = effect.shooter;

	// The projectile will be only switched to "live", meaning that it can hit the
	// shooter himself when the shot exited the shape of the shooter one time.
	if (!never_shooter)
	{
		if (!live)
		{
			var ready = true;
			// We search for all objects with the id of our shooter.
			for (var foo in FindObjects(Find_AtPoint(target->GetX(), target->GetY()), Find_ID(shooter->GetID())))
			{
				// If its the shooter...
				if(foo == shooter)
					// we may not switch to "live" yet.
					ready = false;
			}
			// Otherwise, the shot will be live.
			if (ready)
				effect.live = true;
		}
	}
	return;
}

// flags for the ProjectileHit function
static const ProjectileHit_tumble=1; // the target tumbles
static const ProjectileHit_no_query_catch_blow_callback=2; // if you want to call QueryCatchBlow manually
static const ProjectileHit_exact_damage=4; // exact damage, factor 1000
static const ProjectileHit_no_on_projectile_hit_callback=8;

global func ProjectileHit(object obj, int dmg, int flags, int damage_type)
{
	if(flags == nil) flags=0;
	if(!damage_type) damage_type=FX_Call_EngObjHit;
	var tumble=flags & ProjectileHit_tumble;
	
	if (!this || !obj)
		return;
	
	if(!(flags & ProjectileHit_no_query_catch_blow_callback))
	{
		if (obj->GetAlive())
			if (obj->~QueryCatchBlow(this))
				return;
				
		if (!this || !obj)
			return;
	}
	
	if(!(flags & ProjectileHit_no_on_projectile_hit_callback))
	{
		obj->~OnProjectileHit(this);
		if (!this || !obj)
			return;
	}
	
	this->~OnStrike(obj);
	if (!this || !obj)
		return;
	if (obj->GetAlive())
	{
		obj->DoEnergy(-dmg, (flags & ProjectileHit_exact_damage), damage_type, GetController());
		if(!obj) return;
		
		var true_dmg=dmg;
		if(flags & ProjectileHit_exact_damage) true_dmg/=1000;
		obj->~CatchBlow(-true_dmg, this);
	}
	else
	{
		var true_dmg=dmg;
		if(flags & ProjectileHit_exact_damage) true_dmg/=1000;
		
		obj->DoDamage(true_dmg, damage_type, GetController());
	}
	// Target could have done something with this projectile.
	if (!this || !obj)
		return;
	
	// Tumble target.
	if (obj->GetAlive() && tumble)
	{
		obj->SetAction("Tumble");
		// Constrained by conservation of linear momentum, unrealism != 1 for unrealistic behaviour.
		var unrealism = 3;
		var mass = GetMass();
		var obj_mass = obj->GetMass();
		obj->SetXDir((obj->GetXDir() * obj_mass + GetXDir() * mass * unrealism) / (mass + obj_mass));		
		obj->SetYDir((obj->GetYDir() * obj_mass + GetYDir() * mass * unrealism) / (mass + obj_mass));
	}
	return;
}
