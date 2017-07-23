//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include "Main.h"
#include "ActiveProcesses.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TFActiveProcesses *FActiveProcesses;
//---------------------------------------------------------------------------
__fastcall TFActiveProcesses::TFActiveProcesses(TComponent* Owner)
    : TForm(Owner)
{
    InstPSAPI = 0;
    lpEnumProcesses = 0;
    lpEnumProcessModules = 0;

    InstKernel32 = 0;
    lpCreateToolhelp32Snapshot = 0;
    lpProcess32First = 0;
    lpProcess32Next = 0;
    lpModule32First = 0;
    lpModule32Next = 0;

    //Load PSAPI
    if (IsWindows2000OrHigher())
    {
        //Supported starting from Windows XP/Server 2003
        InstPSAPI = LoadLibrary("PSAPI.DLL");
        if (InstPSAPI)
        {
            lpEnumProcesses         = (TEnumProcesses)GetProcAddress(InstPSAPI, "EnumProcesses");
            lpEnumProcessModules    = (TEnumProcessModules)GetProcAddress(InstPSAPI, "EnumProcessModules");
            lpGetModuleFileNameEx   = (TGetModuleFileNameEx)GetProcAddress(InstPSAPI, "GetModuleFileNameExA");
            lpGetModuleInformation  = (TGetModuleInformation)GetProcAddress(InstPSAPI, "GetModuleInformation");
        }
    }
    else
    {
        InstKernel32 = LoadLibrary("kernel32.dll");
        if (InstKernel32)
        {
            lpCreateToolhelp32Snapshot  = (TCreateToolhelp32Snapshot)GetProcAddress(InstKernel32, "CreateToolhelp32Snapshot");
            lpProcess32First            = (TProcess32First)GetProcAddress(InstKernel32, "Process32First");
            lpProcess32Next             = (TProcess32Next)GetProcAddress(InstKernel32, "Process32Next");
            lpModule32First             = (TModule32First)GetProcAddress(InstKernel32, "Module32First");
            lpModule32Next              = (TModule32Next)GetProcAddress(InstKernel32, "Module32Next");
        }
    }
}
//---------------------------------------------------------------------------
__fastcall TFActiveProcesses::~TFActiveProcesses()
{
    if (InstPSAPI) FreeLibrary(InstPSAPI);
    if (InstKernel32) FreeLibrary(InstKernel32);
}
//---------------------------------------------------------------------------
void TFActiveProcesses::ShowProcesses95()
{
    HANDLE          _hSnapshot, _hSnapshotM;
    TListItem       *_li;
    PROCESSENTRY32  _ppe = {0};
    MODULEENTRY32   _pme = {0};

    lvProcesses->Items->BeginUpdate();
    lvProcesses->Clear();

    _hSnapshot = lpCreateToolhelp32Snapshot(TH32CS_SNAPALL, GetCurrentProcessId());
    if (_hSnapshot != INVALID_HANDLE_VALUE)
    {
        _ppe.dwSize = sizeof(PROCESSENTRY32);
        _pme.dwSize = sizeof(MODULEENTRY32);
        lpProcess32First(_hSnapshot, &_ppe);
        do
        {
            _hSnapshotM = lpCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _ppe.th32ProcessID);
            if (_hSnapshotM == INVALID_HANDLE_VALUE)
                continue;

            if (lpModule32First(_hSnapshotM, &_pme))
            {
                _li = lvProcesses->Items->Add();
                _li->Caption = IntToHex((int)_ppe.th32ProcessID, 4);
                _li->SubItems->Add(ExtractFileName(String(_ppe.szExeFile)));
                _li->SubItems->Add(IntToHex((int)_pme.modBaseSize, 8));
                _li->SubItems->Add("-");
                _li->SubItems->Add(IntToHex((int)_pme.modBaseAddr, 8));
            }
            CloseHandle(_hSnapshotM);
        }
        while (lpProcess32Next(_hSnapshot, &_ppe));
        
        CloseHandle(_hSnapshot);
    }
    lvProcesses->Items->EndUpdate();
}
//---------------------------------------------------------------------------
bool TFActiveProcesses::IsWindows2000OrHigher()
{
   OSVERSIONINFO osvi = {0};
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osvi);
   //https://msdn.microsoft.com/en-us/library/windows/desktop/ms724834%28v=vs.85%29.aspx
   return (osvi.dwMajorVersion >= 5); //win XP+
}
//---------------------------------------------------------------------------
void TFActiveProcesses::ShowProcesses()
{
    if (!IsWindows2000OrHigher())
        FActiveProcesses->ShowProcesses95();
    else
        FActiveProcesses->ShowProcessesNT();
}
//---------------------------------------------------------------------------
void TFActiveProcesses::ShowProcessesNT()
{
    int         n, _len, _pos;
    String      _moduleName;
    TListItem   *_li;
    HANDLE      _hProcess;
    MODULEINFO  _moduleInfo;
    char        _buf[512];

    lpEnumProcesses(ProcessIds, sizeof(ProcessIds), &ProcessesNum);
    ProcessesNum /= sizeof(ProcessIds[0]);
    
    lvProcesses->Items->BeginUpdate();
    lvProcesses->Clear();
    for (n = 0; n < ProcessesNum; n++)
    {
        if (ProcessIds[n])
        {
            _hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, ProcessIds[n]);
            if (_hProcess)
            {
                if (lpEnumProcessModules(_hProcess, ModuleHandles, sizeof(ModuleHandles), &ModulesNum))
                {
                    ModulesNum /= sizeof(ModuleHandles[0]);
                    _len = lpGetModuleFileNameEx(_hProcess, ModuleHandles[0], _buf, sizeof(_buf));
                    _moduleName = String(_buf, _len);
                    lpGetModuleInformation(_hProcess, ModuleHandles[0], &_moduleInfo, sizeof(_moduleInfo));
                    if (_moduleName != "")
                    {
                        _pos = _moduleName.LastDelimiter("\\");
                        if (_pos) _moduleName = _moduleName.SubString(_pos + 1, _moduleName.Length());

                        _li = lvProcesses->Items->Add();
                        _li->Caption = IntToHex((int)ProcessIds[n], 4);
                        _li->SubItems->Add(_moduleName);
                        _li->SubItems->Add(IntToHex((int)_moduleInfo.SizeOfImage, 8));
                        _li->SubItems->Add(IntToHex((int)_moduleInfo.EntryPoint, 8));
                    }
                }
                CloseHandle(_hProcess);
            }
        }
    }
    lvProcesses->Items->EndUpdate();
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::btnCancelClick(TObject *Sender)
{
    Close();    
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::FormShow(TObject *Sender)
{
    btnDump->Enabled = (lvProcesses->Selected);
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::lvProcessesClick(TObject *Sender)
{
    btnDump->Enabled = (lvProcesses->Selected);
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::EnumSections(HANDLE HProcess, BYTE* PProcessBase, IMAGE_SECTION_HEADER* Buffer, DWORD* Secnum)
{
    int                     i;
    BYTE                    *_pBuf, *_pSection;
    DWORD                   _peHdrOffset, _sz;
    IMAGE_NT_HEADERS        _ntHdr;
    IMAGE_SECTION_HEADER    _section;

    //Read offset of PE header
    if (!ReadProcessMemory(HProcess, PProcessBase + 0x3C, &_peHdrOffset, sizeof(_peHdrOffset), &_sz)) return;
    //Read IMAGE_NT_HEADERS.OptionalHeader.BaseOfCode
    if (!ReadProcessMemory(HProcess, PProcessBase + _peHdrOffset, &_ntHdr, sizeof(_ntHdr), &_sz)) return;

    _pSection = PProcessBase + _peHdrOffset + 4 + sizeof(_ntHdr.FileHeader) + _ntHdr.FileHeader.SizeOfOptionalHeader;
    memset((BYTE*)&_section, 0, sizeof(_section));

    *Secnum = _ntHdr.FileHeader.NumberOfSections; _pBuf = (BYTE*)Buffer;
    for (i = 0; i < _ntHdr.FileHeader.NumberOfSections; i++)
    {
        if (!ReadProcessMemory(HProcess, _pSection + i * sizeof(_section), &_section, sizeof(_section), &_sz)) return;
        memmove(_pBuf, &_section, sizeof(_section));
        _pBuf += sizeof(_section);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::DumpProcess(DWORD PID, TMemoryStream* MemStream, DWORD* BoC, DWORD* PoC, DWORD* ImB)
{
    WORD                    _secNum;
    DWORD                   _peHdrOffset, _sz, _sizeOfCode;
    DWORD                   _resPhys, _dd;
    BYTE*                   _buf;
    BYTE                    _b[8];
    HANDLE                  _hProcess, _hSnapshot;
    MODULEINFO              _moduleInfo = {0};
    IMAGE_NT_HEADERS        _ntHdr;
    PROCESSENTRY32          _ppe;
    MODULEENTRY32           _pme;
    IMAGE_SECTION_HEADER    _sections[64];

    _hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, PID); //0x1F0FFF
    if (_hProcess)
    {
        //If Win9x
        if (!IsWindows2000OrHigher())
        {
            _hSnapshot = lpCreateToolhelp32Snapshot(TH32CS_SNAPALL, GetCurrentProcessId());
            if (_hSnapshot != INVALID_HANDLE_VALUE)
            {
                _ppe.dwSize = sizeof(PROCESSENTRY32);
                _pme.dwSize = sizeof(MODULEENTRY32);
                memset(&_ppe.szExeFile, 0, 259);
                lpProcess32First(_hSnapshot, &_ppe);
                lpModule32First(_hSnapshot, &_pme);
                while (lpProcess32Next(_hSnapshot, &_ppe))
                {
                    lpModule32Next(_hSnapshot, &_pme);
                    if (_ppe.th32ProcessID == PID)
                    {
                        _moduleInfo.lpBaseOfDll = _pme.modBaseAddr;
                        _moduleInfo.lpBaseOfDll = (BYTE*)0x400000;
                    }
                }
                CloseHandle(_hSnapshot);
            }
        }
        else
        {
            lpEnumProcessModules(_hProcess, ModuleHandles, sizeof(ModuleHandles), &ModulesNum);
            lpGetModuleInformation(_hProcess, ModuleHandles[0], &_moduleInfo, sizeof(_moduleInfo));
        }
        if (!_moduleInfo.lpBaseOfDll)
        {
            throw Exception("Invalid process, PID: " + PID);
        }
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + 0x3C, &_peHdrOffset, sizeof(_peHdrOffset), &_sz);
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + _peHdrOffset, &_ntHdr, sizeof(_ntHdr), &_sz);
        EnumSections(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _sections, &_sz);
        MemStream->Clear();
        //Dump Header
        _buf = new BYTE[_ntHdr.OptionalHeader.SizeOfHeaders];
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _buf, _ntHdr.OptionalHeader.SizeOfHeaders, &_sz);
        MemStream->WriteBuffer(_buf, _ntHdr.OptionalHeader.SizeOfHeaders);
        delete[] _buf;
        if (_sizeOfCode < _sections[1].Misc.VirtualSize)
            _sizeOfCode = _sections[1].Misc.VirtualSize;
        else
            _sizeOfCode = _ntHdr.OptionalHeader.SizeOfCode;
        //!!!
        MemStream->Clear();
        _buf = new BYTE[_ntHdr.OptionalHeader.SizeOfImage];
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _buf, _ntHdr.OptionalHeader.SizeOfImage, &_sz);
        MemStream->WriteBuffer(_buf, _ntHdr.OptionalHeader.SizeOfImage);
        delete[] _buf;
        //!!!
        /*
        //Dump Code
        _buf = new BYTE[_sizeOfCode];
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + _ntHdr.OptionalHeader.BaseOfCode, _buf, _sizeOfCode, &_sz);
        MemStream->WriteBuffer(_buf, _sizeOfCode);
        delete[] _buf;
        //Find EP
        //Dump Resources
        MemStream->Seek(0, soFromEnd);
        _buf = new BYTE[_ntHdr.OptionalHeader.DataDirectory[2].Size];
        ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + _ntHdr.OptionalHeader.DataDirectory[2].VirtualAddress, _buf, _ntHdr.OptionalHeader.DataDirectory[2].Size, &_sz);
        _resPhys = MemStream->Size;
        MemStream->WriteBuffer(_buf, _ntHdr.OptionalHeader.DataDirectory[2].Size);
        delete[] _buf;
        */
        //Correct PE Header
        //Set SectionNum = 2
        MemStream->Seek(6 + _peHdrOffset, soFromBeginning);
        _secNum = 2;
        MemStream->WriteBuffer(&_secNum, sizeof(_secNum));
        //Set EP
        //Set sections
        MemStream->Seek(0xF8 + _peHdrOffset, soFromBeginning);
        //"CODE"
        memset(_b, 0, 8); _b[0] = 'C'; _b[1] = 'O'; _b[2] = 'D'; _b[3] = 'E';
        MemStream->WriteBuffer(_b, 8);
        _dd = _sizeOfCode;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        _dd = _ntHdr.OptionalHeader.BaseOfCode;
        MemStream->WriteBuffer(&_dd, 4);//RVA
        _dd = _sizeOfCode;
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        _dd = _ntHdr.OptionalHeader.SizeOfHeaders;
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0x60000020;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        /*
        //"DATA"
        memset(_b, 0, 8); _b[0] = 'D'; _b[1] = 'A'; _b[2] = 'T'; _b[3] = 'A';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0xC0000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        //"BSS"
        memset(_b, 0, 8); _b[0] = 'B'; _b[1] = 'S'; _b[2] = 'S';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0xC0000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        //".idata"
        memset(_b, 0, 8); _b[0] = '.'; _b[1] = 'i'; _b[2] = 'd'; _b[3] = 'a'; _b[4] = 't'; _b[5] = 'a';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0xC0000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        //".tls"
        memset(_b, 0, 8); _b[0] = '.'; _b[1] = 't'; _b[2] = 'l'; _b[3] = 's';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0xC0000000;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        //".rdata"
        memset(_b, 0, 8); _b[0] = '.'; _b[1] = 'r'; _b[2] = 'd'; _b[3] = 'a'; _b[4] = 't'; _b[5] = 'a';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0x50000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        //".reloc"
        memset(_b, 0, 8); _b[0] = '.'; _b[1] = 'r'; _b[2] = 'e'; _b[3] = 'l'; _b[4] = 'o'; _b[5] = 'c';
        MemStream->WriteBuffer(_b, 8);
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//RVA
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0x50000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        */
        //".rsrc"
        memset(_b, 0, 8); _b[0] = '.'; _b[1] = 'r'; _b[2] = 's'; _b[3] = 'r'; _b[4] = 'c';
        MemStream->WriteBuffer(_b, 8);
        _dd = _ntHdr.OptionalHeader.DataDirectory[2].Size;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        _dd = _ntHdr.OptionalHeader.DataDirectory[2].VirtualAddress;
        MemStream->WriteBuffer(&_dd, 4);//RVA
        _dd = _ntHdr.OptionalHeader.DataDirectory[2].Size;
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_SIZE
        _dd = _resPhys;
        MemStream->WriteBuffer(&_dd, 4);//PHYSICAL_OFFSET
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//RELOC_PTR
        MemStream->WriteBuffer(&_dd, 4);//LINENUM_PTR
        MemStream->WriteBuffer(&_dd, 4);//RELOC_NUM,LINENUM_NUM
        _dd = 0x50000040;
        MemStream->WriteBuffer(&_dd, 4);//FLAGS
        /*
        //Correct directories
        MemStream->Seek(0x78 + _peHdrOffset, soFromBeginning);
        //Export table
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VA
        MemStream->WriteBuffer(&_dd, 4);//Size
        //Import table
        _dd = 0;
        MemStream->WriteBuffer(&_dd, 4);//VA
        MemStream->WriteBuffer(&_dd, 4);//Size
        //Resource table
        _dd = _ntHdr.OptionalHeader.SizeOfHeaders;
        MemStream->WriteBuffer(&_dd, 4);//RVA
        _dd = _ntHdr.OptionalHeader.DataDirectory[2].Size;
        MemStream->WriteBuffer(&_dd, 4);//VIRTUAL_SIZE
        */
        FMain_11011981->EvaluateInitTable((BYTE*)MemStream->Memory, MemStream->Size, _ntHdr.OptionalHeader.ImageBase + _ntHdr.OptionalHeader.SizeOfHeaders);

        CloseHandle(_hProcess);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::btnDumpClick(TObject *Sender)
{
    DWORD           _pid, _boc, _poc, _imb;
    TListItem       *_li;
    TMemoryStream   *_stream;
    TMemoryStream   *_stream1;
    String          _item;

    if (lvProcesses->Selected)
    {
        try
        {
            _li = lvProcesses->Selected;
            _pid = StrToInt("0x" + _li->Caption);
            _stream = new TMemoryStream;
            DumpProcess(_pid, _stream, &_boc, &_poc, &_imb);
            if (!_stream->Size)
            {
                delete _stream;
                return;
            }
            _stream->SaveToFile("__idrtmp__.exe");
            delete _stream;
            FMain_11011981->DoOpenDelphiFile(DELHPI_VERSION_AUTO, "__idrtmp__.exe", false, false);
        }
        catch(const Exception &ex)
        {
            ShowMessage("Dumper failed: " + ex.Message);
        }
    }
    Close();
}
//---------------------------------------------------------------------------
