//---------------------------------------------------------------------------
#include <windows.h>
#include "globals.h"
//---------------------------------------------------------------------------
//   Important note about DLL memory management when your DLL uses the
//   static version of the RunTime Library:
//
//   If your DLL exports any functions that pass String objects (or structs/
//   classes containing nested Strings) as parameter or function results,
//   you will need to add the library MEMMGR.LIB to both the DLL project and
//   any other projects that use the DLL.  You will also need to use MEMMGR.LIB
//   if any other projects which use the DLL will be performing new or delete
//   operations on any non-TObject-derived classes which are exported from the
//   DLL. Adding MEMMGR.LIB to your project will change the DLL and its calling
//   EXE's to use the BORLNDMM.DLL as their memory manager.  In these cases,
//   the file BORLNDMM.DLL should be deployed along with your DLL.
//
//   To avoid using BORLNDMM.DLL, pass string information using "char *" or
//   ShortString parameters.
//
//   If your DLL uses the dynamic version of the RTL, you do not need to
//   explicitly add MEMMGR.LIB as this will be done implicitly for you
//---------------------------------------------------------------------------
#pragma argsused
// ---------------------------------------------------------------------------
// GLOBAL VARIABLES
LPCTSTR szPluginName = SZPLUGINNAME;

// ---------------------------------------------------------------------------
// EXPORT FUNCTION PROTOTYPES
void __stdcall RegisterPlugIn(LPCTSTR *);
void __stdcall AboutPlugIn(void);
int __stdcall GetDstSize(BYTE* srcData, int srcSize);
boolean __stdcall Decrypt(BYTE* srcData, int srcSize, BYTE* dstData, int dstSize);

// ---------------------------------------------------------------------------
// EXPORT FUNCTION IMPLEMENTATIONS
// ---------------------------------------------------------------------------
void __stdcall __declspec(dllexport) RegisterPlugIn(LPCTSTR *ppPluginName)
{
	*ppPluginName = szPluginName;
}
// ---------------------------------------------------------------------------
void __stdcall __declspec(dllexport) AboutPlugIn(void)
{
	TCHAR szBuffer[MAX_PATH];
	ZeroMemory(szBuffer,MAX_PATH);
	wsprintf(szBuffer, TEXT("%s\r\n%s\r\nAuthor:%s"), SZDESCRIPTION, SZVERSION, SZAUTHORINFO);
	MessageBox(NULL, szBuffer, TEXT("About"), MB_OK|MB_ICONINFORMATION);
}
// ---------------------------------------------------------------------------
int __stdcall __declspec(dllexport) GetDstSize(BYTE* srcData, int srcSize)
{
    return srcSize;
}
// ---------------------------------------------------------------------------
DWORD ROL(DWORD val, BYTE shift)
{
    asm
    {
        mov     eax, [val]
        mov     cl, [shift]
        rol     eax, cl
    }
}
// ---------------------------------------------------------------------------
DWORD ROR(DWORD val, BYTE shift)
{
    asm
    {
        mov     eax, [val]
        mov     cl, [shift]
        ror     eax, cl
    }
}
// ---------------------------------------------------------------------------
boolean __stdcall __declspec(dllexport) Decrypt(BYTE* srcData, int srcSize, BYTE* dstData, int dstSize)
{
    int     n;
    DWORD   eax, ebx, edx;

    BYTE *smp = srcData + 4;
    BYTE *dmp = dstData + 4;
    DWORD lvar_4 = 0x136C9856;
    DWORD lvar_8 = 0x9ABFB3B6;
    DWORD lvar_C = 0x7F6A0DBB;
    DWORD lvar_10 = 0x020F2884;//0x84280F02;//0xD8696A85;
    for (n = 0; n < srcSize - 4; n++)
    {
        eax = ROL(lvar_4, 7);
        edx = ROL(lvar_8, 27);
        eax ^= edx;
        lvar_4 = eax;
        ebx = lvar_C;
        eax ^= ebx;
        ebx = ROL(ebx, eax);
        eax = ROR(eax, 15);
        edx ^= eax;
        edx ^= ebx;
        ebx = ROR(ebx, edx);
        lvar_C = ebx;
        lvar_8 = edx;
        *dmp = lvar_4 ^ *smp ^ lvar_10;
        lvar_10 = ROR(lvar_10, edx);
        smp++;
        dmp++;
    }
    memmove(dstData, "TPF0", 4);
    return TRUE;
}
// ---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved)
{
    return 1;
}
// ---------------------------------------------------------------------------
