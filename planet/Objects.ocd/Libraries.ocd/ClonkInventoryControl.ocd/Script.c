/**
	ClonkInventoryControl
	Handles the Clonk's interaction with the inventory.

*/


/*
	used properties:
	this.inventory.last_slot: last inventory-slot that has been selected. Used for QuickSwitching
	this.inventory.is_picking_up: whether currently picking up
	
	other used properties of "this.inventory" might have been declared in Inventory.ocd
*/


func Construction()
{
	if(this.inventory == nil)
		this.inventory = {};
	this.inventory.last_slot = 0;
	return _inherited(...);
}

public func OnShiftCursor(object new_cursor)
{
	if (this.control.is_interacting)
		AbortPickingUp();
	return _inherited(new_cursor, ...);
}

// Called by other libraries and objects when the Clonk has forcefully dropped (not thrown) an object.
func OnDropped(object obj)
{
	return _inherited(obj, ...);
}

func RejectCollect(id objid, object obj)
{
	var rejected = _inherited(objid, obj, ...);
	if(rejected) return rejected;
	
	// Allow collection only if called via clonk->Collect, to prevent picking up stuff on the ground.
	// Make an exception for containers, though.
	if (!this.inventory.force_collection && !obj->Contained()) return true;
	return false;
}

public func ObjectControl(int plr, int ctrl, int x, int y, int strength, bool repeat, bool release)
{
	if (!this) 
		return inherited(plr, ctrl, x, y, strength, repeat, release, ...);
		
	// Quickswitch changes the active slot to the last selected one
	if (ctrl == CON_QuickSwitch)
	{
		// but ignore quickswitch if we have more than 1 hand-slot
		if(this.HandObjects > 1)
			return inherited(plr, ctrl, x, y, strength, repeat, release, ...);;
		
		// select last slot
		SetHandItemPos(0, this.inventory.last_slot); // last_slot is updated in SetHandItemPos
		return true;
	}
	
	// Quick-pickup item via click? Note that this relies on being executed after the normal Clonk controls
	if (ctrl == CON_Use && !this->GetHandItem(0) && !release)
	{
		var sort = Sort_Distance(x, y);
		var items = FindAllPickupItems(sort);
		for (var item in items)
		{
			if (item && TryToCollect(item)) return true;
		}
	}
	
	// Begin picking up objects.
	if (ctrl == CON_PickUp && !release)
	{
		this->CancelUse();
		BeginPickingUp();
		return true;
	}
	
	// Drop the mouse item?
	if (ctrl == CON_Drop && !release)
	{
		// Do not immediately collect another thing unless chosen with left/right.
		if (this.inventory.is_picking_up)
		{
			SetNextPickupItem(nil);
		}
		
		var item = this->GetHandItem(0);
		if (item)
			this->DropInventoryItem(this->GetHandItemPos(0));
		return true;
	}
	
	// Switching pickup object or finish pickup?
	if (this.inventory.is_picking_up)
	{
		// Stop picking up.
		if (ctrl == CON_PickUpNext_Stop)
		{
			AbortPickingUp();
			return true;
		}
		
		// Quickly pick up everything possible.
		if (ctrl == CON_PickUpNext_All)
		{
			PickUpAll();
			AbortPickingUp();
			return true;
		}
		
		// Finish picking up (aka "collect").
		if (ctrl == CON_PickUp && release)
		{
			EndPickingUp();
			return true;
		}
		
		// Switch left/right through objects.
		var dir = nil;
		if (ctrl == CON_PickUpNext_Left) dir = -1;
		else if (ctrl == CON_PickUpNext_Right) dir = 1;
		
		if (dir != nil)
		{
			var item = FindNextPickupObject(this.inventory.pickup_item, dir);
			if (item)
				SetNextPickupItem(item);
			return true;
		}
	}
	
	
	// shift inventory
	var inventory_shift = 0;
	if (ctrl == CON_InventoryShiftForward) inventory_shift = 1;
	else if (ctrl == CON_InventoryShiftBackward) inventory_shift = -1;
	
	if (inventory_shift)
	{
		var current = (this->GetHandItemPos(0) + inventory_shift) % this.MaxContentsCount;
		if (current < 0) current = this.MaxContentsCount + current;
		this->SetHandItemPos(0, current);
		return true;
	}
	
	var hot;
	
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
		// Do not stop picking-up but nullify object selection.
		// That way, you can still drop during pickup, but you won't automatically pickup stuff when you try to drop.
		if (this.inventory.is_picking_up)
		{
			SetNextPickupItem(nil);
		}
		
		this->~DropInventoryItem(hot-1);
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
	// if 2 hotkeys are held, the earlier one is being treated as released
	if (hot > 0 && hot <= this.MaxContentsCount)
	{
		SetHandItemPos(0, hot-1);
		return true;
	}
	
	return inherited(plr, ctrl, x, y, strength, repeat, release, ...);
}

