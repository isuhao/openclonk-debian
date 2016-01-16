/**
	AI
	Controls NPC behaviour.

	@author Sven2
*/


#include AI_HelperFunctions

local Plane = 300;

static const AI_DefMaxAggroDistance = 200, // lose sight to target if it is this far away (unles we're ranged - then always guard the range rect)
             AI_DefGuardRangeX = 300,  // search targets this far away in either direction (searching in rectangle)
             AI_DefGuardRangeY = 150,  // search targets this far away in either direction (searching in rectangle)
             AI_AlertTime      = 800; // number of frames after alert after which AI no longer checks for projectiles
/* Public interface */

// Add AI execution timer to target Clonk
func AddAI(object clonk)
{
	var fx = GetEffect("AI", clonk);
	if (!fx) fx = AddEffect("AI", clonk, 1, 3, nil, AI);
	if (!fx || !clonk) return nil;
	fx.ai = AI;
	clonk.ExecuteAI = AI.Execute;
	clonk.ai = fx;
	if (clonk->GetProcedure() == "PUSH") fx.vehicle = clonk->GetActionTarget();
	BindInventory(clonk);
	SetHome(clonk);
	SetGuardRange(clonk, fx.home_x-AI_DefGuardRangeX, fx.home_y-AI_DefGuardRangeY, AI_DefGuardRangeX*2, AI_DefGuardRangeY*2);
	SetMaxAggroDistance(clonk, AI_DefMaxAggroDistance);
	// AI editor commands
	if (!clonk.EditCursorCommands)
		clonk.EditCursorCommands = [];
	else if (clonk.EditCursorCommands == clonk.Prototype.EditCursorCommands)
		clonk.EditCursorCommands = clonk.EditCursorCommands[:];
	var idx;
	if ((idx=GetIndexOf(clonk.EditCursorCommands, "AI_Add()"))>=0) clonk.EditCursorCommands[idx] = nil;
	if (GetIndexOf(clonk.EditCursorCommands, AI.AI_SetHome)<0)
	{
		var l = GetLength(clonk.EditCursorCommands);
		clonk.EditCursorCommands[l++] = clonk.AI_SetHome = AI.AI_SetHome;
		clonk.EditCursorCommands[l++] = clonk.AI_BindInventory = AI.AI_BindInventory;
	}
	return fx;
}

func GetAI(object clonk) { return clonk.ai; }

// Set the current inventory to be removed when the clonk dies. Only works if clonk has an AI.
func BindInventory(object clonk)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	var cnt = clonk->ContentsCount();
	fx.bound_weapons = CreateArray(cnt);
	for (var i=0; i<cnt; ++i) fx.bound_weapons[i] = clonk->Contents(i);
	clonk->Call(fx.ai.UpdateDebugDisplay, fx);
	return true;
}

// Set the home position the Clonk returns to if he has no target
func SetHome(object clonk, int x, int y, int dir)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	// nil/nil defaults to current position
	if (!GetType(x)) x=clonk->GetX();
	if (!GetType(y)) y=clonk->GetY();
	if (!GetType(dir)) dir=clonk->GetDir();
	fx.home_x = x; fx.home_y = y;
	fx.home_dir = dir;
	return true;
}

// Put into clonk proplist
func AI_BindInventory() { return this.ai.ai->BindInventory(this); }
func AI_SetHome() { return this.ai.ai->SetHome(this); }
func AI_SetIgnoreAllies() { return (this.ai.ignore_allies = true); }

// Set the guard range to the provided rectangle
func SetGuardRange(object clonk, int x, int y, int wdt, int hgt)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	// clip to landscape size
	if (x<0) { wdt+=x; x=0; }
	if (y<0) { wdt+=x; x=0; }
	wdt = Min(wdt, LandscapeWidth() - x);
	hgt = Min(hgt, LandscapeHeight() - y);
	fx.guard_range = {x=x, y=y, wdt=wdt, hgt=hgt};
	clonk->Call(fx.ai.UpdateDebugDisplay, fx);
	return true;
}

// Set the maximum distance the enemy will follow an attacking Clonk
func SetMaxAggroDistance(object clonk, int max_dist)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	fx.max_aggro_distance = max_dist;
	return true;
}

// Set range in which, on first encounter, allied AI Clonks get the same aggro target set
func SetAllyAlertRange(object clonk, int new_range)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	fx.ally_alert_range = new_range;
	clonk->Call(fx.ai.UpdateDebugDisplay, fx);
	return true;
}

// Set callback function name to be called in game script when this AI is first encountered
// Callback function first parameter is (this) AI clonk, second parameter is player clonk.
// The callback should return true to be cleared and not called again. Otherwise, it will be called every time a new target is found.
func SetEncounterCB(object clonk, string cb_fn)
{
	if (!clonk) return false; var fx = clonk.ai; if (!fx) return false;
	fx.encounter_cb = cb_fn;
	clonk->Call(fx.ai.UpdateDebugDisplay, fx);
	return true;
}

