/*
	Standard clonk controls
	Author: Newton
	
	This object provides handling of the clonk controls including item
	management, backpack controls and standard throwing behaviour. It
	should be included into any clonk/crew definition.
	The controls in System.ocg/PlayerControl.c only provide basic movement
	handling, namely the movement left, right, up and down. The rest is
	handled here:
	Grabbing, ungrabbing, shifting and pushing vehicles into buildings;
	entering and exiting buildings; throwing, dropping; backpack control,
	(object) menu control, hotkey controls, usage and it's callbacks and
	forwards to script. Also handled by this library is the aiming with
	the gamepad conrols.
	
	Objects that inherit this object need to return _inherited() in the
	following callbacks (if defined):
		Construction, Collection2, Ejection, RejectCollect, Departure,
		Entrance, AttachTargetLost, CrewSelection, Death,
		Destruction, OnActionChanged
	
	The following callbacks are made to other objects:
		*Stop
		*Left, *Right, *Up, *Down
		*Use, *UseStop, *UseStart, *UseHolding, *UseCancel
	wheras * is 'Contained' if the clonk is contained and otherwise (riding,
	pushing, to self) it is 'Control'. The item in the inventory only gets
	the Use*-calls. If the callback is handled, you should return true.
	Currently, this is explained more in detail here:
	http://forum.openclonk.org/topic_show.pl?tid=337
	
	The inventory management:
	The objects in the inventory are saved (parallel to Contents()) in the
	array 'inventory'. They are accessed via GetItem(i) and GetItemPos(obj).
	Other functions are MaxContentsCount() (defines the maximum number of
	contents)
	
	Furthermore the clonk has a defined amount of "hands", defined by HandObjects().
	The array 'use_objects' is a mapping of "hands" onto the inventory-slots.
	The functions GetHandItem(i) returns the object in the "i"th hand.
	
	Carryheavy is also handled here. When a clonk picks up a carryheavy object
	it's saved in 'carryheavy'
*/

/* ++++++++++++++++++++++++ Clonk Inventory Control ++++++++++++++++++++++++ */

local inventory;	// items in the inventory, array
local carryheavy;	// object beeing carried with carryheavy
local use_objects;	// hand-slots (mapping onto inventory)

local handslot_choice_pending;	// used to determine if a slot-hotkey (1-9) has already been handled by a mouseclick
local hotkeypressed;			// used to determine if an interaction has already been handled by a hotkey (space + 1-9)

local disableautosort;		// used to get default-Collection-behaviour (see Collection2)
local force_collection;		// used to pick stuff up, even though the hand-slots are all full (see RejectCollect + Collect with CON_Collect)
local forced_ejection;		// used to recognize if an object was thrown out with or without the force-key (Shift). If not, next hand slot will be selected. 

/* Item limit */

private func HandObjects() { return 2; }			// How many "Hands" the clonk has
public func MaxContentsCount() { return 7; }		// Size of the clonks inventory
public func NoStackedContentMenu() { return true; }	// Contents-Menu shall display each object in a seperate slot


/** Get the 'i'th item in the inventory */
public func GetItem(int i)
{
	if (i >= GetLength(inventory))
		return nil;
	if (i < 0) return nil;
		
	return inventory[i];
}

/** Returns all items in the inventory */
public func GetItems()
{
	var inv = inventory[:];
	RemoveHoles(inv);
	return inv;
}

/** Returns how many items are in the clonks inventory
    Does not have to be the same as ContentCounts() because of objects with special handling, like CarryHeavy */
public func GetItemCount()
{
	var count = 0;
	for(var i=0; i < GetLength(inventory); i++)
		if(inventory[i])
			count++;
	
	return count;
}

/** Get the 'i'th item in hands.
    These are the items that will be used with use-commands. (Left mouse click, etc...) */
public func GetHandItem(int i)
{
	// i is valid range
	if (i >= GetLength(use_objects))
		return nil;
	if (i < 0) return nil;

	// carrying a carry heavy item always returns said item. (he holds it in both hands, after all!)
	if (IsCarryingHeavy())
		return GetCarryHeavy();
		
	return GetItem(use_objects[i]);
}

/** Set the 'hand'th use-item to the 'inv'th slot */
public func SetHandItemPos(int hand, int inv)
{
	// indices are in range?	
	if(hand >= HandObjects() || inv >= this->MaxContentsCount())
		return nil;
	if(hand < 0 || inv < 0) return nil;

	// can't use anything except carryheavy if carrying heavy object.
	if(IsCarryingHeavy())
		return nil;

	// changing slots cancels using, if the slot with the used object is contained
	if(using)
	{
		var used_slot = GetItemPos(using);
		if(used_slot != nil)
			if(used_slot == GetHandItemPos(hand) || used_slot == inv)
				CancelUseControl(0,0);
	}

	// If the item is already selected, we can't hold it in another one too.
	var hand2 = GetHandPosByItemPos(inv);
	if(hand2 != nil)
	{
		// switch places
		use_objects[hand2] = use_objects[hand];
		use_objects[hand] = inv;
		
		// additional callbacks
		var hand_item;
		if(hand_item = GetHandItem(hand2))
		{
			this->~OnSlotFull(hand2);
			// OnSlotFull might have done something to the item
			if(GetHandItem(hand2) == hand_item)
				hand_item->~Selection(this, hand2);
		}
		else
			this->~OnSlotEmpty(hand2);
	}
	else
		use_objects[hand] = inv;
	
	// call callbacks
	var item;
	if(item = GetItem(inv))
	{
		this->~OnSlotFull(hand);
		// OnSlotFull might have done something to the item
		if(GetItem(inv) == item)
			GetItem(inv)->~Selection(this, hand);
	}
	else
	{
		this->~OnSlotEmpty(hand);
	}
	
	handslot_choice_pending = false;
}

/** Returns the position in the inventory of the 'i'th use item */
public func GetHandItemPos(int i)
{
	if (i >= GetLength(use_objects))
		return nil;
	if (i < 0) return nil;
	
	return use_objects[i];
}

/** Returns in which hand-slot the inventory-slot is */
private func GetHandPosByItemPos(int o) // sorry for the horribly long name --boni
{
	for(var i=0; i < GetLength(use_objects); i++)
		if(use_objects[i] == o)
			return i;
	
	return nil;
}
 
/** Drops the item in the inventory slot, if any */
public func DropInventoryItem(int slot)
{
	var obj = GetItem(slot);
	if(!obj)
		return nil;
	
	this->SetCommand("Drop",obj);
}

/** Search for the index of an item */
public func GetItemPos(object item)
{
	if (item)
		if (item->Contained() == this)
		{
			var i = 0;
			for(var obj in inventory)
			{
				if (obj == item) return i;
				++i;
			}
		}
	return nil;
}

/** Switch two items in the clonk's inventory */

