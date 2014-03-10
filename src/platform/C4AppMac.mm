/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2006  Julian Raschke
 * Copyright (c) 2008-2009  Günther Brammer
 * Copyright (c) 2005-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

// based on SDL implementation

#include <GL/glew.h>
#include <string>

#include <C4Include.h>
#include <C4Window.h>
#include <C4Draw.h>

#import <Cocoa/Cocoa.h>
#import "C4WindowController.h"
#import "C4DrawGLMac.h"

#include "C4App.h"

bool C4AbstractApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	NSString* string = [NSString stringWithCString:text.getData() encoding:NSUTF8StringEncoding];
	if (![pasteboard setString:string forType:NSStringPboardType])
	{
		Log("Writing to Cocoa pasteboard failed");
		return false;
	}
	return true;
}

StdStrBuf C4AbstractApp::Paste(bool fClipboard)
{
	if (fClipboard)
	{
		NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
		const char* chars = [[pasteboard stringForType:NSStringPboardType] cStringUsingEncoding:NSUTF8StringEncoding];
		return StdStrBuf(chars);
	}
	return StdStrBuf(0);
}

bool C4AbstractApp::IsClipboardFull(bool fClipboard)
{
	return [[NSPasteboard generalPasteboard] availableTypeFromArray:[NSArray arrayWithObject:NSStringPboardType]];
}

void C4AbstractApp::MessageDialog(const char * message)
{
	NSAlert* alert = [NSAlert alertWithMessageText:@"Fatal Error"
		defaultButton:nil
		alternateButton:nil
		otherButton:nil
		informativeTextWithFormat:@"%@",
		[NSString stringWithUTF8String:message]
	];
	[alert runModal];
}

void C4Window::FlashWindow()
{
	[NSApp requestUserAttention:NSCriticalRequest];
}

#ifdef USE_COCOA

C4AbstractApp::C4AbstractApp(): Active(false), fQuitMsgReceived(false), MainThread(pthread_self()), fDspModeSet(false)
{
}
 
C4AbstractApp::~C4AbstractApp() {}

bool C4AbstractApp::Init(int argc, char * argv[])
{
	// Set locale
	setlocale(LC_ALL,"");

	// Custom initialization
	return DoInit (argc, argv);
}


void C4AbstractApp::Clear() {}

void C4AbstractApp::Quit()
{
	[NSApp terminate:[NSApp delegate]];
}

bool C4AbstractApp::FlushMessages()
{
	// Always fail after quit message
	if(fQuitMsgReceived)
		return false;

	while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, TRUE) == kCFRunLoopRunHandledSource);
	NSEvent* event;
	while ((event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSEventTrackingRunLoopMode dequeue:YES]) != nil)
	{
		[NSApp sendEvent:event];
		[NSApp updateWindows];
	}
	return true;
}

static int32_t bitDepthFromPixelEncoding(CFStringRef encoding)
{
	// copy-pasta: http://gitorious.org/ogre3d/mainlinemirror/commit/722dbd024aa91a6401850788db76af89c364d6e7
	if (CFStringCompare(encoding, CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 32;
	else if(CFStringCompare(encoding, CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 16;
	else if(CFStringCompare(encoding, CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
		return 8;
	else
		return -1; // fail
}

bool C4AbstractApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	// No support for multiple monitors.
	CFArrayRef array = CGDisplayCopyAllDisplayModes(iMonitor, NULL);
	bool good_index = iIndex >= 0 && iIndex < (int32_t)CFArrayGetCount(array);
	if (good_index)
	{
		CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(array, iIndex);
		*piXRes = CGDisplayModeGetWidth(displayMode);
		*piYRes = CGDisplayModeGetHeight(displayMode);
		CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(displayMode);
		*piBitDepth = bitDepthFromPixelEncoding(pixelEncoding);
		CFRelease(pixelEncoding);
	}
	CFRelease(array);
	return good_index;
}

void C4AbstractApp::RestoreVideoMode()
{
}

StdStrBuf C4AbstractApp::GetGameDataPath()
{
	return StdCopyStrBuf([[[NSBundle mainBundle] resourcePath] fileSystemRepresentation]);
}

bool C4AbstractApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	fFullScreen &= !lionAndBeyond(); // Always false for Lion since then Lion's true(tm) Fullscreen is used
	C4WindowController* controller = pWindow->objectiveCObject<C4WindowController>();
	NSWindow* window = controller.window;

	size_t dw = CGDisplayPixelsWide(C4OpenGLView.displayID);
	size_t dh = CGDisplayPixelsHigh(C4OpenGLView.displayID);
	if (iXRes == -1)
		iXRes = dw;
	if (iYRes == -1)
		iYRes = dh;
	ActualFullscreenX = iXRes;
	ActualFullscreenY = iYRes;
	[C4OpenGLView setSurfaceBackingSizeOf:[C4OpenGLView mainContext] width:ActualFullscreenX height:ActualFullscreenY];
	if ((window.styleMask & NSFullScreenWindowMask) == 0)
	{
		[window setResizeIncrements:NSMakeSize(1.0, 1.0)];
		pWindow->SetSize(iXRes, iYRes);
		[controller setFullscreen:fFullScreen];
		[window setAspectRatio:[[window contentView] frame].size];
		[window center];
	}
	else
	{
		[window toggleFullScreen:window];
		pWindow->SetSize(dw, dh);
	}
	if (!fFullScreen)
		[window makeKeyAndOrderFront:nil];
	OnResolutionChanged(iXRes, iYRes);
	return true;
}

bool C4AbstractApp::ApplyGammaRamp(struct _GAMMARAMP &ramp, bool fForce)
{
	CGGammaValue r[256];
	CGGammaValue g[256];
	CGGammaValue b[256];
	for (int i = 0; i < 256; i++)
	{
		r[i] = static_cast<float>(ramp.red[i])/65535.0;
		g[i] = static_cast<float>(ramp.green[i])/65535.0;
		b[i] = static_cast<float>(ramp.blue[i])/65535.0;
	}
	CGSetDisplayTransferByTable(C4OpenGLView.displayID, 256, r, g, b);
	return true;
}

bool C4AbstractApp::SaveDefaultGammaRamp(struct _GAMMARAMP &ramp)
{
	CGGammaValue r[256];
	CGGammaValue g[256];
	CGGammaValue b[256];
	uint32_t count;
	CGGetDisplayTransferByTable(C4OpenGLView.displayID, 256, r, g, b, &count);
	for (int i = 0; i < 256; i++)
	{
		ramp.red[i]   = r[i]*65535;
		ramp.green[i] = g[i]*65535;
		ramp.blue[i]  = b[i]*65535;
	}
	return true;
}

#endif

bool IsGermanSystem()
{
	id languages = [[NSUserDefaults standardUserDefaults] valueForKey:@"AppleLanguages"];
	return languages && [[languages objectAtIndex:0] isEqualToString:@"de"];
}

bool OpenURL(const char* szURL)
{
	std::string command = std::string("open ") + '"' + szURL + '"';
	std::system(command.c_str());
	return true;
}

bool EraseItemSafe(const char* szFilename)
{
	NSString* filename = [NSString stringWithUTF8String: szFilename];
	return [[NSWorkspace sharedWorkspace]
		performFileOperation: NSWorkspaceRecycleOperation
		source: [filename stringByDeletingLastPathComponent]
		destination: @""
		files: [NSArray arrayWithObject: [filename lastPathComponent]]
		tag: 0];
}
