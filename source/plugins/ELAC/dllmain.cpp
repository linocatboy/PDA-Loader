#include <iostream>
#include <vector>
#include "windows.h"
#include <Dbt.h>
#include "Constants.h"
#include "MainModule.h"
#include "Input/Mouse/Mouse.h"
#include "Input/Keyboard/Keyboard.h"
#include "Input/DirectInput/DirectInput.h"
#include "Input/Xinput/Xinput.h"
#include "Input/DirectInput/Ds4/DualShock4.h"
#include "Components/EmulatorComponent.h"
#include "Components/Input/InputEmulator.h"
#include "Components/Input/TouchPanelEmulator.h"
#include "Components/SysTimer.h"
#include "Components/PlayerDataManager.h"
#include "Components/FrameRateManager.h"
#include "Components/CameraController.h"
#include "Components/ScaleComponent.h"
#ifdef _600
#include "Components/GLComponent.h"
#endif
#include "Components/FastLoader.h"
#include "Components/DebugComponent.h"
#include "Utilities/Stopwatch.h"
#include "FileSystem/ConfigFile.h"

using namespace ELAC::Components;
using namespace ELAC::Utilities;
using namespace ELAC::FileSystem;

LRESULT CALLBACK MessageWindowProcessCallback(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI WindowMessageDispatcher(LPVOID);
VOID RegisterMessageWindowClass();

struct
{
	DWORD ID = NULL;
	HANDLE Handle = NULL;
} MessageThread;

const wchar_t* MessageWindowClassName = TEXT("MessageWindowClass");
const wchar_t* MessageWindowName = TEXT("MessageWindowTitle");

namespace ELAC
{
	const std::string COMPONENTS_CONFIG_FILE_NAME = "plugins\\components.ini";
	const std::string RESOLUTION_CONFIG_FILE_NAME = "plugins\\graphics.ini";

	const DWORD JMP_HOOK_SIZE = 0x5;

	const DWORD HookByteLength = 0x5;
	const void* UpdateReturnAddress = (void*)(ENGINE_UPDATE_HOOK_TARGET_ADDRESS + HookByteLength);
	const void* TouchReactionReturnAddress = (void*)(TOUCH_REACT_HOOK_TARGET_ADDRESS + HookByteLength);

	const float DefaultResolutionWidth = 1280.0f;
	const float DefaultResolutionHeight = 720.0f;

	void* originalTouchReactAetFunc = (void*)0x408510;

	int* resolutionWidthPtr = (int*)RESOLUTION_WIDTH_ADDRESS;
	int* resolutionHeightPtr = (int*)RESOLUTION_HEIGHT_ADDRESS;

	float renderWidth, renderHeight;

	float elpasedTime;
	bool hasWindowFocus, hadWindowFocus;
	bool firstUpdateTick = true;
	bool DeviceConnected = true;

	Stopwatch updateStopwatch;
	std::vector<EmulatorComponent*> components;

	void InstallHook(BYTE* address, DWORD overrideAddress, DWORD length)
	{
		DWORD oldProtect, dwBkup, dwRelAddr;

		VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtect);

		// calculate the distance between our address and our target location
		// and subtract the 5bytes, which is the size of the jmp
		// (0xE9 0xAA 0xBB 0xCC 0xDD) = 5 bytes
		dwRelAddr = (DWORD)(overrideAddress - (DWORD)address) - JMP_HOOK_SIZE;

		*address = JMP_OPCODE;

		// overwrite the next 4 bytes (which is the size of a DWORD)
		// with the dwRelAddr
		*((DWORD*)(address + 1)) = dwRelAddr;

		// overwrite the remaining bytes with the NOP opcode (0x90)
		for (DWORD x = JMP_HOOK_SIZE; x < length; x++)
			*(address + x) = NOP_OPCODE;

		VirtualProtect(address, length, oldProtect, &dwBkup);
	}

	void AddComponents()
	{
		EmulatorComponent* allComponents[]
		{
			new InputEmulator(),
			new TouchPanelEmulator(),
			new SysTimer(),
			new PlayerDataManager(),
			new FrameRateManager(),
			new CameraController(),
			new ScaleComponent(),
#ifdef _600
			new GLComponent(),
#endif
			new FastLoader(),
			new DebugComponent(),
		};

		ConfigFile componentsConfig(MainModule::GetModuleDirectory(), std::wstring(COMPONENTS_CONFIG_FILE_NAME.begin(), COMPONENTS_CONFIG_FILE_NAME.end()));
		bool success = componentsConfig.OpenRead();

		if (!success)
		{
			printf("AddComponents(): Unable to parse %s\n", COMPONENTS_CONFIG_FILE_NAME.c_str());
			return;
		}

		int componentCount = sizeof(allComponents) / sizeof(EmulatorComponent*);
		components.reserve(componentCount);

		std::string trueString = "true";
		std::string falseString = "false";

		for (int i = 0; i < componentCount; i++)
		{
			std::string* value;

			auto name = allComponents[i]->GetDisplayName();
			//printf("AddComponents(): searching name: %s\n", name);

			if (componentsConfig.TryGetValue(name, value))
			{
				//printf("AddComponents(): %s found\n", name);

				if (*value == trueString)
				{
					//printf("AddComponents(): enabling %s...\n", name);
					components.push_back(allComponents[i]);
				}
				else if (*value == falseString)
				{
					//printf("AddComponents(): disabling %s...\n", name);
				}
				else
				{
					//printf("AddComponents(): invalid value %s for component %s\n", value, name);
				}

				delete value;
			}
			else
			{
				//printf("AddComponents(): component %s not found\n", name);
				delete allComponents[i];
			}
		}
	}

	void InitializeTick()
	{
		RegisterMessageWindowClass();
		if ((MessageThread.Handle = CreateThread(0, 0, WindowMessageDispatcher, 0, 0, 0)) == NULL)
			printf("InitializeTick(): CreateThread() Error: %d\n", GetLastError());
		AddComponents();

		MainModule::DivaWindowHandle = FindWindow(0, MainModule::DivaWindowName);

		if (MainModule::DivaWindowHandle == NULL)
			MainModule::DivaWindowHandle = FindWindow(0, MainModule::GlutDefaultName);

		HRESULT diInitResult = Input::InitializeDirectInput(MainModule::Module);

		if (FAILED(diInitResult))
			printf("InitializeTick(): Failed to initialize DirectInput. Error: 0x%08X\n", diInitResult);

		updateStopwatch.Start();
	}

	void UpdateTick()
	{
		if (firstUpdateTick)
		{
			InitializeTick();
			firstUpdateTick = false;

			for (auto& component : components)
				component->Initialize();
		}

		if (DeviceConnected)
		{
			DeviceConnected = false;

			if (!Input::DualShock4::InstanceInitialized())
			{
				if (Input::DualShock4::TryInitializeInstance())
					printf("UpdateTick(): DualShock4 connected and initialized\n");
			}
		}

		elpasedTime = updateStopwatch.Restart();

		for (auto& component : components)
		{
			component->SetElapsedTime(elpasedTime);
			component->Update();
		}

		hadWindowFocus = hasWindowFocus;
		hasWindowFocus = MainModule::DivaWindowHandle == NULL || GetForegroundWindow() == MainModule::DivaWindowHandle;

		if (hasWindowFocus)
		{
			Input::Keyboard::GetInstance()->PollInput();
			Input::Mouse::GetInstance()->PollInput();
			Input::Xinput::GetInstance()->PollInput();

			if (Input::DualShock4::GetInstance() != nullptr)
			{
				if (!Input::DualShock4::GetInstance()->PollInput())
				{
					Input::DualShock4::DeleteInstance();
					printf("UpdateTick(): DualShock4 connection lost\n");
				}
			}

			for (auto& component : components)
				component->UpdateInput();
		}

		if (hasWindowFocus && !hadWindowFocus)
		{
			for (auto& component : components)
				component->OnFocusGain();
		}

		if (!hasWindowFocus && hadWindowFocus)
		{
			for (auto& component : components)
				component->OnFocusLost();
		}
	}

	void _declspec(naked) UpdateFunctionHook()
	{
		_asm
		{
			call UpdateTick
			jmp[UpdateReturnAddress]
		}
	}

	void _declspec(naked) TouchReactionAetFunctionHook()
	{
		_asm
		{
			// this is the function we replaced with the hook jump
			call originalTouchReactAetFunc

			// move X/Y into registers
			movss xmm1, [esp + 0x154 + 0x4]
			movss xmm2, [esp + 0x154 + 0x8]

			// calculate X scale factor
			movss xmm3, DefaultResolutionWidth
			divss xmm3, renderWidth

			// calculate Y scale factor
			movss xmm4, DefaultResolutionHeight
			divss xmm4, renderHeight

			// multiply positions by scale factors
			mulss xmm1, xmm3
			mulss xmm2, xmm4

			// move X/Y back into stack variables
			movss[esp + 0x154 + 0x4], xmm1
			movss[esp + 0x154 + 0x8], xmm2

			// return to original function
			jmp[TouchReactionReturnAddress]
		}
	}

	void InitializeHooks()
	{
		ConfigFile resolutionConfig(MainModule::GetModuleDirectory(), std::wstring(RESOLUTION_CONFIG_FILE_NAME.begin(), RESOLUTION_CONFIG_FILE_NAME.end()));
		bool success = resolutionConfig.OpenRead();
		if (!success)
		{
			printf("Resolution: Unable to parse %s\n", RESOLUTION_CONFIG_FILE_NAME.c_str());
		}

		renderWidth = (float)*resolutionWidthPtr;
		renderHeight = (float)*resolutionHeightPtr;

		if (success) {
			int maxWidth = 2560;
			int maxHeight = 1440;
			std::string* value;
			std::string trueValue = "true";
			bool isEnabled = false;
			if (resolutionConfig.TryGetValue("customRes", value))
			{
				if (*value == trueValue)
					isEnabled = true;
			}

			if (isEnabled == true)

			{
				if (resolutionConfig.TryGetValue("maxWidth", value))
				{
					maxWidth = std::stoi(*value);
					//printf(value->c_str());
				}
				if (resolutionConfig.TryGetValue("maxHeight", value))
				{
					maxHeight = std::stoi(*value);
					//printf(value->c_str());
				}

				std::vector<unsigned char> arrayOfByte(4);
				std::vector<unsigned char> arrayOfByte2(4);

				for (int i = 0; i < 4; i++)
					arrayOfByte[3 - i] = (maxWidth >> (i * 8));

				for (int i = 0; i < 4; i++)
					arrayOfByte2[3 - i] = (maxHeight >> (i * 8));

				DWORD prtk, bk;
				VirtualProtect((BYTE*)0x00F06290, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00F06290 + 0) = arrayOfByte2[3];
				*((byte*)0x00F06290 + 1) = arrayOfByte2[2];
				*((byte*)0x00F06290 + 2) = arrayOfByte2[1];
				*((byte*)0x00F06290 + 3) = arrayOfByte2[0];
				VirtualProtect((BYTE*)0x00F06290, 4, prtk, &bk);

				VirtualProtect((BYTE*)0x00F0628C, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00F0628C + 0) = arrayOfByte[3];
				*((byte*)0x00F0628C + 1) = arrayOfByte[2];
				*((byte*)0x00F0628C + 2) = arrayOfByte[1];
				*((byte*)0x00F0628C + 3) = arrayOfByte[0];
				VirtualProtect((BYTE*)0x00F0628C, 4, prtk, &bk);

				VirtualProtect((BYTE*)0x00F06268, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00F06268 + 0) = arrayOfByte2[3];
				*((byte*)0x00F06268 + 1) = arrayOfByte2[2];
				*((byte*)0x00F06268 + 2) = arrayOfByte2[1];
				*((byte*)0x00F06268 + 3) = arrayOfByte2[0];
				VirtualProtect((BYTE*)0x00F06268, 4, prtk, &bk);

				VirtualProtect((BYTE*)0x00F06264, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00F06264 + 0) = arrayOfByte[3];
				*((byte*)0x00F06264 + 1) = arrayOfByte[2];
				*((byte*)0x00F06264 + 2) = arrayOfByte[1];
				*((byte*)0x00F06264 + 3) = arrayOfByte[0];
				VirtualProtect((BYTE*)0x00F06264, 4, prtk, &bk);

				/*
				VirtualProtect((BYTE*)0x00B6E288, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00B6E288 + 0) = arrayOfByte2[3];
				*((byte*)0x00B6E288 + 1) = arrayOfByte2[2];
				*((byte*)0x00B6E288 + 2) = arrayOfByte2[1];
				*((byte*)0x00B6E288 + 3) = arrayOfByte2[0];
				VirtualProtect((BYTE*)0x00F06268, 4, prtk, &bk);
				VirtualProtect((BYTE*)0x00B6E284, 4, PAGE_EXECUTE_READWRITE, &prtk);
				*((byte*)0x00B6E284 + 0) = arrayOfByte[3];
				*((byte*)0x00B6E284 + 1) = arrayOfByte[2];
				*((byte*)0x00B6E284 + 2) = arrayOfByte[1];
				*((byte*)0x00B6E284 + 3) = arrayOfByte[0];
				VirtualProtect((BYTE*)0x00F06264, 4, prtk, &bk);
				*/

				VirtualProtect((BYTE*)0x00463542, 6, PAGE_EXECUTE_READWRITE, &prtk);
				for (int i = 0; i < 6; i++) {
					*((byte*)0x00463542 + i) = 0x90;
				}
				VirtualProtect((BYTE*)0x00463542, 6, prtk, &bk);

				VirtualProtect((BYTE*)0x00463548, 6, PAGE_EXECUTE_READWRITE, &prtk);
				for (int i = 0; i < 6; i++) {
					*((byte*)0x00463548 + i) = 0x90;
				}
				VirtualProtect((BYTE*)0x00463548, 6, prtk, &bk);

			}
		}

		renderWidth = (float)*resolutionWidthPtr;
		renderHeight = (float)*resolutionHeightPtr;

		InstallHook((BYTE*)ENGINE_UPDATE_HOOK_TARGET_ADDRESS, (DWORD)UpdateFunctionHook, HookByteLength);

		if (renderWidth != DefaultResolutionWidth || renderHeight != DefaultResolutionHeight)
			InstallHook((BYTE*)TOUCH_REACT_HOOK_TARGET_ADDRESS, (DWORD)TouchReactionAetFunctionHook, HookByteLength);
	}

	void Dispose()
	{
		for (auto& component : components)
			delete component;

		Input::DisposeDirectInput();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		/*if (*(char*)0x004592CC == (char)0x74 || *(char*)0x004592CC == (char)0x90)
		{
		}
		else
		{
			printf("[ELAC] Certain bytes do not match PDA6.00 Exiting...\n");
			break;
		}*/
		ELAC::InitializeHooks();
		ELAC::MainModule::Module = hModule;
		break;

	case DLL_PROCESS_DETACH:
		ELAC::Dispose();
		break;
	}

	return TRUE;
}