public func Switch2Items(int one, int two)
{
	// no valid inventory index: cancel
	if (!Inside(one,0,this->MaxContentsCount()-1)) return;
	if (!Inside(two,0,this->MaxContentsCount()-1)) return;

	// switch them around
	var temp = inventory[one];
	inventory[one] = inventory[two];
	inventory[two] = temp;
	
	// callbacks: cancel use
	if (using == inventory[one] || using == inventory[two])
		CancelUse();
	
	var handone, handtwo;
	handone = GetHandPosByItemPos(one);
	handtwo = GetHandPosByItemPos(two);
	
	// callbacks: (de)selection
	if (handone != nil)
		if (inventory[two]) inventory[two]->~Deselection(this,one);
	if (handtwo != nil)
		if (inventory[one]) inventory[one]->~Deselection(this,two);
		
	if (handone != nil)
		if (inventory[one]) inventory[one]->~Selection(this,one);
	if (handtwo != nil)
		if (inventory[two]) inventory[two]->~Selection(this,two);
	
	// callbacks: to self, for HUD
	if (handone != nil)
	{
		if (inventory[one])
			this->~OnSlotFull(handone);
		else
			this->~OnSlotEmpty(handone);
	}
	if (handtwo != nil)
	{
		if (inventory[two])
			this->~OnSlotFull(handtwo);
		else
			this->~OnSlotEmpty(handtwo);
	}
	
	this->~OnInventoryChange(one, two);
}

/* Overload of Collect function
   Allows inventory/hands-Handling with forced-collection
*/
public func Collect(object item, bool ignoreOCF, int pos, bool force)
{
	force_collection = force;
	var success = false;
	if (pos == nil || item->~IsCarryHeavy())
	{
		success = _inherited(item,ignoreOCF);
		force_collection = false;
		return success;
	}
	// fail if the specified slot is full
	if (GetItem(pos) == nil && pos >= 0 && pos < this->MaxContentsCount())
	{
		if (item)
		{
			disableautosort = true;
			// collect but do not sort in_
			// Collection2 will be called which attempts to automatically sort in
			// the collected item into the next free inventory slot. Since 'pos'
			// is given as a parameter, we don't want that to happen and sort it
			// in manually afterwards
			var success = _inherited(item);
			disableautosort = false;
			if (success)
			{
				inventory[pos] = item;
				var handpos = GetHandPosByItemPos(pos); 
				// if the slot was a selected hand slot -> update it
				if(handpos != nil)
				{
					this->~OnSlotFull(handpos);
				}
			}
		}
	}
	
	force_collection = false;
	return success;
}

/* ################################################# */

protected func Construction()
{
	menu = nil;

	// inventory variables
	inventory = CreateArray();
	use_objects = CreateArray();

	for(var i=0; i < HandObjects(); i++)
		use_objects[i] = i;

	force_collection = false;
	
	// using variables
	alt = false;
	using = nil;
	using_type = nil;
	
	return _inherited(...);
}

protected func Collection2(object obj)
{
	// carryheavy object gets special treatment
	if(obj->~IsCarryHeavy()) // we can assume that we don't have a carryheavy object yet. If we do, Scripters are to blame.
	{
		if(obj != GetCarryHeavy())
			CarryHeavy(obj);
		
		return true;
	}
	
	var sel = 0;

	// See Collect()
	if (disableautosort) return _inherited(obj,...);
	
	var success = false;
	var i;
	
	// sort into selected hands if empty
	for(i = 0; i < HandObjects(); i++)
		if(!GetHandItem(i))
		{
			sel = GetHandItemPos(i);
			inventory[sel] = obj;
			success = true;
			break;
		}
		
	// otherwise, first empty slot
	if(!success)
	{
		for(var i = 0; i < this->MaxContentsCount(); ++i)
		{
			if (!GetItem(i))
			{
				sel = i;
				inventory[sel] = obj;
				success = true;
				break;
			}
		}
	}
	
	// callbacks
	if (success)
	{
		var handpos = GetHandPosByItemPos(sel); 
		// if the slot was a selected hand slot -> update it
		if(handpos != nil)
		{
			this->~OnSlotFull(handpos);
			// OnSlotFull might have done something to obj
			if(GetHandItem(handpos) == obj)
				obj->~Selection(this, handpos);
		}
	}
		

	return _inherited(obj,...);
}

protected func Ejection(object obj)
{
	// carry heavy special treatment
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
		return true;
	}

	// if an object leaves this object
	// find obj in array and delete (cancel using too)
	var i = 0;
	var success = false;
	
	for(var item in inventory)
    {
	   if (obj == item)
	   {
			inventory[i] = nil;
			success = true;
			break;
	   }
	   ++i;
    }
	if (using == obj) CancelUse();

	// callbacks
	if (success)
	{
		var handpos = GetHandPosByItemPos(i); 
		// if the slot was a selected hand slot -> update it
		if(handpos != nil)
		{
			// if it was a forced ejection, the hand will remain empty
			if(forced_ejection == obj)
			{
				this->~OnSlotEmpty(handpos);
				obj->~Deselection(this, handpos);
			}
			// else we'll select the next full slot
			else
			{
				// look for following non-selected non-free slots
				var found_slot = false;
				for(var j=i; j < this->MaxContentsCount(); j++)
					if(GetItem(j) && !GetHandPosByItemPos(j))
					{
						found_slot = true;
						break;
					}
				
				if(found_slot)
					SetHandItemPos(handpos, j); // SetHandItemPos handles the missing callbacks
				// no full next slot could be found. we'll stay at the same, and empty.
				else
				{
					this->~OnSlotEmpty(handpos);
					obj->~Deselection(this, handpos);
				}
			}
		}
	}
	
	// we have over-weight? Put the next unindexed object inside that slot
	// this happens if the clonk is stuffed full with items he can not
	// carry via Enter, CreateContents etc.
	var inventory_count = 0;
	for(var io in inventory)
		if(io != nil)
			inventory_count++;
			
	if (ContentsCount() > inventory_count && !GetItem(i))
	{
		for(var c = 0; c < ContentsCount(); ++c)
		{
			var o = Contents(c);
			if(o->~IsCarryHeavy())
				continue;
			if (GetItemPos(o) == nil)
			{
				// found it! Collect it properly
				inventory[i] = o;
				
				var handpos = GetHandPosByItemPos(i); 
				// if the slot was a selected hand slot -> update it
				if(handpos != nil)
				{
					this->~OnSlotFull(handpos);
					// OnSlotFull might have done something to o
					if(GetHandItem(handpos) == o)
						o->~Selection(this, handpos);
				}
					
				break;
			}
		}
	}
	
	_inherited(obj,...);
}

protected func ContentsDestruction(object obj)
{
	// tell the Hud that something changed
	this->~OnInventoryChange();
	
	// check if it was carryheavy
	if(obj == GetCarryHeavy())
	{
		StopCarryHeavy();
	}

	_inherited(...);
}

