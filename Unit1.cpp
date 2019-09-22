//---------------------------------------------------------------------------

#include <vcl.h>

#include <algorithm>
#include <iterator>
#include <Psapi.h>
#include <Mmsystem.h>
#include <Registry.hpp>
#include <stdio.h>  // printf
#include <io.h>     // _open_osfhandle, dup2

#pragma hdrstop

#include "Unit1.h"

#include "osd.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

struct TermWnd {
	UnicodeString wndtitle;
	UnicodeString wndclass;

	TermWnd(UnicodeString ttl, UnicodeString cls) {
		wndtitle = ttl;
		wndclass = cls;
  }
};

HHOOK hHook = NULL;
bool Working = false;
bool Terminating = false;
DWORD Key = 0;
DWORD TermKey = 0;
int Game = -1;
int SuspendProcess = 1;
TStringList *pGamesList;
TList *pTermList;
tOSD *pOSD;

#define MODULE_NAME L"museca.dll"
#define MEM_OFFSET 0xC00 // offset padding relative to .dll file

#define CONSOLE_TRIGGER0 L"TITLEDEMO_FLOW"
#define CONSOLE_TRIGGER1 L"WARNING_SCENE"
#define CONSOLE_TRIGGER2 L"TITLE_SCENE"
#define CONSOLE_TRIGGER3 L"DEMO_GAME"
#define CONSOLE_TRIGGER4 L"CARD_ENTRY_SCENE"
#define CONSOLE_TRIGGER5 L"CARD_ENTRY_JOIN_SCENE"
#define CONSOLE_TRIGGER6 L"CARD_ENTRY_ERROREND_SCENE"

ULONG data0_offset[] = { 0x17E587 };
ULONG data1_offset[] = { 0x17E4DF };
ULONG data2_offset[] = { 0x17E060 };
#define DATA0_SIZE 6
#define DATA1_SIZE 2
#define DATA2_SIZE 6

BYTE pf_off0[][DATA0_SIZE] = { 0xFF, 0x83, 0x48, 0x14, 0x00, 0x00 };
BYTE pf_off1[][DATA1_SIZE] = { 0x7F, 0x08 };
BYTE pf_off2[][DATA2_SIZE] = { 0x8B, 0x81, 0x48, 0x14, 0x00, 0x00 };

BYTE pf_on0[][DATA0_SIZE] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
BYTE pf_on1[][DATA1_SIZE] = { 0x90, 0x90 };
BYTE pf_on2[][DATA2_SIZE] = { 0xB8, 0x03, 0x00, 0x00, 0x00, 0x90 };

HANDLE hConOut;
const COORD origin = { 0, 0 };
COORD lastpos = { 0, 0 };

//---------------------------------------------------------------------------
void TogglePFree(bool off = false);

//---------------------------------------------------------------------------
bool GrabConsole(DWORD procID)
{
	if (!AttachConsole(procID))
		return false;
	hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Clear console to avoid false triggering on old logs
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwDummy;
	GetConsoleScreenBufferInfo(hConOut, &csbi); // get screen buffer state
	FillConsoleOutputCharacter(hConOut, '\0', csbi.dwSize.X*csbi.dwSize.Y, origin, &dwDummy); // fill screen buffer with zeroes

	lastpos.X = 0;
	lastpos.Y = 0;
    Form1->TimerConsole->Enabled = true;
	return true;
}
//---------------------------------------------------------------------------
void ReleaseConsole()
{
    Form1->TimerConsole->Enabled = false;
	FreeConsole();
}
//---------------------------------------------------------------------------
void PollConsole(HANDLE hConsole)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwDummy;
	// get screen buffer state
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	int lineWidth = csbi.dwSize.X;

	if ((csbi.dwCursorPosition.X == lastpos.X) && (csbi.dwCursorPosition.Y == lastpos.Y))
		return; // text cursor did not move, do nothing
	else
	{
		DWORD count = (csbi.dwCursorPosition.Y-lastpos.Y)*lineWidth+csbi.dwCursorPosition.X-lastpos.X;
		// read newly output characters starting from last cursor position
		LPTSTR buffer = (LPTSTR) LocalAlloc(0, count*sizeof(TCHAR)+2);
		ReadConsoleOutputCharacter(hConsole, buffer, count, lastpos, &count);
		// fill screen buffer with zeroes
		FillConsoleOutputCharacter(hConsole, '\0', count, lastpos, &dwDummy);

		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		lastpos = csbi.dwCursorPosition;
		GetConsoleScreenBufferInfo(hConsole, &csbi);
		if ((csbi.dwCursorPosition.X == lastpos.X) && (csbi.dwCursorPosition.Y == lastpos.Y))
		{ // text cursor did not move since this treatment, hurry to reset it to home
			SetConsoleCursorPosition(hConsole, origin);
			lastpos = origin;
		}
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

		// scan screen buffer and transmit character to real output handle
		LPTSTR scan = buffer;
		UnicodeString text;
		do
		{
			if (*scan)
			{
				DWORD len = 1;
				while (scan[len] && (len < count))
					len++;
				//WriteFile(hOutput, scan, len, &dwDummy, NULL);
				TCHAR term = scan[len];
				scan[len] = 0;
				text += scan;
				scan[len] = term;

				scan += len;
				count -= len;
			}
			else
			{
				DWORD len = 1;
				while (!scan[len] && (len < count))
					len++;
				scan += len;
				count -= len;
				len = (len+lineWidth-1)/lineWidth;
				for (;len;len--) {
					//WriteFile(hOutput, "\r\n", 2, &dwDummy, NULL);
					text += L"\r\n";
				}
			}
		} while (count);
		LocalFree(buffer);

		if (text.Pos(CONSOLE_TRIGGER0) ||
			text.Pos(CONSOLE_TRIGGER1) ||
			text.Pos(CONSOLE_TRIGGER2) ||
			text.Pos(CONSOLE_TRIGGER3) ||
			text.Pos(CONSOLE_TRIGGER4) ||
			text.Pos(CONSOLE_TRIGGER5) ||
			text.Pos(CONSOLE_TRIGGER6)) {
			TogglePFree(true);
		}
	}
}