/* Scenario saving */

func FxAISaveScen(clonk, fx, props)
{
	if (!clonk) return false;
	props->AddCall("AI", AI, "AddAI", clonk);
	if (fx.home_x != clonk->GetX() || fx.home_y != clonk->GetY() || fx.home_dir != clonk->GetDir())
		props->AddCall("AI", AI, "SetHome", clonk, fx.home_x, fx.home_y, GetConstantNameByValueSafe(fx.home_dir, "DIR_"));
	props->AddCall("AI", AI, "SetGuardRange", clonk, fx.guard_range.x, fx.guard_range.y, fx.guard_range.wdt, fx.guard_range.hgt);
	if (fx.max_aggro_distance != AI_DefMaxAggroDistance)
		props->AddCall("AI", AI, "SetMaxAggroDistance", clonk, fx.max_aggro_distance);
	if (fx.ally_alert_range)
		props->AddCall("AI", AI, "SetAllyAlertRange", clonk, fx.ally_alert_range);
	if (fx.encounter_cb)
		props->AddCall("AI", AI, "SetEncounterCB", clonk, Format("%v", fx.encounter_cb));
	return true;
}


/* AI effect callback functions */

protected func FxAITimer(clonk, fx, int time) { clonk->ExecuteAI(fx, time); return FX_OK; }

protected func FxAIStop(clonk, fx, int reason)
{
	// remove debug display
	if (fx.debug) clonk->Call(fx.ai.EditCursorDeselection, fx);
	// remove weapons on death
	if (reason == FX_Call_RemoveDeath)
	{
		if (fx.bound_weapons) for (var obj in fx.bound_weapons) if (obj && obj->Contained()==clonk) obj->RemoveObject();
	}
	// remove AI reference
	if (clonk && clonk.ai == fx) clonk.ai = nil;
	// remove edit cursor commands
	if (clonk && clonk.EditCursorCommands)
	{
		var idx;
		if ((idx=GetIndexOf(clonk.EditCursorCommands, AI.AI_SetHome))>=0) clonk.EditCursorCommands[idx] = nil;
		if ((idx=GetIndexOf(clonk.EditCursorCommands, AI.AI_BindInventory))>=0) clonk.EditCursorCommands[idx] = nil;
	}
	return FX_OK;
}

protected func FxAIDamage(clonk, fx, int dmg, int cause)
{
	// AI takes damage: Make sure we're alert so evasion and healing scripts are executed!
	// It might also be feasible to execute encounter callbacks here (in case an AI is shot from a position it cannot see).
	// However, the attacking Clonk is not known and the callback might be triggered e.g. by an unfortunate meteorite or lightning blast.
	// So let's just keep it at alert state for now
	if (dmg<0) fx.alert=fx.time;
	return dmg;
}


/* AI execution timer functions */

// called in context of the Clonk that is being controlled
private func Execute(proplist fx, int time)
{
	fx.time = time;
	// Evasion, healing etc. if alert
	if (fx.alert) if (ExecuteProtection(fx)) return true;
	// Find something to fight with
	if (!fx.weapon) { CancelAiming(fx); if (!ExecuteArm(fx)) return ExecuteIdle(fx); else if (!fx.weapon) return true; }
	// Weapon out of ammo?
	if (fx.ammo_check && !Call(fx.ammo_check, fx, fx.weapon)) { fx.weapon=nil; return false; }
	// Find an enemy
	if (fx.target) if (!fx.target->GetAlive() || (!fx.ranged && ObjectDistance(fx.target) >= fx.max_aggro_distance)) fx.target = nil;
	if (!fx.target)
	{
		CancelAiming(fx);
		if (!(fx.target = FindTarget(fx))) return ExecuteIdle(fx);
		// first encounter callback. might display a message.
		if (fx.encounter_cb) if (GameCall(fx.encounter_cb, this, fx.target)) fx.encounter_cb = nil;
		// wake up nearby allies
		if (fx.ally_alert_range)
		{
			var ally_fx;
			for (var ally in FindObjects(Find_Distance(fx.ally_alert_range), Find_Exclude(this), Find_OCF(OCF_CrewMember), Find_Owner(GetOwner())))
				if (ally_fx = AI->GetAI(ally))
					if (!ally_fx.target)
					{
						ally_fx.target = fx.target;
						ally_fx.alert = ally_fx.time;
						if (ally_fx.encounter_cb) if (GameCall(ally_fx.encounter_cb, ally, fx.target)) ally_fx.encounter_cb = nil;
					}
			// waking up works only once. after that, AI might have moved and wake up Clonks it shouldn't
			fx.ally_alert_range = nil;
		}
	}
	// Attack it!
	return Call(fx.strategy, fx);
}

