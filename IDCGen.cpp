//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Misc.h"
#include "IDCGen.h"
//---------------------------------------------------------------------------
extern  MDisasm     Disasm;
extern  DWORD       CodeBase;
extern  BYTE        *Code;
extern  int         DelphiVersion;
extern  DWORD       *Flags;
extern  PInfoRec    *Infos;
extern  bool        SplitIDC;
//---------------------------------------------------------------------------
__fastcall TIDCGen::TIDCGen(FILE* FIdc, int splitSize)
{
    idcF = FIdc;
    unitName = "";
    itemName = "";
    names = new TStringList;
    repeated = new TList;
    SplitSize = splitSize;
    CurrentPartNo = 1;
    CurrentBytes = 0;
}
//---------------------------------------------------------------------------
__fastcall TIDCGen::~TIDCGen()
{
    delete names;
    delete repeated;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::NewIDCPart(FILE* FIdc)
{
    idcF = FIdc;
    CurrentBytes = 0;
    CurrentPartNo++;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::DeleteName(int pos)
{
    DWORD adr = Pos2Adr(pos);

    CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", adr);
    CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"\", 0);\n", adr);
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeByte(int pos)
{
    CurrentBytes += fprintf(idcF, "MakeByte(0x%lX);\n", Pos2Adr(pos));
    return pos + 1;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeWord(int pos)
{
    CurrentBytes += fprintf(idcF, "MakeWord(0x%lX);\n", Pos2Adr(pos));
    return pos + 2;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeDword(int pos)
{
    CurrentBytes += fprintf(idcF, "MakeDword(0x%lX);\n", Pos2Adr(pos));
    return pos + 4;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeQword(int pos)
{
    CurrentBytes += fprintf(idcF, "MakeQword(0x%lX);\n", Pos2Adr(pos));
    return pos + 8;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeArray(int pos, int num)
{
    CurrentBytes += fprintf(idcF, "MakeByte(0x%lX);\n", Pos2Adr(pos));
    CurrentBytes += fprintf(idcF, "MakeArray(0x%lX, %d);\n", Pos2Adr(pos), num);
    return pos + num;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeShortString(int pos)
{
    BYTE len = Code[pos];
    //Empty String
    if (!len) return pos + 1;

    if (!IsValidName(len, pos + 1)) return pos;

    CurrentBytes += fprintf(idcF, "SetLongPrm(INF_STRTYPE, ASCSTR_PASCAL);\n");
    CurrentBytes += fprintf(idcF, "MakeStr(0x%lX, 0x%lX);\n", Pos2Adr(pos), Pos2Adr(pos) + len + 1);
    return pos + len + 1;
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeCString(int pos)
{
    int len = strlen(Code + pos);
    CurrentBytes += fprintf(idcF, "SetLongPrm(INF_STRTYPE, ASCSTR_TERMCHR);\n");
    CurrentBytes += fprintf(idcF, "MakeStr(0x%lX, 0x%lX);\n", Pos2Adr(pos), Pos2Adr(pos) + len + 1);
    return pos + len + 1;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::MakeLString(int pos)
{
    CurrentBytes += fprintf(idcF, "SetLongPrm(INF_STRTYPE, ASCSTR_TERMCHR);\n");
    CurrentBytes += fprintf(idcF, "MakeStr(0x%lX, -1);\n", Pos2Adr(pos));
    //Length
    MakeDword(pos - 4);
    //RefCount
    MakeDword(pos - 8);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::MakeWString(int pos)
{
    CurrentBytes += fprintf(idcF, "SetLongPrm(INF_STRTYPE, ASCSTR_UNICODE);\n");
    CurrentBytes += fprintf(idcF, "MakeStr(0x%lX, -1);\n", Pos2Adr(pos));
    //Length
    MakeDword(pos - 4);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::MakeUString(int pos)
{
    CurrentBytes += fprintf(idcF, "SetLongPrm(INF_STRTYPE, ASCSTR_UNICODE);\n");
    CurrentBytes += fprintf(idcF, "MakeStr(0x%lX, -1);\n", Pos2Adr(pos));
    //Length
    MakeDword(pos - 4);
    //RefCount
    MakeDword(pos - 8);
    //Word
    MakeWord(pos - 10);
    //CodePage
    MakeWord(pos - 12);
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::MakeCode(int pos)
{
    DISINFO     DisInfo;

    CurrentBytes += fprintf(idcF, "MakeCode(0x%lX);\n", Pos2Adr(pos));
    int instrLen = Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), 0, 0);
    if (!instrLen) instrLen = 1;
    return instrLen;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::MakeFunction(DWORD adr)
{
    if (adr)
    {
        CurrentBytes += fprintf(idcF, "MakeFunction(0x%lX, -1);\n", adr);
        MakeCode(Adr2Pos(adr));
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::MakeComment(int pos, String text)
{
    CurrentBytes += fprintf(idcF, "MakeComm(0x%lX, \"%s\");\n", Pos2Adr(pos), TransformString(text.c_str(), text.Length()).c_str());
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::OutputAttrData(int pos)
{
    WORD dw = *((WORD*)(Code + pos));
    pos = MakeWord(pos);
    return pos;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputHeaderFull()
{
    CurrentBytes += fprintf(idcF, "#include <idc.idc>\n");
    CurrentBytes += fprintf(idcF, "static clear(from){\n");
    CurrentBytes += fprintf(idcF, "auto ea;\n");
    CurrentBytes += fprintf(idcF, "ea = from;\n");
    CurrentBytes += fprintf(idcF, "while (1){\n");
    CurrentBytes += fprintf(idcF, "ea = NextFunction(ea);\n");
    CurrentBytes += fprintf(idcF, "if (ea == -1) break;\n");
    CurrentBytes += fprintf(idcF, "DelFunction(ea);\n");
    CurrentBytes += fprintf(idcF, "MakeNameEx(ea, \"\", 0);}\n");
    CurrentBytes += fprintf(idcF, "ea = from;\n");
    CurrentBytes += fprintf(idcF, "while (1){\n");
    CurrentBytes += fprintf(idcF, "ea = FindExplored(ea, SEARCH_DOWN | SEARCH_NEXT);\n");
    CurrentBytes += fprintf(idcF, "if (ea == -1) break;\n");
    CurrentBytes += fprintf(idcF, "MakeUnkn(ea, 1);}\n");
    CurrentBytes += fprintf(idcF, "}\n");
    CurrentBytes += fprintf(idcF, "static main(){\n");
    CurrentBytes += fprintf(idcF, "clear(0x%lX);\n", CodeBase);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputHeaderShort()
{
    CurrentBytes += fprintf(idcF, "#include <idc.idc>\n");
    CurrentBytes += fprintf(idcF, "static main(){\n");
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::OutputRTTIHeader(BYTE kind, int pos)
{
    int fromPos = pos;

    BYTE len = *(Code + pos + 5);
    itemName = String((char*)(Code + pos + 6), len);
    DWORD adr = Pos2Adr(pos);
    CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", adr);
    CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"RTTI_%lX_%s_%s\", 0);\n", adr, adr, TypeKind2Name(kind).c_str(), itemName.c_str());
    //Selfptr
    pos = MakeDword(pos);
    //Kind
    //Delete name (often presents)
    DeleteName(pos);
    pos = MakeByte(pos);
    //Name
    pos = MakeShortString(pos);
    return pos - fromPos;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIInteger(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //ordType
    pos = MakeByte(pos);
    //minValue
    pos = MakeDword(pos);
    //maxValue
    pos = MakeDword(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIChar(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //ordType
    pos = MakeByte(pos);
    //minValue
    pos = MakeDword(pos);
    //maxValue
    pos = MakeDword(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIEnumeration(BYTE kind, int pos, DWORD adr)
{
    pos += OutputRTTIHeader(kind, pos);
    //ordType
    pos = MakeByte(pos);
    //minValue
    DWORD minValue = *((DWORD*)(Code + pos));
    pos = MakeDword(pos);
    //maxValue
    DWORD maxValue = *((DWORD*)(Code + pos));
    pos = MakeDword(pos);
    //baseTypeAdr
    DWORD baseTypeAdr = *((DWORD*)(Code + pos));
    pos = MakeDword(pos);

    if (baseTypeAdr == adr)
    {
        if (SameText(itemName, "ByteBool") ||
            SameText(itemName, "WordBool") ||
            SameText(itemName, "LongBool"))
        {
            minValue = 0;
            maxValue = 1;
        }
                    
        for (int n = minValue; n <= maxValue; n++)
        {
            pos = MakeShortString(pos);
        }
    }
    //UnitName
    //pos = MakeShortString(pos);
    //if (DelphiVersion == 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIFloat(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //FloatType
    pos = MakeByte(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIString(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //MaxLength
    pos = MakeByte(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTISet(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //OrdType
    pos = MakeByte(pos);
    //CompType
    pos = MakeDword(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIClass(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //classVMT
    pos = MakeDword(pos);
    //ParentInfo
    pos = MakeDword(pos);
    //PropCount
    pos = MakeWord(pos);
    //UnitName
    pos = MakeShortString(pos);
    //PropData
    WORD Count = *((WORD*)(Code + pos));
    pos = MakeWord(pos);
    for (int n = 0; n < Count; n++)
    {
        //TPropInfo
        for (int m = 0; m < 6; m++)
        {
            pos = MakeDword(pos);
        }
        pos = MakeWord(pos);
        pos = MakeShortString(pos);
    }
    if (DelphiVersion >= 2010)
    {
        //PropDataEx
        Count = *((WORD*)(Code + pos));
        pos = MakeWord(pos);
        for (int n = 0; n < Count; n++)
        {
            //Flags
            pos = MakeByte(pos);
            //Info
            DWORD typeInfo = *((DWORD*)(Code + pos));
            pos = MakeDword(pos);
            for (int m = 0; m < 6; m++)
            {
                MakeDword(Adr2Pos(typeInfo));
                typeInfo += 4;
            }
            MakeWord(Adr2Pos(typeInfo)); typeInfo += 2;
            MakeShortString(Adr2Pos(typeInfo));
            //AttrData
            pos = OutputAttrData(pos);
        }
        //AttrData
        OutputAttrData(pos);
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIMethod(BYTE kind, int pos)
{
    int     _pos = pos, pos1;

    pos += OutputRTTIHeader(kind, pos);
    //MethodKind
    BYTE methodKind = Code[pos];
    pos = MakeByte(pos);
    //ParamCnt
    BYTE paramCnt = Code[pos];
    pos = MakeByte(pos);

    for (int n = 0; n < paramCnt; n++)
    {
        //Flags
        pos = MakeByte(pos);
        //ParamName
        pos = MakeShortString(pos);
        //TypeName
        pos = MakeShortString(pos);
    }

    if (methodKind)
    {
        //ResultType
        pos = MakeShortString(pos);
        if (DelphiVersion > 6)
        {
            //ResultTypeRef
            pos = MakeDword(pos);
        }
    }

    if (DelphiVersion > 6)
    {
        //CC (TCallConv)
        pos = MakeByte(pos);
        //ParamTypeRefs
        for (int n = 0; n < paramCnt; n++)
        {
            pos = MakeDword(pos);
        }
        if (DelphiVersion >= 2010)
        {
            DWORD procSig = *((DWORD*)(Code + pos));
            //MethSig
            pos = MakeDword(pos);
            //AttrData
            OutputAttrData(pos);
            //Procedure Signature
            if (procSig)
            {
                if (IsValidImageAdr(procSig))
                    pos1 = Adr2Pos(procSig);
                else
                    pos1 = _pos + procSig;
                //Flags
                BYTE flags = Code[pos1];
                pos1 = MakeByte(pos1);
                if (flags != 0xFF)
                {
                    //CC
                    pos1 = MakeByte(pos1);
                    //ResultType
                    pos1 = MakeDword(pos1);
                    //ParamCount
                    paramCnt = Code[pos1];
                    pos1 = MakeByte(pos1);
                    for (int n = 0; n < paramCnt; n++)
                    {
                        //Flags
                        pos1 = MakeByte(pos1);
                        //ParamType
                        pos1 = MakeDword(pos1);
                        //Name
                        pos1 = MakeShortString(pos1);
                        //AttrData
                        pos1 = OutputAttrData(pos1);
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIWChar(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //ordType
    pos = MakeByte(pos);
    //minValue
    pos = MakeDword(pos);
    //maxValue
    pos = MakeDword(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTILString(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    if (DelphiVersion >= 2009)
    {
        //CodePage
        pos = MakeWord(pos);
    }
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIWString(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIVariant(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIArray(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //Size
    pos = MakeDword(pos);
    //ElCount
    pos = MakeDword(pos);
    //ElType
    pos = MakeDword(pos);

    if (DelphiVersion >= 2010)
    {
        //DimCount
        BYTE dimCnt = Code[pos];
        pos = MakeByte(pos);
        for (int n = 0; n < dimCnt; n++)
        {
            //Dims
            pos = MakeDword(pos);
        }
        //AttrData
        OutputAttrData(pos);
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIRecord(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //Size
    pos = MakeDword(pos);
    //ManagedFldCount
    int n, m, elNum = *((int*)(Code + pos));
    pos = MakeDword(pos);
    for (n = 0; n < elNum; n++)
    {
        //TypeRef
        pos = MakeDword(pos);
        //FldOffset
        pos = MakeDword(pos);
    }

    if (DelphiVersion >= 2010)
    {
        //NumOps
        BYTE numOps = Code[pos];
        pos = MakeByte(pos);
        for (n = 0; n < numOps; n++)    //RecOps
        {
            pos = MakeDword(pos);
        }
        //RecFldCnt
        elNum = *((int*)(Code + pos));
        pos = MakeDword(pos);

        for (n = 0; n < elNum; n++)
        {
            //TypeRef
            pos = MakeDword(pos);
            //FldOffset
            pos = MakeDword(pos);
            //Flags
            pos = MakeByte(pos);
            //Name
            pos = MakeShortString(pos);
            //AttrData
            pos = OutputAttrData(pos);
        }
        //AttrData
        pos = OutputAttrData(pos);
        if (DelphiVersion >= 2012)
        {
            WORD methCnt = *((WORD*)(Code + pos));
            pos = MakeWord(pos);
            for (n = 0; n < methCnt; n++)
            {
                //Flags
                pos = MakeByte(pos);
                //Code
                pos = MakeDword(pos);
                //Name
                pos = MakeShortString(pos);
                //ProcedureSignature
                //Flags
                BYTE flags = Code[pos];
                pos = MakeByte(pos);
                if (flags != 0xFF)
                {
                    //CC
                    pos = MakeByte(pos);
                    //ResultType
                    pos = MakeDword(pos);
                    BYTE paramCnt = Code[pos];
                    pos = MakeByte(pos);
                    //Params
                    for (m = 0; m < paramCnt; m++)
                    {
                        //Flags
                        pos = MakeByte(pos);
                        //ParamType
                        pos = MakeDword(pos);
                        //Name
                        pos = MakeShortString(pos);
                        //AttrData
                        pos = OutputAttrData(pos);
                    }
                }
                //AttrData
                pos = OutputAttrData(pos);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIInterface(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //IntfParent
    pos = MakeDword(pos);
    //IntfFlags
    pos = MakeByte(pos);
    //GUID
    pos = MakeArray(pos, 16);
    //UnitName
    pos = MakeShortString(pos);
    //PropCount
    WORD Count = *((WORD*)(Code + pos));
    pos = MakeWord(pos);

    if (DelphiVersion >= 6)
    {
        //RttiCount
        WORD dw = *((WORD*)(Code + pos));
        pos = MakeWord(pos);
        if (dw != 0xFFFF)
        {
            if (DelphiVersion >= 2010)
            {
                for (int n = 0; n < Count; n++)
                {
                    //Name
                    pos = MakeShortString(pos);
                    //Kind
                    BYTE methodKind = Code[pos];
                    pos = MakeByte(pos);
                    //CallConv
                    pos = MakeByte(pos);
                    //ParamCount
                    BYTE paramCnt = Code[pos];
                    pos = MakeByte(pos);

                    for (int m = 0; m < paramCnt; m++)
                    {
                        //Flags
                        pos = MakeByte(pos);
                        //ParamName
                        pos = MakeShortString(pos);
                        //TypeName
                        pos = MakeShortString(pos);
                        //ParamType
                        pos = MakeDword(pos);
                    }
                    if (methodKind)
                    {
                        //ResultTypeName
                        BYTE len = Code[pos];
                        pos = MakeShortString(pos);
                        if (len)
                        {
                            //ResultType
                            pos = MakeDword(pos);
                        }
                    }
                }
            }
            else
            {
                for (int n = 0; n < Count; n++)
                {
                    //PropType
                    pos = MakeDword(pos);
                    //GetProc
                    pos = MakeDword(pos);
                    //SetProc
                    pos = MakeDword(pos);
                    //StoredProc
                    pos = MakeDword(pos);
                    //Index
                    pos = MakeDword(pos);
                    //Default
                    pos = MakeDword(pos);
                    //NameIndex
                    pos = MakeWord(pos);
                    //Name
                    pos = MakeShortString(pos);
                }
            }
        }
        if (DelphiVersion >= 2010)
        {
            //AttrData
            OutputAttrData(pos);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIInt64(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //MinVal
    pos = MakeQword(pos);
    //MaxVal
    pos = MakeQword(pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIDynArray(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //elSize
    pos = MakeDword(pos);
    //elType
    pos = MakeDword(pos);
    //varType
    pos = MakeDword(pos);

    if (DelphiVersion >= 6)
    {
        //elType2
        pos = MakeDword(pos);
        //UnitName
        pos = MakeShortString(pos);
    }
    if (DelphiVersion >= 2010)
    {
        //DynArrElType
        pos = MakeDword(pos);
        //AttrData
        OutputAttrData(pos);
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIUString(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    if (DelphiVersion >= 2010) OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIClassRef(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //InstanceType
    pos = MakeDword(pos);
    //AttrData
    OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIPointer(BYTE kind, int pos)
{
    pos += OutputRTTIHeader(kind, pos);
    //RefType
    pos = MakeDword(pos);
    //AttrData
    OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputRTTIProcedure(BYTE kind, int pos)
{
    int     _pos = pos;

    pos += OutputRTTIHeader(kind, pos);
    //MethSig
    DWORD procSig = *((DWORD*)(Code + pos));
    pos = MakeDword(pos);
    //AttrData
    pos = OutputAttrData(pos);
    //Procedure Signature
    if (procSig)
    {
        if (IsValidImageAdr(procSig))
            pos = Adr2Pos(procSig);
        else
            pos = _pos + procSig;
        //Flags
        BYTE flags = Code[pos];
        pos = MakeByte(pos);
        if (flags != 0xFF)
        {
            //CallConv
            pos = MakeByte(pos);
            //ResultType
            pos = MakeDword(pos);
            //ParamCnt
            BYTE paramCnt = Code[pos];
            pos = MakeByte(pos);
            for (int n = 0; n < paramCnt; n++)
            {
                //Flags
                pos = MakeByte(pos);
                //ParamType
                pos = MakeDword(pos);
                //Name
                pos = MakeShortString(pos);
                //AttrData
                pos = OutputAttrData(pos);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputVMT(int pos, PInfoRec recN)
{
    itemName = recN->GetName();
    pos += OutputVMTHeader(pos, itemName);
    if (DelphiVersion >= 3)
    {
        //VmtIntfTable
        OutputIntfTable(pos); pos += 4;
        //VmtAutoTable
        OutputAutoTable(pos); pos += 4;
    }
    //VmtInitTable
    OutputInitTable(pos); pos += 4;
    //VmtTypeInfo
    pos = MakeDword(pos);
    //VmtFieldTable
    OutputFieldTable(pos); pos += 4;
    //VmtMethodTable
    OutputMethodTable(pos); pos += 4;
    //VmtDynamicTable
    OutputDynamicTable(pos); pos += 4;
    //VmtClassName
    DWORD nameAdr = *((DWORD*)(Code + pos));
    pos = MakeDword(pos);
    MakeShortString(Adr2Pos(nameAdr));
    //VmtInstanceSize
    pos = MakeDword(pos);
    //VmtParent
    pos = MakeDword(pos);
    if (DelphiVersion >= 2009)
    {
        //VmtEquals
        pos = MakeDword(pos);
        //VmtGetHashCode
        pos = MakeDword(pos);
        //VmtToString
        pos = MakeDword(pos);
    }
    if (DelphiVersion >= 3)
    {
        //VmtSafeCallException
        pos = MakeDword(pos);
    }
    if (DelphiVersion >= 4)
    {
        //VmtAfterConstruction
        pos = MakeDword(pos);
        //VmtBeforeDestruction
        pos = MakeDword(pos);
        //VmtDispatch
        pos = MakeDword(pos);
    }
    //VmtDefaultHandler
    pos = MakeDword(pos);
    //VmtNewInstance
    pos = MakeDword(pos);
    //VmtFreeInstance
    pos = MakeDword(pos);
    //VmtDestroy
    pos = MakeDword(pos);
    //Vmt
    int stopPos = Adr2Pos(GetStopAt(Pos2Adr(pos)));
    //Virtual Methods
    int ofs = 0;
    while (pos < stopPos)
    {
        MakeComment(pos, "+" + Val2Str0(ofs)); ofs += 4;
        pos = MakeDword(pos);
    }
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::OutputVMTHeader(int pos, String vmtName)
{
    int fromPos = pos;
    DWORD adr = Pos2Adr(pos);

    CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", adr);
    CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"VMT_%lX_%s\", 0);\n", adr, adr, vmtName.c_str());
    //VmtSelfPtr
    pos = MakeDword(pos);
    return pos - fromPos;
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputIntfTable(int pos)
{
    MakeDword(pos);
    DWORD intfTable = *((DWORD*)(Code + pos));
    if (intfTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", intfTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_IntfTable\", 0);\n", intfTable, itemName.c_str());
        pos = Adr2Pos(intfTable);
        //EntryCount
        DWORD EntryCount = *((DWORD*)(Code + pos));
        pos = MakeDword(pos);
        for (int n = 0; n < EntryCount; n++)
        {
            //GUID
            pos = MakeArray(pos, 16);
            //vTableAdr
            OutputIntfVTable(pos, intfTable); pos += 4;
            //IOffset
            pos = MakeDword(pos);
            if (DelphiVersion > 3)
            {
                //ImplGetter
                pos = MakeDword(pos);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputIntfVTable(int pos, DWORD stopAdr)
{
    MakeDword(pos);
    DWORD vTableAdr = *((DWORD*)(Code + pos));
    if (vTableAdr)
    {
        int pos = Adr2Pos(vTableAdr);
        //CC byte address
        DWORD CCadr = vTableAdr;
        for (int n = 0;; n++)
        {
            if (Pos2Adr(pos) == stopAdr) break;
            DWORD vAdr = *((DWORD*)(Code + pos));
            pos = MakeDword(pos);
            MakeFunction(vAdr);
            if (vAdr && vAdr < CCadr) CCadr = vAdr;
        }
        CCadr--;
        MakeByte(Adr2Pos(CCadr));
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputAutoTable(int pos)
{
    MakeDword(pos);
    DWORD autoTable = *((DWORD*)(Code + pos));
    if (autoTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", autoTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_AutoTable\", 0);\n", autoTable, itemName.c_str());
        pos = Adr2Pos(autoTable);
        //EntryCount
        DWORD EntryCount = *((DWORD*)(Code + pos));
        pos = MakeDword(pos);
        for (int n = 0; n < EntryCount; n++)
        {
            //DispID
            pos = MakeDword(pos);
            //NameAdr
            pos = MakeDword(pos);
            //Flags
            pos = MakeDword(pos);
            //ParamsAdr
            OutputAutoPTable(pos); pos += 4;
            //ProcAdr
            //DWORD procAdr = *((DWORD*)(Code + pos));
            pos = MakeDword(pos);
            //MakeFunction(procAdr);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputAutoPTable(int pos)
{
    MakeDword(pos);
    DWORD paramsAdr = *((DWORD*)(Code + pos));
    if (paramsAdr)
    {
        pos = Adr2Pos(paramsAdr);
        BYTE paramCnt = Code[pos + 1];
        MakeArray(Adr2Pos(paramsAdr), paramCnt + 2);
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputInitTable(int pos)
{
    MakeDword(pos);
    DWORD initTable = *((DWORD*)(Code + pos));
    if (initTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", initTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_InitTable\", 0);\n", initTable, itemName.c_str());
        pos = Adr2Pos(initTable);
        //0xE
        pos = MakeByte(pos);
        //Unknown byte
        pos = MakeByte(pos);
        //Unknown dword
        pos = MakeDword(pos);
        //num
        DWORD num = *((DWORD*)(Code + pos));
        pos = MakeDword(pos);

        for (int n = 0; n < num; n++)
        {
            //TypeOfs
            pos = MakeDword(pos);
            //FieldOfs
            pos = MakeDword(pos);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputFieldTable(int pos)
{
    MakeDword(pos);
    DWORD fieldTable = *((DWORD*)(Code + pos));
    if (fieldTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", fieldTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_FieldTable\", 0);\n", fieldTable, itemName.c_str());
        pos = Adr2Pos(fieldTable);
        //num
        WORD num = *((WORD*)(Code + pos));
        pos = MakeWord(pos);
        //TypesTab
        OutputFieldTTable(pos); pos += 4;
        for (int n = 0; n < num; n++)
        {
            //Offset
            pos = MakeDword(pos);
            //Idx
            pos = MakeWord(pos);
            //Name
            pos = MakeShortString(pos);
        }
        if (DelphiVersion >= 2010)
        {
            //num
            num = *((WORD*)(Code + pos));
            pos = MakeWord(pos);

            for (int n = 0; n < num; n++)
            {
                //Flags
                pos = MakeByte(pos);
                //TypeRef
                pos = MakeDword(pos);
                //Offset
                pos = MakeDword(pos);
                //Name
                pos = MakeShortString(pos);
                //AttrData
                pos = OutputAttrData(pos);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputFieldTTable(int pos)
{
    MakeDword(pos);
    DWORD typesTab = *((DWORD*)(Code + pos));
    if (typesTab)
    {
        pos = Adr2Pos(typesTab);
        //num
        WORD num = *((WORD*)(Code + pos));
        pos = MakeWord(pos);
        for (int n = 0; n < num; n++)
            pos = MakeDword(pos);
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputMethodTable(int pos)
{
    MakeDword(pos);
    DWORD methodTable = *((DWORD*)(Code + pos));
    if (methodTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", methodTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_MethodTable\", 0);\n", methodTable, itemName.c_str());
        pos = Adr2Pos(methodTable);
        //Count
        WORD count = *((WORD*)(Code + pos));
        pos = MakeWord(pos);

        for (int n = 0; n < count; n++)
        {
            //Len
            WORD len = *((WORD*)(Code + pos));
            int endpos = pos + len;
            pos = MakeWord(pos);
            //CodeAddress
            //DWORD codeAdr = *((WORD*)(Code + pos));
            pos = MakeDword(pos);
            //MakeFunction(codeAdr);
            //Name
            pos = MakeShortString(pos);
            //Tail
            if (pos < endpos)
            {
                OutputVmtMethodEntryTail(pos);
                pos = endpos;
            }
        }
        if (DelphiVersion >= 2010)
        {
            //ExCount
            WORD excount = *((WORD*)(Code + pos));
            pos = MakeWord(pos);

            for (int n = 0; n < excount; n++)
            {
                //Entry
                OutputVmtMethodEntry(pos); pos += 4;
                //Flags
                pos = MakeWord(pos);
                //VirtualIndex
                pos = MakeWord(pos);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputVmtMethodEntry(int pos)
{
    MakeDword(pos);
    DWORD entry = *((DWORD*)(Code + pos));
    if (entry)
    {
        pos = Adr2Pos(entry);
        //Len
        WORD len = *((WORD*)(Code + pos));
        int endpos = pos + len;
        pos = MakeWord(pos);
        //CodeAddress
        //DWORD codeAdr = *((DWORD*)(Code + pos));
        pos = MakeDword(pos);
        //MakeFunction(codeAdr);
        //Name
        pos = MakeShortString(pos);
        //Tail
        if (pos < endpos)
            pos = OutputVmtMethodEntryTail(pos);
    }
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::OutputVmtMethodEntryTail(int pos)
{
    //Version
    pos = MakeByte(pos);
    //CC
    pos = MakeByte(pos);
    //ResultType
    pos = MakeDword(pos);
    //ParOff
    pos = MakeWord(pos);
    //ParamCount
    BYTE paramCnt = Code[pos];
    pos = MakeByte(pos);

    for (int n = 0; n < paramCnt; n++)
    {
        //Flags
        pos = MakeByte(pos);
        //ParamType
        pos = MakeDword(pos);
        //ParOff
        pos = MakeWord(pos);
        //Name
        pos = MakeShortString(pos);
        //AttrData
        pos = OutputAttrData(pos);
    }
    //AttrData
    return OutputAttrData(pos);
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputDynamicTable(int pos)
{
    MakeDword(pos);
    DWORD dynamicTable = *((DWORD*)(Code + pos));
    if (dynamicTable)
    {
        CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", dynamicTable);
        CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_DynamicTable\", 0);\n", dynamicTable, itemName.c_str());
        pos = Adr2Pos(dynamicTable);
        //Num
        WORD num = *((WORD*)(Code + pos));
        pos = MakeWord(pos);

        for (int n = 0; n < num; n++)
        {
            //Msg
            pos = MakeWord(pos);
        }
        for (int n = 0; n < num; n++)
        {
            //ProcAddress
            pos = MakeDword(pos);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputResString(int pos, PInfoRec recN)
{
    itemName = recN->GetName();
    MakeComment(pos, itemName);
    pos = MakeDword(pos);
    pos = MakeDword(pos);
}
//---------------------------------------------------------------------------
int __fastcall TIDCGen::OutputProc(int pos, PInfoRec recN, bool imp)
{
    itemName = recN->GetName();
    int   fromPos = pos;
    DWORD fromAdr = Pos2Adr(pos);

    if (itemName != "")
    {
        int idx = names->IndexOf(itemName);
        if (idx == -1)
        {
            names->Add(itemName);
            CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", fromAdr);
            CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s\", 0x20);\n", fromAdr, itemName.c_str());
            //CurrentBytes += fprintf(idcF, "ApplyType(0x%lX, \"%s\", 0);\n", fromAdr, recN->MakeIDCPrototype(...));
        }
        else
        {
            PREPNAMEINFO info = GetNameInfo(idx);
            if (!info)
            {
                info = new REPNAMEINFO;
                info->index = idx;
                info->counter = 0;
                repeated->Add((void*)info);
            }
            int cnt = info->counter;
            info->counter++;
            CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", fromAdr);
            CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_%d\", 0x20);\n", fromAdr, itemName.c_str(), cnt);
            //CurrentBytes += fprintf(idcF, "ApplyType(0x%lX, \"%s_%d\", 0);\n", fromAdr, recN->MakeIDCPrototype(...), cnt);
        }
        MakeComment(pos, recN->MakePrototype(fromAdr, true, false, false, true, false));
    }
    int _procSize = GetProcSize(fromAdr);
    //If no procedure just return 0;
    if (!_procSize) return 0;
    
    int instrLen = MakeCode(pos);
    if (imp || _procSize == instrLen)
    {
        CurrentBytes += fprintf(idcF, "MakeFunction(0x%lX, 0x%lX);\n", fromAdr, fromAdr + instrLen);
        return instrLen - 1;//= procSize - 1
    }

    while (1)
    {
        if (pos - fromPos + 1 == _procSize)
        {
            CurrentBytes += fprintf(idcF, "MakeFunction(0x%lX, 0x%lX);\n", fromAdr, Pos2Adr(pos) + 1);
            break;
        }

        PInfoRec recN1 = GetInfoRec(Pos2Adr(pos));
        if (recN1 && recN1->picode) MakeComment(pos, FMain_11011981->MakeComment(recN1->picode));

        if (IsFlagSet(cfExcept | cfFinally, pos))
        {
            MakeCode(pos);
            pos++;
            continue;
        }

        if (IsFlagSet(cfETable, pos))
        {
            DWORD num = *((DWORD*)(Code + pos));
            pos = MakeDword(pos);
            for (int n = 0; n < num; n++)
            {
                pos = MakeDword(pos);   //ExceptionInfo
                pos = MakeDword(pos);   //ExceptionProc
            }
            continue;
        }
        if (IsFlagSet(cfLoc, pos) && (pos != fromPos))
        {
            MakeCode(pos);
            pos++;
            continue;
        }
        pos++;
    }
    return pos - fromPos;//= procSize - 1
}
//---------------------------------------------------------------------------
void __fastcall TIDCGen::OutputData(int pos, PInfoRec recN)
{
    if (recN->HasName())
    {
        MakeByte(pos);
        if (recN->type == ""                  ||
            (!SameText(recN->type, "Single")  &&
            !SameText(recN->type, "Double")   &&
            !SameText(recN->type, "Extended") &&
            !SameText(recN->type, "Comp")     &&
            !SameText(recN->type, "Currency")))
        {
            String _name = recN->GetName();
            int idx = names->IndexOf(_name);
            DWORD adr = Pos2Adr(pos);
            if (idx == -1)
            {
                names->Add(_name);
                CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", adr);
                CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s\", 0);\n", adr, _name.c_str());
            }
            else
            {
                PREPNAMEINFO info = GetNameInfo(idx);
                if (!info)
                {
                    info = new REPNAMEINFO;
                    info->index = idx;
                    info->counter = 0;
                    repeated->Add((void*)info);
                }
                int cnt = info->counter;
                info->counter++;
                CurrentBytes += fprintf(idcF, "MakeUnkn(0x%lX, 1);\n", adr);
                CurrentBytes += fprintf(idcF, "MakeNameEx(0x%lX, \"%s_%d\", 0);\n", adr, _name.c_str(), cnt);
            }
        }
        if (recN->type != "") MakeComment(pos, recN->type.c_str());
    }
}
//---------------------------------------------------------------------------
PREPNAMEINFO __fastcall TIDCGen::GetNameInfo(int idx)
{
    int num = repeated->Count;
    for (int n = 0; n < num; n++)
    {
        PREPNAMEINFO info = (PREPNAMEINFO)repeated->Items[n];
        if (info->index == idx) return info;
    }
    return 0;
}
//---------------------------------------------------------------------------
__fastcall TSaveIDCDialog::TSaveIDCDialog(TComponent* AOwner, char* TemplateName) : TOpenDialog(AOwner)
{
	Options >> ofEnableSizing;
    Template = TemplateName;
    CheckDlgButton(Handle, 101, SplitIDC ? BST_CHECKED : BST_UNCHECKED);
}
//---------------------------------------------------------------------------
void __fastcall TSaveIDCDialog::WndProc(TMessage& Message)
{
    switch (Message.Msg)
    {
    case WM_COMMAND:
        switch (Message.WParamLo)
        {
        case 101:
            if (IsDlgButtonChecked(Handle, 101) == BST_CHECKED)
                SplitIDC = true;
            else
                SplitIDC = false;
            break;
        };
        break;
    };
    TOpenDialog::WndProc(Message);
};
//---------------------------------------------------------------------------