//---------------------------------------------------------------------------
UnicodeString WinFormatError(DWORD errNo)
{
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, errNo, 0,
			(LPTSTR)&lpMsgBuf, 0, NULL);
	UnicodeString errMsg; errMsg.sprintf(L"%d: %s", errNo, (TCHAR*)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return errMsg;
}

void Error(UnicodeString msg)
{
	Form1->Memo1->Color = clRed;
	Form1->Memo1->Font->Color = clYellow;
	Form1->Memo1->Font->Style = TFontStyles() << fsBold;

	Form1->Memo1->Lines->Text = UnicodeString(L"ERROR:\n") + msg;

	Form1->Memo1->SetFocus();
	MessageBeep(MB_ICONHAND);
}

void WinError(UnicodeString msg)
{
	Error(msg + L"\n" + WinFormatError(GetLastError()));
}

//---------------------------------------------------------------------------
HWND FindGameWnd()
{
	HWND hWnd = NULL;
	Game = -1;
	for (int i = 0; i < pGamesList->Count; i++) {
		hWnd = FindWindow(pGamesList->Strings[i].c_str(), NULL);
		if (hWnd) {
			Game = i;
			break;
		}
	}
	return hWnd;
}

//---------------------------------------------------------------------------
HWND FindTermWnd()
{
	HWND hWnd = NULL;
	for (int i = 0; i < pTermList->Count; i++) {
		TermWnd *pwi = (TermWnd*)pTermList->Items[i];
		hWnd = FindWindow(pwi->wndclass.c_str(), pwi->wndtitle.c_str());
		if (hWnd) {
			break;
		}
	}
	return hWnd;
}