private func FxIntHighlightItemStart(object target, proplist fx, temp, object item)
{
	if (temp) return;
	fx.item = item;
	
	fx.dummy = CreateObject(Dummy, item->GetX() - GetX(), item->GetY() - GetY(), GetOwner());
	fx.dummy.ActMap = 
	{
		Attach = 
		{
			Name = "Attach",
			Procedure = DFA_ATTACH,
			FaceBase = 1
		}
	};
	fx.dummy.Visibility = VIS_Owner;
	fx.dummy.Plane = 1000;
	fx.dummy->Message("@%s", item->GetName());
	
	// Center dummy!
	fx.dummy->SetVertexXY(0, item->GetVertex(0, VTX_X), item->GetVertex(0, VTX_Y));
	fx.dummy->SetAction("Attach", item);
	
	fx.width  = item->GetDefWidth();
	fx.height = item->GetDefHeight();
	
	// Draw the item's graphics in front of it again to achieve a highlighting effect.
	fx.dummy->SetGraphics(nil, nil, 1, GFXOV_MODE_Object, nil, GFX_BLIT_Additive, item);

	// Custom selector particle if object says so
	if (item->~PickupHighlight(fx.dummy, fx.width, fx.height, GetOwner()))
		return;

	// Draw a nice selector particle on item change.
	var selector =
	{
		Size = PV_Step(3, 2, 1, Max(fx.width, fx.height)),
		Attach = ATTACH_Front,
		Rotation = PV_Step(1, PV_Random(0, 360), 1),
		Alpha = 200
	};

	fx.dummy->CreateParticle("Selector", 0, 0, 0, 0, 0, Particles_Colored(selector, GetPlayerColor(GetOwner())), 1); 
}

private func FxIntHighlightItemTimer(object target, proplist fx, int time)
{
	if (!fx.dummy) return -1;
	if (!fx.item) return -1;
	if (ObjectDistance(this, fx.item) > 20) return -1;
	if (fx.item->Contained()) return -1;
}

private func FxIntHighlightItemStop(object target, proplist fx, int reason, temp)
{
	if (temp) return;
	if (fx.dummy) fx.dummy->RemoveObject();
	if (!this) return;
	if (fx.item == this.inventory.pickup_item)
		this.inventory.pickup_item = nil;
} 

private func SetNextPickupItem(object to)
{
	// Clear all old markers.
	var e = nil;
	while (e = GetEffect("IntHighlightItem", this))
		RemoveEffect(nil, this, e);
	// And set & mark new one.
	this.inventory.pickup_item = to;
	if (to)
		AddEffect("IntHighlightItem", this, 1, 2, this, nil, to);
}

private func FindAllPickupItems(sorting_criterion)
{
	return FindObjects(Find_Distance(20), Find_NoContainer(), Find_Property("Collectible"), Find_Layer(this->GetObjectLayer()), Find_Not(Find_OCF(OCF_HitSpeed1)), sorting_criterion);
}

