// ---------------------------------------------------------------------------

#ifndef PEHeaderH
#define PEHeaderH

#include <System.hpp>
// ---------------------------------------------------------------------------

class TPEHeader {
public:
	static DWORD EvaluateInitTable(BYTE* Data, DWORD Size, DWORD Base);
};
// ---------------------------------------------------------------------------
#endif
