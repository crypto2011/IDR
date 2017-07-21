//---------------------------------------------------------------------------

#ifndef ActiveProcessesH
#define ActiveProcessesH
//---------------------------------------------------------------------------
#include <tlhelp32.h>
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <psapi.h>
#include <tlhelp32.h>
//---------------------------------------------------------------------------
class TFActiveProcesses : public TForm
{
__published:	// IDE-managed Components
    TButton *btnDump;
    TButton *btnCancel;
    TListView *lvProcesses;
    void __fastcall btnCancelClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall btnDumpClick(TObject *Sender);
    void __fastcall lvProcessesClick(TObject *Sender);
private:	// User declarations
    HINSTANCE   InstKernel32;
    HINSTANCE   InstPSAPI;
    DWORD       ProcessesNum;
    DWORD       ModulesNum;
    DWORD       ProcessIds[1024];
    HMODULE     ModuleHandles[1024];

typedef HANDLE __stdcall (*TCreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL __stdcall (*TProcess32First)(HANDLE hSnapshot, PROCESSENTRY32* lppe);
typedef BOOL __stdcall (*TProcess32Next)(HANDLE hSnapshot, PROCESSENTRY32* lppe);
typedef BOOL __stdcall (*TModule32First)(HANDLE hSnapshot, MODULEENTRY32* lpme);
typedef BOOL __stdcall (*TModule32Next)(HANDLE hSnapshot, MODULEENTRY32* lpme);

    TCreateToolhelp32Snapshot   lpCreateToolhelp32Snapshot;
    TProcess32First             lpProcess32First;
    TProcess32Next              lpProcess32Next;
    TModule32First              lpModule32First;
    TModule32Next               lpModule32Next;
    
typedef BOOL __stdcall (*TEnumProcesses)(HANDLE lpidProcess, DWORD cb, PDWORD cbNeeded);
typedef BOOL __stdcall (*TEnumProcessModules)(HANDLE hProcess, HMODULE lphModule, DWORD cb, PDWORD lpcbNeeded);
typedef DWORD __stdcall (*TGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
typedef BOOL __stdcall (*TGetModuleInformation)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);

    TEnumProcesses          lpEnumProcesses;
    TEnumProcessModules     lpEnumProcessModules;
    TGetModuleFileNameEx    lpGetModuleFileNameEx;
    TGetModuleInformation   lpGetModuleInformation;

    bool IsWindows2000OrHigher();
    void ShowProcesses95();
    void ShowProcessesNT();

public:		// User declarations
    __fastcall TFActiveProcesses(TComponent* Owner);
    __fastcall ~TFActiveProcesses();

    void ShowProcesses();
    void __fastcall EnumSections(HANDLE HProcess, BYTE* PProcessBase, IMAGE_SECTION_HEADER* Buffer, DWORD* Secnum);
    void __fastcall DumpProcess(DWORD PID, TMemoryStream* MemStream, DWORD* BoC, DWORD* PoC, DWORD* ImB);
};
//---------------------------------------------------------------------------
extern PACKAGE TFActiveProcesses *FActiveProcesses;
//---------------------------------------------------------------------------
#endif
