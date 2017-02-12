//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <assert>
#include "KnowledgeBase.h"

#include "Main.h"
#include "Misc.h" 
//---------------------------------------------------------------------------
extern  DWORD       CodeBase;
extern  BYTE        *Code;
                                 String __fastcall TrimTypeName(const String& TypeName);
//---------------------------------------------------------------------------
#define cfCode          0x00000001  //Байт относится к коду
#define cfData          0x00000002  //Байт относится к данным
FIELDINFO::~FIELDINFO()
{
    CleanupList<XrefRec>(xrefs);
}
//---------------------------------------------------------------------------
__fastcall MConstInfo::MConstInfo()
{
    ModuleID = 0xFFFF;
    ConstName = "";
    Type = 0;
    TypeDef = "";
    Value = "";
    DumpSz = 0;
    FixupNum = 0;
    Dump = 0;
}
//---------------------------------------------------------------------------
__fastcall MConstInfo::~MConstInfo()
{
    //as direct cache mem usage
    //if (Dump) delete[] Dump;
}
//---------------------------------------------------------------------------
__fastcall MTypeInfo::MTypeInfo()
{
    Size = 0;
    ModuleID = 0xFFFF;
    TypeName = "";
    Kind = 0xFF;
    VMCnt = 0;
    Decl = "";
    DumpSz = 0;
    FixupNum = 0;
    Dump = 0;
    FieldsNum = 0;
    Fields = 0;
    PropsNum = 0;
    Props = 0;
    MethodsNum = 0;
    Methods = 0;
}
//---------------------------------------------------------------------------
__fastcall MTypeInfo::~MTypeInfo()
{
}
//---------------------------------------------------------------------------
__fastcall MVarInfo::MVarInfo()
{
    ModuleID = 0xFFFF;
    VarName = "";
    Type = 0;
    TypeDef = "";
    AbsName = "";
}
//---------------------------------------------------------------------------
__fastcall MVarInfo::~MVarInfo()
{
}
//---------------------------------------------------------------------------
__fastcall MResStrInfo::MResStrInfo()
{
    ModuleID = 0xFFFF;
    ResStrName = "";
    TypeDef = "";
    //AContext = "";
}
//---------------------------------------------------------------------------
__fastcall MResStrInfo::~MResStrInfo()
{
}
//---------------------------------------------------------------------------
__fastcall MProcInfo::MProcInfo()
{
    ModuleID = 0xFFFF;
    ProcName = "";
    Embedded = false;
    DumpType = 0;
    MethodKind = 0;
    CallKind = 0;
    VProc = 0;
    TypeDef = "";
    DumpSz = 0;
    FixupNum = 0;
    Dump = 0;
    ArgsNum = 0;
    Args = 0;
    //LocalsNum = 0;
    //Locals = 0;
}
//---------------------------------------------------------------------------
__fastcall MProcInfo::~MProcInfo()
{
}
//---------------------------------------------------------------------------
__fastcall  MKnowledgeBase::MKnowledgeBase()
{
    Inited = false;
    Version = 0.0;
    Handle = 0;

    Mods = 0;
    ModuleOffsets = 0;
    ConstOffsets = 0;
    TypeOffsets = 0;
    VarOffsets = 0;
    ResStrOffsets = 0;
    ProcOffsets = 0;
    UsedProcs = 0;

    KBCache = 0;
    SizeKBFile = 0;
    NameKBFile = "";
}
//---------------------------------------------------------------------------
__fastcall MKnowledgeBase::~MKnowledgeBase()
{
    Close();
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::CheckKBFile()
{
    bool    _isMSIL;
    int     _delphiVer;
    DWORD   _crc;
    DWORD   _kbVer;
    char    _signature[24];
    char    _description[256];

    fread(_signature, 1, 24, Handle);
    fread(&_isMSIL, sizeof(_isMSIL), 1, Handle);
    fread(&_delphiVer, sizeof(_delphiVer), 1, Handle);
    fread(&_crc, sizeof(_crc), 1, Handle);
    fread(_description, 1, 256, Handle);
    fread(&_kbVer, sizeof(_kbVer), 1, Handle);
    //Old format
    if (!strcmp(_signature, "IDD Knowledge Base File") && _kbVer == 1)
    {
        Version = _kbVer;
        return true;
    }
    //New format
    if (!strcmp(_signature, "IDR Knowledge Base File") && _kbVer == 2)
    {
        Version = _kbVer;
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::Open(char* filename)
{
    if (Inited)
    {
        if (NameKBFile == String(filename))
            return true;
        Close();
    }

    Inited = false;
    Handle = fopen(filename, "rb");
    if (!Handle) return false;

    if (!CheckKBFile())
    {
        fclose(Handle);
        Handle = 0;
        return false;
    }

    fseek(Handle, -4L, SEEK_END);
    //Read SectionsOffset
    fread(&SectionsOffset, sizeof(SectionsOffset), 1, Handle);
    //Seek at SectionsOffset
    fseek(Handle, SectionsOffset, SEEK_SET);

    //Read section Modules
    fread(&ModuleCount, sizeof(ModuleCount), 1, Handle);
    Mods = new WORD[ModuleCount + 1];
    memset(Mods, 0xFF, (ModuleCount + 1)*sizeof(WORD));
    fread(&MaxModuleDataSize, sizeof(MaxModuleDataSize), 1, Handle);
    ModuleOffsets = new OFFSETSINFO[ModuleCount];
    fread(ModuleOffsets, sizeof(OFFSETSINFO), ModuleCount, Handle);
    //Read section Consts
    fread(&ConstCount, sizeof(ConstCount), 1, Handle);
    fread(&MaxConstDataSize, sizeof(MaxConstDataSize), 1, Handle);
    ConstOffsets = new OFFSETSINFO[ConstCount];
    fread((BYTE*)ConstOffsets, sizeof(OFFSETSINFO), ConstCount, Handle);
    //Read section Types
    fread(&TypeCount, sizeof(TypeCount), 1, Handle);
    fread(&MaxTypeDataSize, sizeof(MaxTypeDataSize), 1, Handle);
    TypeOffsets = new OFFSETSINFO[TypeCount];
    fread((BYTE*)TypeOffsets, sizeof(OFFSETSINFO), TypeCount, Handle);
    //Read section Vars
    fread(&VarCount, sizeof(VarCount), 1, Handle);
    fread(&MaxVarDataSize, sizeof(MaxVarDataSize), 1, Handle);
    VarOffsets = new OFFSETSINFO[VarCount];
    fread((BYTE*)VarOffsets, sizeof(OFFSETSINFO), VarCount, Handle);
    //Read section ResStr
    fread(&ResStrCount, sizeof(ResStrCount), 1, Handle);
    fread(&MaxResStrDataSize, sizeof(MaxResStrDataSize), 1, Handle);
    ResStrOffsets = new OFFSETSINFO[ResStrCount];
    fread((BYTE*)ResStrOffsets, sizeof(OFFSETSINFO), ResStrCount, Handle);
    //Read section Procs
    fread(&ProcCount, sizeof(ProcCount), 1, Handle);
    fread(&MaxProcDataSize, sizeof(MaxProcDataSize), 1, Handle);
    ProcOffsets = new OFFSETSINFO[ProcCount];
    fread((BYTE*)ProcOffsets, sizeof(OFFSETSINFO), ProcCount, Handle);
    UsedProcs = new BYTE[ProcCount];
    memset(UsedProcs, 0, ProcCount);

    //as global KB file cache in RAM (speed up 3..4 times!)
    fseek(Handle, 0L, SEEK_END);
    SizeKBFile = ftell(Handle);
    if (SizeKBFile > 0)
    {
        KBCache = new BYTE[SizeKBFile];
        if (KBCache)
        {
            fseek(Handle, 0L, SEEK_SET);
            fread((BYTE*)KBCache, 1, SizeKBFile, Handle);
        }
    }
    fclose(Handle); Handle = 0;
    
    NameKBFile = String(filename);
    Inited = true;
    return true;
}
//---------------------------------------------------------------------------
void __fastcall MKnowledgeBase::Close()
{
    if (Inited)
    {
        if (Mods) delete[] Mods;
        if (ModuleOffsets) delete[] ModuleOffsets;
        if (ConstOffsets) delete[] ConstOffsets;
        if (TypeOffsets) delete[] TypeOffsets;
        if (VarOffsets) delete[] VarOffsets;
        if (ResStrOffsets) delete[] ResStrOffsets;
        if (ProcOffsets) delete[] ProcOffsets;
        if (UsedProcs) delete[] UsedProcs;

        //as
        if (KBCache) delete[] KBCache;
        Inited = false;
    }
}
//---------------------------------------------------------------------------
WORD __fastcall MKnowledgeBase::GetModuleID(char* ModuleName)
{
	if (!Inited) return 0xFFFF;

    if (!ModuleName || !*ModuleName || !ModuleCount) return 0xFFFF;

    int ID, F = 0, L = ModuleCount - 1;
    const BYTE* p;
    while (F < L)
    {
        int M = (F + L)/2;
        ID = ModuleOffsets[M].NamId;
        p = GetKBCachePtr(ModuleOffsets[ID].Offset, ModuleOffsets[ID].Size);
        if (stricmp(ModuleName, p + 4) <= 0)
            L = M;
        else
            F = M + 1;
    }
    ID = ModuleOffsets[L].NamId;
    p = GetKBCachePtr(ModuleOffsets[ID].Offset, ModuleOffsets[ID].Size);
    if (!stricmp(ModuleName, p + 4)) return *((WORD*)p);
    return 0xFFFF;
}
//---------------------------------------------------------------------------
String __fastcall MKnowledgeBase::GetModuleName(WORD ModuleID)
{
	if (!Inited) return "";

    if (ModuleID == 0xFFFF || !ModuleCount) return "";

    int ID, F = 0, L = ModuleCount - 1;
    const BYTE* p;
    WORD ModID;
    while (F < L)
    {
        int M = (F + L)/2;
        ID = ModuleOffsets[M].ModId;
        p = GetKBCachePtr(ModuleOffsets[ID].Offset, ModuleOffsets[ID].Size);
        ModID = *((WORD*)p);
        if (ModuleID <= ModID)
            L = M;
        else
            F = M + 1;
    }
    ID = ModuleOffsets[L].ModId;
    p = GetKBCachePtr(ModuleOffsets[ID].Offset, ModuleOffsets[ID].Size);
    ModID = *((WORD*)p);
    if (ModuleID == ModID) return String((char*)(p + 4));
    return "";
}
//---------------------------------------------------------------------------
//Return modules ids list containing given proc
void __fastcall MKnowledgeBase::GetModuleIdsByProcName(char* AProcName)
{
    Mods[0] = 0xFFFF;

    if (!Inited) return;

    if (!AProcName || !*AProcName || !ProcCount) return;
    
    int L = 0, R = ProcCount - 1, n;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        int res = stricmp(AProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(AProcName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(AProcName, p + 4);
                if (res) break;
            }

            n = 0;
            for (int N = LN + 1; N < RN; N++)
            {
                ID = ProcOffsets[N].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                Mods[n] = *((WORD*)p); n++;
            }
            Mods[n] = 0xFFFF;
            return;
        }
        if (L > R) return;
    }
}
//---------------------------------------------------------------------------
//Return sections containing given ItemName
int __fastcall MKnowledgeBase::GetItemSection(WORD* ModuleIDs, char* ItemName)
{
    bool    found;
    int     L, R, M, ID, N, LN, RN, Result = KB_NO_SECTION;

    if (!Inited) return Result;

    if (!ItemName ||*ItemName == 0) return Result;
    WORD ModuleID, ModID;
    //CONST
    if (ConstCount)
    {
        L = 0; R = ConstCount - 1;
        while (1)
        {
            M = (L + R)/2;
            ID = ConstOffsets[M].NamId;
            const BYTE* p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
            int res = stricmp(ItemName, p + 4);
            if (res < 0)
                R = M - 1;
            else if (res > 0)
                L = M + 1;
            else
            {
                //Find left boundary
                for (LN = M - 1; LN >= 0; LN--)
                {
                    ID = ConstOffsets[LN].NamId;
                    p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                //Find right boundary
                for (RN = M + 1; RN < ConstCount; RN++)
                {
                    ID = ConstOffsets[RN].NamId;
                    p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                found = false;
                for (N = LN + 1; N < RN; N++)
                {
                    ID = ConstOffsets[N].NamId;
                    p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                    ModID = *((WORD*)p);
                    for (int n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID)
                        {
                            Result |= KB_CONST_SECTION;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                break;
            }
            if (L > R) break;
        }
    }
    //TYPE
    if (TypeCount)
    {
        L = 0; R = TypeCount - 1;
        while (1)
        {
            M = (L + R)/2;
            ID = TypeOffsets[M].NamId;
            const BYTE* p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
            if (Version >= 2) p += 4; //Add by ZGL
            int res = stricmp(ItemName, p + 4);
            if (res < 0)
                R = M - 1;
            else if (res > 0)
                L = M + 1;
            else
            {
                //Find left boundary
                for (LN = M - 1; LN >= 0; LN--)
                {
                    ID = TypeOffsets[LN].NamId;
                    p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                    if (Version >= 2) p += 4; //Add by ZGL
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                //Find right boundary
                for (RN = M + 1; RN < TypeCount; RN++)
                {
                    ID = TypeOffsets[RN].NamId;
                    p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                    if (Version >= 2) p += 4; //Add by ZGL
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                found = false;
                for (N = LN + 1; N < RN; N++)
                {
                    ID = TypeOffsets[N].NamId;
                    p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                    if (Version >= 2) p += 4; //Add by ZGL
                    ModID = *((WORD*)p);
                    for (int n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID)
                        {
                            Result |= KB_TYPE_SECTION;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                break;
            }
            if (L > R) break;
        }
    }
    //VAR
    if (VarCount)
    {
        L = 0; R = VarCount - 1;
        while (1)
        {
            M = (L + R)/2;
            ID = VarOffsets[M].NamId;
            const BYTE* p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
            int res = stricmp(ItemName, p + 4);
            if (res < 0)
                R = M - 1;
            else if (res > 0)
                L = M + 1;
            else
            {
                //Find left boundary
                for (LN = M - 1; LN >= 0; LN--)
                {
                    ID = VarOffsets[LN].NamId;
                    p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                //Find right boundary
                for (RN = M + 1; RN < VarCount; RN++)
                {
                    ID = VarOffsets[RN].NamId;
                    p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                found = false;
                for (N = LN + 1; N < RN; N++)
                {
                    ID = VarOffsets[N].NamId;
                    p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                    ModID = *((WORD*)p);
                    for (int n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID)
                        {
                            Result |= KB_VAR_SECTION;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                break;
            }
            if (L > R) break;
        }
    }
    //RESSTR
    if (ResStrCount)
    {
        L = 0; R = ResStrCount - 1;
        while (1)
        {
            M = (L + R)/2;
            ID = ResStrOffsets[M].NamId;
            const BYTE* p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
            int res = stricmp(ItemName, p + 4);
            if (res < 0)
                R = M - 1;
            else if (res > 0)
                L = M + 1;
            else
            {
                //Find left boundary
                for (LN = M - 1; LN >= 0; LN--)
                {
                    ID = ResStrOffsets[LN].NamId;
                    p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                //Find right boundary
                for (RN = M + 1; RN < ResStrCount; RN++)
                {
                    ID = ResStrOffsets[RN].NamId;
                    p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                found = false;
                for (N = LN + 1; N < RN; N++)
                {
                    ID = ResStrOffsets[N].NamId;
                    p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                    ModID = *((WORD*)p);
                    for (int n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID)
                        {
                            Result |= KB_RESSTR_SECTION;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                break;
            }
            if (L > R) break;
        }
    }
    //PROC
    if (ProcCount)
    {
        L = 0; R = ProcCount - 1;
        while (1)
        {
            M = (L + R)/2;
            ID = ProcOffsets[M].NamId;
            const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
            int res = stricmp(ItemName, p + 4);
            if (res < 0)
                R = M - 1;
            else if (res > 0)
                L = M + 1;
            else
            {
                //Find left boundary
                for (LN = M - 1; LN >= 0; LN--)
                {
                    ID = ProcOffsets[LN].NamId;
                    p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                //Find right boundary
                for (RN = M + 1; RN < ProcCount; RN++)
                {
                    ID = ProcOffsets[RN].NamId;
                    p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                    res = stricmp(ItemName, p + 4);
                    if (res) break;
                }
                found = false;
                for (N = LN + 1; N < RN; N++)
                {
                    ID = ProcOffsets[N].NamId;
                    p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                    ModID = *((WORD*)p);
                    for (int n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID)
                        {
                            Result |= KB_PROC_SECTION;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                break;
            }
            if (L > R) break;
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
//Return constant index by name in given ModuleID
int __fastcall MKnowledgeBase::GetConstIdx(WORD* ModuleIDs, char* ConstName)
{
	if (!Inited) return -1;

    if (!ModuleIDs || !ConstName || !*ConstName || !ConstCount) return -1;
    
    WORD ModuleID, ModID;
    int n, L = 0, R = ConstCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ConstOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
        int res = stricmp(ConstName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ConstOffsets[LN].NamId;
                p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                res = stricmp(ConstName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ConstCount; RN++)
            {
                ID = ConstOffsets[RN].NamId;
                p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                res = stricmp(ConstName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                ID = ConstOffsets[N].NamId;
                p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                ModID = *((WORD*)p);

                for (n = 0;;n++)
                {
                    ModuleID = ModuleIDs[n];
                    if (ModuleID == 0xFFFF) break;
                    if (ModuleID == ModID) return N;
                }
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetConstIdxs(char* ConstName, int* ConstIdx)
{
    *ConstIdx = -1;
    
    if (!Inited) return 0;

    if (!ConstName ||!*ConstName || !ConstCount) return 0;

    int L = 0, R = ConstCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ConstOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
        int res = stricmp(ConstName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int Num = 1;
            *ConstIdx = M;
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ConstOffsets[LN].NamId;
                p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                if (stricmp(ConstName, p + 4)) break;
                Num++;
            }
            //Find right boundary
            for (RN = M + 1; RN < ConstCount; RN++)
            {
                ID = ConstOffsets[RN].NamId;
                p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
                if (stricmp(ConstName, p + 4)) break;
                Num++;
            }
            return Num;
        }
        if (L > R) return 0;
    }
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetTypeIdxByModuleIds(WORD* ModuleIDs, char* TypeName)
{
	if (!Inited) return -1;

    if (!ModuleIDs || !TypeName || !*TypeName || !TypeCount) return -1;
    
    WORD ModuleID, ModID;
    int n, L = 0, R = TypeCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = TypeOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
        if (Version >= 2) p += 4; //Add by ZGL
        int res = stricmp(TypeName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = TypeOffsets[LN].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                res = stricmp(TypeName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < TypeCount; RN++)
            {
                ID = TypeOffsets[RN].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                res = stricmp(TypeName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                ID = TypeOffsets[N].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                ModID = *((WORD*)p);

                for (n = 0;;n++)
                {
                    ModuleID = ModuleIDs[n];
                    if (ModuleID == 0xFFFF) break;
                    if (ModuleID == ModID) return N;
                }
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetTypeIdxsByName(char* TypeName, int* TypeIdx)
{
    *TypeIdx = -1;

    if (!Inited) return 0;

    if (!TypeName || !*TypeName || !TypeCount) return 0;

    int L = 0, R = TypeCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = TypeOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
        if (Version >= 2) p += 4; //Add by ZGL
        int res = stricmp(TypeName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int Num = 1;
            *TypeIdx = M;
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = TypeOffsets[LN].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                if (stricmp(TypeName, p + 4)) break;
                Num++;
            }
            //Find right boundary
            for (RN = M + 1; RN < TypeCount; RN++)
            {
                ID = TypeOffsets[RN].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                if (stricmp(TypeName, p + 4)) break;
                Num++;
            }
            return Num;
        }
        if (L > R) return 0;
    }
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetTypeIdxByUID(char* UID)
{
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetVarIdx(WORD* ModuleIDs, char* VarName)
{
	if (!Inited) return -1;

    if (!ModuleIDs || !VarName || !*VarName || !VarCount) return -1;

    WORD ModuleID, ModID, len;
    int n, L = 0, R = VarCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = VarOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
        int res = stricmp(VarName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = VarOffsets[LN].NamId;
                p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                res = stricmp(VarName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < VarCount; RN++)
            {
                ID = VarOffsets[RN].NamId;
                p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                res = stricmp(VarName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                ID = VarOffsets[N].NamId;
                p = GetKBCachePtr(VarOffsets[ID].Offset, VarOffsets[ID].Size);
                ModID = *((WORD*)p);
                for (n = 0;;n++)
                {
                    ModuleID = ModuleIDs[n];
                    if (ModuleID == 0xFFFF) break;
                    if (ModuleID == ModID) return N;
                }
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetResStrIdx(int from, char* ResStrContext)
{
	if (!Inited) return -1;

    if (!ResStrContext || !*ResStrContext || !ResStrCount) return -1;

    int n, ID;
    for (n = from; n < ResStrCount; n++)
    {
        ID = ResStrOffsets[n].NamId;
        const BYTE* p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
        //ModuleID
        p += 2;
        //ResStrName
        WORD len = *((WORD*)p); p += len + 3;
        //TypeDef
        len = *((WORD*)p); p += len + 5;
        //Context
        if (!stricmp(ResStrContext, p))
        {
            return n;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetResStrIdx(WORD ModuleID, char* ResStrContext)
{
	if (!Inited) return -1;

    if (!ResStrContext || !*ResStrContext || !ResStrCount) return -1;

    WORD ModID, len;
    int n, ID;

    for (n = 0; n < ResStrCount; n++)
    {
        ID = ResStrOffsets[n].NamId;
        const BYTE* p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
        //ModuleID
        ModID = *((WORD*)p); p += 2;
        //ResStrName
        len = *((WORD*)p); p += len + 3;
        //TypeDef
        len = *((WORD*)p); p += len + 5;
        //Context
        if (ModuleID == ModID && !stricmp(ResStrContext, p))
        {
            return n;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetResStrIdx(WORD* ModuleIDs, char* ResStrName)
{
	if (!Inited) return -1;

    if (!ModuleIDs || !ResStrName || !*ResStrName || !ResStrCount) return -1;

    WORD ModuleID, ModID, len;
    int n, L = 0, R = ResStrCount - 1, M, ID, res, LN, RN, N;
    
    while (1)
    {
        M = (L + R)/2;
        ID = ResStrOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
        res = stricmp(ResStrName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ResStrOffsets[LN].NamId;
                p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                res = stricmp(ResStrName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ResStrCount; RN++)
            {
                ID = ResStrOffsets[RN].NamId;
                p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                res = stricmp(ResStrName, p + 4);
                if (res) break;
            }
            for (N = LN + 1; N < RN; N++)
            {
                ID = ResStrOffsets[N].NamId;
                p = GetKBCachePtr(ResStrOffsets[ID].Offset, ResStrOffsets[ID].Size);
                ModID = *((WORD*)p);
                for (n = 0;;n++)
                {
                    ModuleID = ModuleIDs[n];
                    if (ModuleID == 0xFFFF) break;
                    if (ModuleID == ModID) return N;
                }
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
//Return proc index by name in given ModuleID
int __fastcall MKnowledgeBase::GetProcIdx(WORD ModuleID, char* ProcName)
{
	if (!Inited) return -1;

    if (ModuleID == 0xFFFF || !ProcName || !*ProcName || !ProcCount) return -1;

    WORD ModID;
    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        int res = stricmp(ProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                ID = ProcOffsets[N].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                ModID = *((WORD*)p);
                if (ModuleID == ModID) return N;
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
//Return proc index by name in given ModuleID
//Code used for selection required proc (if there are exist several procs with the same name)
int __fastcall MKnowledgeBase::GetProcIdx(WORD ModuleID, char* ProcName, BYTE* code)
{
	if (!Inited) return -1;
    
    if (ModuleID == 0xFFFF || !ProcName || !*ProcName || !ProcCount) return -1;

    MProcInfo aInfo; MProcInfo* pInfo;
    WORD ModID;
    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        int res = stricmp(ProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                pInfo = GetProcInfo(ProcOffsets[N].NamId, INFO_DUMP, &aInfo);
                ModID = pInfo->ModuleID;
                bool match = MatchCode(code, pInfo);
                if (match && ModuleID == ModID) return N;
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
//Return proc index by name in given array of ModuleID
int __fastcall MKnowledgeBase::GetProcIdx(WORD* ModuleIDs, char* ProcName, BYTE* code)
{
	if (!Inited) return -1;

    if (!ModuleIDs || !ProcName || !*ProcName || !ProcCount) return -1;

    MProcInfo aInfo; MProcInfo* pInfo;
    WORD ModuleID, ModID;
    int n, L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        int res = stricmp(ProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            for (int N = LN + 1; N < RN; N++)
            {
                pInfo = GetProcInfo(ProcOffsets[N].NamId, INFO_DUMP, &aInfo);
                ModID = pInfo->ModuleID;
                if (code)
                {
                    bool match = MatchCode(code, pInfo);
                    if (match)
                    {
                        for (n = 0;;n++)
                        {
                            ModuleID = ModuleIDs[n];
                            if (ModuleID == 0xFFFF) break;
                            if (ModuleID == ModID) return N;
                        }
                    }
                }
                else
                {
                    for (n = 0;;n++)
                    {
                        ModuleID = ModuleIDs[n];
                        if (ModuleID == 0xFFFF) break;
                        if (ModuleID == ModID) return N;
                    }
                }
            }
            //Nothing found - exit
            return -1;
        }
        if (L > R) return -1;
    }
}
//---------------------------------------------------------------------------
//Seek first ans last proc ID in given module
bool __fastcall MKnowledgeBase::GetProcIdxs(WORD ModuleID, int* FirstProcIdx, int* LastProcIdx)
{
	if (!Inited) return false;

    if (ModuleID == 0xFFFF || !ProcCount || !FirstProcIdx || !LastProcIdx) return false;

    *FirstProcIdx = -1; *LastProcIdx = -1;
    WORD ModID;
    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].ModId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        ModID = *((WORD*)p);

        if (ModuleID < ModID)
            R = M - 1;
        else if (ModuleID > ModID)
            L = M + 1;
        else
        {
            *FirstProcIdx = M; *LastProcIdx = M;
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].ModId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                ModID = *((WORD*)p);
                if (ModID != ModuleID) break;
                *FirstProcIdx = LN;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].ModId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                ModID = *((WORD*)p);
                if (ModID != ModuleID) break;
                *LastProcIdx = RN;
            }
            return true;
        }
        if (L > R) return false;
    }
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::GetProcIdxs(WORD ModuleID, int* FirstProcIdx, int* LastProcIdx, int* DumpSize)
{
	if (!Inited || ModuleID == 0xFFFF || !ProcCount) return false;

    *FirstProcIdx = -1; *LastProcIdx = -1; *DumpSize = 0;
    WORD ModID;
    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].ModId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        ModID = *((WORD*)p);

        if (ModuleID < ModID)
            R = M - 1;
        else if (ModuleID > ModID)
            L = M + 1;
        else
        {
            *FirstProcIdx = M; *LastProcIdx = M;
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].ModId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                ModID = *((WORD*)p);
                if (ModID != ModuleID) break;
                *FirstProcIdx = LN;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].ModId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                ModID = *((WORD*)p);
                if (ModID != ModuleID) break;
                *LastProcIdx = RN;
            }
            for (int m = *FirstProcIdx; m <= *LastProcIdx; m++)
            {
                p = GetKBCachePtr(ProcOffsets[m].Offset, ProcOffsets[m].Size);
                //ModuleID
                p += 2;
                //ProcName
                WORD Len = *((WORD*)p); p += Len + 3;
                //Embedded
                p++;
                //DumpType
                p++;
                //MethodKind
                p++;
                //CallKind
                p++;
                //VProc
                p += 4;
                //TypeDef
                Len = *((WORD*)p); p += Len + 3;
                //DumpTotal
                p += 4;
                //DumpSz
                int size = *((DWORD*)p);
                *DumpSize += (size + 3) & (-4);
            }
            return true;
        }
        if (L > R) return false;
    }
}
//---------------------------------------------------------------------------
//ConstIdx was given by const name
MConstInfo* __fastcall MKnowledgeBase::GetConstInfo(int AConstIdx, DWORD AFlags, MConstInfo* cInfo)
{
	if (!Inited) return 0;

    if (AConstIdx == -1) return 0;

    const BYTE* p = GetKBCachePtr(ConstOffsets[AConstIdx].Offset, ConstOffsets[AConstIdx].Size);
    cInfo->ModuleID = *((WORD*)p); p += 2;
    WORD Len = *((WORD*)p); p += 2;
    cInfo->ConstName = String((char*)p); p += Len + 1;
    cInfo->Type = *p; p++;
    Len = *((WORD*)p); p += 2;
    cInfo->TypeDef = String((char*)p); p += Len + 1;
    Len = *((WORD*)p); p += 2;
    cInfo->Value = String((char*)p); p += Len + 1;

    DWORD DumpTotal = *((DWORD*)p); p += 4;
    cInfo->DumpSz = *((DWORD*)p); p += 4;
    cInfo->FixupNum = *((DWORD*)p); p += 4;
    cInfo->Dump = 0;
    
    if (AFlags & INFO_DUMP)
    {
        if (cInfo->DumpSz) cInfo->Dump = (BYTE*)p;
    }
    return cInfo;
}
//---------------------------------------------------------------------------
MProcInfo* __fastcall MKnowledgeBase::GetProcInfo(char* ProcName, DWORD AFlags, MProcInfo *pInfo, int *procIdx)
{
	if (!Inited) return 0;

    if (!ProcName || !*ProcName || !ProcCount) return 0;

    WORD ModID;
    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
        int res = stricmp(ProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            int LN, RN;
            //Find left boundary
            for (LN = M - 1; LN >= 0; LN--)
            {
                ID = ProcOffsets[LN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            //Find right boundary
            for (RN = M + 1; RN < ProcCount; RN++)
            {
                ID = ProcOffsets[RN].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
            }
            if (RN - LN - 1 == 1)
            {
                ID = ProcOffsets[M].NamId;
                *procIdx = ID;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);

                pInfo->ModuleID = *((WORD*)p); p += 2;
                WORD Len = *((WORD*)p); p += 2;
                pInfo->ProcName = String((char*)p); p += Len + 1;
                pInfo->Embedded = *p; p++;
                pInfo->DumpType = *p; p++;
                pInfo->MethodKind = *p; p++;
                pInfo->CallKind = *p; p++;
                pInfo->VProc = *((int*)p); p += 4;
                Len = *((WORD*)p); p += 2;
                pInfo->TypeDef = TrimTypeName(String((char*)p)); p += Len + 1;

                DWORD DumpTotal = *((DWORD*)p); p += 4;
                const BYTE *p1 = p;
                pInfo->DumpSz = *((DWORD*)p); p += 4;
                pInfo->FixupNum = *((DWORD*)p); p += 4;
                pInfo->Dump = 0;
                
                if (AFlags & INFO_DUMP)
                {
                    if (pInfo->DumpSz) pInfo->Dump = (BYTE*)p;
                }
                p = p1 + DumpTotal;

                DWORD ArgsTotal = *((DWORD*)p); p += 4;
                p1 = p;
                pInfo->ArgsNum = *((WORD*)p); p += 2;
                pInfo->Args = 0;

                if (AFlags & INFO_ARGS)
                {
                    if (pInfo->ArgsNum) pInfo->Args = (BYTE*)p;
                }
                p = p1 + ArgsTotal;
                /*
                DWORD LocalsTotal = *((DWORD*)p); p += 4;
                pInfo->LocalsNum = *((WORD*)p); p += 2;
                pInfo->Locals = 0;

                if (AFlags & INFO_LOCALS)
                {
                    if (pInfo->LocalsNum) pInfo->Locals = (BYTE*)p;
                }
                */
                return pInfo;
            }
            //More than 2 items found
            /*
            for (int nnn = LN + 1; nnn <= RN - 1; nnn++)
            {
                ID = ProcOffsets[nnn].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);

                pInfo->ModuleID = *((WORD*)p); p += 2;
                WORD Len = *((WORD*)p); p += 2;
                pInfo->ProcName = String((char*)p); p += Len + 1;
                pInfo->Embedded = *p; p++;
                pInfo->DumpType = *p; p++;
                pInfo->MethodKind = *p; p++;
                pInfo->CallKind = *p; p++;
                pInfo->VProc = *((int*)p); p += 4;
                Len = *((WORD*)p); p += 2;
                pInfo->TypeDef = TrimTypeName(String((char*)p)); p += Len + 1;

                DWORD DumpTotal = *((DWORD*)p); p += 4;
                const BYTE *p1 = p;
                pInfo->DumpSz = *((DWORD*)p); p += 4;
                pInfo->FixupNum = *((DWORD*)p); p += 4;
                pInfo->Dump = 0;
                
                if (AFlags & INFO_DUMP)
                {
                    if (pInfo->DumpSz) pInfo->Dump = (BYTE*)p;
                }
                p = p1 + DumpTotal;

                DWORD ArgsTotal = *((DWORD*)p); p += 4;
                p1 = p;
                pInfo->ArgsNum = *((WORD*)p); p += 2;
                pInfo->Args = 0;

                if (AFlags & INFO_ARGS)
                {
                    if (pInfo->ArgsNum) pInfo->Args = (BYTE*)p;
                }
            }
            */
            return (MProcInfo*)-1;
        }
        if (L > R) return 0;
    }
}
//---------------------------------------------------------------------------
MProcInfo* __fastcall MKnowledgeBase::GetProcInfo(int AProcIdx, DWORD AFlags, MProcInfo *pInfo)
{
	if (!Inited) return 0;

    if (AProcIdx == -1) return 0;

    const BYTE *p = GetKBCachePtr(ProcOffsets[AProcIdx].Offset, ProcOffsets[AProcIdx].Size);

    pInfo->ModuleID = *((WORD*)p); p += 2;
    WORD Len = *((WORD*)p); p += 2;
    pInfo->ProcName = String((char*)p); p += Len + 1;
    pInfo->Embedded = *p; p++;
    pInfo->DumpType = *p; p++;
    pInfo->MethodKind = *p; p++;
    pInfo->CallKind = *p; p++;
    pInfo->VProc = *((int*)p); p += 4;
    Len = *((WORD*)p); p += 2;
    pInfo->TypeDef = TrimTypeName(String((char*)p)); p += Len + 1;

    DWORD DumpTotal = *((DWORD*)p); p += 4;
    const BYTE *p1 = p;
    pInfo->DumpSz = *((DWORD*)p); p += 4;
    pInfo->FixupNum = *((DWORD*)p); p += 4;
    pInfo->Dump = 0;

    if (AFlags & INFO_DUMP)
    {
        if (pInfo->DumpSz) pInfo->Dump = (BYTE*)p;
    }
    p = p1 + DumpTotal;

    DWORD ArgsTotal = *((DWORD*)p); p += 4;
    p1 = p;
    pInfo->ArgsNum = *((WORD*)p); p += 2;
    pInfo->Args = 0;

    if (AFlags & INFO_ARGS)
    {
        if (pInfo->ArgsNum) pInfo->Args = (BYTE*)p;
    }
    p = p1 + ArgsTotal;
    /*
    DWORD LocalsTotal = *((DWORD*)p); p += 4;
    pInfo->LocalsNum = *((WORD*)p); p += 2;
    pInfo->Locals = 0;

    if (AFlags & INFO_LOCALS)
    {
        if (pInfo->LocalsNum) pInfo->Locals = (BYTE*)p;
    }
    */
    return pInfo;
}
//---------------------------------------------------------------------------
MTypeInfo* __fastcall MKnowledgeBase::GetTypeInfo(int ATypeIdx, DWORD AFlags, MTypeInfo *tInfo)
{
	if (!Inited) return 0;

    if (ATypeIdx == -1) return 0;

    const BYTE* p = GetKBCachePtr(TypeOffsets[ATypeIdx].Offset, TypeOffsets[ATypeIdx].Size);

    //Modified by ZGL
    if (Version == 1)
        tInfo->Size = TypeOffsets[ATypeIdx].Size;
    else
        tInfo->Size = *((DWORD*)p); p += 4;

    tInfo->ModuleID = *((WORD*)p); p += 2;
    WORD Len = *((WORD*)p); p += 2;
    tInfo->TypeName = String((char*)p); p += Len + 1;
    tInfo->Kind = *p; p++;
    tInfo->VMCnt = *((WORD*)p); p += 2;
    Len = *((WORD*)p); p += 2;
    tInfo->Decl = String((char*)p); p += Len + 1;

    DWORD DumpTotal = *((DWORD*)p); p += 4;
    BYTE *p1 = (BYTE*)p;
    tInfo->DumpSz = *((DWORD*)p); p += 4;
    tInfo->FixupNum = *((DWORD*)p); p += 4;
    tInfo->Dump = 0;

    if (AFlags & INFO_DUMP)
    {
        if (tInfo->DumpSz) tInfo->Dump = (BYTE*)p;
    }
    p = p1 + DumpTotal;

    DWORD FieldsTotal = *((DWORD*)p); p += 4;
    p1 = (BYTE*)p;
    tInfo->FieldsNum = *((WORD*)p); p += 2;
    tInfo->Fields = 0;
    
    if (AFlags & INFO_FIELDS)
    {
        if (tInfo->FieldsNum) tInfo->Fields = (BYTE*)p;
    }
    p = p1 + FieldsTotal;

    DWORD PropsTotal = *((DWORD*)p); p += 4;
    p1 = (BYTE*)p;
    tInfo->PropsNum = *((WORD*)p); p += 2;
    tInfo->Props = 0;

    if (AFlags & INFO_PROPS)
    {
        if (tInfo->PropsNum) tInfo->Props = (BYTE*)p;
    }
    p = p1 + PropsTotal;

    DWORD MethodsTotal = *((DWORD*)p); p += 4;
    tInfo->MethodsNum = *((WORD*)p); p += 2;
    tInfo->Methods = 0;
    
    if (AFlags & INFO_METHODS)
    {
        if (tInfo->MethodsNum) tInfo->Methods = (BYTE*)p;
    }
    return tInfo;
}
//---------------------------------------------------------------------------
MVarInfo* __fastcall MKnowledgeBase::GetVarInfo(int AVarIdx, DWORD AFlags, MVarInfo* vInfo)
{
	if (!Inited) return 0;

    if (AVarIdx == -1) return 0;

    const BYTE* p = GetKBCachePtr(VarOffsets[AVarIdx].Offset, VarOffsets[AVarIdx].Size);

    vInfo->ModuleID = *((WORD*)p); p += 2;
    WORD Len = *((WORD*)p); p += 2;
    vInfo->VarName = String((char*)p); p += Len + 1;
    vInfo->Type = *p; p++;
    Len = *((WORD*)p); p += 2;
    vInfo->TypeDef = String((char*)p); p += Len + 1;

    if (AFlags & INFO_ABSNAME)
    {
        Len = *((WORD*)p); p += 2;
        vInfo->AbsName = String((char*)p);
    }

    return vInfo;
}
//---------------------------------------------------------------------------
MResStrInfo* __fastcall MKnowledgeBase::GetResStrInfo(int AResStrIdx, DWORD AFlags, MResStrInfo* rsInfo)
{
	if (!Inited) return 0;

    if (AResStrIdx == -1) return 0;

    const BYTE* p = GetKBCachePtr(ResStrOffsets[AResStrIdx].Offset, ResStrOffsets[AResStrIdx].Size);

    rsInfo->ModuleID = *((WORD*)p); p += 2;
    WORD Len = *((WORD*)p); p += 2;
    rsInfo->ResStrName = String((char*)p); p += Len + 1;
    Len = *((WORD*)p); p += 2;
    rsInfo->TypeDef = String((char*)p); p += Len + 1;

    if (AFlags & INFO_DUMP)
    {
        //Len = *((WORD*)p); p += 2;
        //rsInfo->AContext = String((char*)p);
    }

    return rsInfo;
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::ScanCode(BYTE* code, DWORD* CodeFlags, DWORD CodeSz, MProcInfo* pInfo)
{
	if (!Inited) return -1;

    if (!pInfo) return -1;

    DWORD DumpSz = pInfo->DumpSz;
    if (!DumpSz || DumpSz >= CodeSz) return -1;
    BYTE* Dump = pInfo->Dump;
    BYTE* Relocs = Dump + DumpSz;
    int i, n;

    //Skip relocs in dump begin
    for (n = 0; n < DumpSz; n++)
    {
    	if (Relocs[n] != 0xFF) break;
    }
    for (int i = n; i < CodeSz - DumpSz + n; i++)
    {
        if (code[i] == Dump[n] && (CodeFlags[i] & cfCode) == 0 && (CodeFlags[i] & cfData) == 0)
        {
            bool found = true;
            for (int k = n; k < DumpSz; k++)
            {
                if ((CodeFlags[i - n + k] & cfCode) != 0 || (CodeFlags[i - n + k] & cfData) != 0)
                {
                    found = false;
                    break;
                }
                if (code[i - n + k] != Dump[k] && Relocs[k] != 0xFF)
                {
                    found = false;
                    break;
                }
            }
            if (found)
            {
                //If "tail" matched (from position i - n), check "head"
                found = true;
                for (int k = 0; k < n; k++)
                {
                    if ((CodeFlags[i - n + k] & cfCode) != 0 || (CodeFlags[i - n + k] & cfData) != 0)
                    {
                        found = false;
                        break;
                    }
                }
                if (found)
                    return i - n;
            }
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
//Fill used modules ids array + module itself
WORD* __fastcall MKnowledgeBase::GetModuleUses(WORD ModuleID)
{
	if (!Inited) return 0;

    if (ModuleID == 0xFFFF) return 0;

    const BYTE* p = GetKBCachePtr(ModuleOffsets[ModuleID].Offset, ModuleOffsets[ModuleID].Size);

    //ID
    p += 2;
    //Name
    WORD len = *((WORD*)p); p += len + 3;
    //Filename
    len = *((WORD*)p); p += len + 3;
    //UsesNum
    WORD UsesNum = *((WORD*)p);
    WORD *uses = new WORD[UsesNum + 2];
    uses[0] = ModuleID;
    int m = 1;
    if (UsesNum)
    {
        p += 2;
        for (int n = 0; n < UsesNum; n++)
        {
            WORD ID = *((WORD*)p); p += 2;
            if (ID != 0xFFFF)
            {
                uses[m] = ID;
                m++;
            }
        }
    }
    uses[m] = 0xFFFF;
    return uses;
}
//---------------------------------------------------------------------------
int __fastcall MKnowledgeBase::GetProcUses(char* ProcName, WORD* uses)
{
	if (!Inited) return 0;

    int     num = 0;

    int L = 0, R = ProcCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = ProcOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);

        int res = stricmp(ProcName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            WORD ModID = *((WORD*)p);
            if (ModID != 0xFFFF)
            {
                uses[num] = ModID;
                num++;
            }
            //Take ModuleIDs in same name block (firstly from right to left)
            for (int N = M - 1; N >= 0; N--)
            {
                ID = ProcOffsets[N].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
                ModID = *((WORD*)p);
                if (ModID != 0xFFFF && uses[num - 1] != ModID)
                {
                    uses[num] = ModID;
                    num++;
                    assert(num < ModuleCount);
                }
            }
            //From ltft to right
            for (int N = M + 1; N < ProcCount; N++)
            {
                ID = ProcOffsets[N].NamId;
                p = GetKBCachePtr(ProcOffsets[ID].Offset, ProcOffsets[ID].Size);
                res = stricmp(ProcName, p + 4);
                if (res) break;
                ModID = *((WORD*)p);
                if (ModID != 0xFFFF && uses[num - 1] != ModID)
                {
                    uses[num] = ModID;
                    num++;
                    assert(num < ModuleCount);
                }
            }
            return num;
        }
        if (L > R) return 0;
    }
}
//---------------------------------------------------------------------------
WORD* __fastcall MKnowledgeBase::GetTypeUses(char* TypeName)
{
	if (!Inited) return 0;

    int     num = 0;
    WORD    *uses = 0;

    int L = 0, R = TypeCount - 1;
    while (1)
    {
        int M = (L + R)/2;
        int ID = TypeOffsets[M].NamId;
        const BYTE* p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
        if (Version >= 2) p += 4; //Add by ZGL

        int res = stricmp(TypeName, p + 4);
        if (res < 0)
            R = M - 1;
        else if (res > 0)
            L = M + 1;
        else
        {
            uses = new WORD[ModuleCount + 1];
            WORD ModID = *((WORD*)p);
            if (ModID != 0xFFFF)
            {
                uses[num] = ModID;
                num++;
            }
            //Take ModuleIDs in same name block (firstly from right to left)
            for (int N = M - 1; N >= 0; N--)
            {
                ID = TypeOffsets[N].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                res = stricmp(TypeName, p + 4);
                if (res) break;
                ModID = *((WORD*)p);
                if (ModID != 0xFFFF && uses[num - 1] != ModID)
                {
                    uses[num] = ModID;
                    num++;
                    assert(num < ModuleCount);
                }
            }
            //From left to right
            for (int N = M + 1; N < TypeCount; N++)
            {
                ID = TypeOffsets[N].NamId;
                p = GetKBCachePtr(TypeOffsets[ID].Offset, TypeOffsets[ID].Size);// + 4;
                if (Version >= 2) p += 4; //Add by ZGL
                res = stricmp(TypeName, p + 4);
                if (res) break;
                ModID = *((WORD*)p);
                if (ModID != 0xFFFF && uses[num - 1] != ModID)
                {
                    uses[num] = ModID;
                    num++;
                    assert(num < ModuleCount);
                }
            }
            //After last element must be word 0xFFFF
            uses[num] = 0xFFFF;
            return uses;
        }
        if (L > R) return 0;
    }
}
//---------------------------------------------------------------------------
WORD* __fastcall MKnowledgeBase::GetConstUses(char* ConstName)
{
	if (!Inited) return 0;

    int         ID, num;
    WORD        ModID, *uses = 0;
    const BYTE  *p;

    int F = 0, L = ConstCount - 1;
    while (F < L)
    {
        int M = (F + L)/2;
        ID = ConstOffsets[M].NamId;
        p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
        if (stricmp(ConstName, p + 4) <= 0)
            L = M;
        else
            F = M + 1;
    }
    ID = ConstOffsets[L].NamId;
    p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
    if (!stricmp(ConstName, p + 4))
    {
        uses = new WORD[ModuleCount + 1]; num = 0;
        ModID = *((WORD*)p);
        if (ModID != 0xFFFF)
        {
            uses[num] = ModID;
            num++;
        }
        //Take ModuleIDs in same name block (firstly from right to left)
        for (int N = L - 1; N >= 0; N--)
        {
            ID = ConstOffsets[N].NamId;
            p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
            if (stricmp(ConstName, p + 4)) break;
            ModID = *((WORD*)p);
            if (ModID != 0xFFFF && uses[num - 1] != ModID)
            {
                uses[num] = ModID;
                num++;
                assert(num < ModuleCount);
            }
        }
        //From left to right
        for (int N = L + 1; N < ConstCount; N++)
        {
            ID = ConstOffsets[N].NamId;
            p = GetKBCachePtr(ConstOffsets[ID].Offset, ConstOffsets[ID].Size);
            if (stricmp(ConstName, p + 4)) break;
            ModID = *((WORD*)p);
            if (ModID != 0xFFFF && uses[num - 1] != ModID)
            {
                uses[num] = ModID;
                num++;
                assert(num < ModuleCount);
            }
        }
        //After last element must be word 0xFFFF
        uses[num] = 0xFFFF;
        return uses;
    }
    return 0;
}
//---------------------------------------------------------------------------
String __fastcall MKnowledgeBase::GetProcPrototype(int ProcIdx)
{
    MProcInfo pInfo;
    if (Inited && GetProcInfo(ProcIdx, INFO_ARGS, &pInfo)) return GetProcPrototype(&pInfo);
    return "";
}
//---------------------------------------------------------------------------
String __fastcall MKnowledgeBase::GetProcPrototype(MProcInfo* pInfo)
{
    String Result = "";

    if (pInfo)
    {
        switch (pInfo->MethodKind)
        {
        case 'C':
            Result += "constructor";
            break;
        case 'D':
            Result += "destructor";
            break;
        case 'F':
            Result += "function";
            break;
        case 'P':
            Result += "procedure";
            break;
        }
        if (Result != "") Result += " ";
        Result += pInfo->ProcName;

        if (pInfo->ArgsNum) Result += "(";

        BYTE *p = pInfo->Args; WORD Len;
        for (int n = 0; n < pInfo->ArgsNum; n++)
        {
            if (n) Result += "; ";
            //Tag
            if (*p == 0x22) Result += "var "; p++;
            //Register
            p += 4;
            //Ndx
            p += 4;
            //Name
            Len = *((WORD*)p); p += 2;
            Result += String((char*)p, Len) + ":"; p += Len + 1;
            //TypeDef
            Len = *((WORD*)p); p += 2;
            Result += String((char*)p, Len); p += Len + 1;
        }
        if (pInfo->ArgsNum) Result += ")";
        if (pInfo->MethodKind == 'F')
        {
            Result += ":" + pInfo->TypeDef;
        }
        Result += ";";
        if (pInfo->CallKind) Result += " ";
        switch (pInfo->CallKind)
        {
        case 1:
            Result += "cdecl";
            break;
        case 2:
            Result += "pascal";
            break;
        case 3:
            Result += "stdcall";
            break;
        case 4:
            Result += "safecall";
            break;
        }
        if (pInfo->CallKind) Result += ";";
    }
    return Result;
}
//---------------------------------------------------------------------------
//as
//return direct pointer to const data of KB
const BYTE* __fastcall MKnowledgeBase::GetKBCachePtr(DWORD Offset, DWORD Size)
{
    assert(Offset >= 0 && Offset < SizeKBFile && (Offset + Size < SizeKBFile));
    return KBCache + Offset;
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::IsUsedProc(int AIdx)
{
    return (UsedProcs[AIdx] == 1);
}
//---------------------------------------------------------------------------
void __fastcall MKnowledgeBase::SetUsedProc(int AIdx)
{
    UsedProcs[AIdx] = 1;
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::GetKBProcInfo(String typeName, MProcInfo* procInfo, int* procIdx)
{
    int     res, idx;

    res = (int)GetProcInfo(typeName.c_str(), INFO_DUMP | INFO_ARGS, procInfo, procIdx);
    if (res && res != -1)
    {
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::GetKBTypeInfo(String typeName, MTypeInfo* typeInfo)
{
    int         idx;
    WORD        *uses;

    uses = GetTypeUses(typeName.c_str());
    idx = GetTypeIdxByModuleIds(uses, typeName.c_str());
    if (uses) delete[] uses;
    if (idx != -1)
    {
        idx = TypeOffsets[idx].NamId;
        if (GetTypeInfo(idx, INFO_FIELDS | INFO_PROPS | INFO_METHODS /*| INFO_DUMP*/, typeInfo))
        {
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall MKnowledgeBase::GetKBPropertyInfo(String className, String propName, MTypeInfo* typeInfo)
{
    int         n, idx, pos;
    BYTE        *p;
    WORD        *uses, Len;
    MTypeInfo   tInfo;
    String      name, type;

    uses = GetTypeUses(className.c_str());
    idx = GetTypeIdxByModuleIds(uses, className.c_str());
    if (uses) delete[] uses;

    if (idx != -1)
    {
        idx = TypeOffsets[idx].NamId;
        if (GetTypeInfo(idx, INFO_PROPS, &tInfo))
        {
            p = tInfo.Props;
            for (n = 0; n < tInfo.PropsNum; n++)
            {
                p++;//Scope
                p += 4;//Index
                p += 4;//DispID
                Len = *((WORD*)p); p += 2;
                name = String((char*)p, Len); p += Len + 1;//Name
                Len = *((WORD*)p); p += 2;
                type = TrimTypeName(String((char*)p, Len)); p += Len + 1;//TypeDef
                Len = *((WORD*)p); p += 2 + Len + 1;//ReadName
                Len = *((WORD*)p); p += 2 + Len + 1;//WriteName
                Len = *((WORD*)p); p += 2 + Len + 1;//StoredName
                
                if (SameText(name, propName))
                {
                    pos = type.LastDelimiter(":.");
                    if (pos) type = type.SubString(pos + 1, type.Length());
                    return GetKBTypeInfo(type, typeInfo);
                }
            }
        }
    }
    return false;
}
//---------------------------------------------------------------------------
String __fastcall MKnowledgeBase::IsPropFunction(String className, String procName)
{
    int         n, idx;
    BYTE        *p;
    WORD        *uses, Len;
    MTypeInfo   tInfo;
    String      pname, type, fname;

    uses = GetTypeUses(className.c_str());
    idx = GetTypeIdxByModuleIds(uses, className.c_str());
    if (uses) delete[] uses;
    if (idx != -1)
    {
        idx = TypeOffsets[idx].NamId;
        if (GetTypeInfo(idx, INFO_PROPS, &tInfo))
        {
            p = tInfo.Props;
            for (n = 0; n < tInfo.PropsNum; n++)
            {
                p++;//Scope
                p += 4;//Index
                p += 4;//DispID
                Len = *((WORD*)p); p += 2;
                pname = String((char*)p, Len); p += Len + 1;//Name
                Len = *((WORD*)p); p += 2;
                type = TrimTypeName(String((char*)p, Len)); p += Len + 1;//TypeDef
                Len = *((WORD*)p); p += 2;
                fname = TrimTypeName(String((char*)p, Len)); p += Len + 1;//ReadName
                if (SameText(procName, fname))
                    return pname;
                Len = *((WORD*)p); p += 2;
                fname = TrimTypeName(String((char*)p, Len)); p += Len + 1;//WriteName
                if (SameText(procName, fname))
                    return pname;
                Len = *((WORD*)p); p += 2;
                fname = TrimTypeName(String((char*)p, Len)); p += Len + 1;//StoredName
                if (SameText(procName, fname))
                    return pname;
            }
        }
    }
    return "";
}
//---------------------------------------------------------------------------

