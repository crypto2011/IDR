// ---------------------------------------------------------------------------

#pragma hdrstop

#include "PEHeader.h"
#include "Misc.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)

// ---------------------------------------------------------------------------
DWORD TPEHeader::EvaluateInitTable(BYTE* Data, DWORD Size, DWORD Base) {
	int i, num, pos, unitsPos = 0, n;
	DWORD initTable, result, iniAdr, finAdr, maxAdr = 0;

	for (i = 0; i < ((Size - 4) & (-4)); i += 4) {
		initTable = result = *((DWORD*)(Data + i));
		if (initTable == Base + i + 4) {
			num = *((DWORD*)(Data + i - 4));
			if (num <= 0 || num > 10000)
				continue;
			pos = unitsPos = i + 4;
			for (n = 0; n < num; n++, pos += 8) {
				iniAdr = *((DWORD*)(Data + pos));
				if (iniAdr) {
					if (iniAdr < Base || iniAdr >= Base + Size)
						// !IsValidImageAdr(iniAdr))
					{
						unitsPos = 0;
						break;
					}
					else if (iniAdr > maxAdr) {
						maxAdr = iniAdr;
					}
				}
				finAdr = *((DWORD*)(Data + pos + 4));
				if (finAdr) {
					if (finAdr < Base || finAdr >= Base + Size)
						// !IsValidImageAdr(finAdr))
					{
						unitsPos = 0;
						break;
					}
					else if (finAdr > maxAdr) {
						maxAdr = finAdr;
					}
				}
				result += 8;
			}
			if (unitsPos)
				break;
		}
	}
	if (maxAdr > result)
		result = (maxAdr + 3) & (-4);

	if (unitsPos)
		return initTable - 8;

	// May be D2010
	maxAdr = 0;
	for (i = 0; i < ((Size - 20) & (-4)); i += 4) {
		initTable = result = *((DWORD*)(Data + i));
		if (initTable == Base + i + 20) {
			num = *((DWORD*)(Data + i - 4));
			if (num <= 0 || num > 10000)
				continue;

			pos = unitsPos = i + 20;
			for (n = 0; n < num; n++, pos += 8) {
				iniAdr = *((DWORD*)(Data + pos));
				if (iniAdr) {
					if (iniAdr < Base || iniAdr >= Base + Size)
						// !IsValidImageAdr(iniAdr))
					{
						unitsPos = 0;
						break;
					}
					else if (iniAdr > maxAdr) {
						if (*((DWORD*)(Data + Adr2Pos(iniAdr))))
							maxAdr = iniAdr;
					}
				}
				finAdr = *((DWORD*)(Data + pos + 4));
				if (finAdr) {
					if (finAdr < Base || finAdr >= Base + Size)
						// !IsValidImageAdr(finAdr))
					{
						unitsPos = 0;
						break;
					}
					else if (finAdr > maxAdr) {
						if (*((DWORD*)(Data + Adr2Pos(finAdr))))
							maxAdr = finAdr;
					}
				}
				result += 8;
			}
			if (unitsPos)
				break;
		}
	}
	if (maxAdr > result)
		result = (maxAdr + 3) & (-4);

	if (unitsPos)
		return initTable - 24;

	return 0;
}
// ---------------------------------------------------------------------------