// Selects an item the clonk is about to use
private func SelectItem(object item)
{
	if (!item) return;
	if (item->Contained() != this) return;
	this->SetHandItemPos(0, this->GetItemPos(item));
}

private func ExecuteProtection(fx)
{
	// Search for nearby projectiles. Ranged AI also searches for enemy clonks to evade.
	var enemy_search;
	if (fx.ranged) enemy_search = Find_And(Find_OCF(OCF_CrewMember), Find_Not(Find_Owner(GetOwner())));
	//Log("ExecProt");
	var projectiles = FindObjects(Find_InRect(-150,-50,300,80), Find_Or(Find_Category(C4D_Object), Find_Func("IsDangerous4AI"), Find_Func("IsArrow"), enemy_search), Find_OCF(OCF_HitSpeed2), Find_NoContainer(), Sort_Distance());
	for (var obj in projectiles)
	{
		var dx = obj->GetX()-GetX(), dy = obj->GetY()-GetY();
		var vx = obj->GetXDir(), vy = obj->GetYDir();
		if (Abs(dx)>40 && vx) dy += (Abs(10*dx/vx)**2)*GetGravity()/200;
		var v2 = Max(vx*vx+vy*vy, 1);
		var d2 = dx*dx+dy*dy;
		if (d2/v2 > 4)
		{
			// won't hit within the next 20 frames
			//obj->Message("ZZZ");
			continue;
		}
		//obj->Message("###%d", Sqrt(100*d2/v2));
		// Distance at which projectile will pass Clonk should be larger than Clonk size (erroneously assumes Clonk is a sphere)
		var l = dx*vx+dy*vy;
		//Log("%s l=%d  d=%d  r=%d   gravcorr=%d", obj->GetName(), l, Sqrt(d2), Sqrt(d2-l*l/v2), (Abs(10*dx/vx)**2)*GetGravity()/200);
		if (l<0 && Sqrt(d2-l*l/v2)<=GetCon()/8)
		{
			// Not if there's a wall between
			if (!PathFree(GetX(),GetY(),obj->GetX(),obj->GetY())) continue;
			// This might hit :o
			fx.alert=fx.time;
			// Use a shield if the object is not explosive.
			if (fx.shield && !obj->~HasExplosionOnImpact())
			{
				// use it!
				SelectItem(fx.shield);
				if (fx.aim_weapon == fx.shield)
				{
					// continue to hold shield
					//Log("shield HOLD %d %d", dx,dy);
					fx.shield->ControlUseHolding(this, dx,dy);
				}
				else
				{
					// start holding shield
					if (fx.aim_weapon) CancelAiming(fx);
					if (!CheckHandsAction(fx)) return true;
					if (!fx.shield->ControlUseStart(this, dx,dy)) return false; // something's broken :(
					//Log("shield START %d %d", dx,dy);
					fx.shield->ControlUseHolding(this, dx,dy);
					fx.aim_weapon = fx.shield;
				}
				return true;
			}
			// no shield. try to jump away
			if (dx<0) SetComDir(COMD_Right); else SetComDir(COMD_Left);
			if (ExecuteJump()) return true;
			// Can only try to evade one projectile
			break;
		}
	}
	//Log("OK");
	// stay alert if there's a target. Otherwise alert state may wear off
	if (!fx.target) fx.target = FindEmergencyTarget(fx);
	if (fx.target) fx.alert=fx.time; else if (fx.time-fx.alert > AI_AlertTime) fx.alert=nil;
	// nothing to do
	return false;
}

private func CheckTargetInGuardRange(fx)
{
	// if target is not in guard range, reset it and return false
	if (!Inside(fx.target->GetX()-fx.guard_range.x, -10, fx.guard_range.wdt+9)
		||!Inside(fx.target->GetY()-fx.guard_range.y, -10, fx.guard_range.hgt+9)) 
		if (ObjectDistance(fx.target) > 250)
			{ fx.target=nil; return false; }
	return true;
}