//---------------------------------------------------------------------------
void TogglePFree(bool off)
{
	Working = true;
	ReleaseConsole();
	Form1->Memo1->Clear();
	UnicodeString txt;
	UnicodeString txt1;
	UnicodeString txt2;

	HANDLE hProc = NULL;
	HWND hWnd = NULL;
	DWORD procID = NULL;

	BYTE data0[DATA0_SIZE];
	BYTE data1[DATA1_SIZE];
	BYTE data2[DATA2_SIZE];

	HMODULE hMods[1024];
	DWORD cbNeeded;
	bool found = false;
	LPVOID baseAddr = 0;

	hWnd = FindGameWnd();
	if (!hWnd) {
		Error(L"Game window not found");
		goto getout;
	}

	if (!GetWindowThreadProcessId(hWnd, &procID)) {
		WinError(L"Get window process ID failed");
		goto getout;
	}

	if (procID != NULL)
	{
		if (SuspendProcess) {
			if (!DebugActiveProcess(procID)) {
				WinError(L"Suspend process failed");
				goto getout;
			}
		}

		hProc = OpenProcess(/*PROCESS_ALL_ACCESS*/
				PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
				PROCESS_QUERY_INFORMATION, FALSE, procID);
	}
	if (!hProc) {
		WinError(L"Can not open process");
		goto getout;
	}

	if (!EnumProcessModules(hProc, hMods, sizeof(hMods), &cbNeeded)) {
		WinError(L"Enumerationg process modules failed");
		goto getout;
	}

	for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
	{
		TCHAR szModName[MAX_PATH];

		// Get the full path to the module's file.
		if (GetModuleFileNameEx(hProc, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
		{
			MODULEINFO info;
			if (!GetModuleInformation(hProc, hMods[i], &info, sizeof(MODULEINFO))) {
				WinError(L"Get module information failed");
				goto getout;
			}

			// find our module and get base address
			UnicodeString name(szModName);
			if (name.Pos(MODULE_NAME) > 0) {
				found = true;
				baseAddr = info.lpBaseOfDll;
				//debug
				txt.sprintf(L"%s  base_addr: 0x%llX", szModName, baseAddr);
				Form1->Memo1->Lines->Add(txt);

				break;
			}
		}
	}
	if (!found) {
		Error(UnicodeString(L"Module ")+ MODULE_NAME + L" not found");
		goto getout;
	}
	//debug
	txt.sprintf(L"target_addr: 0x%llX", (BYTE*)baseAddr + MEM_OFFSET + data1_offset[Game]);
	Form1->Memo1->Lines->Add(txt);

	// read current data
	if (!ReadProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data0_offset[Game], data0, DATA0_SIZE, NULL)) {
		WinError(L"Read process memory failed");
		goto getout;
	}
	if (!ReadProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data1_offset[Game], data1, DATA1_SIZE, NULL)) {
		WinError(L"Read process memory failed");
		goto getout;
	}
	if (!ReadProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data2_offset[Game], data2, DATA2_SIZE, NULL)) {
		WinError(L"Read process memory failed");
		goto getout;
	}

	// OFF test
	if (memcmp(data1, pf_off1[Game], DATA1_SIZE) == 0)
	{
		if (off) { // Turn off patches 0 and 2 after patch 1 was turned off before.
			if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data0_offset[Game], pf_off0[Game], DATA0_SIZE, NULL)) {
				WinError(L"Write process memory failed");
				goto getout;
			}
			if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data2_offset[Game], pf_off2[Game], DATA2_SIZE, NULL)) {
				WinError(L"Write process memory failed");
				goto getout;
			}
			// notify
			Form1->Caption = L"3-Song Mode";
			if (Form1->chkOSDEnabled->Checked)
				pOSD->SendMessage(L"3-Song Mode", Form1->udOSDDuration->Position);
		} else {
			// write PFree ON
			if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data0_offset[Game], pf_on0[Game], DATA0_SIZE, NULL)) {
				WinError(L"Write process memory failed");
				goto getout;
			}
			if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data1_offset[Game], pf_on1[Game], DATA1_SIZE, NULL)) {
				WinError(L"Write process memory failed");
				goto getout;
			}
			if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data2_offset[Game], pf_on2[Game], DATA2_SIZE, NULL)) {
				WinError(L"Write process memory failed");
				goto getout;
			}
			// notify
			Form1->Caption = L"Pfree Mode";
			if (Form1->chkOSDEnabled->Checked)
				pOSD->SendMessage(L"Pfree Mode", Form1->udOSDDuration->Position);
		}
	} else
	// ON test
	if (memcmp(data1, pf_on1[Game], DATA1_SIZE) == 0)
	{
		// write PFree OFF  // Don't turn off patches 0 and 2, or the game will go back to the 1st song instead of ending.
		if (!WriteProcessMemory(hProc, (BYTE*)baseAddr + MEM_OFFSET + data1_offset[Game], pf_off1[Game], DATA1_SIZE, NULL)) {
			WinError(L"Write process memory failed");
			goto getout;
		}
		GrabConsole(procID);
		// notify
		Form1->Caption = L"1-Song Mode";
		if (Form1->chkOSDEnabled->Checked)
			pOSD->SendMessage(L"1-Song Mode", Form1->udOSDDuration->Position);
	} else
	{
		Error(L"Invalid game data. Press [Information...] for supported game version.");
		goto getout;
	}
	Form1->MemoResetStyle();
	//debug