protected func RejectCollect(id objid, object obj)
{
	// collection of that object magically disabled?
	if(GetEffect("NoCollection", obj)) return true;

	// Carry heavy only gets picked up if none held already
	if(obj->~IsCarryHeavy())
	{
		if(IsCarryingHeavy())
			return true;
		else
		{
			return false;
		}
	}

	//No collection if the clonk is carrying a 'carry-heavy' object
	// todo: does this still make sense with carry heavy not beeing in inventory and new inventory in general?
	if(IsCarryingHeavy() && !force_collection) return true;
	
	// try to stuff obj into an object with an extra slot
	for(var i=0; Contents(i); ++i)
		if (Contents(i)->~HasExtraSlot())
			if (!(Contents(i)->Contents(0)))
				if (Contents(i)->Collect(obj,true))
					return true;
					
	// try to stuff an object in clonk into obj if it has an extra slot
	if (obj->~HasExtraSlot())
		if (!(obj->Contents(0)))
			for(var i=0; Contents(i); ++i)
				if (obj->Collect(Contents(i),true))
					return false;
			

	// check max contents
	if (ContentsCount() >= this->MaxContentsCount()) return true;

	// check if the two first slots are full. If the overloaded
	// Collect() is called, this check will be skipped
	if (!force_collection)
		if (GetHandItem(0) && GetHandItem(1))
			return true;
	
	return _inherited(objid,obj,...);
}

// To allow transfer of contents which are not allowed through collection.
public func AllowTransfer(object obj)
{
	// Only check max contents.
	if (GetItemCount() >= this->MaxContentsCount()) 
		return false;

	// don't allow picking up multiple carryheavy-objects
	if(IsCarryingHeavy() && obj->~IsCarryHeavy())
		return false;

	return true;
}

public func GetUsedObject() { return using; }



/* Carry heavy stuff */

/** Tells the clonk that he is carrying the given carry heavy object */
public func CarryHeavy(object target)
{
	if(!target)
		return;
	// actually.. is it a carry heavy object?
	if(!target->~IsCarryHeavy())
		return;
	// only if not carrying a heavy objcet already
	if(IsCarryingHeavy())
		return;	
	
	carryheavy = target;
	
	if(target->Contained() != this)
		target->Enter(this);
	
	// notify UI about carryheavy pickup
	this->~OnCarryHeavyChange(carryheavy);
	
	// Update attach stuff
	this->~OnSlotFull();
	
	return true;
}

/** Drops the carried heavy object, if any */
public func DropCarryHeavy()
{
	// only if actually possible
	if(!IsCarryingHeavy())
		return;
	
	GetCarryHeavy()->Drop();
	StopCarryHeavy();
	
	return true;
}

// Internal function to clear carryheavy-status
private func StopCarryHeavy()
{
	if(!IsCarryingHeavy())
		return;
	
	carryheavy = nil;
	this->~OnCarryHeavyChange(nil);
	this->~OnSlotEmpty();
}

public func GetCarryHeavy() { return carryheavy; }

public func IsCarryingHeavy() { return carryheavy != nil; }

/* ################################################# */

// The using-command hast to be canceled if the clonk is entered into
// or exited from a building.

protected func Entrance()         { CancelUse(); return _inherited(...); }
protected func Departure()        { CancelUse(); return _inherited(...); }

// The same for vehicles
protected func AttachTargetLost() { CancelUse(); return _inherited(...); }

// ...aaand the same for when the clonk is deselected
protected func CrewSelection(bool unselect)
{
	if (unselect)
	{
		// cancel usage on unselect first...
		CancelUse();
		// and if there is still a menu, cancel it too...
		CancelMenu();
	}
	return _inherited(unselect,...);
}

protected func Destruction()
{
	// close open menus, cancel usage...
	CancelUse();
	CancelMenu();
	return _inherited(...);
}

protected func Death()
{
	// close open menus, cancel usage...
	CancelUse();
	CancelMenu();
	return _inherited(...);
}

protected func OnActionChanged(string oldaction)
{
	var old_act = this["ActMap"][oldaction];
	var act = this["ActMap"][GetAction()];
	var old_proc = 0;
	if(old_act) old_proc = old_act["Procedure"];
	var proc = 0;
	if(act) proc = act["Procedure"];
	// if the object's procedure has changed from a non Push/Attach
	// to a Push/Attach action or the other way round, the usage needs
	// to be cancelled
	if (proc != old_proc)
	{
		if (proc == DFA_PUSH || proc == DFA_ATTACH
		 || old_proc == DFA_PUSH || old_proc == DFA_ATTACH)
		{
			CancelUse();
		}
	}
	return _inherited(oldaction,...);
}

/** Returns additional interactions the clonk possesses as an array of function pointers.
	Returned Proplist contains:
		Fn			= Name of the function to call
		Object		= object to call the function in. Will also be displayed on the interaction-button
		Description	= a description of what the interaction does
		IconID		= ID of the definition that contains the icon (like GetInteractionMetaInfo)
		IconName	= Namo of the graphic for teh icon (like GetInteractionMetaInfo)
		[Priority]	= Where to sort in in the interaction-list. 0=front, 1=after script, 2=after vehicles, >=3=at the end, nil equals 3
*/
public func GetExtraInteractions()
{
	var functions = CreateArray();
	
	// flipping construction-preview
	var effect;
	if(effect = GetEffect("ControlConstructionPreview", this))
	{
		if(effect.flipable)
			PushBack(functions, {Fn = "Flip", Description=ConstructionPreviewer->GetFlipDescription(), Object=effect.preview, IconID=ConstructionPreviewer_IconFlip, Priority=0});
	}
	
	// dropping carry heavy
	if(IsCarryingHeavy() && GetAction() == "Walk")
	{
		var ch = GetCarryHeavy();
		PushBack(functions, {Fn = "Drop", Description=ch->GetDropDescription(), Object=ch, IconName="LetGo", IconID=GUI_ObjectSelector, Priority=1});
	}
	
	return functions;
}

/* +++++++++++++++++++++++++++ Clonk Control +++++++++++++++++++++++++++ */

local using, using_type;
local alt;
local mlastx, mlasty;
local virtual_cursor;
local noholdingcallbacks;

