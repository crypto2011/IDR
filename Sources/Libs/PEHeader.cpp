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
bool TPEHeader::ImportsValid(DWORD ImpRVA, DWORD ImpSize) {
	if (ImpRVA || ImpSize) {
		DWORD EntryRVA = ImpRVA;
		DWORD EndRVA = ImpRVA + ImpSize;
		IMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

		while (1) {
			memmove(&ImportDescriptor, (this->FPEHeader32.Image + Adr2Pos(EntryRVA +this->FPEHeader32.ImageBase)), sizeof(IMAGE_IMPORT_DESCRIPTOR));

			if (!ImportDescriptor.OriginalFirstThunk && !ImportDescriptor.TimeDateStamp && !ImportDescriptor.ForwarderChain && !ImportDescriptor.Name && !ImportDescriptor.FirstThunk)
				break;

			if (!IsValidImageAdr(ImportDescriptor.Name +this->FPEHeader32.ImageBase))
				return false;
			int NameLength = strlen((char*)(this->FPEHeader32.Image + Adr2Pos(ImportDescriptor.Name +this->FPEHeader32.ImageBase)));
			if (NameLength < 0 || NameLength > 256)
				return false;
			if (!IsValidModuleName(NameLength, Adr2Pos(ImportDescriptor.Name +this->FPEHeader32.ImageBase)))
				return false;

			EntryRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
			if (EntryRVA >= EndRVA)
				break;
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
int TPEHeader::LoadImage(FILE* f, bool loadExp, bool loadImp) {
	int i, n, m, bytes, pos, SectionsNum, ExpNum, NameLength;
	DWORD DataEnd, Items;
	String moduleName, modName, sEP;
	String impFuncName;
	IMAGE_DOS_HEADER DosHeader;
	IMAGE_NT_HEADERS NTHeaders;
	PIMAGE_SECTION_HEADER SectionHeaders;
	char segname[9];
	char msg[1024];

	fseek(f, 0L, SEEK_SET);
	// IDD_ERR_NOT_EXECUTABLE
	if (fread(&DosHeader, 1, sizeof(IMAGE_DOS_HEADER), f) != sizeof(IMAGE_DOS_HEADER) || DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
		ShowMessage("File is not executable");
		return 0;
	}

	fseek(f, DosHeader.e_lfanew, SEEK_SET);
	// IDD_ERR_NOT_PE_EXECUTABLE
	if (fread(&NTHeaders, 1, sizeof(IMAGE_NT_HEADERS), f) != sizeof(IMAGE_NT_HEADERS) || NTHeaders.Signature != IMAGE_NT_SIGNATURE) {
		ShowMessage("File is not PE-executable");
		return 0;
	}
	// IDD_ERR_INVALID_PE_EXECUTABLE
	if (NTHeaders.FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER) || NTHeaders.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		ShowMessage("File is invalid 32-bit PE-executable");
		return 0;
	}
	// IDD_ERR_INVALID_PE_EXECUTABLE
	SectionsNum = NTHeaders.FileHeader.NumberOfSections;
	if (!SectionsNum) {
		ShowMessage("File is invalid PE-executable");
		return 0;
	}
	// SizeOfOptionalHeader may be > than sizeof(IMAGE_OPTIONAL_HEADER)
	fseek(f, NTHeaders.FileHeader.SizeOfOptionalHeader -sizeof(IMAGE_OPTIONAL_HEADER), SEEK_CUR);
	SectionHeaders = new IMAGE_SECTION_HEADER[SectionsNum];

	if (fread(SectionHeaders, 1, sizeof(IMAGE_SECTION_HEADER) * SectionsNum, f) != sizeof(IMAGE_SECTION_HEADER) * SectionsNum) {
		ShowMessage("Invalid section headers");
		delete[]SectionHeaders;
		return 0;
	}

	this->FPEHeader32.ImageBase = NTHeaders.OptionalHeader.ImageBase;
	this->FPEHeader32.ImageSize = NTHeaders.OptionalHeader.SizeOfImage;
	this->FPEHeader32.EP = NTHeaders.OptionalHeader.AddressOfEntryPoint;

	this->FPEHeader32.TotalSize = 0;
	DWORD rsrcVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
	DWORD relocVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
	// Fill SegmentList
	for (i = 0; i < SectionsNum; i++) {
		PSegmentInfo segInfo = new SegmentInfo;
		segInfo->Start = SectionHeaders[i].VirtualAddress +this->FPEHeader32.ImageBase;
		segInfo->Flags = SectionHeaders[i].Characteristics;

		if (i + 1 < SectionsNum)
			segInfo->Size = SectionHeaders[i + 1].VirtualAddress - SectionHeaders[i].VirtualAddress;
		else
			segInfo->Size = SectionHeaders[i].Misc.VirtualSize;

		if (!SectionHeaders[i].SizeOfRawData) // uninitialized data
		{
			// segInfo->Size = SectionHeaders[i].Misc.VirtualSize;
			segInfo->Flags |= 0x80000;
		}
		else if (SectionHeaders[i].VirtualAddress == rsrcVA || SectionHeaders[i].VirtualAddress == relocVA) {
			// segInfo->Size = SectionHeaders[i].SizeOfRawData;
			segInfo->Flags |= 0x80000;
		}
		else {
			// segInfo->Size = SectionHeaders[i].SizeOfRawData;
			this->FPEHeader32.TotalSize += segInfo->Size;
		}
		memset(segname, 0, 9);
		memmove(segname, SectionHeaders[i].Name, 8);
		segInfo->Name = String(segname);
		this->FPEHeader32.SegmentList->Add((void*)segInfo);
	}
	// DataEnd = TotalSize;

	// Load Image into memory
	this->FPEHeader32.Image = new BYTE[this->FPEHeader32.TotalSize];
	memset((void*)this->FPEHeader32.Image, 0, this->FPEHeader32.TotalSize);
	int num;
	BYTE *p = this->FPEHeader32.Image;
	for (i = 0; i < SectionsNum; i++) {
		if (SectionHeaders[i].VirtualAddress == rsrcVA || SectionHeaders[i].VirtualAddress == relocVA)
			continue;
		BYTE *sp = p;
		fseek(f, SectionHeaders[i].PointerToRawData, SEEK_SET);
		DWORD Items = SectionHeaders[i].SizeOfRawData;
		if (Items) {
			for (n = 0; Items >= MAX_ITEMS; n++) {
				fread(p, 1, MAX_ITEMS, f);
				Items -= MAX_ITEMS;
				p += MAX_ITEMS;
			}
			if (Items) {
				fread(p, 1, Items, f);
				p += Items;
			}
			num = p -this->FPEHeader32.Image;
			if (i + 1 < SectionsNum)
				p = sp + (SectionHeaders[i + 1].VirtualAddress - SectionHeaders[i].VirtualAddress);
		}
	}

	this->FPEHeader32.CodeStart = 0;
	this->FPEHeader32.Code = this->FPEHeader32.Image +this->FPEHeader32.CodeStart;
	this->FPEHeader32.CodeBase = this->FPEHeader32.ImageBase + SectionHeaders[0].VirtualAddress;

	DWORD evalInitTable = TPEHeader::EvaluateInitTable(this->FPEHeader32.Image, this->FPEHeader32.TotalSize, this->FPEHeader32.CodeBase);
	if (!evalInitTable) {
		ShowMessage("Cannot find initialization table");
		delete[]SectionHeaders;
		delete[]this->FPEHeader32.Image;
		this->FPEHeader32.Image = 0;
		return 0;
	}

	DWORD evalEP = 0;
	// Find instruction mov eax,offset InitTable
	for (n = 0; n < this->FPEHeader32.TotalSize - 5; n++) {
		if (this->FPEHeader32.Image[n] == 0xB8 && *((DWORD*)(this->FPEHeader32.Image + n + 1)) == evalInitTable) {
			evalEP = n;
			break;
		}
	}
	// Scan up until bytes 0x55 (push ebp) and 0x8B,0xEC (mov ebp,esp)
	if (evalEP) {
		while (evalEP != 0) {
			if (this->FPEHeader32.Image[evalEP] == 0x55 && this->FPEHeader32.Image[evalEP + 1] == 0x8B && this->FPEHeader32.Image[evalEP + 2] == 0xEC)
				break;
			evalEP--;
		}
	}
	// Check evalEP
	if (evalEP +this->FPEHeader32.CodeBase != NTHeaders.OptionalHeader.AddressOfEntryPoint +this->FPEHeader32.ImageBase) {
		sprintf(
			msg,
			"Possible invalid EP (NTHeader:%lX, Evaluated:%lX). Input valid EP?",
			NTHeaders.OptionalHeader.AddressOfEntryPoint +this->FPEHeader32.ImageBase,
			evalEP +this->FPEHeader32.CodeBase
			);

		if (Application->MessageBox(String(msg).c_str(), L"Confirmation", MB_YESNO) == IDYES) {
			sEP = InputDialogExec("New EP", "EP:", Val2Str0(NTHeaders.OptionalHeader.AddressOfEntryPoint +this->FPEHeader32.ImageBase));
			if (sEP != "") {
				sscanf(AnsiString(sEP).c_str(), "%lX", &this->FPEHeader32.EP);
				if (!IsValidImageAdr(this->FPEHeader32.EP)) {
					delete[]SectionHeaders;
					delete[]this->FPEHeader32.Image;
					this->FPEHeader32.Image = 0;
					return 0;
				}
			}
			else {
				delete[]SectionHeaders;
				delete[]this->FPEHeader32.Image;
				this->FPEHeader32.Image = 0;
				return 0;
			}
		}
		else {
			delete[]SectionHeaders;
			delete[]this->FPEHeader32.Image;
			this->FPEHeader32.Image = 0;
			return 0;
		}
	}
	else {
		this->FPEHeader32.EP = NTHeaders.OptionalHeader.AddressOfEntryPoint +this->FPEHeader32.ImageBase;
	}
	// Find DataStart
	// DWORD _codeEnd = DataEnd;
	// DataStart = CodeStart;
	// for (i = 0; i < SectionsNum; i++)
	// {
	// if (SectionHeaders[i].VirtualAddress + ImageBase > EP)
	// {
	// _codeEnd = SectionHeaders[i].VirtualAddress;
	// DataStart = SectionHeaders[i].VirtualAddress;
	// break;
	// }
	// }
	delete[]SectionHeaders;

	this->FPEHeader32.CodeSize = this->FPEHeader32.TotalSize; // _codeEnd - SectionHeaders[0].VirtualAddress;
	// DataSize = DataEnd - DataStart;
	// DataBase = ImageBase + DataStart;

	this->FPEHeader32.Flags = new DWORD[this->FPEHeader32.TotalSize];
	memset(this->FPEHeader32.Flags, cfUndef, sizeof(DWORD)* this->FPEHeader32.TotalSize);
	this->FPEHeader32.Infos = new PInfoRec[this->FPEHeader32.TotalSize];
	memset(this->FPEHeader32.Infos, 0, sizeof(PInfoRec)* this->FPEHeader32.TotalSize);
	this->FPEHeader32.BSSInfos = new TStringList;
	this->FPEHeader32.BSSInfos->Sorted = true;

	if (loadExp) {
		// Load Exports
		DWORD ExpRVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		// DWORD ExpSize = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

		if (ExpRVA) {
			IMAGE_EXPORT_DIRECTORY ExportDescriptor;
			memmove(&ExportDescriptor, (this->FPEHeader32.Image + Adr2Pos(ExpRVA +this->FPEHeader32.ImageBase)), sizeof(IMAGE_EXPORT_DIRECTORY));
			ExpNum = ExportDescriptor.NumberOfFunctions;
			DWORD ExpFuncNamPos = ExportDescriptor.AddressOfNames;
			DWORD ExpFuncAdrPos = ExportDescriptor.AddressOfFunctions;
			DWORD ExpFuncOrdPos = ExportDescriptor.AddressOfNameOrdinals;

			for (i = 0; i < ExpNum; i++) {
				PExportNameRec recE = new ExportNameRec;

				DWORD dp = *((DWORD*)(this->FPEHeader32.Image + Adr2Pos(ExpFuncNamPos +this->FPEHeader32.ImageBase)));
				NameLength = strlen((char*)(this->FPEHeader32.Image + Adr2Pos(dp +this->FPEHeader32.ImageBase)));
				recE->name = String((char*)(this->FPEHeader32.Image + Adr2Pos(dp +this->FPEHeader32.ImageBase)), NameLength);

				WORD dw = *((WORD*)(this->FPEHeader32.Image + Adr2Pos(ExpFuncOrdPos +this->FPEHeader32.ImageBase)));
				recE->address = *((DWORD*)(this->FPEHeader32.Image + Adr2Pos(ExpFuncAdrPos + 4 * dw +this->FPEHeader32.ImageBase)))+this->FPEHeader32.ImageBase;
				recE->ord = dw + ExportDescriptor.Base;
				this->FPEHeader32.ExpFuncList->Add((void*)recE);

				ExpFuncNamPos += 4;
				ExpFuncOrdPos += 2;
			}
			this->FPEHeader32.ExpFuncList->Sort(ExportsCmpFunction);
		}
	}

	DWORD ImpRVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	DWORD ImpSize = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

	if (loadImp && (ImpRVA || ImpSize)) {
		if (!ImportsValid(ImpRVA, ImpSize)) {
			ShowMessage("Imports not valid, will skip!");
		}
		else {
			// Load Imports
			DWORD EntryRVA; // Next import decriptor RVA
			DWORD EndRVA; // End of imports
			DWORD ThunkRVA; // RVA of next thunk (from FirstThunk)
			DWORD LookupRVA;
			// RVA of next thunk (from OriginalFirstTunk or FirstThunk)
			DWORD ThunkValue;
			// Value of next thunk (from OriginalFirstTunk or FirstThunk)
			WORD Hint; // Ordinal or hint of imported symbol

			IMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

			// DWORD fnProc = 0;

			// First import descriptor
			EntryRVA = ImpRVA;
			EndRVA = ImpRVA + ImpSize;

			while (1) {
				memmove(&ImportDescriptor, (this->FPEHeader32.Image + Adr2Pos(EntryRVA +this->FPEHeader32.ImageBase)), sizeof(IMAGE_IMPORT_DESCRIPTOR));
				// All descriptor fields are NULL - end of list, break
				if (!ImportDescriptor.OriginalFirstThunk && !ImportDescriptor.TimeDateStamp && !ImportDescriptor.ForwarderChain && !ImportDescriptor.Name && !ImportDescriptor.FirstThunk)
					break;

				NameLength = strlen((char*)(this->FPEHeader32.Image + Adr2Pos(ImportDescriptor.Name +this->FPEHeader32.ImageBase)));
				moduleName = String((char*)(this->FPEHeader32.Image + Adr2Pos(ImportDescriptor.Name +this->FPEHeader32.ImageBase)), NameLength);

				int pos = moduleName.Pos(".");
				if (pos)
					modName = moduleName.SubString(1, pos - 1);
				else
					modName = moduleName;

				if (-1 == this->FPEHeader32.ImpModuleList->IndexOf(moduleName))
					this->FPEHeader32.ImpModuleList->Add(moduleName);

				// HINSTANCE hLib = LoadLibraryEx(moduleName.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);

				// Define the source of import names (OriginalFirstThunk or FirstThunk)
				if (ImportDescriptor.OriginalFirstThunk)
					LookupRVA = ImportDescriptor.OriginalFirstThunk;
				else
					LookupRVA = ImportDescriptor.FirstThunk;

				// ThunkRVA get from FirstThunk always
				ThunkRVA = ImportDescriptor.FirstThunk;
				// Get Imported Functions
				while (1) {
					// Names or ordinals get from LookupTable (this table can be inside OriginalFirstThunk or FirstThunk)
					ThunkValue = *((DWORD*)(this->FPEHeader32.Image + Adr2Pos(LookupRVA +this->FPEHeader32.ImageBase)));
					if (!ThunkValue)
						break;

					// fnProc = 0;
					PImportNameRec recI = new ImportNameRec;

					if (ThunkValue & 0x80000000) {
						// By ordinal
						Hint = (WORD)(ThunkValue & 0xFFFF);

						// if (hLib) fnProc = (DWORD)GetProcAddress(hLib, (char*)Hint);

						// Addresse get from FirstThunk only
						// recI->name = modName + "." + String(Hint);
						recI->name = String(Hint);
					}
					else {
						// by name
						Hint = *((WORD*)(this->FPEHeader32.Image + Adr2Pos(ThunkValue +this->FPEHeader32.ImageBase)));
						NameLength = lstrlen((char*)(this->FPEHeader32.Image + Adr2Pos(ThunkValue + 2+this->FPEHeader32.ImageBase)));
						impFuncName = String((char*)(this->FPEHeader32.Image + Adr2Pos(ThunkValue + 2+this->FPEHeader32.ImageBase)), NameLength);

						// if (hLib)
						// {
						// fnProc = (DWORD)GetProcAddress(hLib, impFuncName.c_str());
						// memmove((void*)(Image + ThunkRVA), (void*)&fnProc, sizeof(DWORD));
						// }

						recI->name = impFuncName;
					}
					recI->module = modName;
					recI->address = this->FPEHeader32.ImageBase + ThunkRVA;
					this->FPEHeader32.ImpFuncList->Add((void*)recI);
					//
					SetFlag(cfImport, Adr2Pos(recI->address));
					PInfoRec recN = new InfoRec(Adr2Pos(recI->address), ikData);
					recN->SetName(impFuncName);
					//
					ThunkRVA += 4;
					LookupRVA += 4;
				}
				EntryRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
				if (EntryRVA >= EndRVA)
					break;

				// if (hLib)
				// {
				// FreeLibrary(hLib);
				// hLib = NULL;
				// }
			}
			this->FPEHeader32.ImpFuncList->Sort(ImportsCmpFunction);
		}
	}
	return 1;
}
// ---------------------------------------------------------------------------