//	txt.sprintf(L"data0: %02X %02X %02X %02X %02X %02X", data0[0],data0[1],data0[2],data0[3],data0[4],data0[5]);
//	txt1.sprintf(L"data1: %02X %02X", data1[0],data1[1]);
//	txt2.sprintf(L"data2: %02X %02X %02X %02X %02X %02X", data2[0],data2[1],data2[2],data2[3],data2[4],data2[5]);
//	Form1->Memo1->Lines->Add(txt);
//	Form1->Memo1->Lines->Add(txt1);
//	Form1->Memo1->Lines->Add(txt2);

getout:
	// cleanup
	if (hProc) CloseHandle(hProc);

	if (SuspendProcess && procID)
		DebugActiveProcessStop(procID);

	Working = false;
}

//---------------------------------------------------------------------------
void TerminateGame()
{
	Terminating = true;

	HANDLE hProc = NULL;
	HWND hWnd = NULL;
	DWORD procID = NULL;

	hWnd = FindTermWnd();
	if (!hWnd) {
		goto getout2;
	}
	// Sever link with game
	ReleaseConsole();

	// get process ID
	if (!GetWindowThreadProcessId(hWnd, &procID)) {
		WinError(L"Get window process ID failed");
		goto getout2;
	}

	// soft close window first
	SendNotifyMessage(hWnd, WM_CLOSE, 0, 0);
	Sleep(300);

	// hard terminate process
	hProc = OpenProcess(PROCESS_TERMINATE, FALSE, procID);
	if (!hProc) { // already closed or some error, whatever
		goto getout2;
	}

	if (!TerminateProcess(hProc, 0)) {
		WinError(L"Could not terminate process");
	}

getout2:
	// cleanup
	if (hProc) CloseHandle(hProc);

	Terminating = false;
}

//---------------------------------------------------------------------------
// callback function for keyboard hook
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		// struct to get virtual key codes
		KBDLLHOOKSTRUCT kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);

		if (nCode == HC_ACTION && wParam == WM_KEYDOWN)
		{
			if (Form1->edtKey->Tag) {
				Key = kbdStruct.vkCode;
				Form1->Memo1->SetFocus();
				Form1->Save();
			} else
			if (Form1->edtTermKey->Tag) {
				TermKey = kbdStruct.vkCode;
				Form1->Memo1->SetFocus();
				Form1->Save();
			} else
			if (kbdStruct.vkCode == Key && !Working)
			{
				Form1->Memo1->SetFocus();
				TogglePFree();
			} else
			if (kbdStruct.vkCode == TermKey && !Terminating)
			{
				Form1->Memo1->SetFocus();
				TerminateGame();
			}
		}
	}

	// if the message does not get processed
	return CallNextHookEx (0, nCode, wParam, lParam);
}

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
	pGamesList = new TStringList();
	pGamesList->Add(L"MUSECA");
//	pGamesList->Add(L"SOUND VOLTEX III GRAVITY WARS");

	pTermList = new TList();
	pTermList->Add(new TermWnd(L"SOUND VOLTEX IV HEAVENLY HAVEN 1", L"SOUND VOLTEX IV HEAVENLY HAVEN 1"));
	pTermList->Add(new TermWnd(L"SOUND VOLTEX III GRAVITY WARS", L"SOUND VOLTEX III GRAVITY WARS"));
	pTermList->Add(new TermWnd(L"SOUND VOLTEX II -infinite infection-", L"SOUND VOLTEX II -infinite infection-"));
	pTermList->Add(new TermWnd(L"beatmania IIDX 24 SINOBUZ", L"beatmania IIDX 24 SINOBUZ"));
	pTermList->Add(new TermWnd(L"pop'n music eclale", L"pop'n music eclale"));
	pTermList->Add(new TermWnd(L"MUSECA", L"MUSECA"));
	pTermList->Add(new TermWnd(L"BeatStream", L"BeatStream"));
	pTermList->Add(new TermWnd(L"", L"_")); // GITADORA Tri-Boost Re:EVOLVE / Matixx

	// load .ini
	Load();
	// set global keyboard hook to capture key press
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, GetModuleHandle(NULL), 0);

	pOSD = new tOSD();
}

//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
	if (hHook) UnhookWindowsHookEx(hHook);
	Save();

	delete pGamesList;
	delete pTermList;

	delete pOSD;
}