/* Main control function */
public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) 
		return false;
	
	//Log(Format("%d, %d, %s, strength: %d, repeat: %v, release: %v",  x,y,GetPlayerControlName(ctrl), strength, repeat, release),this);
	
	// some controls should only do something on release (everything that has to do with interaction)
	if(ctrl == CON_Interact || ctrl == CON_PushEnter || ctrl == CON_Ungrab || ctrl == CON_GrabNext || ctrl == CON_Grab || ctrl == CON_Enter || ctrl == CON_Exit)
	{
		if(!release)
		{
			// this is needed to reset the hotkey-memory
			hotkeypressed = false;
			return false;
		}
		// if the interaction-command has already been handled by a hotkey (else it'd double-interact)
		else if(hotkeypressed)
			return false;
		// check if we can handle it by simply accessing the first actionbar item (for consistency)
		else
		{
			if(GetMenu())
				if(!GetMenu()->~Uncloseable())
					return CancelMenu();
			
			if(this->~ControlHotkey(0))
					return true;
		}
	}
	
	// Contents menu
	if (ctrl == CON_Contents)
	{
		// Close any menu if open.
		if (GetMenu())
		{
			// Uncloseable menu?
			if (GetMenu()->~Uncloseable()) return true;

			var is_content = GetMenu()->~IsContentMenu();
			CancelMenu();
			// If contents menu, don't open new one and return.
			if (is_content)
				return true;
		}
		// Open contents menu.
		CancelUse();
		CreateContentsMenus();
		// CreateContentsMenus calls SetMenu(this) in the clonk
		// so after this call menu = the created menu
		if(GetMenu())
			GetMenu()->Show();		
		return true;
	}
	
	/* aiming with mouse:
	   The CON_Aim control is transformed into a use command. Con_Use if
	   repeated does not bear the updated x,y coordinates, that's why this
	   other control needs to be issued and transformed. CON_Aim is a
	   control which is issued on mouse move but disabled when not aiming
	   or when HoldingEnabled() of the used item does not return true.
	   For the rest of the control code, it looks like the x,y coordinates
	   came from CON_Use.
	  */
	if (using && ctrl == CON_Aim)
	{
		if (alt) ctrl = CON_UseAlt;
		else     ctrl = CON_Use;
				
		repeat = true;
		release = false;
	}
	// controls except a few reset a previously given command
	else SetCommand("None");
	
	/* aiming with analog pad or keys:
	   This works completely different. There are CON_AimAxis* and CON_Aim*,
	   both work about the same. A virtual cursor is created which moves in a
	   circle around the clonk and is controlled via these CON_Aim* functions.
	   CON_Aim* is normally on the same buttons as the movement and has a
	   higher priority, thus is called first. The aim is always done, even if
	   the clonk is not aiming. However this returns only true (=handled) if
	   the clonk is really aiming. So if the clonk is not aiming, the virtual
	   cursor aims into the direction in which the clonk is running and e.g.
	   CON_Left is still called afterwards. So if the clonk finally starts to
	   aim, the virtual cursor already aims into the direction in which he ran
	*/
	if (ctrl == CON_AimAxisUp || ctrl == CON_AimAxisDown || ctrl == CON_AimAxisLeft || ctrl == CON_AimAxisRight
	 || ctrl == CON_AimUp     || ctrl == CON_AimDown     || ctrl == CON_AimLeft     || ctrl == CON_AimRight)
	{
		var success = VirtualCursor()->Aim(ctrl,this,strength,repeat,release);
		// in any case, CON_Aim* is called but it is only successful if the virtual cursor is aiming
		return success && VirtualCursor()->IsAiming();
	}
	
	// save last mouse position:
	// if the using has to be canceled, no information about the current x,y
	// is available. Thus, the last x,y position needs to be saved
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		mlastx = x;
		mlasty = y;
	}
	
	// hotkeys (inventory, vehicle and structure control)
	var hot = 0;
	if (ctrl == CON_InteractionHotkey0) hot = 10;
	if (ctrl == CON_InteractionHotkey1) hot = 1;
	if (ctrl == CON_InteractionHotkey2) hot = 2;
	if (ctrl == CON_InteractionHotkey3) hot = 3;
	if (ctrl == CON_InteractionHotkey4) hot = 4;
	if (ctrl == CON_InteractionHotkey5) hot = 5;
	if (ctrl == CON_InteractionHotkey6) hot = 6;
	if (ctrl == CON_InteractionHotkey7) hot = 7;
	if (ctrl == CON_InteractionHotkey8) hot = 8;
	if (ctrl == CON_InteractionHotkey9) hot = 9;
	
	if (hot > 0)
	{
		hotkeypressed = true;
		this->~ControlHotkey(hot-1);
		return true;
	}
	
	// dropping items via hotkey
	hot = 0;
	if (ctrl == CON_DropHotkey0) hot = 10;
	if (ctrl == CON_DropHotkey1) hot = 1;
	if (ctrl == CON_DropHotkey2) hot = 2;
	if (ctrl == CON_DropHotkey3) hot = 3;
	if (ctrl == CON_DropHotkey4) hot = 4;
	if (ctrl == CON_DropHotkey5) hot = 5;
	if (ctrl == CON_DropHotkey6) hot = 6;
	if (ctrl == CON_DropHotkey7) hot = 7;
	if (ctrl == CON_DropHotkey8) hot = 8;
	if (ctrl == CON_DropHotkey9) hot = 9;
	
	if (hot > 0)
	{
		this->~DropInventoryItem(hot-1);
		return true;
	}
	
	// this wall of text is called when 1-0 is beeing held, and left or right mouse button is pressed.
	var hand = 0;
	hot = 0;
	if (ctrl == CON_Hotkey0Select) hot = 10;
	if (ctrl == CON_Hotkey1Select) hot = 1;
	if (ctrl == CON_Hotkey2Select) hot = 2;
	if (ctrl == CON_Hotkey3Select) hot = 3;
	if (ctrl == CON_Hotkey4Select) hot = 4;
	if (ctrl == CON_Hotkey5Select) hot = 5;
	if (ctrl == CON_Hotkey6Select) hot = 6;
	if (ctrl == CON_Hotkey7Select) hot = 7;
	if (ctrl == CON_Hotkey8Select) hot = 8;
	if (ctrl == CON_Hotkey9Select) hot = 9;
	if (ctrl == CON_Hotkey0SelectAlt) {hot = 10; hand=1; }
	if (ctrl == CON_Hotkey1SelectAlt) {hot = 1; hand=1; }
	if (ctrl == CON_Hotkey2SelectAlt) {hot = 2; hand=1; }
	if (ctrl == CON_Hotkey3SelectAlt) {hot = 3; hand=1; }
	if (ctrl == CON_Hotkey4SelectAlt) {hot = 4; hand=1; }
	if (ctrl == CON_Hotkey5SelectAlt) {hot = 5; hand=1; }
	if (ctrl == CON_Hotkey6SelectAlt) {hot = 6; hand=1; }
	if (ctrl == CON_Hotkey7SelectAlt) {hot = 7; hand=1; }
	if (ctrl == CON_Hotkey8SelectAlt) {hot = 8; hand=1; }
	if (ctrl == CON_Hotkey9SelectAlt) {hot = 9; hand=1; }
	
	if(hot > 0  && hot <= this->MaxContentsCount())
	{
		SetHandItemPos(hand, hot-1);
		this->~OnInventoryHotkeyRelease(hot-1);
		return true;
	}
	
	// inventory
	hot = 0;
	if (ctrl == CON_Hotkey0) hot = 10;
	if (ctrl == CON_Hotkey1) hot = 1;
	if (ctrl == CON_Hotkey2) hot = 2;
	if (ctrl == CON_Hotkey3) hot = 3;
	if (ctrl == CON_Hotkey4) hot = 4;
	if (ctrl == CON_Hotkey5) hot = 5;
	if (ctrl == CON_Hotkey6) hot = 6;
	if (ctrl == CON_Hotkey7) hot = 7;
	if (ctrl == CON_Hotkey8) hot = 8;
	if (ctrl == CON_Hotkey9) hot = 9;
	
	// only the last-pressed key is taken into consideration.
	// if 2 hotkeys are held, the earlier one is beeing treated as released
	if (hot > 0 && hot <= this->MaxContentsCount())
	{
		// if released, we chose, if not chosen already
		if(release)
		{
			if(handslot_choice_pending == hot)
			{
				SetHandItemPos(0, hot-1);
				this->~OnInventoryHotkeyRelease(hot-1);
			}
		}
		// else we just highlight
		else
		{
			if(handslot_choice_pending)
			{
				this->~OnInventoryHotkeyRelease(handslot_choice_pending-1);
			}
			handslot_choice_pending = hot;
			this->~OnInventoryHotkeyPress(hot-1);
		}
		
		return true;
	}
	
	var proc = GetProcedure();

	// cancel usage
	if (using && ctrl == CON_Ungrab)
	{
		CancelUse();
		return true;
	}

	// Interact controls
	if(ctrl == CON_Interact)
	{
		if(ObjectControlInteract(plr,ctrl))
			return true;
		else if(IsCarryingHeavy() && GetAction() == "Walk")
		{
			DropCarryHeavy();
			return true;
		}
			
	}
	// Push controls
	if (ctrl == CON_Grab || ctrl == CON_Ungrab || ctrl == CON_PushEnter || ctrl == CON_GrabPrevious || ctrl == CON_GrabNext)
		return ObjectControlPush(plr, ctrl);

	// Entrance controls
	if (ctrl == CON_Enter || ctrl == CON_Exit)
		return ObjectControlEntrance(plr,ctrl);
	
	
	// building, vehicle, mount, contents, menu control
	var house = Contained();
	var vehicle = GetActionTarget();
	// the clonk can have an action target even though he lost his action. 
	// That's why the clonk may only interact with a vehicle if in an
	// appropiate procedure:
	if (proc != "ATTACH" && proc != "PUSH")
		vehicle = nil;
	
	// menu
	if (menu)
	{
		return Control2Menu(ctrl, x,y,strength, repeat, release);
	}
	
	var contents = GetHandItem(0);
	var contents2 = GetHandItem(1);	
	
	// usage
	var use = (ctrl == CON_Use || ctrl == CON_UseDelayed || ctrl == CON_UseAlt || ctrl == CON_UseAltDelayed);
	if (use)
	{
		if (house)
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, release, house);
		}
		// control to grabbed vehicle
		else if (vehicle && proc == "PUSH")
		{
			return ControlUse2Script(ctrl, x, y, strength, repeat, release, vehicle);
		}
		else if (vehicle && proc == "ATTACH")
		{
			/* objects to which clonks are attached (like horses, mechs,...) have
			   a special handling:
			   Use controls are, forwarded to the
			   horse but if the control is considered unhandled (return false) on
			   the start of the usage, the control is forwarded further to the
			   item. If the item then returns true on the call, that item is
			   regarded as the used item for the subsequent ControlUse* calls.
			   BUT the horse always gets the ControlUse*-calls that'd go to the used
			   item, too and before it so it can decide at any time to cancel its
			   usage via CancelUse().
			  */

			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, vehicle))
				return true;
			else
			{
				// handled if the horse is the used object
				// ("using" is set to the object in StartUse(Delayed)Control - when the
				// object returns true on that callback. Exactly what we want)
				if (using == vehicle) return true;
				// has been cancelled (it is not the start of the usage but no object is used)
				if (!using && (repeat || release)) return true;
			}
		}
		// Release commands are always forwarded even if contents is 0, in case we
		// need to cancel use of an object that left inventory
		if ((contents || (release && using)) && (ctrl == CON_Use || ctrl == CON_UseDelayed))
		{
			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, contents))
				return true;
		}
		else if ((contents2 || (release && using)) && (ctrl == CON_UseAlt || ctrl == CON_UseAltDelayed))
		{
			if (ControlUse2Script(ctrl, x, y, strength, repeat, release, contents2))
				return true;
		}
	}

	// Collecting
	if (ctrl == CON_Collect)
	{
		// only if not inside something
		if(Contained()) return false; // not handled
		
		var dx = -GetDefWidth()/2, dy = -GetDefHeight()/2;
		var wdt = GetDefWidth(), hgt = GetDefHeight()+2;
		var obj = FindObject(Find_InRect(dx,dy,wdt,hgt), Find_OCF(OCF_Collectible), Find_NoContainer());
		if(obj)
		{
			// hackery to prevent the clonk from rejecting the collect because it is not being
			// collected into the hands
			Collect(obj,nil,nil,true);
		}
		
		// return not handled to still receive other controls - collection should not block anything else
		return false;
	}
	
	// Throwing and dropping
	// only if not in house, not grabbing a vehicle and an item selected
	// only act on press, not release
	if (!house && (!vehicle || proc == "ATTACH") && !release)
	{
		if (contents)
		{
			// special treatmant so that we know it's a forced throw
			if(ctrl == CON_ForcedThrow)
			{
				ctrl = CON_Throw;
				forced_ejection = contents;
			}
			
			// throw
			if (ctrl == CON_Throw)
			{
				CancelUse();
				
				if (proc == "SCALE" || proc == "HANGLE" || proc == "SWIM")
					return ObjectCommand("Drop", contents);
				else
					return ObjectCommand("Throw", contents, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowDelayed)
			{
				CancelUse();
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return ObjectCommand("Drop", contents);
					else
						return ObjectCommand("Throw", contents, mlastx, mlasty);
				}
				else
				{
					VirtualCursor()->StartAim(this);
					return true;
				}
			}
			// drop
			if (ctrl == CON_Drop)
			{
				CancelUse();
				return ObjectCommand("Drop", contents);
			}
		}
		// same for contents2 (copypasta)
		if (contents2)
		{
			// special treatmant so that we know it's a forced throw
			if(ctrl == CON_ForcedThrowAlt)
			{
				ctrl = CON_ThrowAlt;
				forced_ejection = contents2;
			}
		
			// throw
			if (ctrl == CON_ThrowAlt)
			{
				CancelUse();
				if (proc == "SCALE" || proc == "HANGLE")
					return ObjectCommand("Drop", contents2);
				else
					return ObjectCommand("Throw", contents2, x, y);
			}
			// throw delayed
			if (ctrl == CON_ThrowAltDelayed)
			{
				CancelUse();
				if (release)
				{
					VirtualCursor()->StopAim();
				
					if (proc == "SCALE" || proc == "HANGLE")
						return ObjectCommand("Drop", contents2);
					else
						return ObjectCommand("Throw", contents2, mlastx, mlasty);
				}
				else
				{
					CancelUse();
					VirtualCursor()->StartAim(this);
					return true;
				}
			}
			// drop
			if (ctrl == CON_DropAlt)
			{
				return ObjectCommand("Drop", contents2);
			}
		}
	}
	
	// Movement controls (defined in PlayerControl.c, partly overloaded here)
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Up || ctrl == CON_Down || ctrl == CON_Jump)
	{	
		// forward to script...
		if (house)
		{
			return ControlMovement2Script(ctrl, x, y, strength, repeat, release, house);
		}
		else if (vehicle)
		{
			if (ControlMovement2Script(ctrl, x, y, strength, repeat, release, vehicle)) return true;
		}
	
		return ObjectControlMovement(plr, ctrl, strength, release);
	}
	
	// Unhandled control
	return false;
}

