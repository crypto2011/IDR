// ---------------------------------------------------------------------------

#ifndef ProcessManagerH
#define ProcessManagerH

#include <System.hpp>
#include <System.Classes.hpp>
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

// ---------------------------------------------------------------------------
class TProcessManager {
private:
	vector<TProcess> FList;

	void EnumSections(HANDLE HProcess, BYTE* PProcessBase, IMAGE_SECTION_HEADER* Buffer, DWORD* Secnum);

public:
	bool GenerateProcessList();
	void DumpProcess(DWORD PID, TMemoryStream* MemStream, DWORD* BoC, DWORD* PoC, DWORD* ImB);

	__property vector<TProcess> ProcessList = {read = FList};
};
// ---------------------------------------------------------------------------
#endif