private func FindNextPickupObject(object start_from, int x_dir)
{
	if (!start_from) start_from = this;
	// Returns objects sorted by ascending x-position, with all objects right of the clonk being sorted before objects left of the clonk
	var sort = Sort_Func("Library_ClonkInventoryControl_Sort_Priority", start_from->GetX());
	var objects = FindAllPickupItems(sort);
	var len = GetLength(objects);
	if (!len) return nil;
	// Find object next to the current one.
	// (note that index==-1 accesses the last element)
	var index = GetIndexOf(objects, start_from);
	if (index != -1)
	{
		// Previous item was found in the list.
		// Cycle through list to the right (x_dir==1) or left (x_dir==-1)
		index = (index + x_dir) % len;
	}
	else
	{
		// Previous item is no longer in range or there was no previous item - start with item closest to clonk position
		// Going only in view direction may sound intuitive, but is weird in practice when you're standing just on top
		// of an object that is 1px behind you and it selects an item very far away instead
		// The most intuitive seems to pre-select the item closest to the clonk hands (also e.g. when scaling),
		// so use distance from object center.
		index += (ObjectDistance(objects[-1]) > ObjectDistance(objects[0]));
	}
	var next = objects[index];
	if (next == start_from) return nil;
	return next;
}

private func BeginPickingUp()
{
	this.inventory.is_picking_up = true;
	
	var obj = FindNextPickupObject(this, 0);
	if (obj)
		SetNextPickupItem(obj);
}

// Ends the pickup process without actually doing anything.
private func AbortPickingUp()
{
	this.inventory.pickup_item = nil;
	EndPickingUp();
}

private func EndPickingUp()
{
	this.inventory.is_picking_up = false;

	if (this.inventory.pickup_item)
	{
		TryToCollect(this.inventory.pickup_item);
	}
	
	var e = nil;
	while (e = GetEffect("IntHighlightItem", this))
	{
		RemoveEffect(nil, this, e);
	}
	
	this.inventory.pickup_item = nil;
}

// Shows an effect when you picked up an item.
private func TryToCollect(object item)
{
	// Remember stuff for a possible message - the item might have removed itself later.
	var x = item->GetX();
	var y = item->GetY();
	var name = item->GetName();
	// Try to collect the item.
	Collect(item);
	
	// If anything happened, assume collection.
	if (!item || item->Contained())
	{
		var message = CreateObject(FloatingMessage, AbsX(x), AbsY(y), GetOwner());
		message.Visibility = VIS_Owner;
		message->SetMessage(name);
		message->SetYDir(-10);
		message->FadeOut(1, 20);
		return true;
	}
	return false;
}

// Pick up all objects in the vicinity.
private func PickUpAll()
{
	// First try to find objects with the ID of the currently selected object.
	var preferred_id = nil;
	if (this.inventory.pickup_item != nil)
		preferred_id = this.inventory.pickup_item->GetID();
	
	var sort = Sort_Distance();
	
	var all_objects = FindAllPickupItems(sort);
	// Try the preferred id first.
	if (preferred_id)
		for (var obj in all_objects)
		{
			if (!obj || obj->Contained()) continue;
			if (obj->GetID() != preferred_id) continue;
			TryToCollect(obj);
		}
	// And try the others now..
	for (var obj in all_objects)
	{
		if (!obj || obj->Contained()) continue;
		TryToCollect(obj);
	}
}

// used in Inventory.ocd
public func SetHandItemPos(int hand, int inv)
{
	// save current slot
	if(hand == 0)
		this.inventory.last_slot = this->GetHandItemPos(0);
		
	return _inherited(hand, inv, ...);
}
/* Backpack control */
func Selected(object mnu, object mnu_item)
{
	var backpack_index = mnu_item->GetExtraData();
	var hands_index = 0;
	// Update menu
	var show_new_item = this->GetItem(hands_index);
	mnu_item->SetSymbol(show_new_item);
	// swap index with backpack index
	this->Switch2Items(hands_index, backpack_index);
}