public func ObjectControlMovement(int plr, int ctrl, int strength, bool release)
{
	// from PlayerControl.c
	var result = inherited(plr,ctrl,strength,release,...);

	// do the following only if strength >= CON_Gamepad_Deadzone
	if(!release)
		if(strength != nil && strength < CON_Gamepad_Deadzone)
			return result;

	
	virtual_cursor = FindObject(Find_ID(GUI_Crosshair),Find_Owner(GetOwner()));
	if(!virtual_cursor) return result;

	// change direction of virtual_cursor
	if(!release)
		virtual_cursor->Direction(ctrl);

	return result;
}

public func ObjectCommand(string command, object target, int tx, int ty, object target2)
{
	// special control for throw and jump
	// but only with controls, not with general commands
	if (command == "Throw") return this->~ControlThrow(target,tx,ty);
	else if (command == "Jump") return this->~ControlJump();
	// else standard command
	else return SetCommand(command,target,tx,ty,target2);
	
	// this function might be obsolete: a normal SetCommand does make a callback to
	// script before it is executed: ControlCommand(szCommand, pTarget, iTx, iTy)
}

/* ++++++++++++++++++++++++ Use Controls ++++++++++++++++++++++++ */

public func CancelUse()
{
	if (!using) return;

	// use the saved x,y coordinates for canceling
	CancelUseControl(mlastx, mlasty);
}