DWORD WINAPI WindowMessageDispatcher(LPVOID lpParam)
{
	HWND windowHandle = CreateWindowW(
		MessageWindowClassName,
		MessageWindowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL,
		ELAC::MainModule::Module,
		NULL);

	if (!windowHandle)
	{
		printf("WindowMessageDispatcher(): CreateWindowW() Error: %d\n", GetLastError());
		return 1;
	}

	MessageThread.ID = GetCurrentThreadId();

	MSG message;
	DWORD returnValue;

	printf("WindowMessageDispatcher(): Entering message loop...\n");

	while (1)
	{
		returnValue = GetMessage(&message, NULL, 0, 0);
		if (returnValue != -1)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		else
		{
			printf("WindowMessageDispatcher(): GetMessage() Error: %d\n", returnValue);
		}
	}

	DestroyWindow(windowHandle);
	return 0;
}

BOOL RegisterDeviceInterface(HWND hWnd, HDEVNOTIFY* hDeviceNotify)
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter = {};

	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

	*hDeviceNotify = RegisterDeviceNotification(hWnd, &NotificationFilter, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);

	return *hDeviceNotify != NULL;
}

LRESULT CALLBACK MessageWindowProcessCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		HDEVNOTIFY hDevNotify = NULL;

		if (!RegisterDeviceInterface(hWnd, &hDevNotify))
			printf("MessageWindowProcessCallback(): RegisterDeviceInterface() Error: %d\n", GetLastError());

		break;
	}

	case WM_DEVICECHANGE:
	{
		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
			ELAC::DeviceConnected = true;
			break;

		default:
			break;
		}
	}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

VOID RegisterMessageWindowClass()
{
	WNDCLASS windowClass = { };

	windowClass.lpfnWndProc = MessageWindowProcessCallback;
	windowClass.hInstance = ELAC::MainModule::Module;
	windowClass.lpszClassName = MessageWindowClassName;

	RegisterClass(&windowClass);
}