#pragma once
#include <vector>
#include "framework.h"

// By linocatboy; backported from 301
const Patch patches_200[] =
{
	// Debug
	{ (void*)0x0054BE20,{ 0xB0, 0x01 }, "dwgui" }, //done
	{ (void*)0x0054BE30,{ 0xB0, 0x01 }, "dwgui" }, //done
	//{ (void*)0x0060EC48,{ 0x76, 0xEB, 0xFF, 0x00 } }, //broken
	{ (void*)0x005ED9E4,{ 0x00 }, "ShowCursor" }, //done cursor
	{ (void*)0x005ED862,{ 0x00 }, "DebugCursor" }, //done cursor
	// Enable Debug Cursor Moving
	//{ (void*)0x005EDFF7,{ 0xE8, 0xB4, 0x0B, 0xE3, 0xFF }, "MoveDebugCursor" },
	// NOP out JVS board error JMP //done
	{ (void*)0x005ED3EC,{ 0x90, 0x90 }, "JVSBoard" },
	// Skip amMaster checks //done
	{ (void*)0x00756630,{ 0xB0, 0x01, 0xC3 }, "amMaster" },
	// Skip pcpaOpenClient loop //done
	{ (void*)0x0090BAf0,{ 0xC2, 0x14, 0x00 }, "pcpaOpenClient" },
	// *But of course we have a valid keychip*, return true //done
	{ (void*)0x00757E80,{ 0xB0, 0x01, 0xC3 }, "KeychipTrue" },
	// Just completely ignore all SYSTEM_STARTUP errors //done
	{ (void*)0x005F8F90,{ 0xC3 }, "SYSTEM_STARTUP_ERROR" },
	// Skip parts of the network check state
	//{ (void*)0x00761655,{ 0xE9, 0x13, 0x01, 0x00, 0x00 }, "NetworkCheck" },
	// Ignore SYSTEM_STARTUP Location Server checks
	{ (void*)0x00760FD9,{ 0x90, 0x90 }, "LocServer" },
	// Let the router checks time out so we can skip the error //done
	{ (void*)0x00761686,{ 0x90, 0x90 }, "noRouterTimeout" },
	// Set the initial wait timer value to 0
	{ (void*)0x00706D2E,{ 0x00, 0x00 }, "NoWait" },
	{ (void*)0x00762F82,{ 0x00, 0x00 }, "NoWait" },
	{ (void*)0x00762F99,{ 0x00, 0x00 }, "NoWait" },
	// Always skip the SYSTEM_STARTUP_ERROR game state //done
	{ (void*)0x005F9CD4,{ 0xB0, 0x01 }, "SYSTEM_STARTUP_ERROR" },
	// Avoid JVS related null reference //done
	//{ (void*)0x007E23A5,{ 0x90, 0x90, 0x90, 0x90, 0x90 } },
	// Set the full screen glutGameModeString refresh rate to its default value
	{ (void*)0x005ED9BA,{ 0x00 }, "glutGameRefresh" },
	// Jump past the PollInput function so we can write our own input //done
	{ (void*)0x007123B9,{ 0xC3 }, "PollInput" },
	// Ignore CLOSE SETTINGS check //done
	{ (void*)0x004A0910,{ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 }, "CLOSE_SETTINGS" },
	// Always return true for the SelCredit enter SelPv check //done
	{ (void*)0x005B5B30,{ 0xB0, 0x01, 0xC3 }, "CreditSkip" },
	// Return early before resetting to the default PlayerData so we don't need to keep updating the PlayerData struct //done
	{ (void*)0x0066AE00,{ 0xC3 }, "EarlyPlayerData" },
	// Ignore the EngineClear variable to clear the framebuffer at all resolutions //done
	{ (void*)0x006983BE,{ 0x90, 0x90 }, "CleanFramebuffer" },
	{ (void*)0x0069843D,{ 0xEB }, "CleanFramebuffer" },
	// Jump past the VP SelWatch button enabled checks //done
	{ (void*)0x00706148,{ 0xEB, 0x2D, 0x90, 0x90, 0x90 }, "FreePVWatch" },
	// Write ram files to the current directory instead of Y:/ram //done
	{ (void*)0x0075AE81,{ 0xEB }, "ram" },
	PATCHES_END
};