//---------------------------------------------------------------------------
void TForm1::Load()
{
	TIniFile *ini = new TIniFile(ChangeFileExt(Application->ExeName, ".ini"));
	Key = (DWORD)ini->ReadInteger(L"GENERAL", L"Hotkey", 106); // by default Numpad * key
	edtKey->Text = IntToStr((int)Key);
	TermKey = (DWORD)ini->ReadInteger(L"GENERAL", L"TerminateHotkey", 109); // by default Numpad - key
	edtTermKey->Text = IntToStr((int)TermKey);
	SuspendProcess = ini->ReadInteger(L"GENERAL", L"SuspendProcess", 0);
	rdgVoice->ItemIndex = ini->ReadInteger(L"GENERAL", L"NotifyVoice", 0);
	udOSDDuration->Position = ini->ReadInteger(L"GENERAL", L"OSDDuration", 120);
	chkOSDEnabled->Checked = ini->ReadInteger(L"GENERAL", L"OSDEnabled", 1) == 1;
	delete ini;
}

//---------------------------------------------------------------------------
void TForm1::Save()
{
	TIniFile *ini = new TIniFile(ChangeFileExt(Application->ExeName, ".ini"));
	ini->WriteInteger(L"GENERAL", L"Hotkey", (int)Key);
	ini->WriteInteger(L"GENERAL", L"TerminateHotkey", (int)TermKey);
	ini->WriteInteger(L"GENERAL", L"SuspendProcess", SuspendProcess);
	ini->WriteInteger(L"GENERAL", L"NotifyVoice", rdgVoice->ItemIndex);
	ini->WriteInteger(L"GENERAL", L"OSDDuration", udOSDDuration->Position);
	ini->WriteInteger(L"GENERAL", L"OSDEnabled", chkOSDEnabled->Checked?1:0);
	delete ini;
}

//---------------------------------------------------------------------------
void TForm1::MemoResetStyle()
{
	Memo1->Color = clWindow;
	Memo1->Font->Color = clWindowText;
	Memo1->Font->Style = TFontStyles();
}

//---------------------------------------------------------------------------
void __fastcall TForm1::edtKeyEnter(TObject *Sender)
{
	edtKey->Tag = 1;
	edtKey->Text = L"Press single key";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtKeyExit(TObject *Sender)
{
	edtKey->Tag = 0;
	edtKey->Text = IntToStr((int)Key);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtTermKeyEnter(TObject *Sender)
{
	edtTermKey->Tag = 1;
	edtTermKey->Text = L"Press single key";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtTermKeyExit(TObject *Sender)
{
	edtTermKey->Tag = 0;
	edtTermKey->Text = IntToStr((int)TermKey);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::btnInfoClick(TObject *Sender)
{
	ShowMessage(L"How to use:\n"
		"\t1. Run the app\n"
		"\t2. Assign keys by clicking the corresponding edit box\n"
		"\t3. Start the game while keeping app running in the background\n"
		"\t4. To change mode press assigned key\n"
		"\t    (usually while in song selection screen or pre-song lobby)\n"
		"\t5. PROFIT\n\n"
		"\t(!) Make sure game's console window is present and have decent size\n"
		"\tat least 80x25 characters.\n"
		"\n"
		"PFree mode is supported on:\n"
		"\tMUSECA 1+1/2 (2018-07-30)\n\n"
		"Terminate game is supported on:\n"
		"\tSOUND VOLTEX IV HEAVENLY HAVEN 1\n"
		"\tSOUND VOLTEX III GRAVITY WARS\n"
		"\tSOUND VOLTEX II -infinite infection-\n"
		"\tbeatmania IIDX 24 SINOBUZ\n"
		"\tpop'n music eclale\n"
		"\tMUSECA\n"
		"\tBeatStream\n"
		"\tGITADORA Tri-Boost Re:EVOLVE\n"
		"\tGITADORA Matixx"
		);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::btnOSDTestClick(TObject *Sender)
{
	if (!pOSD->SendMessage(L"Test Message", udOSDDuration->Position)) {
		Error(L"Unable to send message. Press [?] for instructions.");
	}
}

//---------------------------------------------------------------------------
void __fastcall TForm1::btnOSDHelpClick(TObject *Sender)
{
	ShowMessage(L"How to use OSD:\n"
		"\t1. Place dx9osd64.dll in the game folder\n"
		"\t2. Add it to command line with -k option\n"
		"\t    (e.g. spice64.exe -k dx9osd64.dll)\n"
		"\t3. Run the game\n"
		"\t4. Press [Test] button\n"
		"\t5. Test message should appear in game\n"
		"\nOSD message duration is in frames (e.g. 60 for 1 second)"
		);
}

//---------------------------------------------------------------------------
void __fastcall TForm1::TimerConsoleTimer(TObject *Sender)
{
	PollConsole(hConOut);
}

//---------------------------------------------------------------------------

