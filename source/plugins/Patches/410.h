#pragma once
#include <vector>
#include "framework.h"

const Patch patches_410[] =
{
	// Debug tests
	{ (void*)0x005956d0,{ 0xB0, 0x01 }, "Debug" },
	{ (void*)0x005956e0,{ 0xB0, 0x01 }, "Debug" },
	//{ (void*)0x,{ 0xB0, 0x01, 0xC3 } },
	// NOP out JVS board error JMP
	{ (void*)0x00452f4c,{ 0x90, 0x90 }, "JVSBoard" },
	// NOP out sys_am sram/eprom initialization call
	{ (void*)0x00454bba,{ 0x90, 0x90, 0x90, 0x90, 0x90 }, "NopSRAM" },
	// Return early to prevent null reference for some sys_am object
	{ (void*)0x004b5250,{ 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 }, "sys_amEarly" },
	// Skip the sys_am eprom time bomb checks
	{ (void*)0x00645103,{ 0xEB, 0x5C, 0x90, 0x90, 0x90 }, "sys_amBomb" },
	// Jump past the Wrong Resolution Setting error
	//{ (void*)0x006521F3,{ 0xEB }, "RightResoultion" },
	// Always skip the SYSTEM_STARTUP_ERROR game state
	{ (void*)0x00646d14,{ 0xB0, 0x01 }, "SYSTEM_STARTUP_ERROR" },
	// *But of course we have a valid keychip*, return true
	{ (void*)0x007ed5e0,{ 0xB0, 0x01, 0xC3 }, "KeychipTrue" },
	// Jump past the initial amMaster initialization
	//{ (void*)0x007f3d4a,{ 0xE9, 0x96, 0x00, 0x00 }, "amMasterSkip" },
	// Skip pcpaOpenClient loop
	{ (void*)0x009d8620,{ 0xC2, 0x14, 0x00 }, "pcpaOpenClient" },
	// Move SYSTEM_OK into edx
	{ (void*)0x007f84b1,{ 0xBA, 0xFF, 0xFF }, "SYSTEM_OK" },
	// Move edx into the LocationServerStatus variable and jump past the router timeout initialization
	{ (void*)0x007f84b6,{ 0x89, 0x15, 0xB0, 0x6E, 0x08, 0x01, 0xEB, 0x14, 0x90, 0x90 }, "noRouterTimeout" },
	// Let the router checks time out so we can skip the error
	{ (void*)0x007f85e1,{ 0x90, 0x90 }, "noRouterTimeout" },
	// Then immediately move on to the DHCP 3 initialization phase
	{ (void*)0x007f866f,{ 0xE9, 0x43, 0x01, 0x00, 0x00, 0x90, 0x90 }, "noRouterTimeout" },
	{ (void*)0x007f875c,{ 0x5A }, "noRouterTimeout" },

	//Ignore SYSTEM_STARTUP Location Server checks
	{ (void*)0x007f80bf,{ 0x90, 0x90 }, "noRouterTimeout" },
	// Avoid JVS related null reference
	//{ (void*)0x00820685,{ 0x90, 0x90, 0x90, 0x90, 0x90 }, "NullJVS" },
	// Jump past the PollInput function so we can write our own input
	{ (void*)0x0044f9b8,{ 0xEB }, "PollInput" },
	// Set the full screen glutGameModeString refresh rate to its default value
	{ (void*)0x0045390A,{ 0x00 }, "glutGameRefresh" },
	// Use GLUT_CURSOR_RIGHT_ARROW instead of GLUT_CURSOR_NONE
	{ (void*)0x00453934,{ 0x00 }, "GLUT_CURSOR" },
	// Ignore CLOSE SETTINGS check
	{ (void*)0x004d98b0,{ 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 }, "CLOSE_SETTINGS" },
	// Always return true for the SelCredit enter SelPv check
	{ (void*)0x00601950,{ 0xB0, 0x01, 0xC3 }, "CreditSkip" },
	// Return early before resetting to the default PlayerData so we don't need to keep updating the PlayerData struct
	{ (void*)0x006cfd50,{ 0xC3 }, "EarlyPlayerData" },
	// Ignore the EngineClear variable to clear the framebuffer at all resolutions
	{ (void*)0x007084ae,{ 0x90, 0x90 }, "CleanFramebuffer" },
	{ (void*)0x0070859d,{ 0xEB }, "CleanFramebuffer" },
	// Jump past the VP SelWatch button enabled checks
	{ (void*)0x00786158,{ 0xEB, 0x2D, 0x90, 0x90, 0x90 }, "FreePVWatch" },
	// Write ram files to the current directory instead of Y:/ram
	{ (void*)0x007f12c1,{ 0xEB }, "ram" },
	PATCHES_END
};