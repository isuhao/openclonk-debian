// Executes melee attacks on target.

#appendto Clonk

protected func AI_MeleeAttack(object clonk, int x, int y, object target)
{
	clonk->AI_LogCommandStack();
	clonk->AI_Log("Melee attack on %s", target->GetName());
	// target in range? Only melee attacks on relatively nearby targets.
	if (ObjectDistance(clonk, target) > 100 || target->Contained())
		return clonk->AI_Log("Target %s out of range", target->GetName());
		
	// target still alive?
	if (!(target->GetOCF() & OCF_Alive))
		return clonk->AI_Log("Target %s has died", target->GetName());
		
	// Look for melee weapon in contents.
	for (var weapon in FindObjects(Find_Container(clonk), Find_Func("AI_IsMeleeWeapon"), Find_Func("AI_CanStrike")))
	{
		// Check if weapon can hit the target.
		if (weapon->~AI_TargetInRange(target))
		{
			clonk->AI_Log("Use melee weapon %s", weapon->GetName());
			clonk->AppendCommand("Call", weapon, nil, nil, target, nil, weapon->AI_CommandString()); 
			return;
		}		
	}
	
	// Maybe a melee weapon can be found nearby.
	if (!AI_HasMeleeWeapon())
		for (var weapon in FindObjects(Find_Distance(100), Find_Func("AI_IsMeleeWeapon"), Find_Not(Find_Container(clonk))))
		{
			clonk->AI_Log("Search for melee weapon %s", weapon->GetName());
			clonk->AppendCommand("Get", weapon);
			clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_MeleeAttack");
			return;
		}
		
	// Move to target
	var angle = Angle(clonk->GetX(), clonk->GetY(), target->GetX(), target->GetY());
	clonk->AI_Log("Move to position (%d,%d)", target->GetX() - Sin(angle, 5), clonk->GetY() /*+Cos(angle, 10)*/);
	clonk->AppendCommand("MoveTo", nil, target->GetX() - Sin(angle, 5), clonk->GetY() /*+ Cos(angle, 10)*/);
	clonk->AppendCommand("Call", clonk, nil, nil, target, nil, "AI_MeleeAttack");
	
	// Return to basic attacks.

	return;
}

protected func AI_HasMeleeWeapon()
{
	return !!FindObject(Find_Container(this), Find_Func("AI_IsMeleeWeapon"));
}