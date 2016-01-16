/* Wall kit */

#include Library_Stackable

func MaxStackCount() { return 4; }

/* Item usage */

func ControlUseStart(object clonk, int x, int y)
{
	clonk->Sound("Objects::WallKit::Click");
	SetPreview(clonk,x,y);
	return true;
}

func HoldingEnabled() { return true; }

func ControlUseHolding(object clonk, int new_x, int new_y)
{
	SetPreview(clonk, new_x, new_y);
	return true;
}

public func ControlUseStop(object clonk, int x, int y)
{
	StopPreview(clonk);
	var item = TakeObject();
	if (!item) return true; // zero stack count?
	item->CreateBridge(clonk, x, y);
	if (item) item->RemoveObject();
	return true;
}

public func ControlUseCancel(object clonk, int x, int y)
{
	StopPreview(clonk);
	return true;
}


/* Bridge building */

private func CreateBridge(object clonk, int x, int y)
{
	var c = Offset2BridgeCoords(clonk, x, y);
	x = clonk->GetX(); 
	y = clonk->GetY();
	DrawMaterialQuad(BridgeMaterial, x + c.x1 - c.dxm, y + c.y1 - c.dym, x + c.x1 + c.dxp, y + c.y1 + c.dyp, x + c.x2 + c.dxp, y + c.y2 + c.dyp, x + c.x2 - c.dxm, y + c.y2 - c.dym, DMQ_Bridge);
	clonk->Sound("Objects::WallKit::Lock");
	return true;
}


/* Bridge position calculation */

private func Offset2BridgeCoords(object clonk, int x, int y)
{
	// Returns starting and end point offset of bridge to be built as player points to offset x/y
	var dx = clonk->GetDefWidth(), dy = clonk->GetDefHeight(), ox, oy, rx, ry;
	var l = BridgeLength;
	var d = BridgeThickness;
	ox = x * 2 / Abs(y + !y); 
	oy = y * 2 / Abs(x + !x);
	ry = ox /= Abs(ox) + !ox;
	rx = oy /= Abs(oy) + !oy;
	// Offset of the bridge: more for vertical and horizontal bridges.
	ox *= dx / 2 + 3 * !oy;
	oy *= dy / 2 + 3 * !ox;
	// Reduce thickness and length for diagonal bridges.
	l -= l * 3 * Abs(rx * ry) / 10;
	d -= d * 3 * Abs(rx * ry) / 10;
	return { dxp = ry * d / 2, dyp = rx * d / 2, dxm = ry * (d + 1) / 2, dym = rx * (d + 1) / 2, x1 = ox + rx * l, y1 = oy - ry * l, x2 = ox - rx * l, y2 = oy + ry * l };
}


/* Preview */

local preview;

func SetPreview(object clonk, int x, int y)
{
	var c = Offset2BridgeCoords(clonk, x, y), clr = 0xffa0a0a0;
	x=clonk->GetX(); y=clonk->GetY();
	if (!preview)
	{
		preview = WallKit_Preview->Create(x+c.x1,y+c.y1,x+c.x2,y+c.y2,clr);
		preview->SetOwner(clonk->GetOwner());
		preview.Visibility = VIS_Owner;
	}
	else
	{
		preview->Set(x+c.x1,y+c.y1,x+c.x2,y+c.y2,clr);
	}
	return true;
}

func StopPreview(object clonk)
{
	if (preview) preview->RemoveObject();
	return true;
}


/* Impact sound */

func Hit()
{
	Sound("Hits::GeneralHit?");
	return true;
}


/* Status */

public func IsTool() { return true; }
public func IsToolProduct() { return true; }

local Collectible = 1;
local Name = "$Name$";
local Description = "$Description$";
local UsageHelp = "$UsageHelp$";
local BridgeLength = 20;
local BridgeThickness = 5;
local BridgeMaterial = "Granite-granite";