private func ExecuteVehicle(fx)
{
	// only knows how to use catapult
	if (!fx.vehicle || fx.vehicle->GetID() != Catapult) { fx.vehicle = nil; return false; }
	// still pushing it?
	if (GetProcedure() != "PUSH" || GetActionTarget() != fx.vehicle)
	{
		if (!GetCommand() || !Random(4)) SetCommand("Grab", fx.vehicle);
		return true;
	}
	// Target still in guard range?
	if (!CheckTargetInGuardRange(fx)) return false;
	// turn in correct direction
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY()-4;
	if (tx>x && !fx.vehicle.dir) { fx.vehicle->ControlRight(this); return true; }
	if (tx<x &&  fx.vehicle.dir) { fx.vehicle->ControlLeft (this); return true; }
	// make sure we're aiming
	if (!fx.aim_weapon)
	{
		if (!fx.vehicle->~ControlUseStart(this)) return false;
		fx.aim_weapon = fx.vehicle;
		fx.aim_time = fx.time;
		return true;
	}
	// update catapult animation
	fx.vehicle->~ControlUseHolding(this, tx-x, ty-y);
	// Determine power needed to hit target
	var dx = tx-x, dy=ty-y+20;
	var power = Sqrt((GetGravity()*dx*dx) / Max(Abs(dx)+dy,1));
	var dt = dx * 10 / power;
	tx += GetTargetXDir(fx.target, dt);
	ty += GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1)) dy += GetGravity()*dt*dt/200;
	power = Sqrt((GetGravity()*dx*dx) / Max(Abs(dx)+dy,1));
	power = power + Random(11)-5; // some variation
	fx.projectile_speed = power = BoundBy(power, 20, 100); // limits imposed by catapult
	// Can shoot now?
	if (fx.time >= fx.aim_time + fx.aim_wait) if (PathFree(x,y-20,x+dx,y+dy-20))
	{
		fx.aim_weapon->~DoFire(this, power, 0);
		fx.aim_weapon = nil;
	}
	return true;
}

private func CancelAiming(fx)
{
	if (fx.aim_weapon)
	{
		//Log("CancelAiming"); LogCallStack();
		//if (fx.aim_weapon==fx.shield) Log("cancel shield");
		fx.aim_weapon->~ControlUseCancel(this);
		fx.aim_weapon = nil;
	}
	else
	{
		// Also cancel aiming done outside AI control
		this->~CancelAiming();
	}
	return true;
}

private func IsAimingOrLoading() { return !!GetEffect("IntAim*", this); }

private func ExecuteRanged(fx)
{
	// Still carrying the bow?
	if (fx.weapon->Contained() != this) { fx.weapon=fx.post_aim_weapon=nil; return false; }
	// Finish shooting process
	if (fx.post_aim_weapon)
	{
		// wait max one second after shot (otherwise may be locked in wait animation forever if something goes wrong during shot)
		if (FrameCounter() - fx.post_aim_weapon_time < 36)
			if (IsAimingOrLoading()) return true;
		fx.post_aim_weapon = nil;
	}
	// Target still in guard range?
	if (!CheckTargetInGuardRange(fx)) return false;
	// Look at target
	ExecuteLookAtTarget(fx);
	// Make sure we can shoot
	if (!IsAimingOrLoading() || !fx.aim_weapon)
	{
		CancelAiming(fx);
		if (!CheckHandsAction(fx)) return true;
		// Start aiming
		SelectItem(fx.weapon);
		if (!fx.weapon->ControlUseStart(this, fx.target->GetX()-GetX(), fx.target->GetY()-GetY())) return false; // something's broken :(
		fx.aim_weapon = fx.weapon;
		fx.aim_time = fx.time;
		fx.post_aim_weapon = nil;
		// Enough for now
		return;
	}
	// Stuck in aim procedure check?
	if (GetEffect("IntAimCheckProcedure", this) && !this->ReadyToAction()) 
		return ExecuteStand(fx);
	// Calculate offset to target. Take movement into account
	// Also aim for the head (y-4) so it's harder to evade by jumping
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY()-4;
	var d = Distance(x,y,tx,ty);
	var dt = d * 10 / fx.projectile_speed; // projected travel time of the arrow
	tx += GetTargetXDir(fx.target, dt);
	ty += GetTargetYDir(fx.target, dt);
	if (!fx.target->GetContact(-1)) if (!fx.target->GetCategory() & C4D_StaticBack) ty += GetGravity()*dt*dt/200;
	// Path to target free?
	if (PathFree(x,y,tx,ty))
	{
		// Get shooting angle
		var shooting_angle;
		if (fx.ranged_direct)
			shooting_angle = Angle(x, y, tx, ty, 10);
		else
			shooting_angle = GetBallisticAngle(tx-x, ty-y, fx.projectile_speed, 160);
		//Log("AI: Ranged attack calculated angle %d from coordinates (%d, %d) and (%d, %d)", shooting_angle, x, y, tx, ty); 
		if (GetType(shooting_angle) != C4V_Nil)
		{
			// No ally on path? Also search for allied animals, just in case.
			var ally;
			if (!fx.ignore_allies) ally = FindObject(Find_OnLine(0,0,tx-x,ty-y), Find_Exclude(this), Find_OCF(OCF_Alive), Find_Owner(GetOwner()));
			if (ally)
			{
				if (ExecuteJump()) return true;
				// can't jump and ally is in the way. just wait.
			}
			else
			{
				//Message("Bow @ %d!!!", shooting_angle);
				// Aim/Shoot there
				x = Sin(shooting_angle, 1000, 10);
				y = -Cos(shooting_angle, 1000, 10);
				fx.aim_weapon->ControlUseHolding(this, x,y);
				if (this->IsAiming() && fx.time >= fx.aim_time + fx.aim_wait)
				{
					//Log("Throw angle %v speed %v to reach %d %d", shooting_angle, fx.projectile_speed, tx-GetX(), ty-GetY());
					fx.aim_weapon->ControlUseStop(this, x,y);
					fx.post_aim_weapon = fx.aim_weapon; // assign post-aim status to allow slower shoot animations to pass
					fx.post_aim_weapon_time = FrameCounter();
					fx.aim_weapon = nil;
				}
				return true;
			}
		}
	}
	// Path not free or out of range. Just wait for enemy to come...
	//Message("Bow @ %s!!!", fx.target->GetName());
	fx.aim_weapon->ControlUseHolding(this,tx-x,ty-y);
	// Might also change target if current is unreachable
	var new_target;
	if (!Random(3)) if (new_target = FindTarget(fx)) fx.target = new_target;
	return true;
}

