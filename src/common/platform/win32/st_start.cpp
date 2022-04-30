/*
** st_start.cpp
** Handles the startup screen.
**
**---------------------------------------------------------------------------
** Copyright 2006-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "resource.h"

#include "st_start.h"
#include "cmdlib.h"

#include "i_system.h"
#include "i_input.h"
#include "hardware.h"
#include "filesystem.h"
#include "m_argv.h"
#include "engineerrors.h"
#include "s_music.h"
#include "printf.h"
#include "startupinfo.h"
#include "i_interface.h"
#include "texturemanager.h"

// MACROS ------------------------------------------------------------------

// How many ms elapse between blinking text flips. On a standard VGA
// adapter, the characters are on for 16 frames and then off for another 16.
// The number here therefore corresponds roughly to the blink rate on a
// 60 Hz display.
#define BLINK_PERIOD			267
#define TEXT_FONT_NAME			"vga-rom-font.16"


// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void RestoreConView();
void LayoutMainWindow (HWND hWnd, HWND pane);
int LayoutNetStartPane (HWND pane, int w);

bool ST_Util_CreateStartupWindow ();
void ST_Util_SizeWindowForBitmap (int scale);
void ST_Util_InvalidateRect (HWND hwnd, BitmapInfo *bitmap_info, int left, int top, int right, int bottom);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static INT_PTR CALLBACK NetStartPaneProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern HINSTANCE g_hInst;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FStartupScreen *StartScreen;

CUSTOM_CVAR(Int, showendoom, 0, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	if (self < 0) self = 0;
	else if (self > 2) self=2;
}

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// FStartupScreen :: CreateInstance
//
// Initializes the startup screen for the detected game.
// Sets the size of the progress bar and displays the startup screen.
//
//==========================================================================

FStartupScreen *FStartupScreen::CreateInstance(int max_progress)
{
	FStartupScreen *scr = NULL;
	HRESULT hr = HRESULT(-1);

	if (!Args->CheckParm("-nostartup"))
	{
		if (GameStartupInfo.Type == FStartupInfo::HexenStartup)
		{
			scr = new FHexenStartupScreen(max_progress, hr);
		}
		else if (GameStartupInfo.Type == FStartupInfo::HereticStartup)
		{
			scr = new FHereticStartupScreen(max_progress, hr);
		}
		else if (GameStartupInfo.Type == FStartupInfo::StrifeStartup)
		{
			scr = new FStrifeStartupScreen(max_progress, hr);
		}
		if (scr != NULL && FAILED(hr))
		{
			delete scr;
			scr = NULL;
		}
	}
	if (scr == NULL)
	{
		scr = new FBasicStartupScreen(max_progress, true);
	}
	return scr;
}

//==========================================================================
//
// FBasicStartupScreen Constructor
//
// Shows a progress bar at the bottom of the window.
//
//==========================================================================

FBasicStartupScreen::FBasicStartupScreen(int max_progress, bool show_bar)
: FStartupScreen(max_progress)
{
	if (show_bar)
	{
	}
	NetMaxPos = 0;
	NetCurPos = 0;
}

//==========================================================================
//
// FBasicStartupScreen Destructor
//
// Called just before entering graphics mode to deconstruct the startup
// screen.
//
//==========================================================================

FBasicStartupScreen::~FBasicStartupScreen()
{
}

//==========================================================================
//
// FBasicStartupScreen :: Progress
//
// Bumps the progress meter one notch.
//
//==========================================================================

void FBasicStartupScreen::Progress()
{
	if (CurPos < MaxPos)
	{
		CurPos++;
	}
}

//==========================================================================
//
// FBasicStartupScreen :: NetInit
//
// Shows the network startup pane if it isn't visible. Sets the message in
// the pane to the one provided. If numplayers is 0, then the progress bar
// is a scrolling marquee style. If numplayers is 1, then the progress bar
// is just a full bar. If numplayers is >= 2, then the progress bar is a
// normal one, and a progress count is also shown in the pane.
//
//==========================================================================

void FBasicStartupScreen::NetInit(const char *message, int numplayers)
{
	NetMaxPos = numplayers;
	NetMaxPos = numplayers;
	NetCurPos = 0;
	NetProgress(1);	// You always know about yourself
}

//==========================================================================
//
// FBasicStartupScreen :: NetDone
//
// Removes the network startup pane.
//
//==========================================================================

void FBasicStartupScreen::NetDone()
{
}

//==========================================================================
//
// FBasicStartupScreen :: NetMessage
//
// Call this between NetInit() and NetDone() instead of Printf() to
// display messages, in case the progress meter is mixed in the same output
// stream as normal messages.
//
//==========================================================================

void FBasicStartupScreen::NetMessage(const char *format, ...)
{
	FString str;
	va_list argptr;

	va_start (argptr, format);
	str.VFormat (format, argptr);
	va_end (argptr);
	Printf ("%s\n", str.GetChars());
}

//==========================================================================
//
// FBasicStartupScreen :: NetProgress
//
// Sets the network progress meter. If count is 0, it gets bumped by 1.
// Otherwise, it is set to count.
//
//==========================================================================

void FBasicStartupScreen :: NetProgress(int count)
{
	if (count == 0)
	{
		NetCurPos++;
	}
	else
	{
		NetCurPos = count;
	}
}

//==========================================================================
//
// FBasicStartupScreen :: NetLoop
//
// The timer_callback function is called at least two times per second
// and passed the userdata value. It should return true to stop the loop and
// return control to the caller or false to continue the loop.
//
// ST_NetLoop will return true if the loop was halted by the callback and
// false if the loop was halted because the user wants to abort the
// network synchronization.
//
//==========================================================================

bool FBasicStartupScreen::NetLoop(bool (*timer_callback)(void *), void *userdata)
{
	return false;
}

//==========================================================================
//
// NetStartPaneProc
//
// DialogProc for the network startup pane. It just waits for somebody to
// click a button, and the only button available is the abort one.
//
//==========================================================================

static INT_PTR CALLBACK NetStartPaneProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

//==========================================================================
//
// FGraphicalStartupScreen Constructor
//
// This doesn't really do anything. The subclass is responsible for
// creating the resources that will be freed by this class's destructor.
//
//==========================================================================

FGraphicalStartupScreen::FGraphicalStartupScreen(int max_progress)
: FBasicStartupScreen(max_progress, false)
{
}

//==========================================================================
//
// FGraphicalStartupScreen Destructor
//
//==========================================================================

FGraphicalStartupScreen::~FGraphicalStartupScreen()
{
	if (StartupBitmap != NULL)
	{
		ST_Util_FreeBitmap (StartupBitmap);
		StartupBitmap = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FHexenStartupScreen::SetWindowSize()
{
	ST_Util_SizeWindowForBitmap(1);
	
}

//==========================================================================
//
//
//
//==========================================================================

void FHereticStartupScreen::SetWindowSize()
{
	ST_Util_SizeWindowForBitmap(1);
	
}

//==========================================================================
//
//
//
//==========================================================================

void FStrifeStartupScreen::SetWindowSize()
{
	ST_Util_SizeWindowForBitmap(2);
	
}

//==========================================================================
//
// ST_Endoom
//
// Shows an ENDOOM text screen
//
//==========================================================================

int RunEndoom()
{
	if (showendoom == 0 || endoomName.Len() == 0) 
	{
		return 0;
	}

	int endoom_lump = fileSystem.CheckNumForFullName (endoomName, true);

	uint8_t endoom_screen[4000];
	uint8_t *font;
	bool blinking = false, blinkstate = false;
	int i;

	if (endoom_lump < 0 || fileSystem.FileLength (endoom_lump) != 4000)
	{
		return 0;
	}

	if (fileSystem.GetFileContainer(endoom_lump) == fileSystem.GetMaxIwadNum() && showendoom == 2)
	{
		// showendoom==2 means to show only lumps from PWADs.
		return 0;
	}

	font = ST_Util_LoadFont (TEXT_FONT_NAME);
	if (font == NULL)
	{
		return 0;
	}

	if (!ST_Util_CreateStartupWindow())
	{
		ST_Util_FreeFont (font);
		return 0;
	}

	I_ShutdownGraphics ();
	RestoreConView ();
	S_StopMusic(true);

	fileSystem.ReadFile (endoom_lump, endoom_screen);

	// Draw the loading screen to a bitmap.
	StartupBitmap = ST_Util_AllocTextBitmap (font);
	ST_Util_DrawTextScreen (StartupBitmap, endoom_screen, font);

	ST_Util_SizeWindowForBitmap (1);

	// Does this screen need blinking?
	for (i = 0; i < 80*25; ++i)
	{
		if (endoom_screen[1+i*2] & 0x80)
		{
			blinking = true;
			break;
		}
	}
	/*
	if (blinking && SetTimer (W indow, 0x5A15A, BLINK_PERIOD, NULL) == 0)
	{
		blinking = false;
	}
	*/
	return 0;

}

void ST_Endoom()
{
	TexMan.DeleteAll();
	int code = RunEndoom();
	throw CExitEvent(code);

}

//==========================================================================
//
// ST_Util_CreateStartupWindow
//
// Creates the static control that will draw the startup screen.
//
//==========================================================================

bool ST_Util_CreateStartupWindow ()
{
	return true;
}

//==========================================================================
//
// ST_Util_SizeWindowForBitmap
//
// Resizes the main window so that the startup bitmap will be drawn
// at the desired scale.
//
//==========================================================================

void ST_Util_SizeWindowForBitmap (int scale)
{
}

//==========================================================================
//
// ST_Util_InvalidateRect
//
// Invalidates the portion of the window that the specified rect of the
// bitmap appears in.
//
//==========================================================================

void ST_Util_InvalidateRect(BitmapInfo* bitmap_info, int left, int top, int right, int bottom)
{
	//ST_Util_InvalidateRect(StartupScreen , bitmap_info, left, top, right, bottom);
}
