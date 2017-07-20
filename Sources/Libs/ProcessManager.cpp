// ---------------------------------------------------------------------------

#pragma hdrstop

#include "ProcessManager.h"

#include <TlHelp32.h>
#include <Psapi.h>
// ---------------------------------------------------------------------------
#pragma package(smart_init)

// ---------------------------------------------------------------------------
bool TProcessManager::GenerateProcessList() {

	// Clear current process list
	this->FList.clear();

	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
		return false;
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.
	for (i = 0; i < cProcesses; i++) {
		if (aProcesses[i] != 0) {

			// Get a handle to the process.
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);

			// Get the process name.
			if (NULL != hProcess) {
				HMODULE hModule;
				DWORD cbNeeded;

				// Get modules list (for this case, only the first module)
				if (EnumProcessModules(hProcess, &hModule, sizeof(hModule), &cbNeeded)) {
					HANDLE hSnapshot;
					MODULEENTRY32 me = {};
					me.dwSize = sizeof(MODULEENTRY32);
					// Get name and base
					hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, aProcesses[i]);
					if (hSnapshot != INVALID_HANDLE_VALUE) {
						// Get module info
						Module32First(hSnapshot, &me);
						// Release the handle to the process.
						CloseHandle(hSnapshot);

						MODULEINFO modinfo = {};
						if (GetModuleInformation(hProcess, hModule, &modinfo, sizeof(modinfo))) {
							// Save process info
							TProcess Process = {};
							Process.Pid = aProcesses[i];
							Process.ImageSize = modinfo.SizeOfImage;
							Process.EntryPoint = (DWORD) modinfo.EntryPoint;
							Process.BaseAddress = (DWORD) me.modBaseAddr;
							Process.Name = ExtractFileName(String(me.szExePath));
							this->FList.push_back(Process);
						}
					}
				}
			}

			// Release the handle to the process.
			CloseHandle(hProcess);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
void TProcessManager::EnumSections(HANDLE HProcess, BYTE * PProcessBase, IMAGE_SECTION_HEADER * Buffer, DWORD * Secnum) {
	int i;
	BYTE *_pBuf, *_pSection;
	DWORD _peHdrOffset, _sz;
	IMAGE_NT_HEADERS _ntHdr;
	IMAGE_SECTION_HEADER _section;

	// Read offset of PE header
	if (!ReadProcessMemory(HProcess, PProcessBase + 0x3C, &_peHdrOffset, sizeof(_peHdrOffset), &_sz))
		return;
	// Read IMAGE_NT_HEADERS.OptionalHeader.BaseOfCode
	if (!ReadProcessMemory(HProcess, PProcessBase + _peHdrOffset, &_ntHdr, sizeof(_ntHdr), &_sz))
		return;

	_pSection = PProcessBase + _peHdrOffset + 4+sizeof(_ntHdr.FileHeader) + _ntHdr.FileHeader.SizeOfOptionalHeader;
	memset((BYTE*)&_section, 0, sizeof(_section));

	*Secnum = _ntHdr.FileHeader.NumberOfSections;
	_pBuf = (BYTE*)Buffer;
	for (i = 0; i < _ntHdr.FileHeader.NumberOfSections; i++) {
		if (!ReadProcessMemory(HProcess, _pSection + i*sizeof(_section), &_section, sizeof(_section), &_sz))
			return;
		memmove(_pBuf, &_section, sizeof(_section));
		_pBuf += sizeof(_section);
	}
}

// ---------------------------------------------------------------------------
void TProcessManager::DumpProcess(DWORD PID, TMemoryStream * MemStream, DWORD * BoC, DWORD * PoC, DWORD * ImB) {
	WORD _secNum;
	DWORD _peHdrOffset, _sz, _sizeOfCode;
	DWORD _resPhys, _dd;
	BYTE* _buf;
	BYTE _b[8];
	HANDLE _hProcess, _hSnapshot;
	MODULEINFO _moduleInfo = {0};
	IMAGE_NT_HEADERS _ntHdr;
	PROCESSENTRY32 _ppe;
	MODULEENTRY32 _pme;
	IMAGE_SECTION_HEADER _sections[64];
	HMODULE hMod;
	DWORD ModulesNum;

	_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, PID); // 0x1F0FFF
	if (_hProcess) {

		EnumProcessModules(_hProcess, &hMod, sizeof(hMod), &ModulesNum);
		GetModuleInformation(_hProcess, hMod, &_moduleInfo, sizeof(_moduleInfo));

		if (!_moduleInfo.lpBaseOfDll) {
			throw Exception("Invalid process, PID: " + IntToStr((int)PID));
		}
		ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + 0x3C, &_peHdrOffset, sizeof(_peHdrOffset), &_sz);
		ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll + _peHdrOffset, &_ntHdr, sizeof(_ntHdr), &_sz);
		EnumSections(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _sections, &_sz);
		MemStream->Clear();
		// Dump Header
		_buf = new BYTE[_ntHdr.OptionalHeader.SizeOfHeaders];
		ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _buf, _ntHdr.OptionalHeader.SizeOfHeaders, &_sz);
		MemStream->WriteBuffer(_buf, _ntHdr.OptionalHeader.SizeOfHeaders);
		delete[]_buf;
		if (_sizeOfCode < _sections[1].Misc.VirtualSize)
			_sizeOfCode = _sections[1].Misc.VirtualSize;
		else
			_sizeOfCode = _ntHdr.OptionalHeader.SizeOfCode;
		// !!!
		MemStream->Clear();
		_buf = new BYTE[_ntHdr.OptionalHeader.SizeOfImage];
		ReadProcessMemory(_hProcess, (BYTE*)_moduleInfo.lpBaseOfDll, _buf, _ntHdr.OptionalHeader.SizeOfImage, &_sz);
		MemStream->WriteBuffer(_buf, _ntHdr.OptionalHeader.SizeOfImage);
		delete[]_buf;
		// !!!
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
		// Correct PE Header
		// Set SectionNum = 2
		MemStream->Seek(6 + _peHdrOffset, soBeginning);
		_secNum = 2;
		MemStream->WriteBuffer(&_secNum, sizeof(_secNum));
		// Set EP
		// Set sections
		MemStream->Seek(0xF8 + _peHdrOffset, soBeginning);
		// "CODE"
		memset(_b, 0, 8);
		_b[0] = 'C';
		_b[1] = 'O';
		_b[2] = 'D';
		_b[3] = 'E';
		MemStream->WriteBuffer(_b, 8);
		_dd = _sizeOfCode;
		MemStream->WriteBuffer(&_dd, 4); // VIRTUAL_SIZE
		_dd = _ntHdr.OptionalHeader.BaseOfCode;
		MemStream->WriteBuffer(&_dd, 4); // RVA
		_dd = _sizeOfCode;
		MemStream->WriteBuffer(&_dd, 4); // PHYSICAL_SIZE
		_dd = _ntHdr.OptionalHeader.SizeOfHeaders;
		MemStream->WriteBuffer(&_dd, 4); // PHYSICAL_OFFSET
		_dd = 0;
		MemStream->WriteBuffer(&_dd, 4); // RELOC_PTR
		MemStream->WriteBuffer(&_dd, 4); // LINENUM_PTR
		MemStream->WriteBuffer(&_dd, 4); // RELOC_NUM,LINENUM_NUM
		_dd = 0x60000020;
		MemStream->WriteBuffer(&_dd, 4); // FLAGS
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
		// ".rsrc"
		memset(_b, 0, 8);
		_b[0] = '.';
		_b[1] = 'r';
		_b[2] = 's';
		_b[3] = 'r';
		_b[4] = 'c';
		MemStream->WriteBuffer(_b, 8);
		_dd = _ntHdr.OptionalHeader.DataDirectory[2].Size;
		MemStream->WriteBuffer(&_dd, 4); // VIRTUAL_SIZE
		_dd = _ntHdr.OptionalHeader.DataDirectory[2].VirtualAddress;
		MemStream->WriteBuffer(&_dd, 4); // RVA
		_dd = _ntHdr.OptionalHeader.DataDirectory[2].Size;
		MemStream->WriteBuffer(&_dd, 4); // PHYSICAL_SIZE
		_dd = _resPhys;
		MemStream->WriteBuffer(&_dd, 4); // PHYSICAL_OFFSET
		_dd = 0;
		MemStream->WriteBuffer(&_dd, 4); // RELOC_PTR
		MemStream->WriteBuffer(&_dd, 4); // LINENUM_PTR
		MemStream->WriteBuffer(&_dd, 4); // RELOC_NUM,LINENUM_NUM
		_dd = 0x50000040;
		MemStream->WriteBuffer(&_dd, 4); // FLAGS
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

		TPEHeader::EvaluateInitTable((BYTE*)MemStream->Memory, MemStream->Size, _ntHdr.OptionalHeader.ImageBase + _ntHdr.OptionalHeader.SizeOfHeaders);

		CloseHandle(_hProcess);
	}
}
// ---------------------------------------------------------------------------// ---------------------------------------------------------------------------