private func ExecuteLookAtTarget(fx)
{
	// set direction to look at target. can assume this is instantanuous
	var dir;
	if (fx.target->GetX() > GetX()) dir = DIR_Right; else dir = DIR_Left;
	SetDir(dir);
	return true;
}

private func ExecuteThrow(fx)
{
	// Still carrying the weapon to throw?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Path to target free?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	if (PathFree(x,y,tx,ty))
	{
		var throw_speed = this.ThrowSpeed;
		if (fx.weapon->GetID() == Javelin) throw_speed *= 2;
		var rx = (throw_speed*throw_speed)/(100*GetGravity()); // horizontal range for 45 degree throw if enemy is on same height as we are
		var ry = throw_speed*7/(GetGravity()*10); // vertical range of 45 degree throw
		var dx = tx-x, dy = ty-y+15*GetCon()/100; // distance to target. Reduce vertical distance a bit because throwing exit point is not at center
		// Check range
		// Could calculate the optimal parabulum here, but that's actually not very reliable on moving targets
		// It's usually better to throw straight at the target and only throw upwards a bit if the target stands on high ground or is far away
		// Also ignoring speed added by own velocity, etc...
		if (Abs(dx)*ry-Min(dy)*rx <= rx*ry)
		{
			// We're in range. Can throw?
			if (!CheckHandsAction(fx)) return true;
			// OK. Calc throwing direction
			dy -= dx*dx/rx; // big math!
			// And throw!
			//Message("Throw!");
			SetCommand("None"); SetComDir(COMD_Stop);
			SelectItem(fx.weapon);
			return this->ControlThrow(fx.weapon, dx, dy);
		}
	}
	// Can't reach target yet. Walk towards it.
	if (!GetCommand() || !Random(3)) SetCommand("MoveTo", fx.target);
	//Message("Throw %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

private func CheckHandsAction(fx)
{
	// can use hands?
	if (this->~HasHandAction()) return true;
	// Can't throw: Is it because e.g. we're scaling?
	if (!this->HasActionProcedure()) { ExecuteStand(fx); return false; }
	// Probably hands busy. Just wait.
	//Message("HandsBusy");
	return false;
}

private func ExecuteStand(fx)
{
	//Message("Stand");
	SetCommand("None");
	if (GetProcedure() == "SCALE")
	{
		var tx;
		if (fx.target) tx = fx.target->GetX() - GetX();
		// Scale: Either scale up if target is beyond this obstacle or let go if it's not
		if (GetDir()==DIR_Left)
		{
			if (tx < -20)
				SetComDir(COMD_Left);//ObjectControlMovement(GetOwner(), CON_Up, 100); // scale up
			else
				ObjectControlMovement(GetOwner(), CON_Right, 100); // let go
		}
		else
		{
			if (tx > -20)
				SetComDir(COMD_Right);//ObjectControlMovement(GetOwner(), CON_Up, 100); // scale up
			else
				ObjectControlMovement(GetOwner(), CON_Left, 100); // let go
		}
	}
	else if (GetProcedure() == "HANGLE")
	{
		ObjectControlMovement(GetOwner(), CON_Down, 100);
	}
	else
	{
		// Hm. What could it be? Let's just hope it resolves itself somehow...
		SetComDir(COMD_Stop);
		//Message("???");
	}
	return true;
}

private func ExecuteMelee(fx)
{
	// Still carrying the melee weapon?
	if (fx.weapon->Contained() != this) { fx.weapon=nil; return false; }
	// Are we in range?
	var x=GetX(), y=GetY(), tx=fx.target->GetX(), ty=fx.target->GetY();
	var dx = tx-x, dy = ty-y;
	if (Abs(dx) <= 10 && PathFree(x,y,tx,ty))
	{
		if (dy >= -15)
		{
			// target is under us - sword slash downwards!
			if (!CheckHandsAction(fx)) return true;
			// Stop here
			SetCommand("None"); SetComDir(COMD_None);
			// cooldown?
			if (!fx.weapon->CanStrikeWithWeapon(this))
			{
				//Message("MeleeWAIT %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
				// While waiting for the cooldown, we try to evade...
				ExecuteEvade(fx,dx,dy);
				return true;
			}
			// OK, slash!
			//Message("MeleeSLASH %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
			SelectItem(fx.weapon);
			return fx.weapon->ControlUse(this, tx,ty);
		}
		// Clonk is above us - jump there
		//Message("MeleeJump %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
		ExecuteJump();
		if (dx<-5) SetComDir(COMD_Left); else if (dx>5) SetComDir(COMD_Right); else SetComDir(COMD_None);
	}
	// Not in range. Walk there.
	if (!GetCommand() || !Random(10)) SetCommand("MoveTo", fx.target);
	//Message("Melee %s @ %s!!!", fx.weapon->GetName(), fx.target->GetName());
	return true;
}

private func ExecuteEvade(fx,int threat_dx,int threat_dy)
{
	// Evade from threat at position delta threat_*
	if (threat_dx < 0) SetComDir(COMD_Left); else SetComDir(COMD_Right);
	if (threat_dy >= -5 && !Random(2)) if (ExecuteJump(fx)) return true;
	// shield? todo
	return true;
}

private func ExecuteJump(fx)
{
	//LogCallStack("Jump");
	// Jump if standing on floor
	if (GetProcedure() == "WALK") // if (GetContact(-1, CNAT_Bottom)) - implied by walk
	{
		if (this->~ControlJump()) return true; // for clonks
		return this->Jump(); // for others
	}
	return false;
}

private func ExecuteArm(fx)
{
	// Find shield
	//if (fx.shield = FindContents(Shield)) this->SetHandItemPos(1, this->GetItemPos(fx.shield));
	fx.shield = FindContents(Shield);
	// Find a weapon. For now, just search own inventory
	if (FindInventoryWeapon(fx) && fx.weapon->Contained()==this)
	{
		SelectItem(fx.weapon);
		return true;
	}
	// no weapon :(
	return false;
}

private func FindInventoryWeapon(fx)
{
	fx.ammo_check = nil; fx.ranged = false;
	// Find weapon in inventory, mark it as equipped and set according strategy, etc.
	if (fx.weapon = fx.vehicle)
	{
		if (CheckVehicleAmmo(fx, fx.weapon))
			{ fx.strategy = fx.ai.ExecuteVehicle; fx.ranged=true; fx.aim_wait = 20; fx.ammo_check = fx.ai.CheckVehicleAmmo; return true; }
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(GrenadeLauncher))
	{
		if (HasBombs(fx, fx.weapon))
			{ fx.strategy = fx.ai.ExecuteRanged; fx.projectile_speed = 75; fx.aim_wait = 85; fx.ammo_check = fx.ai.HasBombs; fx.ranged=true; return true; }
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Bow))
	{
		if (HasArrows(fx, fx.weapon))
			{ fx.strategy = fx.ai.ExecuteRanged; fx.projectile_speed = 100; fx.aim_wait = 0; fx.ammo_check = fx.ai.HasArrows; fx.ranged=true; return true; }
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Musket))
	{
		if (HasAmmo(fx, fx.weapon))
			{ fx.strategy = fx.ai.ExecuteRanged; fx.projectile_speed = 200; fx.aim_wait = 85; fx.ammo_check = fx.ai.HasAmmo; fx.ranged=true; fx.ranged_direct = true; return true; }
		else
			fx.weapon = nil;
	}
	if (fx.weapon = FindContents(Javelin)) 
		{ fx.strategy = fx.ai.ExecuteRanged; fx.projectile_speed = this.ThrowSpeed*21/100; fx.aim_wait = 16; fx.ranged=true; return true; }
	if (fx.weapon = FindContents(Firestone)) 
		{ fx.strategy = fx.ai.ExecuteThrow; return true; }
	if (fx.weapon = FindContents(Rock)) 
		{ fx.strategy = fx.ai.ExecuteThrow; return true; }
	if (fx.weapon = FindContents(Lantern)) 
		{ fx.strategy = fx.ai.ExecuteThrow; return true; }
	if (fx.weapon = FindContents(Sword)) 
		{ fx.strategy = fx.ai.ExecuteMelee; return true; }
	//if (fx.weapon = Contents(0)) { fx.strategy = fx.ai.ExecuteThrow; return true; } - don't throw empty bow, etc.
	// no weapon :(
	return false;
}