// for testing if the calls in ControlUse2Script are issued correctly
//global func Call(string call)
//{
//	Log("calling %s to %s",call,GetName());
//	return inherited(call,...);
//}

private func DetermineUsageType(object obj)
{
	// house
	if (obj == Contained())
		return C4D_Structure;
	// object
	if (obj)
		if (obj->Contained() == this)
			return C4D_Object;
	// vehicle
	var proc = GetProcedure();
	if (obj == GetActionTarget())
		if (proc == "ATTACH" && proc == "PUSH")
			return C4D_Vehicle;
	// unkown
	return nil;
}

private func GetUseCallString(string action) {
	// Control... or Contained...
	var control = "Control";
	if (using_type == C4D_Structure) control = "Contained";
	// ..Use.. or ..UseAlt...
	var estr = "";
	if (alt && using_type != C4D_Object) estr = "Alt";
	// Action
	if (!action) action = "";
	
	return Format("~%sUse%s%s",control,estr,action);
}

private func StartUseControl(int ctrl, int x, int y, object obj)
{
	using = obj;
	using_type = DetermineUsageType(obj);
	alt = ctrl != CON_Use;
	
	var hold_enabled = obj->Call("~HoldingEnabled");
	
	if (hold_enabled)
		SetPlayerControlEnabled(GetOwner(), CON_Aim, true);

	// first call UseStart. If unhandled, call Use (mousecontrol)
	var handled = obj->Call(GetUseCallString("Start"),this,x,y);
	if (!handled)
	{
		handled = obj->Call(GetUseCallString(),this,x,y);
		noholdingcallbacks = handled;
	}
	if (!handled)
	{
		using = nil;
		using_type = nil;
		if (hold_enabled)
			SetPlayerControlEnabled(GetOwner(), CON_Aim, false);
		return false;
	}
		
	return handled;
}

private func StartUseDelayedControl(int ctrl, object obj)
{
	using = obj;
	using_type = DetermineUsageType(obj);
	alt = ctrl != CON_UseDelayed;
				
	VirtualCursor()->StartAim(this);
			
	// call UseStart
	var handled = obj->Call(GetUseCallString("Start"),this,mlastx,mlasty);
	noholdingcallbacks = !handled;
	
	return handled;
}

private func CancelUseControl(int x, int y)
{
	// to horse first (if there is one)
	var horse = GetActionTarget();
	if(horse && GetProcedure() == "ATTACH" && using != horse)
		StopUseControl(x, y, horse, true);

	return StopUseControl(x, y, using, true);
}

private func StopUseControl(int x, int y, object obj, bool cancel)
{
	var stop = "Stop";
	if (cancel) stop = "Cancel";
	
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, ContainedUseCancel, etc...
	var handled = obj->Call(GetUseCallString(stop),this,x,y);
	if (obj == using)
	{
		// if ControlUseStop returned -1, the current object is kept as "used object"
		// but no more callbacks except for ControlUseCancel are made. The usage of this
		// object is finally cancelled on ControlUseCancel.
		if(cancel || handled != -1)
		{
			using = nil;
			using_type = nil;
			alt = false;
		}
		noholdingcallbacks = false;
		
		SetPlayerControlEnabled(GetOwner(), CON_Aim, false);

		if (virtual_cursor)
			virtual_cursor->StopAim();
	}
		
	return handled;
}

private func HoldingUseControl(int ctrl, int x, int y, object obj)
{
	var mex = x;
	var mey = y;
	if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		mex = mlastx;
		mey = mlasty;
	}
	
	//Message("%d,%d",this,mex,mey);

	// automatic adjustment of the direction
	// --------------------
	// if this is desired for ALL objects is the question, we will find out.
	// For now, this is done for items and vehicles, not for buildings and
	// mounts (naturally). If turning vehicles just liket hat without issuing
	// a turning animation is the question. But after all, the clonk can turn
	// by setting the dir too.
	
	
	//   not riding and                not in building  not while scaling
	if (GetProcedure() != "ATTACH" && !Contained() &&   GetProcedure() != "SCALE")
	{
		// pushing vehicle: object to turn is the vehicle
		var dir_obj = GetActionTarget();
		if (GetProcedure() != "PUSH") dir_obj = nil;

		// otherwise, turn the clonk
		if (!dir_obj) dir_obj = this;
	
		if ((dir_obj->GetComDir() == COMD_Stop && dir_obj->GetXDir() == 0) || dir_obj->GetProcedure() == "FLIGHT")
		{
			if (dir_obj->GetDir() == DIR_Left)
			{
				if (mex > 0)
					dir_obj->SetDir(DIR_Right);
			}
			else
			{
				if (mex < 0)
					dir_obj->SetDir(DIR_Left);
			}
		}
	}
	
	var handled = obj->Call(GetUseCallString("Holding"),this,mex,mey);
			
	return handled;
}

