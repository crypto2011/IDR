// ---------------------------------------------------------------------------

#ifndef ProcessManagerH
#define ProcessManagerH

#include <System.hpp>
#include <System.Classes.hpp>
#include <TlHelp32.h>
#include <Psapi.h>
#include <vector>

#include "PEheader.h"

using namespace std;

// ---------------------------------------------------------------------------

struct TProcess {
	DWORD Pid;
	DWORD ImageSize;
	DWORD EntryPoint;
	DWORD BaseAddress;
	String Name;
};

class TProcessManager {
private:
	vector<TProcess> FList;

	HINSTANCE InstKernel32;
	HINSTANCE InstPSAPI;
	DWORD ProcessesNum;
	DWORD ModulesNum;
	DWORD ProcessIds[1024];
	HMODULE ModuleHandles[1024];

	typedef HANDLE __stdcall(*TCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
	typedef BOOL __stdcall(*TProcess32First)(HANDLE hSnapshot, PROCESSENTRY32 * lppe);
	typedef BOOL __stdcall(*TProcess32Next)(HANDLE hSnapshot, PROCESSENTRY32 * lppe);
	typedef BOOL __stdcall(*TModule32First)(HANDLE hSnapshot, MODULEENTRY32 * lpme);
	typedef BOOL __stdcall(*TModule32Next)(HANDLE hSnapshot, MODULEENTRY32 * lpme);

	TCreateToolhelp32Snapshot lpCreateToolhelp32Snapshot;
	TProcess32First lpProcess32First;
	TProcess32Next lpProcess32Next;
	TModule32First lpModule32First;
	TModule32Next lpModule32Next;

	typedef BOOL __stdcall(*TEnumProcesses)(HANDLE lpidProcess, DWORD cb, PDWORD cbNeeded);
	typedef BOOL __stdcall(*TEnumProcessModules)(HANDLE hProcess, HMODULE * lphModule, DWORD cb, LPDWORD lpcbNeeded);
	typedef DWORD __stdcall(*TGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
	typedef BOOL __stdcall(*TGetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);

	TEnumProcesses lpEnumProcesses;
	TEnumProcessModules lpEnumProcessModules;
	TGetModuleFileNameEx lpGetModuleFileNameEx;
	TGetModuleInformation lpGetModuleInformation;

	bool IsWindows2000OrHigher();
	void ShowProcesses95();
	bool ShowProcessesNT();
	void EnumSections(HANDLE HProcess, BYTE* PProcessBase, IMAGE_SECTION_HEADER* Buffer, DWORD* Secnum);

public:
	TProcessManager();
	~TProcessManager();

	bool GenerateProcessList();
	void DumpProcess(DWORD PID, TMemoryStream* MemStream, DWORD* BoC, DWORD* PoC, DWORD* ImB);

	__property vector<TProcess> ProcessList = {read = FList};
};
// ---------------------------------------------------------------------------
#endif