private func HasArrows(fx, object weapon)
{
	if (weapon->Contents(0)) return true;
	if (FindObject(Find_Container(this), Find_Func("IsArrow"))) return true;
	return false;
}

private func HasAmmo(fx, object weapon)
{
	if (weapon->Contents(0)) return true;
	if (FindObject(Find_Container(this), Find_Func("IsMusketAmmo"))) return true;
	return false;
}

private func HasBombs(fx, object weapon)
{
	if (weapon->Contents(0)) return true;
	if (FindObject(Find_Container(this), Find_Func("IsGrenadeLauncherAmmo"))) return true;
	return false;
}

private func CheckVehicleAmmo(fx, object catapult)
{
	if (catapult->ContentsCount()) return true;
	// Vehicle out of ammo: Can't really be refilled. Stop using that weapon.
	fx.vehicle = nil;
	this->ObjectCommand("UnGrab");
	return false;
}

private func ExecuteIdle(fx)
{
	if (!Inside(GetX()-fx.home_x, -5,5) || !Inside(GetY()-fx.home_y, -15,15))
	{
		SetCommand("MoveTo", nil, fx.home_x, fx.home_y);
	}
	else
	{
		SetCommand("None"); SetComDir(COMD_Stop); SetDir(fx.home_dir);
	}
	//Message("zZz");
	return true;
}

