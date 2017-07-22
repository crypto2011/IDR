//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "TypeInfo.h"
#include "Main.h"
#include "Misc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

extern  DWORD       ImageBase;
extern  DWORD       CodeBase;
extern  DWORD       *Flags;
extern  PInfoRec    *Infos;
extern  BYTE        *Code;
extern  int         DelphiVersion;
extern  TList       *TypeList;
extern  MKnowledgeBase  KnowledgeBase;

TFTypeInfo_11011981 *FTypeInfo_11011981;
//---------------------------------------------------------------------------
__fastcall TFTypeInfo_11011981::TFTypeInfo_11011981(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFTypeInfo_11011981::ShowKbInfo(MTypeInfo* tInfo)
{
    WORD    Len;
    String  line;

    memDescription->ReadOnly = True;
    Panel1->Visible = False;

    if (tInfo->ModuleID != 0xFFFF)
        Caption = KnowledgeBase.GetModuleName(tInfo->ModuleID) + ".";
    Caption = Caption + tInfo->TypeName;
    if (tInfo->Size)
        Caption = Caption + " (size = " + Val2Str0(tInfo->Size) + ")";

    memDescription->Clear();
    if (tInfo->Decl != "") memDescription->Lines->Add(tInfo->Decl);
    if (tInfo->FieldsNum)
    {
        memDescription->Lines->Add("//FIELDS//");
        BYTE *p = tInfo->Fields;
        FIELDINFO fInfo;

        for (int n = 0; n < tInfo->FieldsNum; n++)
        {
            fInfo.Scope = *p; p++;
            fInfo.Offset = *((int*)p); p += 4;
            fInfo.Case = *((int*)p); p += 4;
            Len = *((WORD*)p); p += 2;
            fInfo.Name = String((char*)p, Len); p += Len + 1;
            Len = *((WORD*)p); p += 2;
            fInfo.Type = TrimTypeName(String((char*)p, Len)); p += Len + 1;

            line = "f" + Val2Str0(fInfo.Offset) + " ";

            switch (fInfo.Scope)
            {
            case 9:
                line += "private";
                break;
            case 10:
                line += "protected";
                break;
            case 11:
                line += "public";
                break;
            case 12:
                line += "published";
                break;
            }
            if (fInfo.Case != -1) line += " case " + String(fInfo.Case);
            line += " " + fInfo.Name + ":" + fInfo.Type;
            memDescription->Lines->Add(line);
        }
    }
    if (tInfo->PropsNum)
    {
        memDescription->Lines->Add("//PROPERTIES//");
        BYTE *p = tInfo->Props;
        PROPINFO pInfo;

        for (int n = 0; n < tInfo->PropsNum; n++)
        {
            pInfo.Scope = *p; p++;
            pInfo.Index = *((int*)p); p += 4;
            pInfo.DispID = *((int*)p); p += 4;
            Len = *((WORD*)p); p += 2;
            pInfo.Name = String((char*)p, Len); p += Len + 1;
            Len = *((WORD*)p); p += 2;
            pInfo.TypeDef = String((char*)p, Len); p += Len + 1;
            Len = *((WORD*)p); p += 2;
            pInfo.ReadName = String((char*)p, Len); p += Len + 1;
            Len = *((WORD*)p); p += 2;
            pInfo.WriteName = String((char*)p, Len); p += Len + 1;
            Len = *((WORD*)p); p += 2;
            pInfo.StoredName = String((char*)p, Len); p += Len + 1;

            line = "";

            switch (pInfo.Scope)
            {
            case 9:
                line += "private";
                break;
            case 10:
                line += "protected";
                break;
            case 11:
                line += "public";
                break;
            case 12:
                line += "published";
                break;
            }

            line += " " + pInfo.Name + ":" + pInfo.TypeDef;
            line += " " + pInfo.ReadName;
            line += " " + pInfo.WriteName;
            line += " " + pInfo.StoredName;

            if (pInfo.Index != -1)
            {
                switch (pInfo.Index & 6)
                {
                case 2:
                    line += " read";
                    break;
                case 4:
                    line += " write";
                    break;
                }
            }

            memDescription->Lines->Add(line);
        }
    }
    if (tInfo->MethodsNum)
    {
        memDescription->Lines->Add("//METHODS//");
        BYTE *p = tInfo->Methods;
        METHODINFO mInfo;

        for (int n = 0; n < tInfo->MethodsNum; n++)
        {
            mInfo.Scope = *p; p++;
            mInfo.MethodKind = *p; p++;
            Len = *((WORD*)p); p += 2;
            mInfo.Prototype = String((char*)p, Len); p += Len + 1;

            line = "";

            switch (mInfo.Scope)
            {
            case 9:
                line += "private";
                break;
            case 10:
                line += "protected";
                break;
            case 11:
                line += "public";
                break;
            case 12:
                line += "published";
                break;
            }

            line += " " + mInfo.Prototype;
            memDescription->Lines->Add(line);
        }
    }
    if (tInfo->Dump)
    {
		memDescription->Lines->Add("//Dump//");
        BYTE *p = tInfo->Dump; line = "";
        for (int n = 0; n < tInfo->DumpSz; n++)
        {
        	if (n) line += " ";
        	line += Val2Str2((int)*p); p++;
        }
        memDescription->Lines->Add(line);
    }
    ShowModal();
}
//---------------------------------------------------------------------------
String __fastcall GetVarTypeString(int val)
{
    String  res = "";
    switch (val)
    {
    case 0:
        res = "Empty";
        break;
    case 1:
        res = "Null";
        break;
    case 2:
        res = "Smallint";
        break;
    case 3:
        res = "Integer";
        break;
    case 4:
        res = "Single";
        break;
    case 5:
        res = "Double";
        break;
    case 6:
        res = "Currency";
        break;
    case 7:
        res = "Date";
        break;
    case 8:
        res = "OleStr";
        break;
    case 9:
        res = "Dispatch";
        break;
    case 0xA:
        res = "Error";
        break;
    case 0xB:
        res = "Boolean";
        break;
    case 0xC:
        res = "Variant";
        break;
    case 0xD:
        res = "Unknown";
        break;
    case 0xE:
        res = "Decimal";
        break;
    case 0xF:
        res = "Undef0F";
        break;
    case 0x10:
        res = "ShortInt";
        break;
    case 0x11:
        res = "Byte";
        break;
    case 0x12:
        res = "Word";
        break;
    case 0x13:
        res = "LongWord";
        break;
    case 0x14:
        res = "Int64";
        break;
    case 0x15:
        res = "Word64";
        break;
    case 0x48:
        res = "StrArg";
        break;
    case 0x100:
        res = "String";
        break;
    case 0x101:
        res = "Any";
        break;
    case 0xFFF:
        res = "TypeMask";
        break;
    case 0x2000:
        res = "Array";
        break;
    case 0x4000:
        res = "ByRef";
        break;
    }
    return res;
}
//---------------------------------------------------------------------------
String __fastcall TFTypeInfo_11011981::GetRTTI(DWORD adr)
{
    bool        found;
    BYTE        floatType, methodKind, paramCount, numOps, ordType, callConv, paramFlags, propFlags, flags, dimCount;
    WORD        dw, propCount, methCnt;
    DWORD       minValue, maxValue, minValueB, maxValueB;
    int         i, m, n, vmtofs, pos, posn, spos, _ap;
    __int64     minInt64Value, maxInt64Value;
    int         elSize, varType;    //for tkDynArray
    DWORD       elType;             //for tkDynArray
    DWORD       typeAdr, classVMT, parentAdr, Size, elNum, elOff, resultTypeAdr;
    DWORD       propType, getProc, setProc, storedProc, methAdr, procSig;
    BYTE        GUID[16];
    String		typname, name, FldFileName;
    FILE        *fldf;
    PInfoRec    recN;

    RTTIAdr = adr;
    _ap = Adr2Pos(RTTIAdr); pos = _ap;
    pos += 4;
    RTTIKind = Code[pos]; pos++;
    BYTE len = Code[pos]; pos++;
    RTTIName = String((char*)(Code + pos), len); pos += len;
    Caption = RTTIName;
    String result = "", proto;

    switch (RTTIKind)
    {
    case ikUnknown:
        break;
    case ikInteger:
    case ikChar:
    case ikWChar:
        ordType = Code[pos]; pos++;
        minValue = *((DWORD*)(Code + pos)); pos += 4;
        maxValue = *((DWORD*)(Code + pos));
        //Signed type
        if (!(ordType & 1))
            result = IntToStr((int)minValue) + ".." + IntToStr((int)maxValue);
        //Unsigned type
        else
            result = "$" + IntToHex((__int64)minValue, 0) + ".." + "$" + IntToHex((__int64)maxValue, 0);
        break;
    case ikEnumeration:
        result = "(";
        ordType = Code[pos]; pos++;
        minValue = *((DWORD*)(Code + pos)); pos += 4;
        maxValue = *((DWORD*)(Code + pos)); pos += 4;
        //BaseTypeAdr
        typeAdr = *((DWORD*)(Code + pos)); pos += 4;

        if (SameText(RTTIName, "ByteBool") ||
            SameText(RTTIName, "WordBool") ||
            SameText(RTTIName, "LongBool"))
        {
            minValue = 0;
            maxValue = 1;
        }

        //If BaseTypeAdr != SelfAdr then fields extracted from BaseType
        if (typeAdr != RTTIAdr)
        {
            pos = Adr2Pos(typeAdr);
            pos += 4;   //SelfPointer
            pos++;      //typeKind
            len = Code[pos]; pos++;
            pos += len; //BaseClassName
            pos++;      //ordType
            minValueB = *((DWORD*)(Code + pos)); pos += 4;
            maxValueB = *((DWORD*)(Code + pos)); pos += 4;
            pos += 4;   //BaseClassPtr
        }
        else
        {
            minValueB = minValue;
            maxValueB = maxValue;
        }

        for (i = minValueB; i <= maxValueB; i++)
        {
            len = Code[pos]; pos++;
            if (i >= minValue && i <= maxValue)
            {
                name = String((char*)(Code + pos), len);
                if (i != minValue) result += ", ";
                result += name;
            }
            pos += len;
        }
        result += ")";
        //UnitName
        len = Code[pos];
        if (IsValidName(len, pos + 1))
        {
            pos++;
            Caption = String((char*)(Code + pos), len).Trim() + "." + Caption;
        }
        break;
    case ikFloat:
        if (SameText(RTTIName, "Single")   ||
            SameText(RTTIName, "Double")   ||
            SameText(RTTIName, "Extended") ||
            SameText(RTTIName, "Comp")     ||
            SameText(RTTIName, "Currency"))
            result = "float";
        else
        {
            floatType = Code[pos]; pos++;
            result = "type ";
            switch (floatType)
            {
            case FtSingle:
                result += "Single";
                break;
            case FtDouble:
                result += "Double";
                break;
            case FtExtended:
                result += "Extended";
                break;
            case FtComp:
                result += "Comp";
                break;
            case FtCurr:
                result += "Currency";
                break;
            }
        }
        break;
    case ikString:
        result = "String";
        break;
    case ikSet:
        result = "set of ";
        pos++;  //skip ordType
        typeAdr = *((DWORD*)(Code + pos));
        result = "set of " + GetTypeName(typeAdr);
        break;
    case ikClass:
        result = "class";
        classVMT = *((DWORD*)(Code + pos)); pos += 4;
        parentAdr = *((DWORD*)(Code + pos)); pos += 4;
        if (parentAdr) result += "(" + GetTypeName(parentAdr) + ")";
        propCount = *((WORD*)(Code + pos)); pos += 2;
        
        //UnitName
        len = Code[pos]; pos++;
        Caption = String((char*)(Code + pos), len).Trim() + "." + Caption;
        pos += len;

        propCount = *((WORD*)(Code + pos)); pos += 2;
        for (i = 0; i < propCount; i++)
        {
            propType = *((DWORD*)(Code + pos)); pos += 4;
            posn = Adr2Pos(propType); posn += 4;
            posn++; //property type
            len = Code[posn]; posn++;
            typname = String((char*)(Code + posn), len);
            getProc = *((DWORD*)(Code + pos)); pos += 4;
            setProc = *((DWORD*)(Code + pos)); pos += 4;
            storedProc = *((DWORD*)(Code + pos)); pos += 4;
            //idx
            pos += 4;
            //defval
            pos += 4;
            //nameIdx
            pos += 2;
            len = Code[pos]; pos++;
            name = String((char*)(Code + pos), len); pos += len;
            result += "\n" + name + ":" + typname;
            if ((getProc & 0xFFFFFF00))
            {
                result += "\n read ";
                if ((getProc & 0xFF000000) == 0xFF000000)
                    result += name + " f" + Val2Str0(getProc & 0x00FFFFFF);
                else if ((getProc & 0xFF000000) == 0xFE000000)
                {
                    if ((getProc & 0x00008000))
                        vmtofs = -((int)getProc & 0x0000FFFF);
                    else
                        vmtofs = getProc & 0x0000FFFF;
                    posn = Adr2Pos(classVMT) + vmtofs;
                    getProc = *((DWORD*)(Code + posn));

                    result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(getProc);
                    recN = GetInfoRec(getProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
                else
                {
                    result += Val2Str0(getProc);
                    recN = GetInfoRec(getProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
            }
            if ((setProc & 0xFFFFFF00))
            {
                result += "\n write ";
                if ((setProc & 0xFF000000) == 0xFF000000)
                    result += name + " f" + Val2Str0(setProc & 0x00FFFFFF);
                else if ((setProc & 0xFF000000) == 0xFE000000)
                {
                    if ((setProc & 0x00008000))
                        vmtofs = -((int)setProc & 0x0000FFFF);
                    else \
                        vmtofs = setProc & 0x0000FFFF;
                    posn = Adr2Pos(classVMT) + vmtofs;
                    setProc = *((DWORD*)(Code + posn));
                    result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(setProc);
                    recN = GetInfoRec(setProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
                else
                {
                    result += Val2Str0(setProc);
                    recN = GetInfoRec(setProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
            }
            if ((storedProc & 0xFFFFFF00))
            {
                result += "\n stored ";
                if ((storedProc & 0xFF000000) == 0xFF000000)
                    result += name + " f" + Val2Str0(storedProc & 0x00FFFFFF);
                else if ((storedProc & 0xFF000000) == 0xFE000000)
                {
                    if ((storedProc & 0x00008000))
                        vmtofs = -((int)storedProc & 0x0000FFFF);
                    else
                        vmtofs = storedProc & 0x0000FFFF;
                    posn = Adr2Pos(classVMT) + vmtofs;
                    storedProc = *((DWORD*)(Code + posn));
                    result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(storedProc);
                    recN = GetInfoRec(storedProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
                else
                {
                    result += Val2Str0(storedProc);
                    recN = GetInfoRec(storedProc);
                    if (recN && recN->HasName()) result += " " + recN->GetName();
                }
            }
        }
        if (DelphiVersion >= 2010)
        {
            propCount = *((WORD*)(Code + pos)); pos += 2;
            for (i = 0; i < propCount; i++)
            {
                //Flags
                propFlags = Code[pos]; pos++;
                //PPropInfo
                propType = *((DWORD*)(Code + pos)); pos += 4;
                //AttrData
                dw = *((WORD*)(Code + pos));
                pos += dw;//ATR!!
                spos = pos;
                //PropInfo
                pos = Adr2Pos(propType);
                propType = *((DWORD*)(Code + pos)); pos += 4;

                if (IsFlagSet(cfImport, Adr2Pos(propType)))
                {
                    recN = GetInfoRec(propType);
                    typname = recN->GetName();
                }
                else
                {
                    posn = Adr2Pos(propType);
                    posn += 4;
                    posn++; //property type
                    len = Code[posn]; posn++;
                    typname = String((char*)(Code + posn), len);
                }

                getProc = *((DWORD*)(Code + pos)); pos += 4;
                setProc = *((DWORD*)(Code + pos)); pos += 4;
                storedProc = *((DWORD*)(Code + pos)); pos += 4;
                //idx
                pos += 4;
                //defval
                pos += 4;
                //nameIdx
                pos += 2;
                len = Code[pos]; pos++;
                name = String((char*)(Code + pos), len); pos += len;
                result += "\n" + name + ":" + typname;
                if ((getProc & 0xFFFFFF00))
                {
                    result += "\n read ";
                    if ((getProc & 0xFF000000) == 0xFF000000)
                        result += name + " f" + Val2Str0(getProc & 0x00FFFFFF);
                    else if ((getProc & 0xFF000000) == 0xFE000000)
                    {
                        if ((getProc & 0x00008000))
                            vmtofs = -((int)getProc & 0x0000FFFF);
                        else
                            vmtofs = getProc & 0x0000FFFF;
                        posn = Adr2Pos(classVMT) + vmtofs;
                        getProc = *((DWORD*)(Code + pos));

                        result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(getProc);
                        recN = GetInfoRec(getProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                    else
                    {
                        result += Val2Str0(getProc);
                        recN = GetInfoRec(getProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                }
                if ((setProc & 0xFFFFFF00))
                {
                    result += "\n write ";
                    if ((setProc & 0xFF000000) == 0xFF000000)
                        result += name + " f" + Val2Str0(setProc & 0x00FFFFFF);
                    else if ((setProc & 0xFF000000) == 0xFE000000)
                    {
                        if ((setProc & 0x00008000))
                            vmtofs = -((int)setProc & 0x0000FFFF);
                        else
                            vmtofs = setProc & 0x0000FFFF;
                        posn = Adr2Pos(classVMT) + vmtofs;
                        setProc = *((DWORD*)(Code + pos));
                        result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(setProc);
                        recN = GetInfoRec(setProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                    else
                    {
                        result += Val2Str0(setProc);
                        recN = GetInfoRec(setProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                }
                if ((storedProc & 0xFFFFFF00))
                {
                    result += "\n stored ";
                    if ((storedProc & 0xFF000000) == 0xFF000000)
                        result += name + " f" + Val2Str0(storedProc & 0x00FFFFFF);
                    else if ((storedProc & 0xFF000000) == 0xFE000000)
                    {
                        if ((storedProc & 0x00008000))
                            vmtofs = -((int)storedProc & 0x0000FFFF);
                        else
                            vmtofs = storedProc & 0x0000FFFF;
                        posn = Adr2Pos(classVMT) + vmtofs;
                        storedProc = *((DWORD*)(Code + pos));
                        result += " vmt" + Val2Str0(vmtofs) + " " + Val2Str0(storedProc);
                        recN = GetInfoRec(storedProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                    else
                    {
                        result += Val2Str0(storedProc);
                        recN = GetInfoRec(storedProc);
                        if (recN && recN->HasName()) result += " " + recN->GetName();
                    }
                }
                pos = spos;
            }
            //AttrData
            dw = *((WORD*)(Code + pos));
            pos += dw;//ATR!!
            if (DelphiVersion >= 2012)
            {
                //ArrayPropCount
                propCount = *((WORD*)(Code + pos)); pos += 2;
                for (i = 0; i < propCount; i++)
                {
                    //Flags
                    pos++;
                    //ReadIndex
                    pos += 2;
                    //WriteIndex
                    pos += 2;
                    //Name
                    len = Code[pos]; pos++;
                    name = String((char*)(Code + pos), len); pos += len;
                    result += "\n" + name;
                    //AttrData
                    dw = *((WORD*)(Code + pos));
                    pos += dw;//ATR!!
                }
            }
        }
        break;
    case ikMethod:
        methodKind = Code[pos]; pos++;
        switch (methodKind)
        {
        case MkProcedure:
            result = "procedure";
            break;
        case MkFunction:
            result = "function";
            break;
        case MkConstructor:
            result = "constructor";
            break;
        case MkDestructor:
            result = "destructor";
            break;
        case MkClassProcedure:
            result = "class procedure";
            break;
        case MkClassFunction:
            result = "class function";
            break;
        case 6:
            if (DelphiVersion < 2009)
                result = "safeprocedure";
            else
                result = "class constructor";
            break;
        case 7:
            if (DelphiVersion < 2009)
                result = "safefunction";
            else
                result = "operator overload";
            break;
        }

        paramCount = Code[pos]; pos++;
        if (paramCount > 0) proto = "(";

        for (i = 0; i < paramCount; i++)
        {
            BYTE paramFlags = Code[pos]; pos++;
            if (paramFlags & PfVar)
                proto += "var ";
            else if (paramFlags & PfConst)
                proto += "const ";
            else if (paramFlags & PfArray)
                proto += "array ";
            len = Code[pos]; pos++;
            name = String((char*)(Code + pos), len); pos += len;
            proto += name + ":";
            len = Code[pos]; pos++;
            name = String((char*)(Code + pos), len); pos += len;
            proto += name;
            if (i != paramCount - 1) proto += "; ";
        }
        if (paramCount > 0) proto += ")";
        
        if (methodKind)
        {
            len = Code[pos]; pos++;
            name = String((char*)(Code + pos), len); pos += len;
            if (DelphiVersion > 6)
            {
                typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                name = GetTypeName(typeAdr);
            }
            proto += ":" + name;
        }
        if (DelphiVersion > 6)
        {
            //CC
            callConv = Code[pos]; pos++;
            //ParamTypeRefs
            pos += 4*paramCount;
            if (DelphiVersion >= 2010)
            {
                //MethSig
                procSig = *((DWORD*)(Code + pos)); pos += 4;
                //AttrData
                dw = *((WORD*)(Code + pos));
                pos += dw;//ATR!!
                //Procedure Signature
                if (procSig)
                {
                    if (IsValidImageAdr(procSig))
                        posn = Adr2Pos(procSig);
                    else
                        posn = _ap + procSig;
                    //Flags
                    flags = Code[posn]; posn++;
                    if (flags != 0xFF)
                    {
                        //CC
                        callConv = Code[posn]; posn++;
                        //ResultType
                        resultTypeAdr = *((DWORD*)(Code + posn)); posn += 4;
                        //ParamCount
                        paramCount = Code[posn]; posn++;
                        if (paramCount > 0) proto = "(";
                        for (i = 0; i < paramCount; i++)
                        {
                            BYTE paramFlags = Code[posn]; posn++;
                            if (paramFlags & PfVar)
                                proto += "var ";
                            else if (paramFlags & PfConst)
                                proto += "const ";
                            else if (paramFlags & PfArray)
                                proto += "array ";
                            typeAdr = *((DWORD*)(Code + posn)); posn += 4;
                            len = Code[posn]; posn++;
                            name = String((char*)(Code + posn), len); posn += len;
                            proto += name + ":" + GetTypeName(typeAdr);
                            if (i != paramCount - 1) proto += "; ";
                            //AttrData
                            dw = *((WORD*)(Code + posn));
                            posn += dw;//ATR!!
                        }
                        if (paramCount > 0) proto += ")";
                        if (resultTypeAdr) proto += ":" + GetTypeName(resultTypeAdr);
                    }
                }
            }
        }
        result += proto + " of object;";
        break;
    case ikLString:
        result = "String";
        break;
    case ikWString:
        result = "WideString";
        break;
    case ikVariant:
        result = "Variant";
        break;
    case ikArray:
        Size = *((DWORD*)(Code + pos)); pos += 4;
        elNum = *((DWORD*)(Code + pos)); pos += 4;
        resultTypeAdr = *((DWORD*)(Code + pos)); pos += 4;
        result = "array [1.." + String(elNum);
        if (DelphiVersion >= 2010)
        {
            result = "array [";
            //DimCount
            dimCount = Code[pos]; pos++;
            for (i = 0; i < dimCount; i++)
            {
                typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                if (IsValidImageAdr(typeAdr))
                    result += GetTypeName(typeAdr);
                else
                {
                    if (!typeAdr)
                    {
                        if (dimCount == 1)
                            result += "1.." + String(elNum);
                        else
                            result += "1..?";
                    }
                    else
                        result += "1.." + String(typeAdr);
                }
                if (i != dimCount - 1) result += ",";
            }
        }
        result += "] of " + GetTypeName(resultTypeAdr);
        break;
    case ikRecord:
        Size = *((DWORD*)(Code + pos)); pos += 4;
        result = RTTIName + " = record//size=" + Val2Str0(Size);
        elNum = *((DWORD*)(Code + pos)); pos += 4;  //FldCount
        if (elNum)
        {
            for (i = 0; i < elNum; i++)
            {
                typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                elOff = *((DWORD*)(Code + pos)); pos += 4;
                result += "\nf" + Val2Str0(elOff) + ":" + GetTypeName(typeAdr) + ";//f" + Val2Str0(elOff);
            }
            result += "\nend;";
        }

        if (DelphiVersion >= 2010)
        {
            //NumOps
            numOps = Code[pos]; pos++;
            for (i = 0; i < numOps; i++)    //RecOps
            {
                pos += 4;
            }
            elNum = *((DWORD*)(Code + pos)); pos += 4;  //RecFldCnt

            if (elNum)
            {
                for (i = 0; i < elNum; i++)
                {
                    //TypeRef
                    typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                    //FldOffset
                    elOff = *((DWORD*)(Code + pos)); pos += 4;
                    //Flags
                    pos++;
                    //Name
                    len = Code[pos]; pos++;
                    name = String((char*)(Code + pos), len); pos += len;
                    result += "\n" + name + ":" + GetTypeName(typeAdr) + ";//f" + Val2Str0(elOff);
                    //AttrData
                    dw = *((WORD*)(Code + pos));
                    pos += dw;//ATR!!
                }
                result += "\nend;";
            }
            //AttrData
            dw = *((WORD*)(Code + pos));
            pos += dw;//ATR!!
            if (DelphiVersion >= 2012)
            {
                methCnt = *((WORD*)(Code + pos)); pos += 2;
                if (methCnt > 0) result += "\n//Methods:";
                for (m = 0; m < methCnt; m++)
                {
                    //Flags
                    pos++;
                    //Code
                    methAdr = *((DWORD*)(Code + pos)); pos += 4;
                    //Name
                    len = Code[pos]; pos++;
                    name = String((char*)(Code + pos), len); pos += len;
                    result += "\n" + name;
                    //ProcedureSignature
                    //Flags
                    flags = Code[pos]; pos++;
                    if (flags != 0xFF)
                    {
                        //CC
                        pos++;
                        //ResultType
                        resultTypeAdr = *((DWORD*)(Code + pos)); pos += 4;
                        //ParamCnt
                        paramCount = Code[pos]; pos++;
                        if (paramCount > 0) result += "(";
                        //Params
                        for (n = 0; n < paramCount; n++)
                        {
                            //Flags
                            pos++;
                            //ParamType
                            typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                            //Name
                            len = Code[pos]; pos++;
                            name = String((char*)(Code + pos), len); pos += len;
                            result += name + ":" + GetTypeName(typeAdr);
                            if (n != paramCount - 1) result += ";";
                            //AttrData
                            dw = *((WORD*)(Code + pos));
                            pos += dw;//ATR!!
                        }
                        if (paramCount > 0) result += ")";
                        if (resultTypeAdr) result += ":" + GetTypeName(resultTypeAdr);
                        result += ";//" + Val2Str8(methAdr);
                    }
                    //AttrData
                    dw = *((WORD*)(Code + pos));
                    pos += dw;//ATR!!
                }
            }
        }
        break;
    case ikInterface:
        result = "interface";
        //IntfParent
        typeAdr = *((DWORD*)(Code + pos)); pos += 4;
        if (typeAdr) result += "(" + GetTypeName(typeAdr) + ")";
        //IntfFlags
        pos++;
        //GUID
        memmove(GUID, &Code[pos], 16); pos += 16;
        result += Guid2String(GUID) + "\n";

        //UnitName
        len = Code[pos]; pos++;
        Caption = String((char*)(Code + pos), len).Trim() + "." + Caption;
        pos += len;

        //Methods
        propCount = *((WORD*)(Code + pos)); pos += 2;
        result += "Methods Count = " + String(propCount);
        if (DelphiVersion >= 6)
        {
            //RttiCount
            dw = *((WORD*)(Code + pos)); pos += 2;
            if (dw != 0xFFFF)
            {
                if (DelphiVersion >= 2010)
                {
                    for (i = 0; i < propCount; i++)
                    {
                        //Name
                        len = Code[pos]; pos++;
                        result += "\n" + String((char*)(Code + pos), len); pos += len;
                        //Kind
                        methodKind = Code[pos]; pos++;
                        //CallConv
                        pos++;
                        //ParamCount
                        paramCount = Code[pos]; pos++;
                        if (paramCount) result += "(";
                        for (m = 0; m < paramCount; m++)
                        {
                            if (m) result += ";";
                            //Flags
                            pos++;
                            //ParamName
                            len = Code[pos]; pos++;
                            result += String((char*)(Code + pos), len); pos += len;
                            //TypeName
                            len = Code[pos]; pos++;
                            result += ":" + String((char*)(Code + pos), len); pos += len;
                            //ParamType
                            pos += 4;
                        }
                        if (paramCount) result += ")";
                        if (methodKind)
                        {
                            result += ":";
                            //ResultTypeName
                            len = Code[pos]; pos++;
                            result += String((char*)(Code + pos), len); pos += len;
                            if (len)
                            {
                                //ResultType
                                pos += 4;
                            }
                        }
                    }
                }
                else
                {
                    for (i = 0; i < propCount; i++)
                    {
                        //PropType
                        pos += 4;
                        //GetProc
                        pos += 4;
                        //SetProc
                        pos += 4;
                        //StoredProc
                        pos += 4;
                        //Index
                        pos += 4;
                        //Default
                        pos += 4;
                        //NameIndex
                        pos += 2;
                        //Name
                        len = Code[pos]; pos++;
                        result += "\n" + String((char*)(Code + pos), len); pos += len;
                    }
                }
            }
        }
        break;
    case ikInt64:
        minInt64Value = *((__int64*)(Code + pos)); pos += sizeof(__int64);
        maxInt64Value = *((__int64*)(Code + pos));
        result = IntToStr(minInt64Value) + ".." + IntToStr(maxInt64Value);
        break;
    case ikDynArray:
        //elSize
        elSize = *((DWORD*)(Code + pos)); pos += 4;
        //elType
        elType = *((DWORD*)(Code + pos)); pos += 4;
        result = "array of " + GetTypeName(elType);
        //varType
        varType = *((DWORD*)(Code + pos)); pos += 4;
        if (DelphiVersion >= 6)
        {
            //elType2
            pos += 4;
            //UnitName
            len = Code[pos]; pos++;
            Caption = String((char*)(Code + pos), len).Trim() + "." + Caption;
            pos += len;
        }
        if (DelphiVersion >= 2010)
        {
            //DynArrElType
            elType = *((DWORD*)(Code + pos));
            result = "array of " + GetTypeName(elType);
        }
        result += ";";
        if (elSize) result += "\n//elSize = " + Val2Str0(elSize);
        if (varType != -1) result += "\n//varType equivalent: var" + GetVarTypeString(varType);
        break;
    case ikUString:
        result = "UString";
        break;
    case ikClassRef:    //0x13
        typeAdr = *((DWORD*)(Code + pos));
        if (typeAdr) result = "class of " + GetTypeName(typeAdr);
        break;
    case ikPointer:     //0x14
        typeAdr = *((DWORD*)(Code + pos));
        if (typeAdr) result = "^" + GetTypeName(typeAdr);
        break;
    case ikProcedure:   //0x15
        result = RTTIName;
        //MethSig
        procSig = *((DWORD*)(Code + pos)); pos += 4;
        //AttrData
        dw = *((WORD*)(Code + pos));
        pos += dw;//ATR!!
        //Procedure Signature
        if (procSig)
        {
            if (IsValidImageAdr(procSig))
                pos = Adr2Pos(procSig);
            else
                pos = _ap + procSig;
            //Flags
            flags = Code[pos]; pos++;
            if (flags != 0xFF)
            {
                //CallConv
                callConv = Code[pos]; pos++;
                //ResultType
                resultTypeAdr = *((DWORD*)(Code + pos)); pos += 4;
                if (resultTypeAdr)
                    result = "function ";
                else
                    result = "procedure ";
                result += RTTIName;

                //ParamCount
                paramCount = Code[pos]; pos++;

                if (paramCount) result += "(";
                for (i = 0; i < paramCount; i++)
                {
                    //Flags
                    paramFlags = Code[pos]; pos++;
                    if (paramFlags & 1) result += "var ";
                    if (paramFlags & 2) result += "const ";
                    //ParamType
                    typeAdr = *((DWORD*)(Code + pos)); pos += 4;
                    //Name
                    len = Code[pos]; pos++;
                    result += String((char*)(Code + pos), len) + ":";
                    pos += len;
                    result += GetTypeName(typeAdr);
                    if (i != paramCount - 1) result += "; ";
                    //AttrData
                    dw = *((WORD*)(Code + pos));
                    pos += dw;//ATR!!
                }
                if (paramCount) result += ")";

                if (resultTypeAdr) result += ":" + GetTypeName(resultTypeAdr);
                result += ";";
                switch (callConv)
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
            }
        }
        break;
    }
    return result;
}
//---------------------------------------------------------------------
void __fastcall TFTypeInfo_11011981::ShowRTTI(DWORD adr)
{
    memDescription->ReadOnly = True;
    Panel1->Visible = False;

	String line = GetRTTI(adr);
    if (line != "")
    {
    	memDescription->Clear();
        int len = line.Length();
        int b = 1, e = line.Pos("\n");
        while (1)
        {
            if (e)
            {
                line[e] = ' ';
                memDescription->Lines->Add(line.SubString(b, e - b));
            }
            else
            {
                memDescription->Lines->Add(line.SubString(b, len - b + 1));
                break;
            }
            b = e + 1; e = line.Pos("\n");
        }
        if (RTTIKind == ikDynArray)
        {
            memDescription->ReadOnly = False;
            Panel1->Visible = True;
        }
    	ShowModal();
    }
}
//---------------------------------------------------------------------
String __fastcall Guid2String(BYTE* Guid)
{
    int     n;
    char    sbyte[8];
    String  Result = "['{";

    for (int i = 0; i < 16; i++)
    {
        switch (i)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            n = 3 - i;
            break;
        case 4:
            n = 5;
            break;
        case 5:
            n = 4;
            break;
        case 6:
            n = 7;
            break;
        case 7:
            n = 6;
            break;
        default:
            n = i;
            break;
        }
        if (i == 4 || i == 6 || i == 8 || i == 10) Result += '-';
        sprintf(sbyte, "%02X", Guid[n]);
        Result += String(sbyte);
    }
    Result += "}']";
    return Result;
}
//---------------------------------------------------------------------
void __fastcall TFTypeInfo_11011981::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_ESCAPE) ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFTypeInfo_11011981::bSaveClick(TObject *Sender)
{
    BYTE        len;
    int         pos, p;
    String      line, decl;

    for (int n = 0; n < memDescription->Lines->Count; n++)
    {
        line = memDescription->Lines->Strings[n].Trim();
        if (line == "") continue;
        decl += line;
    }
    decl = decl.Trim();
    p = decl.Pos(";");
    if (p > 0)
    {
        decl = decl.SubString(1, p - 1);
    }
    
    pos = Adr2Pos(RTTIAdr);
    pos += 4;
    pos++;//Kind
    len = Code[pos]; pos++;
    pos += len;//Name

    switch (RTTIKind)
    {
    case ikDynArray:
        //elSize
        pos += 4;
        //elType
        *((DWORD*)(Code + pos)) = GetOwnTypeAdr(GetArrayElementType(decl));
        break;
    }
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFTypeInfo_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

