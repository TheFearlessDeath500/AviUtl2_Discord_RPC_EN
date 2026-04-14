#include <windows.h>
#include <commctrl.h>
#include <vfw.h>
#include <process.h>
#include <string>
#include <shlwapi.h>
#include <iostream>
#include <chrono>
#include "discord_rpc.h"
#include "plugin2.h"

#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "discord-rpc.lib")

#define MENU_ID 1000 
#define MENU_ID_ENABLE_RPC 1001
#define MENU_ID_SHOW_PFNAME 1002


WNDPROC g_pfnOriginalWndProc = NULL;
HWND    g_hExEdit2 = NULL;
HMENU   g_hViewMenu = NULL;
HMENU   hRpcMenu = NULL;
bool    discordInitialized = false;
wchar_t pfName[256];
LRESULT CALLBACK RPCSetting(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitDiscord();
void Update();
bool isrpcenable = false;
bool ispfname = false;
HINSTANCE hInstance;
static EDIT_HANDLE* g_edit_handle = nullptr;
int current_frame = 0;
const char* edit_state = nullptr;

bool g_timeline_ready = false;

std::string GetConfigIniPath()
{
	char path[MAX_PATH];
	GetModuleFileNameA(hInstance, path, MAX_PATH);
	PathRemoveFileSpecA(path);

	strcat_s(path, "\\discordrpc.ini");
	return path;
}

void SaveSettings()
{
	std::string iniPath = GetConfigIniPath();
	WritePrivateProfileStringA("Settings", "isrpcenable", isrpcenable ? "1" : "0", iniPath.c_str());
	WritePrivateProfileStringA("Settings", "ispfname", ispfname ? "1" : "0", iniPath.c_str());
}

void LoadSettings()
{
	std::string iniPath = GetConfigIniPath();
	char value[8];
	GetPrivateProfileStringA("Settings", "isrpcenable", "1", value, sizeof(value), iniPath.c_str());
	isrpcenable = (strcmp(value, "1") == 0);

	GetPrivateProfileStringA("Settings", "ispfname", "1", value, sizeof(value), iniPath.c_str());
	ispfname = (strcmp(value, "1") == 0);

}

std::string WcharToChar(const wchar_t* wide_str) {
	if (wide_str == nullptr) return "";
	int required_size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, NULL, 0, NULL, NULL);
	if (required_size == 0) return "";
	std::string result(required_size, 0);
	int converted_size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, &result[0], required_size, NULL, NULL);
	result.resize(converted_size - 1);
	return result;
}
bool CheckPluginMenu()
{
	if (!g_hViewMenu) return false;

	int count = GetMenuItemCount(g_hViewMenu);
	for (int i = 0; i < count; i++) {
		wchar_t text[256]{};
		GetMenuStringW(g_hViewMenu, i, text, 256, MF_BYPOSITION);
		if (wcscmp(text, L"DiscordRPCの設定") == 0) {
			return true;
		}
	}
	return false;
}

void Add_PopupMenu()
{
	if (!g_hExEdit2) return;

	HMENU hMenuBar = GetMenu(g_hExEdit2);
	if (!hMenuBar) return;

	g_hViewMenu = GetSubMenu(hMenuBar, 3);
	if (!g_hViewMenu) return;

	if (CheckPluginMenu()) return;

	if (hRpcMenu) {
		DestroyMenu(hRpcMenu);
		hRpcMenu = NULL;
	}

	hRpcMenu = CreatePopupMenu();
	if (!hRpcMenu) return;

	UINT rpcFlag = isrpcenable ? MF_CHECKED : MF_UNCHECKED;
	AppendMenuW(hRpcMenu, MF_STRING | rpcFlag, MENU_ID_ENABLE_RPC, L"RPCを有効化");

	UINT pfnameFlag = ispfname ? MF_CHECKED : MF_UNCHECKED;
	AppendMenuW(hRpcMenu, MF_STRING | pfnameFlag, MENU_ID_SHOW_PFNAME, L"プロジェクト名を表示");

	InsertMenuW(g_hViewMenu, 0, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hRpcMenu, L"DiscordRPCの設定");
	DrawMenuBar(g_hExEdit2);

	if (!g_pfnOriginalWndProc) {
		g_pfnOriginalWndProc =
			(WNDPROC)SetWindowLongPtrW(g_hExEdit2, GWLP_WNDPROC, (LONG_PTR)RPCSetting);
	}
}
void ProjectLoad(PROJECT_FILE* project)
{
	g_timeline_ready = true;
}