private func FindTarget(fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(GetOwner());
	if (GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(GetOwner()));
	for (var target in FindObjects(Find_InRect(fx.guard_range.x-GetX(),fx.guard_range.y-GetY(),fx.guard_range.wdt,fx.guard_range.hgt), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Random()))
		if (PathFree(GetX(),GetY(),target->GetX(),target->GetY()))
			return target;
	// Nothing found.
	return;
}

private func FindEmergencyTarget(fx)
{
	// Consider hostile clonks, or all clonks if the AI does not have an owner.
	var hostile_criteria = Find_Hostile(GetOwner());
	if (GetOwner() == NO_OWNER)
		hostile_criteria = Find_Not(Find_Owner(GetOwner()));
	// Search nearest enemy clonk in area even if not in guard range, used e.g. when outside guard range (AI fell down) and being attacked.
	for (var target in FindObjects(Find_Distance(200), Find_OCF(OCF_CrewMember), hostile_criteria, Find_NoContainer(), Sort_Distance()))
		if (PathFree(GetX(),GetY(),target->GetX(),target->GetY()))
			return target;
	// Nothing found.
	return;
}

// Helper function: Convert target cordinates and bow out speed to desired shooting angle
// Because http://en.wikipedia.org/wiki/Trajectory_of_a_projectile says so
// No SimFlight checks to check upper angle (as that is really easy to evade anyway)
// just always shoot the lower angle if sight is free
private func GetBallisticAngle(int dx, int dy, int v, int max_angle)
{
	// v is in 1/10 pix/frame
	// gravity is in 1/100 pix/frame^2
	var g = GetGravity();
	// correct vertical distance to account for integration error
	// engine adds gravity after movement, so targets fly higher than they should
	// thus, we aim lower. we don't know the travel time yet, so we assume some 90% of v is horizontal
	// (it's ~2px correction for 200px shooting distance)
	dy += Abs(dx)*g*10/(v*180);
	//Log("Correction: Aiming %d lower!", Abs(dx)*q*10/(v*180));
	// q is in 1/10000 (pix/frame)^4
	var q = v**4 - g*(g*dx*dx-2*dy*v*v); // dy is negative up
	if (q<0) return nil; // out of range
	var a = (Angle(0, 0, g * dx, Sqrt(q) - v * v, 10) + 1800) % 3600 - 1800;
	// Check bounds
	if(!Inside(a, -10 * max_angle, 10 * max_angle)) return nil;
	return a;
}


/* Editor display */

// called in clonk context
func UpdateDebugDisplay(fx, int ignore_range_marker)
{
	if (fx.debug) CreateDebugDisplay(fx, ignore_range_marker);
	return true;
}

func EditCursorSelection(fx)
{
	// Object selected in editor. Show markers!
	return CreateDebugDisplay(fx);
}

func EditCursorDeselection(fx, object next_target)
{
	// Is the user manipulating markers? Then don't remove them!
	if (next_target && next_target->GetID() == RangeMarker) return true;
	// Otherwise, clear markers
	return RemoveDebugDisplay(fx);
}

