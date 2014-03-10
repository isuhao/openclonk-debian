/**
	HUD ExtraSlot		
	Little spin-off of the big Object selector
	Is shown for ExtraSlots for weapons with munition etc.
	
	@authors Newton
*/

local container, myobject, crew;

protected func Construction()
{
	myobject = nil;
	container = nil;
	crew = nil;
	
	// parallaxity
	this.Parallaxity = [0,0];	
	// visibility
	this.Visibility = VIS_None;	
	// mouse drag
	this.MouseDrag = MD_DragSource | MD_DropTarget;
}

public func MouseSelectionAlt(int plr)
{
	if(!myobject) return;
	
	var desc = myobject.UsageHelp;
	if(!desc) desc = myobject.Description; // fall back to general description
	
	// close other messages...
	crew->OnDisplayInfoMessage();
	
	if(desc)
	{
		var msg = Format("<c ff0000>%s</c>",desc);
		CustomMessage(msg,this,plr);
	}
	return true;
}

public func OnMouseDragDone(obj, object target)
{
	// not on landscape
	if(target) return;
	
	if(obj == myobject)
		obj->Exit();
}

public func OnMouseDrag(int plr)
{
	if(plr != GetOwner()) return nil;
	
	return myobject;
}

public func OnMouseDrop(int plr, obj)
{
	if(plr != GetOwner()) return false;
	if(GetType(obj) != C4V_C4Object) return false;

	// a collectible object
	if(obj->GetOCF() & OCF_Collectible)
	{
		var objcontainer = obj->Contained();
		
		// already full: switch places with other object
		if(myobject != nil)
		{
			var myoldobject = myobject;
			
			// 1. exit my old object
			myoldobject->Exit();
			// 2. try to collect new obj
			if(container->Collect(obj,true))
			{
				// 3. try to enter my old object into other container
				if(!(objcontainer->Collect(myoldobject,true)))
					// otherwise recollect my old object
					container->Collect(myoldobject,true);
			}
			else
				// otherwise recollect my old object
				container->Collect(myoldobject,true);
		}
		// otherwise, just collect
		else
		{
				container->Collect(obj,true);
		}
	}
	return true;
}

public func SetObject(object obj)
{

	this.Visibility = VIS_Owner;

	myobject = obj;
	
	RemoveEffect("IntRemoveGuard",myobject);
	
	if(!myobject)
	{
		SetGraphics(nil,nil,1);
		SetName("$TxtEmpty$");
		this.MouseDragImage = nil;
	}
	else
	{
		SetGraphics(nil,nil,1,GFXOV_MODE_ObjectPicture,nil,0,myobject);
		this.MouseDragImage = myobject;
		
		SetName(myobject->GetName());
		
		// create an effect which monitors whether the object is removed
		AddEffect("IntRemoveGuard",myobject,1,0,this);
	}

}

public func FxIntRemoveGuardStop(object target, effect, int reason, bool temp)
{
	if(reason == 3)
		if(target == myobject)
			SetObject(nil);
}

public func Update()
{
	SetObject(container->Contents());
}

public func SetContainer(object c)
{
	if(container == c) return;
	
	container = c;
	container->~SetHUDObject(this);
	
	Update();
	
	this["Visibility"] = VIS_Owner;
}

public func SetCrew(object c)
{
	if(crew == c) return;
	crew = c;
}

public func GetCrew() { return crew; }