private func StopUseDelayedControl(object obj)
{
	// ControlUseStop, ControlUseAltStop, ContainedUseAltStop, etc...
	
	var handled = obj->Call(GetUseCallString("Stop"),this,mlastx,mlasty);
	if (!handled)
		handled = obj->Call(GetUseCallString(),this,mlastx,mlasty);
	
	if (obj == using)
	{
		VirtualCursor()->StopAim();
		// see StopUseControl
		if(handled != -1)
		{
			using = nil;
			using_type = nil;
			alt = false;
		}
		noholdingcallbacks = false;
	}
		
	return handled;
}

/* Control to menu */

private func Control2Menu(int ctrl, int x, int y, int strength, bool repeat, bool release)
{

	/* all this stuff is already done on a higher layer - in playercontrol.c
	   now this is just the same for gamepad control */
	if (!PlayerHasVirtualCursor(GetOwner()))
		return true;

	// fix pos of x and y
	var mex = mlastx+GetX()-GetMenu()->GetX();
	var mey = mlasty+GetY()-GetMenu()->GetY();
	
	// update angle for visual effect on the menu
	if (repeat)
	{
		if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
			this->GetMenu()->~UpdateCursor(mex,mey);
	}
	// click on menu
	if (release)
	{
		// select
		if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
			this->GetMenu()->~OnMouseClick(mex,mey, ctrl == CON_UseAltDelayed);
	}
	
	return true;
}

// Control use redirected to script
private func ControlUse2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, object obj)
{
	// click on secondary cancels primary and the other way round
	if (using)
	{
		if (ctrl == CON_Use && alt || ctrl == CON_UseAlt && !alt
		||  ctrl == CON_UseDelayed && alt || ctrl == CON_UseAltDelayed && !alt)
		{
			CancelUseControl(x, y);
			return true;
		}
	}
	
	// standard use
	if (ctrl == CON_Use || ctrl == CON_UseAlt)
	{
		if (!release && !repeat)
		{
			return StartUseControl(ctrl,x, y, obj);
		}
		else if (release && obj == using)
		{
			return StopUseControl(x, y, obj);
		}
	}
	// gamepad use
	else if (ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		if (!release && !repeat)
		{
			return StartUseDelayedControl(ctrl, obj);
		}
		else if (release && obj == using)
		{
			return StopUseDelayedControl(obj);
		}
	}
	
	// more use (holding)
	if (ctrl == CON_Use || ctrl == CON_UseAlt || ctrl == CON_UseDelayed || ctrl == CON_UseAltDelayed)
	{
		if (release)
		{
			// leftover use release
			CancelUse();
			return true;
		}
		else if (repeat && !noholdingcallbacks)
		{
			return HoldingUseControl(ctrl, x, y, obj);
		}
	}
		
	return false;
}

// Control use redirected to script
private func ControlMovement2Script(int ctrl, int x, int y, int strength, bool repeat, bool release, object obj)
{
	// overloads of movement commandos
	if (ctrl == CON_Left || ctrl == CON_Right || ctrl == CON_Down || ctrl == CON_Up || ctrl == CON_Jump)
	{
		var control = "Control";
		if (Contained() == obj) control = "Contained";
	
		if (release)
		{
			// if any movement key has been released, ControlStop is called
			if (obj->Call(Format("~%sStop",control),this,ctrl))  return true;
		}
		else
		{
			// what about deadzone?
			if (strength != nil && strength < CON_Gamepad_Deadzone)
				return true;
			
			// Control*
			if (ctrl == CON_Left)  if (obj->Call(Format("~%sLeft",control),this))  return true;
			if (ctrl == CON_Right) if (obj->Call(Format("~%sRight",control),this)) return true;
			if (ctrl == CON_Up)    if (obj->Call(Format("~%sUp",control),this))    return true;
			if (ctrl == CON_Down)  if (obj->Call(Format("~%sDown",control),this))  return true;
			
			// for attached (e.g. horse: also Jump command
			if (GetProcedure() == "ATTACH")
				if (ctrl == CON_Jump)  if (obj->Call("ControlJump",this)) return true;
		}
	}

}

// returns true if the clonk is able to enter a building (procedurewise)
public func CanEnter()
{
	var proc = GetProcedure();
	if (proc != "WALK" && proc != "SWIM" && proc != "SCALE" &&
		proc != "HANGLE" && proc != "FLOAT" && proc != "FLIGHT" &&
		proc != "PUSH") return false;
	return true;
}

// Handles enter and exit
private func ObjectControlEntrance(int plr, int ctrl)
{
	// enter
	if (ctrl == CON_Enter)
	{
		// contained
		if (Contained()) return false;
		// enter only if... one can
		if (!CanEnter()) return false;

		// a building with an entrance at right position is there?
		var obj = GetEntranceObject();
		if (!obj) return false;
		
		ObjectCommand("Enter", obj);
		return true;
	}
	
	// exit
	if (ctrl == CON_Exit)
	{
		if (!Contained()) return false;
		
		ObjectCommand("Exit");
		return true;
	}
	
	return false;
}

private func ObjectControlInteract(int plr, int ctrl)
{
	var interactables = FindObjects(Find_Or(Find_Container(this), Find_AtPoint(0,0)),
						Find_Func("IsInteractable",this), Find_Layer(GetObjectLayer()));
	// if there are several interactable objects, just call the first that returns true
	for (var interactable in interactables)
	{
		if (interactable->~Interact(this))
		{
			return true;
		}
	}
	return false;
}

// Handles push controls
private func ObjectControlPush(int plr, int ctrl)
{
	if (!this) return false;
	
	var proc = GetProcedure();
	
	// grabbing
	if (ctrl == CON_Grab)
	{
		// grab only if he walks
		if (proc != "WALK") return false;
		
		// only if there is someting to grab
		var obj = FindObject(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this), Find_Layer(GetObjectLayer()));
		if (!obj) return false;
		
		// grab
		ObjectCommand("Grab", obj);
		return true;
	}
	
	// grab next/previous
	if (ctrl == CON_GrabNext)
		return ShiftVehicle(plr, false);
	if (ctrl == CON_GrabPrevious)
		return ShiftVehicle(plr, true);
	
	// ungrabbing
	if (ctrl == CON_Ungrab)
	{
		// ungrab only if he pushes
		if (proc != "PUSH") return false;

		ObjectCommand("UnGrab");
		return true;
	}
	
	// push into building
	if (ctrl == CON_PushEnter)
	{
		if (proc != "PUSH") return false;
		
		// respect no push enter
		if (GetActionTarget()->GetDefCoreVal("NoPushEnter","DefCore")) return false;
		
		// a building with an entrance at right position is there?
		var obj = GetActionTarget()->GetEntranceObject();
		if (!obj) return false;

		ObjectCommand("PushTo", GetActionTarget(), 0, 0, obj);
		return true;
	}
	
}