// called in clonk context
func CreateDebugDisplay(fx, int ignore_range_marker)
{
	// clear previous
	if (fx.debug) RemoveDebugDisplay(fx, ignore_range_marker);
	// encounter message
	var msg = "";
	if (fx.encounter_cb) msg = Format("%s%v", msg, fx.encounter_cb);
	// contents message
	for (var i=0; i<ContentsCount(); ++i)
		msg = Format("%s{{%i}}", msg, Contents(i)->GetID());
	Message("@AI %s", msg);
	// draw ally alert range. draw as square, although a circle would be more appropriate
	if (!fx.debug) fx.debug = {};
	var clr;
	var x=GetX(), y=GetY();
	if (fx.ally_alert_range)
	{
		clr = 0xffffff00;
		fx.debug.a1 = DebugLine->Create(x,y-fx.ally_alert_range,x+fx.ally_alert_range,y,clr);
		fx.debug.a2 = DebugLine->Create(x+fx.ally_alert_range,y,x,y+fx.ally_alert_range,clr);
		fx.debug.a3 = DebugLine->Create(x,y+fx.ally_alert_range,x-fx.ally_alert_range,y,clr);
		fx.debug.a4 = DebugLine->Create(x-fx.ally_alert_range,y,x,y-fx.ally_alert_range,clr);
	}
	// draw guard range
	if (fx.guard_range.wdt && fx.guard_range.hgt)
	{
		clr = 0xffff0000;
		fx.debug.r1 = DebugLine->Create(fx.guard_range.x,fx.guard_range.y,fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y,clr);
		fx.debug.r2 = DebugLine->Create(fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y,fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y+fx.guard_range.hgt,clr);
		fx.debug.r3 = DebugLine->Create(fx.guard_range.x+fx.guard_range.wdt,fx.guard_range.y+fx.guard_range.hgt,fx.guard_range.x,fx.guard_range.y+fx.guard_range.hgt,clr);
		fx.debug.r4 = DebugLine->Create(fx.guard_range.x,fx.guard_range.y+fx.guard_range.hgt,fx.guard_range.x,fx.guard_range.y,clr);
		if (!fx.debug.rm1) fx.debug.rm1 = RangeMarker->Create(fx.guard_range.x                   , fx.guard_range.y                   ,  0, this, AI.ChangeGuardRange, 0);
		if (!fx.debug.rm2) fx.debug.rm2 = RangeMarker->Create(fx.guard_range.x+fx.guard_range.wdt, fx.guard_range.y                   , 90, this, AI.ChangeGuardRange, 1);
		if (!fx.debug.rm3) fx.debug.rm3 = RangeMarker->Create(fx.guard_range.x+fx.guard_range.wdt, fx.guard_range.y+fx.guard_range.hgt,180, this, AI.ChangeGuardRange, 2);
		if (!fx.debug.rm4) fx.debug.rm4 = RangeMarker->Create(fx.guard_range.x                   , fx.guard_range.y+fx.guard_range.hgt,270, this, AI.ChangeGuardRange, 3);
	}
	return true;
}

func ChangeGuardRange(int corner_idx, int x, int y)
{
	var fx = this.ai;
	if (!fx) return false;
	if (corner_idx == 0 || corner_idx == 3)
		{ fx.guard_range.wdt += fx.guard_range.x - x; fx.guard_range.x = x; }
	else
		{ fx.guard_range.wdt = x - fx.guard_range.x; }
	if (corner_idx < 2)
		{ fx.guard_range.hgt += fx.guard_range.y - y; fx.guard_range.y = y; }
	else
		{ fx.guard_range.hgt = y - fx.guard_range.y; }
	Call(fx.ai.UpdateDebugDisplay, fx, corner_idx+1);
	return true;
}

// called in clonk context
func RemoveDebugDisplay(fx, int ignore_range_marker)
{
	// Remove any marker objects
	if (fx.debug)
	{
		if (fx.debug.r1) fx.debug.r1->RemoveObject();
		if (fx.debug.r2) fx.debug.r2->RemoveObject();
		if (fx.debug.r3) fx.debug.r3->RemoveObject();
		if (fx.debug.r4) fx.debug.r4->RemoveObject();
		if (ignore_range_marker != 1 && fx.debug.rm1) fx.debug.rm1->RemoveObject();
		if (ignore_range_marker != 2 && fx.debug.rm2) fx.debug.rm2->RemoveObject();
		if (ignore_range_marker != 3 && fx.debug.rm3) fx.debug.rm3->RemoveObject();
		if (ignore_range_marker != 4 && fx.debug.rm4) fx.debug.rm4->RemoveObject();
		if (fx.debug.a1) fx.debug.a1->RemoveObject();
		if (fx.debug.a2) fx.debug.a2->RemoveObject();
		if (fx.debug.a3) fx.debug.a3->RemoveObject();
		if (fx.debug.a4) fx.debug.a4->RemoveObject();
		Message("");
	}
	if (!ignore_range_marker) fx.debug = nil;
	return true;
}