void InitDiscord()
{
	if (discordInitialized) {
		return;
	}
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	Discord_Initialize("1278325804794642494", &handlers, 1, NULL);
	discordInitialized = true;
}

void UpdateDiscordPresence() {
	if (!g_timeline_ready) {
		return;
	}
	if (!g_edit_handle || !g_edit_handle->get_edit_state) {
		return;
	}
	int g_edit_state = g_edit_handle->get_edit_state();
	if (g_edit_state == EDIT_HANDLE::EDIT_STATE_EDIT) {
		edit_state = u8"編集中";
	}
	else if (g_edit_state == EDIT_HANDLE::EDIT_STATE_PLAY) {
		edit_state = u8"再生中";
	}
	else if (g_edit_state == EDIT_HANDLE::EDIT_STATE_SAVE) {
		edit_state = u8"出力中";
	}
	else {
		edit_state = "";
	}
	EDIT_INFO info{};
	g_edit_handle->get_edit_info(&info, sizeof(info));
	if (isrpcenable == true)
	{
		InitDiscord();
		DiscordRichPresence presence;
		memset(&presence, 0, sizeof(presence));
		presence.state = edit_state;

		if (ispfname == false) {
			presence.details = "";
		}
		else {
			InitDiscord();
			GetWindowTextW(g_hExEdit2, pfName, 256);
			std::string pfNameS = WcharToChar(pfName);
			presence.details = pfNameS.c_str();
		}

		presence.largeImageKey = "aviutl2";
		presence.largeImageText = "AviUtl2";
		Discord_UpdatePresence(&presence);
	}
}

unsigned __stdcall Update(void* pArguments) {
	//g_hExEdit2 = FindWindowW(L"aviutl2Manager", NULL);
	Add_PopupMenu();
	while (1) {
		UpdateDiscordPresence();
		Sleep(1000);
	}
	return 0;
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) {
	LoadSettings();
	_beginthreadex(NULL, 0, &Update, NULL, 0, NULL);
	return true;
}

COMMON_PLUGIN_TABLE common_plugin_table = {
	L"DiscordRPC",
	L"DiscordRPC 1.3.1"
};

EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
	return 2004100;
}

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void) {
	return &common_plugin_table;
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host) {
	g_edit_handle = host->create_edit_handle();

	if (g_edit_handle && g_edit_handle->get_host_app_window) {
		g_hExEdit2 = g_edit_handle->get_host_app_window();
	}
	host->register_project_load_handler(ProjectLoad);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		hInstance = hModule;
	}
	return TRUE;
}

LRESULT CALLBACK RPCSetting(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_COMMAND) {
		switch (LOWORD(wParam)) {
		case MENU_ID_ENABLE_RPC:
		{
			isrpcenable = !isrpcenable;
			if (isrpcenable == false) {
				Discord_Shutdown();
				discordInitialized = false;
			}
			else {
				UpdateDiscordPresence();
			}
			UINT rpcFlag = isrpcenable ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hRpcMenu, MENU_ID_ENABLE_RPC, MF_BYCOMMAND | rpcFlag);

			SaveSettings();
			return 0;
		}
		case MENU_ID_SHOW_PFNAME:
		{
			ispfname = !ispfname;
			UpdateDiscordPresence();
			UINT pfnameFlag = ispfname ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hRpcMenu, MENU_ID_SHOW_PFNAME, MF_BYCOMMAND | pfnameFlag);

			SaveSettings();
			return 0;
		}
		}
	}
	return CallWindowProc(g_pfnOriginalWndProc, hWnd, uMsg, wParam, lParam);
}