// grabs the next/previous vehicle (if there is any)
private func ShiftVehicle(int plr, bool back)
{
	if (!this) return false;
	
	if (GetProcedure() != "PUSH") return false;

	var lorry = GetActionTarget();
	// get all grabbable objects
	var objs = FindObjects(Find_OCF(OCF_Grab), Find_AtPoint(0,0), Find_Exclude(this), Find_Layer(GetObjectLayer()));
		
	// nothing to switch to (there is no other grabbable object)
	if (GetLength(objs) <= 1) return false;
		
	// find out at what index of the array objs the vehicle is located
	var index = 0;
	for(var obj in objs)
	{
		if (obj == lorry) break;
		index++;
	}
		
	// get the next/previous vehicle
	if (back)
	{
		--index;
		if (index < 0) index = GetLength(objs)-1;
	}
	else
	{
		++index;
		if (index >= GetLength(objs)) index = 0;
	}
	
	ObjectCommand("Grab", objs[index]);
	
	return true;
}

/* Virtual cursor stuff */

// get virtual cursor, if noone is there, create it
private func VirtualCursor()
{
	if (!virtual_cursor)
	{
		virtual_cursor = FindObject(Find_ID(GUI_Crosshair),Find_Owner(GetOwner()));
	}
	if (!virtual_cursor)
	{
		virtual_cursor = CreateObject(GUI_Crosshair,0,0,GetOwner());
	}
	
	return virtual_cursor;
}

// virtual cursor is visible
private func VirtualCursorAiming()
{
	if (!virtual_cursor) return false;
	return virtual_cursor->IsAiming();
}

// store pos of virtual cursor into mlastx, mlasty
public func UpdateVirtualCursorPos()
{
	mlastx = VirtualCursor()->GetX()-GetX();
	mlasty = VirtualCursor()->GetY()-GetY();
}

public func TriggerHoldingControl()
{
	// using has been commented because it must be possible to use the virtual
	// cursor aim also without a used object - for menus
	// However, I think the check for 'using' here is just an unecessary safeguard
	// since there is always a using-object if the clonk is aiming for a throw
	// or a use. If the clonk uses it, there will be callbacks that cancel the
	// callbacks to the virtual cursor
	// - Newton
	if (/*using && */!noholdingcallbacks)
	{
		var ctrl;
		if (alt) ctrl = CON_UseAltDelayed;
		else     ctrl = CON_UseDelayed;
				
		ObjectControl(GetOwner(), ctrl, 0, 0, 0, true, false);
	}

}

public func IsMounted() { return GetProcedure() == "ATTACH"; }

/* +++++++++++++++++++++++ Menu control +++++++++++++++++++++++ */

local menu;

func HasMenuControl()
{
	return true;
}

func SetMenu(object m)
{
	// already the same
	if (menu == m && m)
	{
		return;
	}
	// no multiple menus: close old one
	if (menu && m)
	{
		menu->RemoveObject();
	}
	// new one
	menu = m;
	if (menu)
	{
		CancelUse();
		// stop clonk
		SetComDir(COMD_Stop);
	
		if (PlayerHasVirtualCursor(GetOwner()))
			VirtualCursor()->StartAim(this,false,menu);
		else
		{
			if (menu->~CursorUpdatesEnabled()) 
				SetPlayerControlEnabled(GetOwner(), CON_GUICursor, true);

			SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, true);
			SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, true);
		}
	}
	// close menu
	if (!menu)
	{
		if (virtual_cursor)
			virtual_cursor->StopAim();
		SetPlayerControlEnabled(GetOwner(), CON_GUICursor, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick1, false);
		SetPlayerControlEnabled(GetOwner(), CON_GUIClick2, false);
	}
}

func MenuClosed()
{
	SetMenu(nil);
}

func GetMenu()
{
	return menu;
}

func CancelMenu()
{
	if (menu)
	{
		menu->Close();
		SetMenu(nil);
		return true;
	}
	
	return false;
}

func ReinitializeControls()
{
	if(PlayerHasVirtualCursor(GetOwner()))
	{
		// if is aiming or in menu and no virtual cursor is there? Create one
		if (!virtual_cursor)
			if (menu || using)
				VirtualCursor()->StartAim(this,false,menu);
	}
	else
	{
		// remove any virtual cursor
		if (virtual_cursor)
			virtual_cursor->RemoveObject();
	}
}

/* Backpack control */

func Selected(object mnu, object mnu_item, bool alt)
{
	var backpack_index = mnu_item->GetExtraData();
	var hands_index = 0;
	if (alt) hands_index = 1;
	// Update menu
	var show_new_item = GetItem(hands_index);
	mnu_item->SetSymbol(show_new_item);
	// swap index with backpack index
	Switch2Items(hands_index, backpack_index);
}

/* +++++++++++++++  Throwing, jumping +++++++++++++++ */

// Throwing
private func DoThrow(object obj, int angle)
{
	// parameters...
	var iX, iY, iR, iXDir, iYDir, iRDir;
	iX = 4; if (!GetDir()) iX = -iX;
	iY = Cos(angle,-4);
	iR = Random(360);
	iRDir = RandomX(-10,10);

	iXDir = Sin(angle,this.ThrowSpeed);
	iYDir = Cos(angle,-this.ThrowSpeed);
	// throw boost (throws stronger upwards than downwards)
	if (iYDir < 0) iYDir = iYDir * 13/10;
	if (iYDir > 0) iYDir = iYDir * 8/10;
	
	// add own velocity
	iXDir += GetXDir(100)/2;
	iYDir += GetYDir(100)/2;

	// throw
	obj->Exit(iX, iY, iR, 0, 0, iRDir);
	obj->SetXDir(iXDir,100);
	obj->SetYDir(iYDir,100);
	
	return true;
}

// custom throw
public func ControlThrow(object target, int x, int y)
{
	// standard throw after all
	if (!x && !y) return false;
	if (!target) return false;
	
	return false;
}

public func ControlJump()
{
	var ydir = 0;
	
	if (GetProcedure() == "WALK")
	{
		ydir = this.JumpSpeed;
	}
	
	if (InLiquid())
	{
		if (!GBackSemiSolid(0,-5))
			ydir = BoundBy(this.JumpSpeed * 3 / 5, 240, 380);
	}

	if (GetProcedure() == "SCALE")
	{
		ydir = this.JumpSpeed/2;
	}
	
	if (ydir && !Stuck())
	{
		SetPosition(GetX(),GetY()-1);

		//Wall kick
		if(GetProcedure() == "SCALE")
		{
			AddEffect("WallKick",this,1);
			SetAction("Jump");

			var xdir;
			if(GetDir() == DIR_Right)
			{
				xdir = -1;
				SetDir(DIR_Left);
			}
			else if(GetDir() == DIR_Left)
			{
				xdir = 1;
				SetDir(DIR_Right);
			}

			SetYDir(-ydir * GetCon(), 100 * 100);
			SetXDir(xdir * 17);
			return true;
		}
		//Normal jump
		else
		{
			SetAction("Jump");
			SetYDir(-ydir * GetCon(), 100 * 100);
			return true;
		}
	}
	return false;
}

func FxIsWallKickStart(object target, int num, bool temp)
{
	return 1;
}