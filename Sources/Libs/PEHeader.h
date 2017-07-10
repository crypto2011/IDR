// ---------------------------------------------------------------------------

#ifndef PEHeaderH
#define PEHeaderH

#include <System.hpp>
#include <System.Classes.hpp>
#include "Singleton.h"
#include "Infos.h"
// ---------------------------------------------------------------------------

struct TPEHeader32 {
	DWORD EP;
	DWORD ImageBase;
	DWORD ImageSize;
	DWORD TotalSize; // Size of sections CODE + DATA
	DWORD CodeBase;
	DWORD CodeSize;
	DWORD CodeStart;
	PInfoRec *Infos; // = 0; // Array of pointers to store items data
	TStringList *BSSInfos; // = 0; // Data from BSS
	DWORD *Flags; // = 0; // flags for used data
	TList *SegmentList; // Information about Image Segments
	TList *ExpFuncList; // Exported functions list (temporary)
	TList *ImpFuncList; // Imported functions list (temporary)
	TStringList *ImpModuleList; // Imported modules   list (temporary)

	/*
	 * Buffers
	 */
	BYTE *Code;
	BYTE *Image;
};

// ---------------------------------------------------------------------------
class TPEHeader {
private:
	TPEHeader32 FPEHeader32;

	bool ImportsValid(DWORD ImpRVA, DWORD ImpSize);

public:
	static DWORD EvaluateInitTable(BYTE* Data, DWORD Size, DWORD Base);
	int LoadImage(FILE* f, bool loadExp, bool loadImp);

	__property TPEHeader32 PEHeader32 = {read = FPEHeader32};
};

// ---------------------------------------------------------------------------
typedef TSingleton<TPEHeader> STPEHeader;
// ---------------------------------------------------------------------------
#endif
