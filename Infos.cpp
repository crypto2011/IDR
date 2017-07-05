//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <SyncObjs.hpp>
#include "Infos.h"
#include "Main.h"
#include "Misc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
extern  MDisasm     Disasm;
extern  int         MaxBufLen;
extern  DWORD       CodeBase;
extern  DWORD       *Flags;
extern	PInfoRec	*Infos;
extern  BYTE        *Code;
extern  TCriticalSection* CrtSection;
extern  MKnowledgeBase  KnowledgeBase;
extern  char        StringBuf[MAXSTRBUFFER];
//as some statistics for memory leaks detection (remove it when fixed)
long stat_InfosOverride = 0;
//---------------------------------------------------------------------------
int __fastcall FieldsCmpFunction(void *item1, void *item2)
{
    PFIELDINFO rec1 = (PFIELDINFO)item1;
    PFIELDINFO rec2 = (PFIELDINFO)item2;
    if (rec1->Offset > rec2->Offset) return 1;
    if (rec1->Offset < rec2->Offset) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall LocalsCmpFunction(void *item1, void *item2)
{
    PLOCALINFO rec1 = (PLOCALINFO)item1;
    PLOCALINFO rec2 = (PLOCALINFO)item2;

    if (rec1->Ofs > rec2->Ofs) return 1;
    if (rec1->Ofs < rec2->Ofs) return -1;
    return 0;
}
//---------------------------------------------------------------------------
__fastcall InfoVmtInfo::InfoVmtInfo()
{
	interfaces = 0;
    fields = 0;
    methods = 0;
}
//---------------------------------------------------------------------------
__fastcall InfoVmtInfo::~InfoVmtInfo()
{
	if (interfaces) delete interfaces;
    CleanupList<FIELDINFO>(fields);
    CleanupList<MethodRec>(methods);
}
//---------------------------------------------------------------------------
void __fastcall InfoVmtInfo::AddInterface(String Value)
{
    if (!interfaces) interfaces = new TStringList;
    interfaces->Add(Value);
}
//---------------------------------------------------------------------------
PFIELDINFO __fastcall InfoVmtInfo::AddField(DWORD ProcAdr, int ProcOfs, BYTE Scope, int Offset, int Case, String Name, String Type)
{
	PFIELDINFO	fInfo = 0;

    if (!fields) fields = new TList;
    if (!fields->Count)
    {
        fInfo = new FIELDINFO;
        fInfo->Scope = Scope;
        fInfo->Offset = Offset;
        fInfo->Case = Case;
        fInfo->xrefs = 0;
        fInfo->Name = Name;
        fInfo->Type = Type;
        fields->Add((void*)fInfo);
        return fInfo;
    }

    int F = 0, L = fields->Count - 1;
    while (F < L)
    {
        int M = (F + L)/2;
        fInfo = (PFIELDINFO)fields->Items[M];
        if (Offset <= fInfo->Offset)
            L = M;
        else
            F = M + 1;
    }
    fInfo = (PFIELDINFO)fields->Items[L];
    if (fInfo->Offset != Offset)
    {
        fInfo = new FIELDINFO;
        fInfo->Scope = Scope;
        fInfo->Offset = Offset;
        fInfo->Case = Case;
        fInfo->xrefs = 0;
        fInfo->Name = Name;
        fInfo->Type = Type;
        fields->Add((void*)fInfo);
        fields->Sort(FieldsCmpFunction);
    }
    else
    {
        if (fInfo->Name == "")
            fInfo->Name = Name;
        if (fInfo->Type == "")
            fInfo->Type = Type;
    }
    return fInfo;
}
//---------------------------------------------------------------------------
void __fastcall InfoVmtInfo::RemoveField(int Offset)
{
    PFIELDINFO  fInfo;

    if (!fields || !fields->Count) return;
    int F = 0, L = fields->Count - 1;
    while (F < L)
    {
        int M = (F + L)/2;
        fInfo = (PFIELDINFO)fields->Items[M];
        if (Offset <= fInfo->Offset)
            L = M;
        else
            F = M + 1;
    }

    fInfo = (PFIELDINFO)fields->Items[L];
    if (fInfo->Offset == Offset) fields->Delete(L);
}
//---------------------------------------------------------------------------
bool __fastcall InfoVmtInfo::AddMethod(bool Abstract, char Kind, int Id, DWORD Address, String Name)
{
	PMethodRec	recM;

    if (!methods) methods = new TList;

    for (int n = 0; n < methods->Count; n++)
    {
        recM = (PMethodRec)methods->Items[n];
        if (Kind == 'A' || Kind == 'V')
        {
            if (recM->kind == Kind && recM->id == Id)
            {
                if (recM->name == ""&& Name != "") recM->name = Name;
                return false;
            }
        }
        else
        {
            if (recM->kind == Kind && recM->address == Address)
            {
                if (recM->name == "" && Name != "") recM->name = Name;
                return false;
            }
        }
    }

    recM = new MethodRec;
    recM->abstract = Abstract;
    recM->kind = Kind;
    recM->id = Id;
    recM->address = Address;
    recM->name = Name;
    methods->Add((void*)recM);
    return true;
}
//---------------------------------------------------------------------------
_fastcall InfoProcInfo::InfoProcInfo()
{
	flags = 0;
    bpBase = 0;
    retBytes = 0;
    procSize = 0;
    stackSize = 0;
    args = 0;
    locals = 0;
}
//---------------------------------------------------------------------------
__fastcall InfoProcInfo::~InfoProcInfo()
{
    CleanupList<ARGINFO>(args);
    CleanupList<LOCALINFO>(locals);
}
//---------------------------------------------------------------------------
PARGINFO __fastcall InfoProcInfo::AddArg(PARGINFO aInfo)
{
    return AddArg(aInfo->Tag, aInfo->Ndx, aInfo->Size, aInfo->Name, aInfo->TypeDef);
}
//---------------------------------------------------------------------------
PARGINFO __fastcall InfoProcInfo::AddArg(BYTE Tag, int Ofs, int Size, String Name, String TypeDef)
{
    PARGINFO 	argInfo;
    if (!args) args = new TList;
    if (!args->Count)
    {
        argInfo = new ARGINFO;
        argInfo->Tag = Tag;
        argInfo->Register = false;
        argInfo->Ndx = Ofs;
        argInfo->Size = Size;
        argInfo->Name = Name;
        argInfo->TypeDef = TypeDef;
        args->Add((void*)argInfo);
        return argInfo;
    }

    int F = 0;
    argInfo = (PARGINFO)args->Items[F];
    if (Ofs < argInfo->Ndx)
    {
        argInfo = new ARGINFO;
        argInfo->Tag = Tag;
        argInfo->Register = false;
        argInfo->Ndx = Ofs;
        argInfo->Size = Size;
        argInfo->Name = Name;
        argInfo->TypeDef = TypeDef;
        args->Insert(F, (void*)argInfo);
        return argInfo;
    }
    int L = args->Count - 1;
    argInfo = (PARGINFO)args->Items[L];
    if (Ofs > argInfo->Ndx)
    {
        argInfo = new ARGINFO;
        argInfo->Tag = Tag;
        argInfo->Register = false;
        argInfo->Ndx = Ofs;
        argInfo->Size = Size;
        argInfo->Name = Name;
        argInfo->TypeDef = TypeDef;
        args->Add((void*)argInfo);
        return argInfo;
    }
    while (F < L)
    {
        int M = (F + L)/2;
        argInfo = (PARGINFO)args->Items[M];
        if (Ofs <= argInfo->Ndx)
            L = M;
        else
            F = M + 1;
    }
    argInfo = (PARGINFO)args->Items[L];
    if (argInfo->Ndx != Ofs)
    {
        argInfo = new ARGINFO;
        argInfo->Tag = Tag;
        argInfo->Register = false;
        argInfo->Ndx = Ofs;
        argInfo->Size = Size;
        argInfo->Name = Name;
        argInfo->TypeDef = TypeDef;
        args->Insert(L, (void*)argInfo);
        return argInfo;
    }

    if (argInfo->Name == "") argInfo->Name = Name;
    if (argInfo->TypeDef == "") argInfo->TypeDef = TypeDef;
    if (!argInfo->Tag) argInfo->Tag = Tag;
    return argInfo;
}
//---------------------------------------------------------------------------
String __fastcall InfoProcInfo::AddArgsFromDeclaration(char* Decl, int from, int callKind)
{
    bool    fColon;
    char    *p, *pp, *cp, c, sc;
    int     ss, num;
    ARGINFO argInfo;
    char    _Name[256];
    char    _Type[256];
    char    _Size[256];

    p = strchr(Decl, '(');
    if (p)
    {
        p++; pp = p; num = 0; fColon = false;
        while (1)
        {
            c = *pp;
            if (c == ')') break;
            if (c == ':' && !fColon)
            {
                *pp = ' ';
                num++;
                fColon = true;
            }
            if (c == ';') fColon = false;
            pp++;
        }
        if (num)
        {
            ss = bpBase;
            for (int arg = from;;arg++)
            {
                _Name[0] = _Type[0] = 0;
                sc = ';'; pp = strchr(p, sc);
                if (!pp)
                {
                    sc = ')';
                    pp = strchr(p, sc);
                }
                *pp = 0;
                //Tag
                argInfo.Tag = 0x21;
                while (*p == ' ') p++;
                sscanf(p, "%s", _Name);
                if (!stricmp(_Name, "var"))
                {
                    argInfo.Tag = 0x22;
                    p += strlen(_Name);
                    while (*p == ' ') p++;
                    //Name
                    sscanf(p, "%s", _Name);
                }

                //Insert by ZGL
                else if (!stricmp(_Name, "const"))
                {
                    argInfo.Tag = 0x23;
                    p += strlen(_Name);
                    while (*p == ' ') p++;
                    //Name
                    sscanf(p, "%s", _Name);
                }
                ////////////////

                else if (!stricmp(_Name, "val"))
                {
                    p += strlen(_Name);
                    while (*p == ' ') p++;
                    //Name
                    sscanf(p, "%s", _Name);
                }
                p += strlen(_Name);
                while (*p == ' ') p++;
                //Type
                strcpy(_Type, p);
                p += strlen(_Type);
                while (*p == ' ') p++;
                argInfo.Size = 4;
                cp = strchr(_Type, ':');
                if (cp)
                {
                    sscanf(cp + 1, "%d", &argInfo.Size);
                    *cp = 0;
                }
                if (callKind == 0)//fastcall
                {
                    if (arg < 3 && argInfo.Size == 4)
                        argInfo.Ndx = arg;
                    else
                    {
                        argInfo.Ndx = ss;
                        ss += argInfo.Size;
                    }
                }
                else
                {
                    argInfo.Ndx = ss;
                    ss += argInfo.Size;
                }
                if (_Name[0] == '?' && _Name[1] == 0)
                    argInfo.Name = "";
                else
                    argInfo.Name = String(_Name).Trim();

                if (_Type[0] == '?' && _Type[1] == 0)
                    argInfo.TypeDef = "";
                else
                    argInfo.TypeDef = TrimTypeName(String(_Type));

                AddArg(&argInfo);
                *pp = ' ';
                p = pp + 1;
                if (sc == ')') break;
            }
        }
    }
    else
    {
        p = Decl;
    }
    p = strchr(p, ':');
    if (p)
    {
        p++;
        pp = strchr(p, ';');
        if (pp)
        {
            *pp = 0;
            strcpy(_Name, p);
            *pp = ';';
        }
        else
            strcpy(_Name, p);
        return String(_Name).Trim();
    }
    return "";
}
//---------------------------------------------------------------------------
PARGINFO __fastcall InfoProcInfo::GetArg(int n)
{
    if (args && n >= 0 && n < args->Count) return (PARGINFO)args->Items[n];
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall InfoProcInfo::DeleteArg(int n)
{
    if (args && n >= 0 && n < args->Count) args->Delete(n);
}
//---------------------------------------------------------------------------
void __fastcall InfoProcInfo::DeleteArgs()
{
    if (args)
    {
        for (int n = 0; n < args->Count; n++) args->Delete(n);
        args->Clear();
    }
}
//---------------------------------------------------------------------------
PLOCALINFO __fastcall InfoProcInfo::AddLocal(int Ofs, int Size, String Name, String TypeDef)
{
    PLOCALINFO  locInfo;

    if (!locals) locals = new TList;
    if (!locals->Count)
    {
        locInfo = new LOCALINFO;
        locInfo->Ofs = Ofs;
        locInfo->Size = Size;
        locInfo->Name = Name;
        locInfo->TypeDef = TypeDef;
        locals->Add((void*)locInfo);
        return locInfo;
    }

    int F = 0;
    locInfo = (PLOCALINFO)locals->Items[F];
    if (-Ofs < -locInfo->Ofs)
    {
        locInfo = new LOCALINFO;
        locInfo->Ofs = Ofs;
        locInfo->Size = Size;
        locInfo->Name = Name;
        locInfo->TypeDef = TypeDef;
        locals->Insert(F, (void*)locInfo);
        return locInfo;
    }
    int L = locals->Count - 1;
    locInfo = (PLOCALINFO)locals->Items[L];
    if (-Ofs > -locInfo->Ofs)
    {
        locInfo = new LOCALINFO;
        locInfo->Ofs = Ofs;
        locInfo->Size = Size;
        locInfo->Name = Name;
        locInfo->TypeDef = TypeDef;
        locals->Add((void*)locInfo);
        return locInfo;
    }
    while (F < L)
    {
        int M = (F + L)/2;
        locInfo = (PLOCALINFO)locals->Items[M];
        if (-Ofs <= -locInfo->Ofs)
            L = M;
        else
            F = M + 1;
    }
    locInfo = (PLOCALINFO)locals->Items[L];
    if (locInfo->Ofs != Ofs)
    {
        locInfo = new LOCALINFO;
        locInfo->Ofs = Ofs;
        locInfo->Size = Size;
        locInfo->Name = Name;
        locInfo->TypeDef = TypeDef;
        locals->Insert(L, (void*)locInfo);
    }
    else
    {
        if (Name != "") locInfo->Name = Name;
        if (TypeDef != "") locInfo->TypeDef = TypeDef;
    }
    return locInfo;
}
//---------------------------------------------------------------------------
PLOCALINFO __fastcall InfoProcInfo::GetLocal(int Ofs)
{
    if (locals)
    {
        for (int n = 0; n < locals->Count; n++)
        {
            PLOCALINFO  locInfo = (PLOCALINFO)locals->Items[n];
            if (locInfo->Ofs == Ofs) return locInfo;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
PLOCALINFO __fastcall InfoProcInfo::GetLocal(String Name)
{
    if (locals)
    {
        for (int n = 0; n < locals->Count; n++)
        {
            PLOCALINFO  locInfo = (PLOCALINFO)locals->Items[n];
            if (SameText(locInfo->Name, Name)) return locInfo;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall InfoProcInfo::DeleteLocal(int n)
{
    if (locals && n >= 0 && n < locals->Count) locals->Delete(n);
}
//---------------------------------------------------------------------------
void __fastcall InfoProcInfo::DeleteLocals()
{
    if (locals)
    {
        for (int n = 0; n < locals->Count; n++) locals->Delete(n);
        locals->Clear();
    }
}
//---------------------------------------------------------------------------
void __fastcall InfoProcInfo::SetLocalType(int Ofs, String TypeDef)
{
    PLOCALINFO locInfo = GetLocal(Ofs);
    if (locInfo)
    {
        String  fname = locInfo->Name;
        int pos = fname.Pos(".");
        if (pos)
            fname = fname.SubString(1, pos - 1);
        locInfo->TypeDef = TypeDef;
        int size;
        if (TypeDef != "" && GetTypeKind(TypeDef, &size) == ikRecord)
        {
            String recFileName = FMain_11011981->WrkDir + "\\types.idr";
            FILE* recFile = fopen(recFileName.c_str(), "rt");
            if (recFile)
            {
                while (1)
                {
                    if (!fgets(StringBuf, 1024, recFile)) break;
                    String str = String(StringBuf);
                    if (str.Pos(TypeDef + "=") == 1)
                    {
                        while (1)
                        {
                            if (!fgets(StringBuf, 1024, recFile)) break;
                            str = String(StringBuf);
                            if (str.Pos("end;")) break;
                            if (str.Pos("//"))
                            {
                                int ofs = StrGetRecordFieldOffset(str);
                                String name = StrGetRecordFieldName(str);
                                String type = StrGetRecordFieldType(str);
                                if (ofs >= 0)
                                    AddLocal(ofs + Ofs, 1, fname + "." + name, type);
                            }
                        }
                    }
                }
                fclose(recFile);
            }
            while (1)
            {
                //KB
                WORD* uses = KnowledgeBase.GetTypeUses(TypeDef.c_str());
                int idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, TypeDef.c_str());
                if (uses) delete[] uses;

                if (idx == -1) break;

                idx = KnowledgeBase.TypeOffsets[idx].NamId;
                MTypeInfo tInfo;
                if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS, &tInfo))
                {
                    if (tInfo.FieldsNum)
                    {
                        char* p = tInfo.Fields;
                        for (int k = 0; k < tInfo.FieldsNum; k++)
                        {
                            //Scope
                            p++;
                            int elofs = *((int*)p); p += 4;
                            p += 4;//case
                            //Name
                            int len = *((WORD*)p); p += 2;
                            String name = String((char*)p, len); p += len + 1;
                            //Type
                            len = *((WORD*)p); p += 2;
                            String type = TrimTypeName(String((char*)p, len)); p += len + 1;
                            AddLocal(Ofs + elofs, 1, fname + "." + name, type);
                        }
                        break;
                    }
                    if (tInfo.Decl != "")
                    {
                        TypeDef = tInfo.Decl;
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
__fastcall InfoRec::InfoRec(int APos, BYTE AKind)
{
    counter = 0;
    kind = AKind;
    kbIdx = -1;
    name = "";
    type = "";
    picode = 0;
    xrefs = 0;
    rsInfo = 0;
    vmtInfo = 0;
    procInfo = 0;

    if (kind == ikResString)
    	rsInfo = new InfoResStringInfo;
    else if (kind == ikVMT)
    	vmtInfo = new InfoVmtInfo;
    else if (kind >= ikRefine && kind <= ikFunc)
    	procInfo = new InfoProcInfo;

    if (APos >= 0)
    {
        if (Infos[APos])
        {
            //as: if we here - memory leak then!
            ++stat_InfosOverride;
        }
        Infos[APos] = this;
    }
}
//---------------------------------------------------------------------------
__fastcall InfoRec::~InfoRec()
{
    if (picode) delete picode;

    CleanupList<XrefRec>(xrefs);

    if (kind == ikResString)
    {
        delete rsInfo; rsInfo = 0;
    }
    else if (kind == ikVMT)
    {
        delete vmtInfo; vmtInfo = 0;
    }
    else if (kind >= ikRefine && kind <= ikFunc)
    {
        delete procInfo; procInfo = 0;
    }
}
//---------------------------------------------------------------------------
bool __fastcall InfoRec::HasName()
{
    return (name != "");
}
//---------------------------------------------------------------------------
String __fastcall InfoRec::GetName()
{
    return name;
}
//---------------------------------------------------------------------------
int __fastcall InfoRec::GetNameLength()
{
    return name.Length();
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::SetName(String AValue)
{
    CrtSection->Enter();
    name = AValue;
    if (ExtractClassName(name) != "" && (kind >= ikRefine && kind <= ikFunc) && procInfo) procInfo->flags |= PF_METHOD;
    CrtSection->Leave();
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::ConcatName(String AValue)
{
    name += AValue;
}
//---------------------------------------------------------------------------
bool __fastcall InfoRec::SameName(String AValue)
{
    return SameText(name, AValue);
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::AddXref(char Type, DWORD Adr, int Offset)
{
    PXrefRec 	recX;

	if (!xrefs) xrefs = new TList;
    if (!xrefs->Count)
    {
        recX = new XrefRec;
        recX->type = Type;
        recX->adr = Adr;
        recX->offset = Offset;
        xrefs->Add((void*)recX);
        return;
    }

    int F = 0;
    recX = (PXrefRec)xrefs->Items[F];
    if (Adr + Offset < recX->adr + recX->offset)
    {
        recX = new XrefRec;
        recX->type = Type;
        recX->adr = Adr;
        recX->offset = Offset;
        xrefs->Insert(F, (void*)recX);
        return;
    }
    int L = xrefs->Count - 1;
    recX = (PXrefRec)xrefs->Items[L];
    if (Adr + Offset > recX->adr + recX->offset)
    {
        recX = new XrefRec;
        recX->type = Type;
        recX->adr = Adr;
        recX->offset = Offset;
        xrefs->Add((void*)recX);
        return;
    }
    while (F < L)
    {
        int M = (F + L)/2;
        recX = (PXrefRec)xrefs->Items[M];
        if (Adr + Offset <= recX->adr + recX->offset)
            L = M;
        else
            F = M + 1;
    }
    recX = (PXrefRec)xrefs->Items[L];
    if (recX->adr + recX->offset != Adr + Offset)
    {
        recX = new XrefRec;
        recX->type = Type;
        recX->adr = Adr;
        recX->offset = Offset;
        xrefs->Insert(L, (void*)recX);
    }
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::DeleteXref(DWORD Adr)
{
    for (int n = 0; n < xrefs->Count; n++)
    {
        PXrefRec recX = (PXrefRec)xrefs->Items[n];
        if (Adr == recX->adr + recX->offset)
        {
            xrefs->Delete(n);
            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::ScanUpItemAndAddRef(int fromPos, DWORD itemAdr, char refType, DWORD refAdr)
{
    while (fromPos >= 0)
    {
        fromPos--;
        if (IsFlagSet(cfProcStart, fromPos))
            break;
        if (IsFlagSet(cfInstruction, fromPos))
        {
            DISINFO _disInfo;
            Disasm.Disassemble(Code + fromPos, (__int64)Pos2Adr(fromPos), &_disInfo, 0);
            if (_disInfo.Immediate == itemAdr || _disInfo.Offset == itemAdr)
            {
                AddXref(refType, refAdr, Pos2Adr(fromPos) - refAdr);
                break;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::Save(TStream* outs)
{
    int     m, xm, num, xnum, len;

    //kbIdx
    outs->Write(&kbIdx, sizeof(kbIdx));
    //name
    len = name.Length(); if (len > MaxBufLen) MaxBufLen = len;
    outs->Write(&len, sizeof(len));
    outs->Write(name.c_str(), len);
    //type
    len = type.Length(); if (len > MaxBufLen) MaxBufLen = len;
    outs->Write(&len, sizeof(len));
    outs->Write(type.c_str(), len);
    //picode
    outs->Write(&picode, sizeof(picode));
    if (picode)
    {
    	//Op
        outs->Write(&picode->Op, sizeof(picode->Op));
        //Ofs
        if (picode->Op == OP_CALL)
        	outs->Write(&picode->Ofs.Address, sizeof(picode->Ofs.Address));
        else
        	outs->Write(&picode->Ofs.Offset, sizeof(picode->Ofs.Offset));
        //Name
        len = picode->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
        outs->Write(&len, sizeof(len));
        outs->Write(picode->Name.c_str(), len);
    }
    //xrefs
    if (xrefs && xrefs->Count)
    	num = xrefs->Count;
    else
    	num = 0;
    outs->Write(&num, sizeof(num));
    for (m = 0; m < num; m++)
    {
        PXrefRec recX = (PXrefRec)xrefs->Items[m];
        //type
        outs->Write(&recX->type, sizeof(recX->type));
        //adr
        outs->Write(&recX->adr, sizeof(recX->adr));
        //offset
        outs->Write(&recX->offset, sizeof(recX->offset));
    }
    if (kind == ikResString)
    {
        //value
        len = rsInfo->value.Length(); if (len > MaxBufLen) MaxBufLen = len;
        outs->Write(&len, sizeof(len));
        outs->Write(rsInfo->value.c_str(), len);
    }
    else if (kind == ikVMT)
    {
    	//interfaces
        if (vmtInfo->interfaces && vmtInfo->interfaces->Count)
        	num = vmtInfo->interfaces->Count;
        else
        	num = 0;
        outs->Write(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            len = vmtInfo->interfaces->Strings[m].Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(vmtInfo->interfaces->Strings[m].c_str(), len);
        }
    	//fields
        if (vmtInfo->fields && vmtInfo->fields->Count)
        	num = vmtInfo->fields->Count;
        else
        	num = 0;
        outs->Write(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            PFIELDINFO fInfo = (PFIELDINFO)vmtInfo->fields->Items[m];
            //Scope
            outs->Write(&fInfo->Scope, sizeof(fInfo->Scope));
            //Offset
            outs->Write(&fInfo->Offset, sizeof(fInfo->Offset));
            //Case
            outs->Write(&fInfo->Case, sizeof(fInfo->Case));
            //Name
            len = fInfo->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(fInfo->Name.c_str(), len);
            //Type
            len = fInfo->Type.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(fInfo->Type.c_str(), len);
            //xrefs
            if (fInfo->xrefs && fInfo->xrefs->Count)
                xnum = fInfo->xrefs->Count;
            else
                xnum = 0;
            outs->Write(&xnum, sizeof(xnum));
            for (xm = 0; xm < xnum; xm++)
            {
                PXrefRec recX = (PXrefRec)fInfo->xrefs->Items[xm];
                //type
                outs->Write(&recX->type, sizeof(recX->type));
                //adr
                outs->Write(&recX->adr, sizeof(recX->adr));
                //offset
                outs->Write(&recX->offset, sizeof(recX->offset));
            }
        }
    	//methods
        if (vmtInfo->methods && vmtInfo->methods->Count)
        	num = vmtInfo->methods->Count;
        else
        	num = 0;
        outs->Write(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            PMethodRec recM = (PMethodRec)vmtInfo->methods->Items[m];
            outs->Write(&recM->abstract, sizeof(recM->abstract));
            outs->Write(&recM->kind, sizeof(recM->kind));
            outs->Write(&recM->id, sizeof(recM->id));
            outs->Write(&recM->address, sizeof(recM->address));
            len = recM->name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(recM->name.c_str(), len);
        }
    }
    else if (kind >= ikRefine && kind <= ikFunc)
    {
        //flags
        outs->Write(&procInfo->flags, sizeof(procInfo->flags));
        //bpBase
        outs->Write(&procInfo->bpBase, sizeof(procInfo->bpBase));
        //retBytes
        outs->Write(&procInfo->retBytes, sizeof(procInfo->retBytes));
        //procSize
        outs->Write(&procInfo->procSize, sizeof(procInfo->procSize));
        //stackSize
        outs->Write(&procInfo->stackSize, sizeof(procInfo->stackSize));
    	//args
        if (procInfo->args && procInfo->args->Count)
        	num = procInfo->args->Count;
        else
        	num = 0;
        outs->Write(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            PARGINFO argInfo = (PARGINFO)procInfo->args->Items[m];
            //Tag
            outs->Write(&argInfo->Tag, sizeof(argInfo->Tag));
            //Register
            outs->Write(&argInfo->Register, sizeof(argInfo->Register));
            //Ndx
            outs->Write(&argInfo->Ndx, sizeof(argInfo->Ndx));
            //Size
            outs->Write(&argInfo->Size, sizeof(argInfo->Size));
            //Name
            len = argInfo->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(argInfo->Name.c_str(), len);
            //TypeDef
            len = argInfo->TypeDef.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(argInfo->TypeDef.c_str(), len);
        }
        //locals
        if (procInfo->locals && procInfo->locals->Count)
        	num = procInfo->locals->Count;
        else
        	num = 0;
        outs->Write(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            PLOCALINFO locInfo = (PLOCALINFO)procInfo->locals->Items[m];
            //Ofs
            outs->Write(&locInfo->Ofs, sizeof(locInfo->Ofs));
            //Size
            outs->Write(&locInfo->Size, sizeof(locInfo->Size));
            //Name
            len = locInfo->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(locInfo->Name.c_str(), len);
            //TypeDef
            len = locInfo->TypeDef.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outs->Write(&len, sizeof(len));
            outs->Write(locInfo->TypeDef.c_str(), len);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall InfoRec::Load(TStream* ins, char* buf)
{
    int         m, xm, num, xnum, len, xrefAdr, pxrefAdr;
    XrefRec     recX1;

    //kbIdx
    ins->Read(&kbIdx, sizeof(kbIdx));
    //name
    ins->Read(&len, sizeof(len));
    ins->Read(buf, len);
    name = String(buf, len);
    //type
    ins->Read(&len, sizeof(len));
    ins->Read(buf, len);
    type = String(buf, len);
    //picode
    ins->Read(&picode, sizeof(picode));
    if (picode)
    {
    	picode = new PICODE;
        //Op
        ins->Read(&picode->Op, sizeof(picode->Op));
        //Ofs
        if (picode->Op == OP_CALL)
        	ins->Read(&picode->Ofs.Address, sizeof(picode->Ofs.Address));
        else
        	ins->Read(&picode->Ofs.Offset, sizeof(picode->Ofs.Offset));
        //Name
        ins->Read(&len, sizeof(len));
        ins->Read(buf, len);
        picode->Name = String(buf, len);
    }
    //xrefs
    ins->Read(&num, sizeof(num));
    if (num) xrefs = new TList;
    pxrefAdr = 0;
    for (m = 0; m < num; m++)
    {
        //type
        ins->Read(&recX1.type, sizeof(recX1.type));
        //adr
        ins->Read(&recX1.adr, sizeof(recX1.adr));
        //offset
        ins->Read(&recX1.offset, sizeof(recX1.offset));
        xrefAdr = recX1.adr + recX1.offset;
        if (!pxrefAdr || pxrefAdr != xrefAdr)   //clear duplicates
        {
        	PXrefRec recX = new XrefRec;
            recX->type = recX1.type;
            recX->adr = recX1.adr;
            recX->offset = recX1.offset;
            xrefs->Add((void*)recX);
            pxrefAdr = xrefAdr;
        }
    }
    if (kind == ikResString)
    {
    	//value
        ins->Read(&len, sizeof(len));
        ins->Read(buf, len);
        rsInfo->value = String(buf, len);
    }
    else if (kind == ikVMT)
    {
    	//interfaces
        ins->Read(&num, sizeof(num));
        if (num) vmtInfo->interfaces = new TStringList;
        for (m = 0; m < num; m++)
        {
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            vmtInfo->interfaces->Add(String(buf, len));
        }
    	//fields
        ins->Read(&num, sizeof(num));
        if (num) vmtInfo->fields = new TList;
        for (m = 0; m < num; m++)
        {
        	PFIELDINFO fInfo = new FIELDINFO;
            //Scope
            ins->Read(&fInfo->Scope, sizeof(fInfo->Scope));
            //Offset
            ins->Read(&fInfo->Offset, sizeof(fInfo->Offset));
            //Case
            ins->Read(&fInfo->Case, sizeof(fInfo->Case));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            fInfo->Name = String(buf, len);
            //Type
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            fInfo->Type = String(buf, len);
            //xrefs
            ins->Read(&xnum, sizeof(xnum));
            if (xnum)
            	fInfo->xrefs = new TList;
            else
            	fInfo->xrefs = 0;
            for (xm = 0; xm < xnum; xm++)
            {
                PXrefRec recX = new XrefRec;
                //type
                ins->Read(&recX->type, sizeof(recX->type));
                //adr
                ins->Read(&recX->adr, sizeof(recX->adr));
                //offset
                ins->Read(&recX->offset, sizeof(recX->offset));
                fInfo->xrefs->Add((void*)recX);
            }
            vmtInfo->fields->Add((void*)fInfo);
        }
    	//methods
        ins->Read(&num, sizeof(num));
        if (num) vmtInfo->methods = new TList;
        for (m = 0; m < num; m++)
        {
        	PMethodRec recM = new MethodRec;
            ins->Read(&recM->abstract, sizeof(recM->abstract));
            ins->Read(&recM->kind, sizeof(recM->kind));
            ins->Read(&recM->id, sizeof(recM->id));
            ins->Read(&recM->address, sizeof(recM->address));
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            recM->name = String(buf, len);
            vmtInfo->methods->Add((void*)recM);
        }
    }
    else if (kind >= ikRefine && kind <= ikFunc)
    {
        //flags
        ins->Read(&procInfo->flags, sizeof(procInfo->flags));
        //bpBase
        ins->Read(&procInfo->bpBase, sizeof(procInfo->bpBase));
        //retBytes
        ins->Read(&procInfo->retBytes, sizeof(procInfo->retBytes));
        //procSize
        ins->Read(&procInfo->procSize, sizeof(procInfo->procSize));
        //stackSize
        ins->Read(&procInfo->stackSize, sizeof(procInfo->stackSize));
   		//args
        ins->Read(&num, sizeof(num));
        if (num) procInfo->args = new TList;
        for (m = 0; m < num; m++)
        {
        	PARGINFO argInfo = new ARGINFO;
            //Tag
            ins->Read(&argInfo->Tag, sizeof(argInfo->Tag));
            //Register
            ins->Read(&argInfo->Register, sizeof(argInfo->Register));
            //Ndx
            ins->Read(&argInfo->Ndx, sizeof(argInfo->Ndx));
            //Size
            ins->Read(&argInfo->Size, sizeof(argInfo->Size));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            argInfo->Name = String(buf, len);
            //TypeDef
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            argInfo->TypeDef = TrimTypeName(String(buf, len));
            procInfo->args->Add((void*)argInfo);
        }
    //locals
        ins->Read(&num, sizeof(num));
        if (num) procInfo->locals = new TList;
        for (m = 0; m < num; m++)
        {
        	PLOCALINFO locInfo = new LOCALINFO;
            //Ofs
            ins->Read(&locInfo->Ofs, sizeof(locInfo->Ofs));
            //Size
            ins->Read(&locInfo->Size, sizeof(locInfo->Size));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            locInfo->Name = String(buf, len);
            //TypeDef
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            locInfo->TypeDef = TrimTypeName(String(buf, len));
            procInfo->locals->Add((void*)locInfo);
        }
    }
}
//---------------------------------------------------------------------------
/*
void __fastcall InfoRec::Skip(TStream* ins, char* buf, BYTE asKind)
{
    DWORD       dummy;
    int         m, xm, len, num, xnum;
    PICODE      picode;
    XrefRec     recX;
    FIELDINFO   fInfo;
    MethodRec   recM;
    ARGINFO     argInfo;
    LOCALINFO   locInfo;

    //kbIdx
    ins->Read(&dummy, sizeof(kbIdx));
    //name
    ins->Read(&len, sizeof(len));
    ins->Read(buf, len);
    //type
    ins->Read(&len, sizeof(len));
    ins->Read(buf, len);
    //picode
    ins->Read(&dummy, sizeof(dummy));
    if (dummy)
    {
        //Op
        ins->Read(&picode.Op, sizeof(picode.Op));
        //Ofs
        if (dummy == OP_CALL)
        	ins->Read(&picode.Ofs.Address, sizeof(picode.Ofs.Address));
        else
        	ins->Read(&picode.Ofs.Offset, sizeof(picode.Ofs.Offset));
        //Name
        ins->Read(&len, sizeof(len));
        ins->Read(buf, len);
    }
    //xrefs
    ins->Read(&num, sizeof(num));
    for (m = 0; m < num; m++)
    {
        //type
        ins->Read(&recX.type, sizeof(recX.type));
        //adr
        ins->Read(&recX.adr, sizeof(recX.adr));
        //offset
        ins->Read(&recX.offset, sizeof(recX.offset));
    }
    if (asKind == ikResString)
    {
    	//value
        ins->Read(&len, sizeof(len));
        ins->Read(buf, len);
    }
    else if (asKind == ikVMT)
    {
    	//interfaces
        ins->Read(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
        }
    	//fields
        ins->Read(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            //Scope
            ins->Read(&fInfo.Scope, sizeof(fInfo.Scope));
            //Offset
            ins->Read(&fInfo.Offset, sizeof(fInfo.Offset));
            //Case
            ins->Read(&fInfo.Case, sizeof(fInfo.Case));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            //Type
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            //xrefs
            ins->Read(&xnum, sizeof(xnum));
            for (xm = 0; xm < xnum; xm++)
            {
                //type
                ins->Read(&recX.type, sizeof(recX.type));
                //adr
                ins->Read(&recX.adr, sizeof(recX.adr));
                //offset
                ins->Read(&recX.offset, sizeof(recX.offset));
            }
        }
    	//methods
        ins->Read(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            ins->Read(&recM.abstract, sizeof(recM.abstract));
            ins->Read(&recM.kind, sizeof(recM.kind));
            ins->Read(&recM.id, sizeof(recM.id));
            ins->Read(&recM.address, sizeof(recM.address));
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
        }
    }
    else if (asKind >= ikRefine && asKind <= ikFunc)
    {
        //flags
        ins->Read(&dummy, sizeof(info.procInfo->flags));
        //bpBase
        ins->Read(&dummy, sizeof(info.procInfo->bpBase));
        //retBytes
        ins->Read(&dummy, sizeof(info.procInfo->retBytes));
        //procSize
        ins->Read(&dummy, sizeof(info.procInfo->procSize));
        //stackSize
        ins->Read(&dummy, sizeof(info.procInfo->stackSize));
   		//args
        ins->Read(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            //Tag
            ins->Read(&argInfo.Tag, sizeof(argInfo.Tag));
            //Register
            ins->Read(&argInfo.Register, sizeof(argInfo.Register));
            //Ndx
            ins->Read(&argInfo.Ndx, sizeof(argInfo.Ndx));
            //Size
            ins->Read(&argInfo.Size, sizeof(argInfo.Size));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            //TypeDef
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
        }
    //locals
        ins->Read(&num, sizeof(num));
        for (m = 0; m < num; m++)
        {
            //Ofs
            ins->Read(&locInfo.Ofs, sizeof(locInfo.Ofs));
            //Size
            ins->Read(&locInfo.Size, sizeof(locInfo.Size));
            //Name
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
            //TypeDef
            ins->Read(&len, sizeof(len));
            ins->Read(buf, len);
        }
    }
}
*/
//---------------------------------------------------------------------------
String __fastcall InfoRec::MakePrototype(int adr, bool showKind, bool showTail, bool multiline, bool fullName, bool allArgs)
{
    BYTE        callKind;
    int         n, num, argsNum, firstArg;
    PARGINFO    argInfo;
    String      result = "";

    if (showKind)
    {
        if (kind == ikConstructor)
            result += "constructor";
        else if (kind == ikDestructor)
            result += "destructor";
        else if (kind == ikFunc)
            result += "function";
        else if (kind == ikProc)
            result += "procedure";

        if (result != "") result += " ";
    }

    if (name != "")
    {
        if (!fullName)
            result += ExtractProcName(name);
        else
            result += name;
    }
    else
        result += GetDefaultProcName(adr);

    num = argsNum = (procInfo->args) ? procInfo->args->Count : 0; firstArg = 0;

    if (num && !allArgs)
    {
        if (kind == ikConstructor || kind == ikDestructor)
        {
            firstArg = 2;
            num -= 2;
        }
        else if (procInfo->flags & PF_ALLMETHODS)
        {
            firstArg = 1;
            num--;
        }
    }
    if (num)
    {
        result += "(";
        if (multiline) result += "\r\n";

        for (n = firstArg; n < argsNum; n++)
        {
            if (n != firstArg)
            {
                result += ";";
                if (multiline)
                    result += "\r\n";
                else
                    result += " ";
            }

            argInfo = (PARGINFO)procInfo->args->Items[n];
            if (argInfo->Tag == 0x22) result += "var ";

            else if (argInfo->Tag == 0x23) result += "const ";  //Add by ZGL

            if (argInfo->Name != "")
                result += argInfo->Name;
            else
                result += "?";
            result += ":";
            if (argInfo->TypeDef != "")
                result += argInfo->TypeDef;
            else
                result += "?";
        }

        if (multiline) result += "\r\n";
        result += ")";
    }

    if (kind == ikFunc)
    {
        result += ":";
        if (type != "")
            result += TrimTypeName(type);
        else
            result += "?";
    }
    result += ";";
    callKind = procInfo->flags & 7;
    switch (callKind)
    {
    case 1:
        result += " cdecl;";
        break;
    case 2:
        result += " pascal;";
        break;
    case 3:
        result += " stdcall;";
        break;
    case 4:
        result += " safecall;";
        break;
    }
    String argres = "";
    //fastcall
    if (!callKind)//(!IsFlagSet(cfImport, Adr2Pos(Adr)))
    {
        for (n = 0; n < argsNum; n++)
        {
            argInfo = (PARGINFO)procInfo->args->Items[n];
            //if (!callKind)
            //{
                if (argInfo->Ndx == 0)
                    argres += "A";
                else if (argInfo->Ndx == 1)
                    argres += "D";
                else if (argInfo->Ndx == 2)
                    argres += "C";
            //}
        }
    }
    if (showTail)
    {
        if (argres != "") result += " in" + argres;
        //output registers
        //if (info.procInfo->flags & PF_OUTEAX) result += " out";

        if (procInfo->retBytes)
            result += " RET" + Val2Str0(procInfo->retBytes);

        if (procInfo->flags & PF_ARGSIZEG) result += "+";
        if (procInfo->flags & PF_ARGSIZEL) result += "-";
    }
    return result;
}
//---------------------------------------------------------------------------
String __fastcall InfoRec::MakeDelphiPrototype(int Adr, PMethodRec recM)
{
    bool        abstract = false;
    BYTE        callKind;
    int         n, num, argsNum, firstArg, len = 0;
    PARGINFO    argInfo;

    if (kind == ikConstructor)
        len = sprintf(StringBuf, "constructor");
    else if (kind == ikDestructor)
        len = sprintf(StringBuf, "destructor");
    else if (kind == ikFunc)
        len = sprintf(StringBuf, "function");
    else if (kind == ikProc)
        len = sprintf(StringBuf, "procedure");

    if (len > 0) len += sprintf(StringBuf + len, " ");

    if (SameText(name, "@AbstractError")) abstract = true;

    if (name != "")
    {
        if (!abstract)
            len += sprintf(StringBuf + len, "%s", ExtractProcName(name).c_str());
        else if (recM->name != "")
            len += sprintf(StringBuf + len, "%s", ExtractProcName(recM->name).c_str());
        else
            len += sprintf(StringBuf + len, "v%X", recM->id);
    }
    else
        len += sprintf(StringBuf + len, "v%X", recM->id);

    num = argsNum = (procInfo->args) ? procInfo->args->Count : 0; firstArg = 0;

    if (num)
    {
        if (procInfo->flags & PF_ALLMETHODS)
        {
            firstArg = 1;
            num--;
            if (kind == ikConstructor || kind == ikDestructor)
            {
                firstArg++;
                num--;
            }
        }
        if (num)
        {
            len += sprintf(StringBuf + len, "(");

            callKind = procInfo->flags & 7;
            for (n = firstArg; n < argsNum; n++)
            {
                if (n != firstArg) len += sprintf(StringBuf + len, "; ");

                argInfo = (PARGINFO)procInfo->args->Items[n];
                if (argInfo->Tag == 0x22) len += sprintf(StringBuf + len, "var ");

                else if (argInfo->Tag == 0x23) len += sprintf(StringBuf + len, "const ");   //Add by ZGL

                if (argInfo->Name != "")
                    len += sprintf(StringBuf + len, "%s", argInfo->Name.c_str());
                else
                    len += sprintf(StringBuf + len, "?");
                len += sprintf(StringBuf + len, ":");
                if (argInfo->TypeDef != "")
                    len += sprintf(StringBuf + len, "%s", argInfo->TypeDef.c_str());
                else
                    len += sprintf(StringBuf + len, "?");
            }
            len += sprintf(StringBuf + len, ")");
        }
    }

    if (kind == ikFunc)
    {
        len += sprintf(StringBuf + len, ":");
        if (type != "")
            len += sprintf(StringBuf + len, "%s", TrimTypeName(type).c_str());
        else
            len += sprintf(StringBuf + len, "?");
    }
    len += sprintf(StringBuf + len, "; virtual;");

    if (abstract)
        len += sprintf(StringBuf + len, " abstract;");
    else
    {
    switch (callKind)
    {
        case 1:
            len += sprintf(StringBuf + len, " cdecl;");
            break;
        case 2:
            len += sprintf(StringBuf + len, " pascal;");
            break;
        case 3:
            len += sprintf(StringBuf + len, " stdcall;");
            break;
        case 4:
            len += sprintf(StringBuf + len, " safecall;");
            break;
        }
    }
    return String(StringBuf, len);
}
//---------------------------------------------------------------------------
String __fastcall InfoRec::MakeMultilinePrototype(int Adr, int* ArgsBytes, String MethodType)
{
    BYTE        callKind;
    int         n, num, argsNum, firstArg, argsBytes = 0;
    PARGINFO    argInfo;
    String      result;

    if (name != "")
        result = name;
    else
        result = GetDefaultProcName(Adr);

    num = argsNum = (procInfo->args) ? procInfo->args->Count : 0;
    firstArg = 0;

    if (kind == ikConstructor || kind == ikDestructor)
    {
        firstArg = 2;
        num -= 2;
    }
    else if (procInfo->flags & PF_ALLMETHODS)
    {
        if (!num)
        {
            argInfo = new ARGINFO;
            argInfo->Tag = 0x21;
            argInfo->Ndx = 0;
            argInfo->Size = 4;
            argInfo->Name = "Self";
            argInfo->TypeDef = MethodType;
            procInfo->AddArg(argInfo);
        }
        else
        {
            if (MethodType != "")
            {
                argInfo = (PARGINFO)procInfo->args->Items[0];
                argInfo->Name = "Self";
                argInfo->TypeDef = MethodType;
                procInfo->args->Items[0] = argInfo;
            }
            num--;
        }
        firstArg = 1;
    }

    if (num > 0) result += "(\r\n";
    callKind = procInfo->flags & 7;
    for (n = firstArg; n < argsNum; n++)
    {
        if (n != firstArg) result += ";\r\n";

        argInfo = (PARGINFO)procInfo->args->Items[n];
        //var
        if (argInfo->Tag == 0x22) result += "var ";

        else if (argInfo->Tag == 0x23) result += "const ";  //Add by ZGL

        //name
        if (argInfo->Name != "")
            result += argInfo->Name;
        else
            result += "?";
        //type
        result += ":";
        if (argInfo->TypeDef != "")
            result += argInfo->TypeDef;
        else
            result += "?";
        //size
        if (argInfo->Size > 4) result += ":" + String(argInfo->Size);

        if (argInfo->Ndx > 2) argsBytes += argInfo->Size;
    }
    if (num > 0) result += "\r\n)";

    if (kind == ikFunc)
    {
        result += ":";
        if (type != "")
            result += TrimTypeName(type);
        else
            result += "?";
    }
    result += ";";
    *ArgsBytes = argsBytes;
    return result;
}
//---------------------------------------------------------------------------
String __fastcall InfoRec::MakeMapName(int Adr)
{
    String result;
    if (name != "")
        result = name;
    else
        result = GetDefaultProcName(Adr);

    return result;
}
//---------------------------------------------------------------------------
bool __fastcall InfoRec::MakeArgsManually()
{
    String _sname;

	//if (info.procInfo->flags & PF_KBPROTO) return true;
    //Some procedures not begin with '@'
    //function QueryInterface(Self:Pointer; IID:TGUID; var Obj:Pointer);
    if (name.Pos(".QueryInterface"))
    {
        kind = ikFunc;
        type = "HRESULT";
        procInfo->flags &= 0xFFFFFFF8;
        procInfo->flags |= 3;//stdcall
        procInfo->AddArg(0x21, 8, 4, "Self", "");
        procInfo->AddArg(0x23, 12, 4, "IID", "TGUID");  //Midify by ZGL
        procInfo->AddArg(0x22, 16, 4, "Obj", "Pointer");
        return true;
    }
    //function _AddRef(Self:Pointer):Integer;
    //function _Release(Self:Pointer):Integer;
    if (name.Pos("._AddRef") || name.Pos("._Release"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->flags &= 0xFFFFFFF8;
        procInfo->flags |= 3;//stdcall
        procInfo->AddArg(0x21, 8, 4, "Self", "");
        return true;
    }
    //procedure DynArrayClear(var arr:Pointer; typeInfo:PDynArrayTypeInfo);
    if (SameText(name, "DynArrayClear"))
    {
    	kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "typeInfo", "PDynArrayTypeInfo");
        return true;
    }
    //procedure DynArraySetLength(var arr:Pointer; typeInfo:PDynArrayTypeInfo; dimCnt:Longint; lenVec:PLongint);
    if (SameText(name, "DynArraySetLength"))
    {
    	kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "typeInfo", "PDynArrayTypeInfo");
        procInfo->AddArg(0x21, 2, 4, "dimCnt", "Longint");
        procInfo->AddArg(0x21, 8, 4, "lenVec", "PLongint");
        return true;
    }

	if (name[1] != '@') return false;
    
    //Strings
    if (name[2] == 'L')
        _sname = "AnsiString";
    else if (name[2] == 'W')
        _sname = "WideString";
    else if (name[2] == 'U')
        _sname = "UnicodeString";
    else
        _sname = "?";

    if (_sname != "?")
    {
        //@LStrClr, @WStrClr, @UStrClr
        if (SameText(&name[3], "StrClr"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "S", _sname);
            return true;
        }
        //@LStrArrayClr, @WStrArrayClr, @UStrArrayClr
        if (SameText(&name[3], "StrArrayClr"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "StrArray", "Pointer");
            procInfo->AddArg(0x21, 1, 4, "Count", "Integer");
            return true;
        }
        //@LStrAsg, @WStrAsg, @UStrAsg
        if (SameText(&name[3], "StrAsg"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", _sname);
            return true;
        }
        //@LStrLAsg, @WStrLAsg, @UStrLAsg
        if (SameText(&name[3], "StrLAsg"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x23, 1, 4, "Source", _sname); //Modify by ZGL
            return true;
        }
        //@LStrFromPCharLen, @WStrFromPCharLen, @UStrFromPCharLen
        if (SameText(&name[3], "StrFromPCharLen"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PAnsiChar");
            procInfo->AddArg(0x21, 2, 4, "Length", "Integer");
            return true;
        }
        //@LStrFromPWCharLen, @WStrFromPWCharLen, @UStrFromPWCharLen
        if (SameText(&name[3], "StrFromPWCharLen"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PWideChar");
            procInfo->AddArg(0x21, 2, 4, "Length", "Integer");
            return true;
        }
        //@LStrFromChar, @WStrFromChar, @UStrFromChar
        if (SameText(&name[3], "StrFromChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "AnsiChar");
            return true;
        }
        //@LStrFromWChar, @WStrFromWChar, @UStrFromWChar
        if (SameText(&name[3], "StrFromWChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "WideChar");
            return true;
        }
        //@LStrFromPChar, @WStrFromPChar, @UStrFromPChar
        if (SameText(&name[3], "StrFromPChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PAnsiChar");
            return true;
        }
        //@LStrFromPWChar, @WStrFromPWChar, @UStrFromPWChar
        if (SameText(&name[3], "StrFromPWChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PWideChar");
            return true;
        }
        //@LStrFromString, @WStrFromString, @UStrFromString
        if (SameText(&name[3], "StrFromString"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x23, 1, 4, "Source", "ShortString");  //Modify by ZGL
            return true;
        }
        //@LStrFromArray, @WStrFromArray, @UStrFromArray
        if (SameText(&name[3], "StrFromArray"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PAnsiChar");
            procInfo->AddArg(0x21, 2, 4, "Length", "Integer");
            return true;
        }
        //@LStrFromWArray, @WStrFromWArray, @UStrFromWArray
        if (SameText(&name[3], "StrFromWArray"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", "PWideChar");
            procInfo->AddArg(0x21, 2, 4, "Length", "Integer");
            return 1;
        }
        //@LStrFromWStr, @UStrFromWStr
        if (SameText(&name[3], "StrFromWStr"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x23, 1, 4, "Source", "WideString");   //Modify by ZGL
            return true;
        }
        //@LStrToString, @WStrToString, @UStrToString
        if (SameText(&name[3], "StrToString"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", "ShortString");
            procInfo->AddArg(0x23, 1, 4, "Source", _sname); //Modify by ZGL
            procInfo->AddArg(0x21, 2, 4, "MaxLen", "Integer");
            return true;
        }
        //@LStrLen, @WStrLen, @UStrLen
        if (SameText(&name[3], "StrLen"))
        {
            kind = ikFunc;
            type = "Integer";
            procInfo->AddArg(0x21, 0, 4, "S", _sname);
            return true;
        }
        //@LStrCat, @WStrCat, @UStrCat
        if (SameText(&name[3], "StrCat"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source", _sname);
            return true;
        }
        //@LStrCat3, @WStrCat3, @UStrCat3
        if (SameText(&name[3], "StrCat3"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "Source1", _sname);
            procInfo->AddArg(0x21, 2, 4, "Source2", _sname);
            return true;
        }
        //@LStrCatN, @WStrCatN, @UStrCatN
        if (SameText(&name[3], "StrCatN"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x21, 1, 4, "ArgCnt", "Integer");
            return true;
        }
        //@LStrCmp, @WStrCmp, @UStrCmp
        if (SameText(&name[3], "StrCmp")) //return Flags
        {
            kind = ikFunc;
            procInfo->AddArg(0x21, 0, 4, "Left", _sname);
            procInfo->AddArg(0x21, 1, 4, "Right", _sname);
            return true;
        }
        //@LStrAddRef, @WStrAddRef, @UStrAddRef
        if (SameText(&name[3], "StrAddRef"))
        {
            kind = ikFunc;
            type = "Pointer";
            procInfo->AddArg(0x22, 0, 4, "S", _sname);
            return true;
        }
        //@LStrToPChar, @WStrToPChar, @UStrToPChar
        if (SameText(&name[3], "StrToPChar"))
        {
            kind = ikFunc;
            type = "PChar";
            procInfo->AddArg(0x21, 0, 4, "S", _sname);
            return true;
        }
        //@LStrCopy, @WStrCopy, @UStrCopy
        if (SameText(&name[3], "StrCopy"))
        {
            kind = ikFunc;
            type = _sname;
            procInfo->AddArg(0x23, 0, 4, "S", _sname);  //Modify by ZGL
            procInfo->AddArg(0x21, 1, 4, "Index", "Integer");
            procInfo->AddArg(0x21, 2, 4, "Count", "Integer");
            return true;
        }
        //@LStrDelete, @WStrDelete, @UStrDelete
        if (SameText(&name[3], "StrDelete"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "S", _sname);
            procInfo->AddArg(0x21, 1, 4, "Index", "Integer");
            procInfo->AddArg(0x21, 2, 4, "Count", "Integer");
            return true;
        }
        //@LStrInsert, @WStrInsert, @UStrInsert
        if (SameText(&name[3], "StrInsert"))
        {
            kind = ikProc;
            procInfo->AddArg(0x23, 0, 4, "Source", _sname); //Modify by ZGL
            procInfo->AddArg(0x22, 1, 4, "S", _sname);
            procInfo->AddArg(0x21, 2, 4, "Index", "Integer");
            return true;
        }
        //@LStrPos, @WStrPos, @UStrPos
        if (SameText(&name[3], "StrPos"))
        {
            kind = ikFunc;
            type = "Integer";
            procInfo->AddArg(0x23, 0, 4, "Substr", _sname); //Modify by ZGL
            procInfo->AddArg(0x23, 1, 4, "S", _sname);  //Modify by ZGL
            return true;
        }
        //@LStrSetLength, @WStrSetLength, @UStrSetLength
        if (SameText(&name[3], "StrSetLength"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "S", _sname);
            procInfo->AddArg(0x21, 1, 4, "NewLen", "Integer");
            return true;
        }
        //@LStrOfChar, @WStrOfChar, @UStrOfChar
        if (SameText(&name[3], "StrOfChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x21, 0, 1, "C", "Char");
            procInfo->AddArg(0x21, 1, 4, "Count", "Integer");
            procInfo->AddArg(0x22, 2, 4, "Result", _sname);
            return 1;
        }
        //@WStrToPWChar, @UStrToPWChar
        if (SameText(&name[3], "StrToPWChar"))
        {
            kind = ikFunc;
            type = "PWideChar";
            procInfo->AddArg(0x21, 0, 4, "S", _sname);
            return true;
        }
        //@WStrFromLStr, @UStrFromLStr
        if (SameText(&name[3], "StrFromLStr"))
        {
            kind = ikProc;
            procInfo->AddArg(0x22, 0, 4, "Dest", _sname);
            procInfo->AddArg(0x23, 1, 4, "Source", "AnsiString");   //Modify by ZGL
            return true;
        }
        //@WStrOfWChar
        if (SameText(&name[3], "StrOfWChar"))
        {
            kind = ikProc;
            procInfo->AddArg(0x21, 0, 4, "C", "WideChar");
            procInfo->AddArg(0x21, 1, 4, "Count", "Integer");
            procInfo->AddArg(0x22, 2, 4, "Result", _sname);
            return true;
        }
        //@UStrEqual
        if (SameText(&name[3], "StrEqual"))
        {
            kind = ikProc;
            procInfo->AddArg(0x21, 0, 4, "Left", _sname);
            procInfo->AddArg(0x21, 1, 4, "Right", _sname);
            return true;
        }
    }
    //ShortStrings--------------------------------------------------------------
    _sname = "ShortString";
    //@Copy
    if (SameText(name, "@Copy"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "S", _sname);
        procInfo->AddArg(0x21, 1, 4, "Index", "Integer");
        procInfo->AddArg(0x21, 2, 4, "Count", "Integer");
        procInfo->AddArg(0x21, 8, 4, "Result", _sname);
        return true;
    }
    //@Delete
    if (SameText(name, "@Delete"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "S", _sname);
        procInfo->AddArg(0x21, 1, 4, "Index", "Integer");
        procInfo->AddArg(0x21, 2, 4, "Count", "Integer");
        return true;
    }
    //@Insert
    if (SameText(name, "@Insert"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Source", _sname);
        procInfo->AddArg(0x22, 1, 4, "Dest", "ShortString");
        procInfo->AddArg(0x21, 2, 4, "Index", "Integer");
        return true;
    }
    //@Pos
    if (SameText(name, "@Pos"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x21, 0, 4, "Substr", _sname);
        procInfo->AddArg(0x21, 1, 4, "S", _sname);
        return true;
    }
    _sname = "PShortString";
    //@SetLength
    if (SameText(name, "@SetLength"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "S", _sname);
        procInfo->AddArg(0x21, 1, 4, "NewLength", "Byte");
        return true;
    }
    //@SetString
    if (SameText(name, "@SetString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "S", _sname);
        procInfo->AddArg(0x21, 1, 4, "Buffer", "PChar");
        procInfo->AddArg(0x21, 2, 4, "Length", "Byte");
        return true;
    }
    //@PStrCat
    if (SameText(name, "@PStrCat"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", _sname);
        procInfo->AddArg(0x21, 1, 4, "Source", _sname);
        return true;
    }
    //@PStrNCat
    if (SameText(name, "@PStrNCat"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", _sname);
        procInfo->AddArg(0x21, 1, 4, "Source", _sname);
        procInfo->AddArg(0x21, 2, 4, "MaxLen", "Byte");
        return true;
    }
    //@PStrCpy
    if (SameText(name, "@PStrCpy"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", _sname);
        procInfo->AddArg(0x21, 1, 4, "Source", _sname);
        return true;
    }
    //@PStrNCpy
    if (SameText(name, "@PStrNCpy"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", _sname);
        procInfo->AddArg(0x21, 1, 4, "Source", _sname);
        procInfo->AddArg(0x21, 2, 4, "MaxLen", "Byte");
        return true;
    }
    //@AStrCmp
    if (SameText(name, "@AStrCmp"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "S1", _sname);
        procInfo->AddArg(0x21, 1, 4, "S2", _sname);
        procInfo->AddArg(0x21, 2, 4, "Bytes", "Integer");
        return true;
    }
    //@PStrCmp
    if (SameText(name, "@PStrCmp"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "S1", _sname);
        procInfo->AddArg(0x21, 1, 4, "S2", _sname);
        return true;
    }
    //@Str0Long
    if (SameText(name, "@Str0Long"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Val", "Longint");
        procInfo->AddArg(0x21, 1, 4, "S", _sname);
        return true;
    }
    //@StrLong
    if (SameText(name, "@StrLong"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Val", "Integer");
        procInfo->AddArg(0x21, 1, 4, "Width", "Integer");
        procInfo->AddArg(0x21, 2, 4, "S", _sname);
        return true;
    }
    //Files---------------------------------------------------------------------
    //@Append(
    if (SameText(name, "@Append"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@Assign
    if (SameText(name, "@Assign"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "S", "String");
        return true;
    }
    //@BlockRead
    if (SameText(name, "@BlockRead"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "buffer", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "recCnt", "Longint");
        procInfo->AddArg(0x22, 8, 4, "recsRead", "Longint");
        return true;
    }
    //@BlockWrite
    if (SameText(name, "@BlockWrite"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "buffer", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "recCnt", "Longint");
        procInfo->AddArg(0x22, 8, 4, "recsWritten", "Longint");
        return true;
    }
    //@Close
    if (SameText(name, "@Close"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@EofFile
    if (SameText(name, "@EofFile"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@EofText
    if (SameText(name, "@EofText"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@Eoln
    if (SameText(name, "@Eoln"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@Erase
    if (SameText(name, "@Erase"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@FilePos
    if (SameText(name, "@FilePos"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@FileSize
    if (SameText(name, "@FileSize"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@Flush
    if (SameText(name, "@Flush"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@ReadRec
    if (SameText(name, "@ReadRec"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "Buffer", "Pointer");
        return true;
    }
    //@ReadChar
    if (SameText(name, "@ReadChar"))
    {
        kind = ikFunc;
        type = "Char";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@ReadLong
    if (SameText(name, "@ReadLong"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@ReadString
    if (SameText(name, "@ReadString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PShortString");
        procInfo->AddArg(0x21, 2, 4, "MaxLen", "Longint");
        return true;
    }
    //@ReadCString
    if (SameText(name, "@ReadCString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PAnsiChar");
        procInfo->AddArg(0x21, 2, 4, "MaxLen", "Longint");
        return true;
    }
    //@ReadLString
    if (SameText(name, "@ReadLString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x22, 1, 4, "S", "AnsiString");
        return true;
    }
    //@ReadWString
    if (SameText(name, "@ReadWString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x22, 1, 4, "S", "WideString");
        return true;
    }
    //@ReadUString
    if (SameText(name, "@ReadUString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x22, 1, 4, "S", "UnicodeString");
        return true;
    }
    //@ReadWCString
    if (SameText(name, "@ReadWCString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PWideChar");
        procInfo->AddArg(0x21, 2, 4, "MaxBytes", "Longint");
        return true;
    }
    //@ReadWChar
    if (SameText(name, "@ReadWChar"))
    {
        kind = ikFunc;
        type = "WideChar";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@ReadExt
    if (SameText(name, "@ReadExt"))
    {
        //kind = ikFunc;
        //type = "Extended";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@ReadLn
    if (SameText(name, "@ReadLn"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@Rename
    if (SameText(name, "@Rename"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "NewName", "PChar");
        return true;
    }
    //@ResetFile
    if (SameText(name, "@ResetFile"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "RecSize", "Longint");
        return true;
    }
    //@ResetText
    if (SameText(name, "@ResetText"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@RewritFile
    if (SameText(name, "@RewritFile"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "RecSize", "Longint");
        return true;
    }
    //@RewritText
    if (SameText(name, "@RewritText"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        //procInfo->AddArg(0x21, 1, 4, "recSize", "Longint");
        return true;
    }
    //@Seek
    if (SameText(name, "@Seek"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "RecNum", "Longint");
        return true;
    }
    //@SeekEof
    if (SameText(name, "@SeekEof"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@SeekEoln
    if (SameText(name, "@SeekEoln"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@SetTextBuf
    if (SameText(name, "@SetTextBuf"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "P", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "Size", "Longint");
        return true;
    }
    //@Truncate
    if (SameText(name, "@Truncate"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        return true;
    }
    //@WriteRec
    if (SameText(name, "@WriteRec"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "F", "TFileRec");
        procInfo->AddArg(0x21, 1, 4, "Buffer", "Pointer");
        return true;
    }
    //@WriteChar
    if (SameText(name, "@WriteChar"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "C", "AnsiChar");
        procInfo->AddArg(0x21, 2, 4, "Width", "Integer");
        return true;
    }
    //@Write0Char
    if (SameText(name, "@Write0Char"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "C", "AnsiChar");
        return true;
    }
    //@WriteBool
    if (SameText(name, "@WriteBool"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "Val", "Boolean");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0Bool
    if (SameText(name, "@Write0Bool"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "Val", "Boolean");
        return true;
    }
    //@WriteLong
    if (SameText(name, "@WriteLong"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "Val", "Longint");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0Long
    if (SameText(name, "@Write0Long"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "Val", "Longint");
        return true;
    }
    //@WriteString
    if (SameText(name, "@WriteString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "ShortString");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0String
    if (SameText(name, "@Write0String"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "ShortString");
        return true;
    }
    //@WriteCString
    if (SameText(name, "@WriteCString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PAnsiChar");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0CString
    if (SameText(name, "@Write0CString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PAnsiChar");
        return true;
    }
    //@WriteLString
    if (SameText(name, "@WriteLString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "AnsiString");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0LString
    if (SameText(name, "@Write0LString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "AnsiString");
        return true;
    }
    //@Write2Ext
    if (SameText(name, "@Write2Ext") ||
        SameText(name, "@Str2Ext"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 8, 12, "Val", "Extended");
        procInfo->AddArg(0x21, 1, 4, "Width", "Longint");
        procInfo->AddArg(0x21, 2, 4, "Precision", "Longint");
        return true;
    }
    //@WriteWString
    if (SameText(name, "@WriteWString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "WideString");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0WString
    if (SameText(name, "@Write0WString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "WideString");
        return true;
    }
    //@WriteWCString
    if (SameText(name, "@WriteWCString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PWideChar");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0WCString
    if (SameText(name, "@Write0WCString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "PWideChar");
        return true;
    }
    //@WriteWChar
    if (SameText(name, "@WriteWChar"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "C", "WideChar");
        procInfo->AddArg(0x21, 2, 4, "Width", "Longint");
        return true;
    }
    //@Write0WChar
    if (SameText(name, "@Write0WChar"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "C", "WideChar");
        return true;
    }
    //@Write1Ext
    if (SameText(name, "@Write1Ext"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "Width", "Longint");
        return true;
    }
    //@Write0Ext
    if (SameText(name, "@Write0Ext"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@WriteLn
    if (SameText(name, "@WriteLn"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        return true;
    }
    //@WriteUString
    if (SameText(name, "@WriteUString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "UnicodeString");
        procInfo->AddArg(0x21, 2, 4, "Width", "Integer");
        return true;
    }
    //@Write0UString
    if (SameText(name, "@Write0UString"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "S", "UnicodeString");
        return true;
    }
    //@WriteVariant
    if (SameText(name, "@WriteVariant"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "V", "TVarData");
        procInfo->AddArg(0x21, 2, 4, "Width", "Integer");
        return true;
    }
    //@Write0Variant
    if (SameText(name, "@Write0Variant"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "T", "TTextRec");
        procInfo->AddArg(0x21, 1, 4, "V", "TVarData");
        return true;
    }
    //Other---------------------------------------------------------------------
    //A
    //function @AsClass(child:TObject; parent:TClass):TObject;
    if (SameText(name, "@AsClass"))
    {
        kind = ikFunc;
        type = "TObject";
        procInfo->AddArg(0x21, 0, 4, "child", "TObject");
        procInfo->AddArg(0x21, 1, 4, "parent", "TClass");
        return true;
    }
    //procedure @Assert(Message: String; Filename:String; LineNumber:Integer);
    if (SameText(name, "@Assert"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Message", "String");
        procInfo->AddArg(0x21, 1, 4, "Filename", "String");
        procInfo->AddArg(0x21, 2, 4, "LineNumber", "Integer");
        return true;
    }
    //@BoundErr
    if (SameText(name, "@BoundErr"))
    {
        kind = ikProc;
        return true;
    }
    //@CopyArray
    if (SameText(name, "@CopyArray"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "Source", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "TypeInfo", "Pointer");
        procInfo->AddArg(0x21, 8, 4, "Cnt", "Integer");
        return true;
    }
    //@CopyRecord
    if (SameText(name, "@CopyRecord"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Dest", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "Source", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "TypeInfo", "Pointer");
        return true;
    }
    //DynArrays
    //@DynArrayAddRef
    if (SameText(name, "@DynArrayAddRef"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Arr", "Pointer");
        return true;
    }
    //@DynArrayAsg
    if (SameText(name, "@DynArrayAsg"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "Dest", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "Source", "Pointer");
        procInfo->AddArg(0x21, 2, 4, "TypeInfo", "PDynArrayTypeInfo");
        return true;
    }
    //@DynArrayClear
    if (SameText(name, "@DynArrayClear"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "Arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "TypeInfo", "PDynArrayTypeInfo");
        return true;
    }
    //@DynArrayCopy
    if (SameText(name, "@DynArrayCopy"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "TypeInfo", "PDynArrayTypeInfo");
        procInfo->AddArg(0x22, 2, 4, "Result", "Pointer");
        return true;
    }
    //@DynArrayCopyRange
    if (SameText(name, "@DynArrayCopyRange"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "Arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "TypeInfo", "PDynArrayTypeInfo");
        procInfo->AddArg(0x21, 2, 4, "Index", "Integer");
        procInfo->AddArg(0x21, 8, 4, "Count", "Integer");
        procInfo->AddArg(0x22, 12, 4, "Result", "Pointer");
        return true;
    }
    //@DynArrayHigh
    if (SameText(name, "@DynArrayHigh"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x21, 0, 4, "Arr", "Pointer");
        return true;
    }
    //@DynArrayLength
    if (SameText(name, "@DynArrayLength"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x21, 0, 4, "Arr", "Pointer");
        return true;
    }
    //@DynArraySetLength
    if (SameText(name, "@DynArraySetLength"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "Arr", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "TypeInfo", "PDynArrayTypeInfo");
        procInfo->AddArg(0x21, 2, 4, "DimCnt", "Longint");
        procInfo->AddArg(0x21, 8, 4, "LengthVec", "Longint");
        return true;
    }
    //@FillChar
    if (SameText(name, "@FillChar"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "dst", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "cnt", "Integer");
        procInfo->AddArg(0x21, 2, 4, "val", "Char");
        return true;
    }
    //@FreeMem
    if (SameText(name, "@FreeMem"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x21, 0, 4, "p", "Pointer");
        return true;
    }
    //@GetMem
    if (SameText(name, "@GetMem"))
    {
        kind = ikFunc;
        type = "Pointer";
        procInfo->AddArg(0x21, 0, 4, "size", "Integer");
        return true;
    }
    //@IntOver
    if (SameText(name, "@IntOver"))
    {
        kind = ikProc;
        return true;
    }
    //@IsClass
    if (SameText(name, "@IsClass"))
    {
        kind = ikFunc;
        type = "Boolean";
        procInfo->AddArg(0x21, 0, 4, "Child", "TObject");
        procInfo->AddArg(0x21, 1, 4, "Parent", "TClass");
        return true;
    }
    //@RandInt
    if (SameText(name, "@RandInt"))
    {
        kind = ikFunc;
        type = "Integer";
        procInfo->AddArg(0x21, 0, 4, "Range", "Integer");
        return true;
    }
    //@ReallocMem
    if (SameText(name, "@ReallocMem"))
    {
        kind = ikFunc;
        type = "Pointer";
        procInfo->AddArg(0x22, 0, 4, "P", "Pointer");
        procInfo->AddArg(0x21, 1, 4, "NewSize", "Integer");
        return true;
    }
    //@SetElem
    if (SameText(name, "@SetElem"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "Dest", "SET");
        procInfo->AddArg(0x21, 1, 1, "Elem", "Byte");
        procInfo->AddArg(0x21, 2, 1, "Size", "Byte");
        return true;
    }
    //Trunc
    if (SameText(name, "@Trunc"))
    {
        kind = ikFunc;
        type = "Int64";
        procInfo->AddArg(0x21, 0, 12, "X", "Extended");
        return true;
    }
    //V
    //function @ValExt(s:AnsiString; var code:Integer):Extended;
    if (SameText(name, "@ValExt"))
    {
        kind = ikProc;
        procInfo->AddArg(0x21, 0, 4, "s", "AnsiString");
        procInfo->AddArg(0x22, 1, 4, "code", "Integer");
        return true;
    }
    //function @ValLong(s:AnsiString; var code:Integer):Longint;
    if (SameText(name, "@ValLong"))
    {
        kind = ikFunc;
        type = "Longint";
        procInfo->AddArg(0x21, 0, 4, "s", "AnsiString");
        procInfo->AddArg(0x22, 1, 4, "code", "Integer");
        return true;
    }
    //procedure @VarFromCurr(var v: Variant; val:Currency);
    if (SameText(name, "@VarFromCurr"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "v", "Variant");
        //procInfo->AddArg(0x21, ?, 8, "val", "Currency");
        return true;
    }
    //procedure @VarFromReal(var v:Variant; val:Real);
    if (SameText(name, "@VarFromReal"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "v", "Variant");
        //procInfo->AddArg(0x21, ?, 8, "val", "Real"); 	fld!
        return true;
    }
    //procedure @VarFromTDateTime(var v: Variant; val:TDateTime);
    if (SameText(name, "@VarFromTDateTime"))
    {
        kind = ikProc;
        procInfo->AddArg(0x22, 0, 4, "v", "Variant");
        //procInfo->AddArg(0x21, ?, 8, "val", "TDateTime");	fld!
        return true;
    }
    //SET
    //procedure @SetRange(lo:Byte; hi:Byte; size:Byte; VAR d:SET); AL,DL,ECX,AH {Usage: d = [lo..hi]}
    //function @SetEq(left:SET; right:SET; size:Byte):ZF; {Usage: left = right}
    //function @SetLe(left:SET; right:SET; size:Byte):ZF;
    //procedure @SetIntersect(var dest:SET; src:SET; size:Byte);
    //procedure @SetIntersect3(var dest:SET; src:SET; size:Longint; src2:SET);
    //procedure @SetUnion(var dest:SET; src:SET; size:Byte);
    //procedure @SetUnion3(var dest:SET; src:SET; size:Longint; src2:SET);
    //procedure @SetSub(var dest:SET; src:SET; size:Byte);
    //procedure @SetSub3(var dest:SET; src:SET; size:Longint; src2:SET);
    //procedure @SetExpand(src:SET; var dest:SET; lo:Byte; hi:Byte); EAX,EDX,CH,CL
    //in <-> bt
    return false;
}
//---------------------------------------------------------------------------
/*
{ Procedures and functions that need compiler magic }
procedure _COS(st0):st0;
procedure _EXP(st1):st0;
procedure _INT;
procedure _SIN(st0):st0;
procedure _FRAC;
procedure _ROUND;

function _RandExt:Extended; ST0

procedure _Str2Ext(val:Extended; width:Longint; precision:Longint; var s:String); [ESP+4],EAX,EDX,ECX
procedure _Str0Ext(val:Extended; var s:String); [ESP+4],EAX
procedure _Str1Ext(val:Extended; width:Longint; var s:String); [ESP+4],EAX,EDX
function _ValExt(s:AnsiString; var code:Integer):Extended; EAX,EDX:ST0
function _Pow10(val:Extended; Power:Integer):Extended; ST0,EAX:ST0
procedure _Real2Ext(val:Real):Extended; EAX:ST0
procedure _Ext2Real(val:Extended):Real; ST0,EAX

function _ObjSetup(Self:TObject; vmtPtrOff:Longint):TObject; AD        //internal
procedure _ObjCopy(Dest:TObject; Src:TObject, vmtPtrOff:Longint); ADC  //internal
function _Fail(Self:TObject; allocFlag:Longint):TObject; AD             //internal

function @_llmul(val1:Int64; val2:Int64):Int64; EAX;EDX;[ESP+4]
function @_llmulo(val1:Int64; val2:Int64):Int64,OF; EAX;EDX;[ESP+4]
function @_lldiv(val1:Int64; val2:Int64):Int64; EAX;EDX;[ESP+4]
function @_lldivo(val1:Int64; val2:Int64):Int64,OF; EAX;EDX;[ESP+4]
function @_llmod(val1:Int64; val2:Int64):Int64; EAX;EDX;[ESP+4]
function @_llshl(val:Int64):Int64; EAX,EDX
function @_llushr(val:Int64):Int64; EAX,EDX
*/

