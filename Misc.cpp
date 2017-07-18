//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Misc.h"
#include <Clipbrd.hpp>
#include <Imagehlp.h>
#include "assert.h"
#include "InputDlg.h"
//---------------------------------------------------------------------------
extern  int         dummy;
extern  String      IDRVersion;
extern  String      SelectedAsmItem;
extern  char        StringBuf[MAXSTRBUFFER];
extern  DWORD       ImageBase;
extern  DWORD       ImageSize;
extern  DWORD       TotalSize;
extern  DWORD       CodeBase;
extern  DWORD       CodeSize;
extern  DWORD       DataBase;
extern  DWORD       *Flags;
extern  PInfoRec    *Infos;
extern  TStringList *BSSInfos;
extern  MDisasm     Disasm;
extern  MKnowledgeBase  KnowledgeBase;
extern  BYTE        *Code;
extern  int         DelphiVersion;
extern  TList       *SegmentList;
extern  TList       *OwnTypeList;
extern  TList       *VmtList;
extern  int         VmtSelfPtr;
extern  int         VmtIntfTable;
extern  int         VmtInitTable;
extern  int         VmtParent;
extern  int         VmtClassName;
extern  int         VmtInstanceSize;
extern  char*       Reg8Tab[8];
extern  char*       Reg16Tab[8];
extern  char*       Reg32Tab[8];
extern  char*       SegRegTab[8];
//---------------------------------------------------------------------------
void __fastcall ScaleForm(TForm* AForm)
{
    HDC _hdc = GetDC(0);
    if (_hdc)
    {
        AForm->ScaleBy(GetDeviceCaps(_hdc, 0x58), 100);
        ReleaseDC(0, _hdc);
    }
}
//---------------------------------------------------------------------------
int __fastcall Adr2Pos(DWORD adr)
{
    int     ofs = 0;
    for (int n = 0; n < SegmentList->Count; n++)
    {
        PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
        if (segInfo->Start <= adr && adr < segInfo->Start + segInfo->Size)
        {
            if (segInfo->Flags & 0x80000)
                return -1;
            return ofs + (adr - segInfo->Start);
        }
        if (!(segInfo->Flags & 0x80000))
            ofs += segInfo->Size;
    }
    return -2;
}
//---------------------------------------------------------------------------
DWORD __fastcall Pos2Adr(int Pos)
{
    int     fromPos = 0;
    int     toPos = 0;
    for (int n = 0; n < SegmentList->Count; n++)
    {
        PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
        if (!(segInfo->Flags & 0x80000))
        {
            fromPos = toPos;
            toPos += segInfo->Size;
            if (fromPos <= Pos && Pos < toPos)
                return segInfo->Start + (Pos - fromPos);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//Can replace type "fromName" to type "toName"?
bool __fastcall CanReplace(const String& fromName, const String& toName)
{
	//Skip empty toName
	if (toName == "") return false;
    //We can replace empty "fromName" or name "byte", "word", "dword'
    if (fromName == "" || SameText(fromName, "byte") || SameText(fromName, "word") || SameText(fromName, "dword")) return true;
    return false;
}
//---------------------------------------------------------------------------
String __fastcall GetDefaultProcName(DWORD adr)
{
    return "sub_" + Val2Str8(adr);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str0(DWORD Val)
{
    return IntToHex((int)Val, 0);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str1(DWORD Val)
{
    return IntToHex((int)Val, 1);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str2(DWORD Val)
{
    return IntToHex((int)Val, 2);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str4(DWORD Val)
{
    return IntToHex((int)Val, 4);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str5(DWORD Val)
{
    return IntToHex((int)Val, 5);
}
//---------------------------------------------------------------------------
String __fastcall Val2Str8(DWORD Val)
{
    return IntToHex((int)Val, 8);
}
//---------------------------------------------------------------------------
PInfoRec __fastcall AddToBSSInfos(DWORD Adr, String AName, String ATypeName)
{
    PInfoRec recN;
    String _key = Val2Str8(Adr);
    int _idx = BSSInfos->IndexOf(_key);
    if (_idx == -1)
    {
        recN = new InfoRec(-1, ikData);
        recN->SetName(AName);
        recN->type = ATypeName;
        BSSInfos->AddObject(_key, (TObject*)recN);
    }
    else
    {
        recN = (PInfoRec)BSSInfos->Objects[_idx];
        if (recN->type == "")
        {
            recN->type = ATypeName;
        }
    }
    return recN;
}
//---------------------------------------------------------------------------
String __fastcall MakeGvarName(DWORD adr)
{
    return "gvar_" + Val2Str8(adr);
}
//---------------------------------------------------------------------------
void __fastcall MakeGvar(PInfoRec recN, DWORD adr, DWORD xrefAdr)
{
    if (!recN->HasName()) recN->SetName(MakeGvarName(adr));
    if (xrefAdr) recN->AddXref('C', xrefAdr, 0);
}
//---------------------------------------------------------------------------
void __fastcall FillArgInfo(int k, BYTE callkind, PARGINFO argInfo, BYTE** p, int* s)
{
    BYTE* pp = *p; int ss = *s;
    argInfo->Tag = *pp; pp++;
    int locflags = *((int*)pp); pp += 4;

    if ((locflags & 7) == 1) argInfo->Tag = 0x23; //Add by ZGL
    
    argInfo->Register = (locflags & 8);
    int ndx = *((int*)pp); pp += 4;

    //fastcall
    if (!callkind)
    {
        if (argInfo->Register && k < 3)
        {
            argInfo->Ndx = k;// << 24;???
        }
        else
        {
            argInfo->Ndx = ndx;
        }
    }
    //stdcall, cdecl, pascal
    else
    {
        argInfo->Ndx = ss;
        ss += 4;
    }

    argInfo->Size = 4;
    WORD wlen = *((WORD*)pp); pp += 2;
    argInfo->Name = String((char*)pp, wlen); pp += wlen + 1;
    wlen = *((WORD*)pp); pp += 2;
    argInfo->TypeDef = TrimTypeName(String((char*)pp, wlen)); pp += wlen + 1;
    *p = pp; *s = ss;
}
//---------------------------------------------------------------------------
String __fastcall TrimTypeName(const String& TypeName)
{
    if (TypeName.IsEmpty())
        return TypeName;
    int pos = TypeName.Pos(".");
    //No '.' in TypeName or TypeName begins with '.'
    if (pos == 0 || pos == 1)
        return TypeName;
    //или это имя типа range
    else if (TypeName[pos + 1] == '.')
        return TypeName;
    else
    {
        char c, *p = TypeName.c_str();
        //Check special symbols upto '.'
        while (1)
        {
            c = *p++;
            if (c == '.') break;
            if (c < '0' || c == '<')
                return TypeName;
        }
        return ExtractProcName(TypeName);
    }
}
//---------------------------------------------------------------------------
bool __fastcall IsValidImageAdr(DWORD Adr)
{
    if (Adr >= CodeBase && Adr < CodeBase + ImageSize)
        return true;
    else
        return false;
}
//---------------------------------------------------------------------------
bool __fastcall IsValidCodeAdr(DWORD Adr)
{
    if (Adr >= CodeBase && Adr < CodeBase + CodeSize)
        return true;
    else
        return false;
}
//---------------------------------------------------------------------------
String __fastcall ExtractClassName(const String& AName)
{
    if (AName == "") return "";
    int pos = AName.Pos(".");
    if (pos)
        return AName.SubString(1, pos - 1);
    else
        return "";
}
//---------------------------------------------------------------------------
String __fastcall ExtractProcName(const String& AName)
{
    if (AName == "") return "";
    int pos = AName.Pos(".");
    if (pos)
        return AName.SubString(pos + 1, AName.Length());
    else
        return AName;
}
//---------------------------------------------------------------------------
String __fastcall ExtractName(const String& AName)
{
    if (AName == "") return "";
    int _pos = AName.Pos(":");
    if (_pos)
        return AName.SubString(1, _pos - 1);
    else
        return AName;
}
//---------------------------------------------------------------------------
String __fastcall ExtractType(const String& AName)
{
    if (AName == "") return "";
    int _pos = AName.Pos(":");
    if (_pos)
        return AName.SubString(_pos + 1, AName.Length());
    else
        return "";
}
//---------------------------------------------------------------------------
//Return position of nearest up argument eax from position fromPos
int __fastcall GetNearestArgA(int fromPos)
{
    int         curPos = fromPos;

    for (curPos = fromPos - 1;;curPos--)
    {
        if (IsFlagSet(cfInstruction, curPos))
        {
            if (IsFlagSet(cfProcStart, curPos)) break;
            if (IsFlagSet(cfSetA, curPos)) return curPos;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest up instruction with segment prefix fs:
int __fastcall GetNearestUpPrefixFs(int fromPos)
{
    int         _pos;
    DISINFO     _disInfo;

    assert(fromPos >= 0);
    for (_pos = fromPos - 1; _pos >= 0; _pos--)
    {
        if (IsFlagSet(cfInstruction, _pos))
        {
            Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
            if (_disInfo.SegPrefix == 4) return _pos;
        }
        if (IsFlagSet(cfProcStart, _pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest up instruction from position fromPos
int __fastcall GetNearestUpInstruction(int fromPos)
{
    assert(fromPos >= 0);
    for (int pos = fromPos - 1; pos >= 0; pos--)
    {
        if (IsFlagSet(cfInstruction, pos)) return pos;
        if (IsFlagSet(cfProcStart, pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of N-th up instruction from position fromPos
int __fastcall GetNthUpInstruction(int fromPos, int N)
{
if (fromPos < 0)
return -1;
    assert(fromPos >= 0);
    for (int pos = fromPos - 1; pos >= 0; pos--)
    {
        if (IsFlagSet(cfInstruction, pos))
        {
            N--;
            if (!N) return pos;
        }
        if (IsFlagSet(cfProcStart, pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest up instruction from position fromPos
int __fastcall GetNearestUpInstruction(int fromPos, int toPos)
{
    assert(fromPos >= 0);
    for (int pos = fromPos - 1; pos >= toPos; pos--)
    {
        if (IsFlagSet(cfInstruction, pos)) return pos;
        if (IsFlagSet(cfProcStart, pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
int __fastcall GetNearestUpInstruction(int fromPos, int toPos, int no)
{
    assert(fromPos >= 0);
    for (int pos = fromPos - 1; pos >= toPos; pos--)
    {
        if (IsFlagSet(cfInstruction, pos))
        {
            no--;
            if (!no) return pos;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest up instruction from position fromPos
int __fastcall GetNearestUpInstruction1(int fromPos, int toPos, char* Instruction)
{
    int         len = strlen(Instruction);
    int         pos;
    DISINFO     DisInfo;

    assert(fromPos >= 0);
    for (pos = fromPos - 1; pos >= toPos; pos--)
    {
        if (IsFlagSet(cfInstruction, pos))
        {
            Disasm.Disassemble(Pos2Adr(pos), &DisInfo, 0);
            if (len && !memicmp(DisInfo.Mnem, Instruction, len)) return pos;
        }
        if (IsFlagSet(cfProcStart, pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest up instruction from position fromPos
int __fastcall GetNearestUpInstruction2(int fromPos, int toPos, char* Instruction1, char* Instruction2)
{
    int         len1 = strlen(Instruction1), len2 = strlen(Instruction2);
    int         pos;
    DISINFO     DisInfo;

    assert(fromPos >= 0);
    for (pos = fromPos - 1; pos >= toPos; pos--)
    {
        if (IsFlagSet(cfInstruction, pos))
        {
            Disasm.Disassemble(Pos2Adr(pos), &DisInfo, 0);
            if ((len1 && !memicmp(DisInfo.Mnem, Instruction1, len1)) ||
                (len2 && !memicmp(DisInfo.Mnem, Instruction2, len2))) return pos;
        }
        if (IsFlagSet(cfProcStart, pos)) break;
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return position of nearest down instruction from position fromPos
int __fastcall GetNearestDownInstruction(int fromPos)
{
    int         instrLen;
    DISINFO     DisInfo;

    assert(fromPos >= 0);
    instrLen = Disasm.Disassemble(Pos2Adr(fromPos), &DisInfo, 0);
    if (!instrLen) return -1;
    return fromPos + instrLen;
}
//---------------------------------------------------------------------------
//Return position of nearest down "Instruction" from position fromPos
int __fastcall GetNearestDownInstruction(int fromPos, char* Instruction)
{
    int         instrLen, len = strlen(Instruction);
    int         curPos = fromPos;
    DISINFO     DisInfo;

    assert(fromPos >= 0);
    while (1)
    {
        instrLen = Disasm.Disassemble(Pos2Adr(curPos), &DisInfo, 0);
        if (!instrLen)
        {
            curPos++;
            continue;
        }
        if (len && !memcmp(DisInfo.Mnem, Instruction, len)) return curPos;
        if (DisInfo.Ret) break;
        curPos += instrLen;
    }
    return -1;
}
//---------------------------------------------------------------------------
//-1 - error
//0 - simple if
//1 - jcc down
//2 - jcc up
//3 - jmp down
//4 - jump up
int __fastcall BranchGetPrevInstructionType(DWORD fromAdr, DWORD* jmpAdr, PLoopInfo loopInfo)
{
    int         _pos;
    DISINFO     _disInfo;

    *jmpAdr = 0;
    _pos = GetNearestUpInstruction(Adr2Pos(fromAdr));
    if (_pos == -1) return -1;
    Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
    if (_disInfo.Branch)
    {
        if (IsExit(_disInfo.Immediate)) return 0;
        if (_disInfo.Conditional)
        {
            if (_disInfo.Immediate > CodeBase + _pos)
            {
                if (loopInfo && loopInfo->BreakAdr == _disInfo.Immediate) return 0;
                return 1;
            }
            return 2;
        }
        if (_disInfo.Immediate > CodeBase + _pos)
        {
            *jmpAdr = _disInfo.Immediate;
            return 3;
        }
        //jmp after jmp @HandleFinally
        if (IsFlagSet(cfFinally, _pos))
        {
            _pos = GetNearestUpInstruction(Adr2Pos(_disInfo.Immediate));
            //push Adr
            Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
            if (_disInfo.Immediate == fromAdr) return 0;
            *jmpAdr = _disInfo.Immediate;
            return 3;
        }
        return 4;
    }
    return 0;
}
//---------------------------------------------------------------------------
bool __fastcall IsFlagSet(DWORD flag, int pos)
{
//!!!
if (pos < 0 || pos >= TotalSize)
{
  dummy = 1;
  return false;
}
    assert(pos >= 0 && pos < TotalSize);
    return (Flags[pos] & flag);
}
//---------------------------------------------------------------------------
void __fastcall SetFlag(DWORD flag, int pos)
{
//!!!
if (pos < 0 || pos >= TotalSize)
{
  dummy = 1;
  return;
}
    assert(pos >= 0 && pos < TotalSize);
    Flags[pos] |= flag;
}
//---------------------------------------------------------------------------
void __fastcall SetFlags(DWORD flag, int pos, int num)
{
//!!!
if (pos < 0 || pos + num >= TotalSize)
{
dummy = 1;
return;
}
    assert(pos >= 0 && pos + num < TotalSize);
    for (int i = pos; i < pos + num; i++)
    {
        Flags[i] |= flag;
    }
}
//---------------------------------------------------------------------------
void __fastcall ClearFlag(DWORD flag, int pos)
{
//!!!
if (pos < 0 || pos >= TotalSize)
{
  dummy = 1;
  return;
}
    assert(pos >= 0 && pos < TotalSize);
    Flags[pos] &= ~flag;
}
//---------------------------------------------------------------------------
void __fastcall ClearFlags(DWORD flag, int pos, int num)
{
if (pos < 0 || pos + num > TotalSize)
{
dummy = 1;
return;
}
    assert(pos >= 0 && pos + num <= TotalSize);
    for (int i = pos; i < pos + num; i++)
    {
        Flags[i] &= ~flag;
    }
}
//---------------------------------------------------------------------------
//pInfo must contain pInfo->
int __fastcall GetProcRetBytes(MProcInfo* pInfo)
{
    int     _pos = pInfo->DumpSz - 1;
    DWORD   _curAdr = CodeBase + _pos;
    DISINFO _disInfo;

    while (_pos >= 0)
    {
        Disasm.Disassemble(pInfo->Dump + _pos, (__int64)_curAdr, &_disInfo, 0);
        if (_disInfo.Ret)
        {
            if (_disInfo.OpType[0] == otIMM)//ImmPresent)
                return _disInfo.Immediate;
            else
                return 0;
        }
        _pos--; _curAdr--;
    }
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall GetProcSize(DWORD fromAdr)
{
    int     _size = 0;
    PInfoRec recN = GetInfoRec(fromAdr);
    if (recN && recN->procInfo) _size = recN->procInfo->procSize;
    if (!_size) _size = FMain_11011981->EstimateProcSize(fromAdr);
    return _size;
/*
    int     pos, size = 0;
    for (pos = Adr2Pos(fromAdr); pos < TotalSize; pos++)
    {
        size++;
        if (IsFlagSet(cfProcEnd, pos)) break;
    }
    return size;
*/
}
//---------------------------------------------------------------------------
int __fastcall StrGetRecordSize(String str)
{
    int _size = 0;
    int _bpos = str.Pos("size=");
    int _epos = str.LastDelimiter("\n");
    if (_bpos && _epos)
    {
        String _sz = str.SubString(_bpos + 5, _epos - _bpos - 5);
        _size = StrToInt("$" + _sz);
    }
    return _size;
}
//---------------------------------------------------------------------------
int __fastcall StrGetRecordFieldOffset(String str)
{
    int _offset = -1;
    int _bpos = str.Pos("//");
    int _epos = str.LastDelimiter("\n");
    if (_bpos && _epos)
    {
        String _ofs = str.SubString(_bpos + 2, _epos - _bpos - 2);
        _offset = StrToInt("$" + _ofs);
    }
    return _offset;
}
//---------------------------------------------------------------------------
String __fastcall StrGetRecordFieldName(String str)
{
    String _name = "";
    int _pos = str.Pos(":");
    if (_pos)
        _name = str.SubString(1, _pos - 1);
    return _name;
}
//---------------------------------------------------------------------------
String __fastcall StrGetRecordFieldType(String str)
{
    String _type = "";
    int _epos = str.LastDelimiter(";");
    int _bpos = str.Pos(":");
    if (_bpos && _epos)
        _type = str.SubString(_bpos + 1, _epos - _bpos - 1);
    return _type;
}
//---------------------------------------------------------------------------
int __fastcall GetRecordSize(String AName)
{
    BYTE        _len;
    WORD        *_uses;
    int         _idx, _pos, _size;
    MTypeInfo   _tInfo;
    PTypeRec    _recT;
    String      _str, _sz;

    //File
    String _recFileName = FMain_11011981->WrkDir + "\\types.idr";
    FILE* _recFile = fopen(_recFileName.c_str(), "rt");
    if (_recFile)
    {
        while (1)
        {
            if (!fgets(StringBuf, 1024, _recFile)) break;
            _str = String(StringBuf);
            if (_str.Pos(AName + "=") == 1)
            {
                _size = StrGetRecordSize(_str);
                fclose(_recFile);
                return _size;
            }
        }
        fclose(_recFile);
    }
    //KB
    _uses = KnowledgeBase.GetTypeUses(AName.c_str());
    _idx = KnowledgeBase.GetTypeIdxByModuleIds(_uses, AName.c_str());
    if (_uses) delete[] _uses;

    if (_idx != -1)
    {
        _idx = KnowledgeBase.TypeOffsets[_idx].NamId;
        if (KnowledgeBase.GetTypeInfo(_idx, INFO_DUMP, &_tInfo))
            return _tInfo.Size;
    }
    //RTTI
    _recT = GetOwnTypeByName(AName);
    if (_recT && _recT->kind == ikRecord)
    {
        _pos = Adr2Pos(_recT->adr);
        _pos += 4;//SelfPtr
        _pos++;//TypeKind
        _len = Code[_pos]; _pos += _len + 1;//Name
        return *((DWORD*)(Code + _pos));
    }
    //Manual
    return 0;
}
//---------------------------------------------------------------------------
PFIELDINFO __fastcall GetClassField(String TypeName, int Offset)
{
    int         n, Ofs1, Ofs2;
    DWORD       classAdr, prevClassAdr = 0;
    PInfoRec    recN;
    PFIELDINFO	fInfo1, fInfo2;

    classAdr = GetClassAdr(TypeName);
    while (classAdr && Offset < GetClassSize(classAdr))
    {
        prevClassAdr = classAdr;
        classAdr = GetParentAdr(classAdr);
    }
    classAdr = prevClassAdr;
    if (classAdr)
    {
        recN = GetInfoRec(classAdr);
        if (recN && recN->vmtInfo && recN->vmtInfo->fields)
        {
            for (n = 0; n < recN->vmtInfo->fields->Count; n++)
            {
                fInfo1 = (PFIELDINFO)recN->vmtInfo->fields->Items[n]; Ofs1 = fInfo1->Offset;
                if (n == recN->vmtInfo->fields->Count - 1)
                {
                    Ofs2 = GetClassSize(classAdr);
                }
                else
                {
                    fInfo2 = (PFIELDINFO)recN->vmtInfo->fields->Items[n + 1];
                    Ofs2 = fInfo2->Offset;
                }
                if (Offset >= Ofs1 && Offset < Ofs2)
                {
                    return fInfo1;
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall GetRecordField(String ARecType, int AOfs, String& name, String& type)
{
    bool        _brk;
    BYTE        _len, _numOps, _kind;
    char        *p, *ps;
    WORD        _dw;
    WORD        *_uses;
    int         n, m, k, _idx, _pos, _elNum, Ofs, Ofs1, Ofs2, _case, _fieldsNum, _size;
    DWORD       _typeAdr;
    PTypeRec    _recT;
    MTypeInfo   _tInfo;
    String      _str, _prevstr, _name, _typeName, _ofs, _sz, _result = "";
    int         _fieldOfsets[1024];
    CaseInfo    _cases[256];

    if (ARecType == "") return -1;

    name = ""; type = "";

    _pos = ARecType.LastDelimiter(".");
    if (_pos > 1 && ARecType[_pos + 1] != ':')
        ARecType = ARecType.SubString(_pos + 1, ARecType.Length());

    //File
    String _recFileName = FMain_11011981->WrkDir + "\\types.idr";
    FILE* _recFile = fopen(_recFileName.c_str(), "rt");
    if (_recFile)
    {
        while (1)
        {
            if (!fgets(StringBuf, 1024, _recFile)) break;
            _str = String(StringBuf);
            if (_str.Pos(ARecType + "=") == 1)
            {
                _size = StrGetRecordSize(_str);

                _prevstr = ""; _brk = false;
                //Ofs2 = _size;
                while (1)
                {
                    if (!fgets(StringBuf, 1024, _recFile)) break;
                    _str = String(StringBuf);
                    Ofs1 = StrGetRecordFieldOffset(_prevstr);
                    if (_str.Pos("end;"))
                    {
                        Ofs2 = _size;
                        _brk = true;
                    }
                    else
                        Ofs2 = StrGetRecordFieldOffset(_str);
                    if (Ofs1 >= 0 && AOfs >= Ofs1 && AOfs < Ofs2)
                    {
                        name = StrGetRecordFieldName(_prevstr);
                        type = StrGetRecordFieldType(_prevstr);
                        fclose(_recFile);
                        return Ofs1;
                    }
                    if (_brk) break;
                    _prevstr = _str;
                }
                fclose(_recFile);
            }
        }
        fclose(_recFile);
    }
    int tries = 5;
    while (tries >= 0)
    {
        tries--;
        //KB
        _uses = KnowledgeBase.GetTypeUses(ARecType.c_str());
        _idx = KnowledgeBase.GetTypeIdxByModuleIds(_uses, ARecType.c_str());
        if (_uses) delete[] _uses;

        if (_idx != -1)
        {
            _idx = KnowledgeBase.TypeOffsets[_idx].NamId;
            if (KnowledgeBase.GetTypeInfo(_idx, INFO_FIELDS, &_tInfo))
            {
                if (_tInfo.FieldsNum)
                {
                    memset(_cases, 0, 256 * sizeof(CaseInfo));
                    p = _tInfo.Fields;
                    for (m = 0, n = 0; n < _tInfo.FieldsNum; n++)
                    {
                        p++;//scope
                        p += 4;//offset
                        _case = *((int*)p); p += 4;//case
                        if (!_cases[m].count)
                        {
                            _cases[m].caseno = _case;
                        }
                        else if (_cases[m].caseno != _case)
                        {
                            m++;
                            _cases[m].caseno = _case;
                        }
                        _cases[m].count++;
                        _len = *((WORD*)p); p += _len + 3;//name
                        _len = *((WORD*)p); p += _len + 3;//type
                    }

                    for (m = 0; m < 256; m++)
                    {
                        if (_cases[m].count)
                        {
                            p = _tInfo.Fields;
                            for (n = 0, k = 0; n < _tInfo.FieldsNum; n++)
                            {
                                ps = p;
                                p++;//scope
                                Ofs1 = *((int*)p); p += 4;//offset
                                _case = *((int*)p); p += 4;//case
                                _len = *((WORD*)p); p += _len + 3;//name
                                _len = *((WORD*)p); p += _len + 3;//type
                                if (_case == _cases[m].caseno)
                                {
                                    if (k == _cases[m].count - 1)
                                    {
                                        Ofs2 = GetRecordSize(ARecType);
                                    }
                                    else
                                    {
                                        Ofs2 = *((int*)(p + 1));
                                    }
                                    if (AOfs >= Ofs1 && AOfs < Ofs2)
                                    {
                                        p = ps;
                                        p++;//scope
                                        Ofs1 = *((int*)p); p += 4;//offset
                                        p += 4;//case
                                        _len = *((WORD*)p); p += 2;
                                        name = String(p, _len); p += _len + 1;
                                        _len = *((WORD*)p); p += 2;
                                        type = String(p, _len); p += _len + 1;
                                        return Ofs1;
                                    }
                                    k++;
                                }
                            }
                        }
                    }
                }
                else if (_tInfo.Decl != "")
                {
                    ARecType = _tInfo.Decl;
                    //Ofs = GetRecordField(_tInfo.Decl, AOfs, name, type);
                    //if (Ofs >= 0)
                    //    return Ofs;
                }
            }
        }
    }
    //RTTI
    _recT = GetOwnTypeByName(ARecType);
    if (_recT && _recT->kind == ikRecord)
    {
        _pos = Adr2Pos(_recT->adr);
        _pos += 4;//SelfPtr
        _pos++;//TypeKind
        _len = Code[_pos]; _pos++;
        _name = String((char*)(Code + _pos), _len); _pos += _len;//Name
        _pos += 4;//Size
        _elNum = *((DWORD*)(Code + _pos)); _pos += 4;
        for (n = 0; n < _elNum; n++)
        {
            _typeAdr = *((DWORD*)(Code + _pos)); _pos += 4;
            Ofs1 = *((DWORD*)(Code + _pos)); _pos += 4;
            if (n == _elNum - 1)
                Ofs2 = 0;
            else
                Ofs2 = *((DWORD*)(Code + _pos + 4));
            if (AOfs >= Ofs1 && AOfs < Ofs2)
            {
                name = _name + ".f" + Val2Str0(Ofs1);
                type = GetTypeName(_typeAdr);
                return Ofs1;
            }

        }
        if (DelphiVersion >= 2010)
        {
            //NumOps
            _numOps = Code[_pos]; _pos++;
            for (n = 0; n < _numOps; n++)    //RecOps
            {
                _pos += 4;
            }
            _elNum = *((DWORD*)(Code + _pos)); _pos += 4;  //RecFldCnt

            for (n = 0; n < _elNum; n++)
            {
                _typeAdr = *((DWORD*)(Code + _pos)); _pos += 4;
                Ofs1 = *((DWORD*)(Code + _pos)); _pos += 4;
                _pos++;//Flags
                _len = Code[_pos]; _pos++;
                _name = String((char*)(Code + _pos), _len); _pos += _len;
                //AttrData
                _dw = *((WORD*)(Code + _pos));
                _pos += _dw;//ATR!!

                if (n == _elNum - 1)
                    Ofs2 = 0;
                else
                    Ofs2 = *((DWORD*)(Code + _pos + 4));

                if (AOfs >= Ofs1 && AOfs < Ofs2)
                {
                    if (_name != "")
                        name = _name;
                    else
                        name = "f" + Val2Str0(Ofs1);
                    type = GetTypeName(_typeAdr);
                    return Ofs1;
                }
            }
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
int __fastcall GetField(String TypeName, int Offset, String& name, String& type)
{
    int         size, kind, ofs;
    PFIELDINFO	fInfo;
    String      _fname, _ftype, _type = TypeName;

    _fname = ""; _ftype = "";

    while (Offset >= 0)
    {
        kind = GetTypeKind(_type, &size);
        if (kind != ikVMT && kind != ikArray && kind != ikRecord && kind != ikDynArray) break;
        
        if (kind == ikVMT)
        {
            if (!Offset) return 0;
            fInfo = GetClassField(_type, Offset);
            if (name != "")
                name += ".";
            if (fInfo->Name != "")
                name += fInfo->Name;
            else
                name += "f" + IntToHex((int)fInfo->Offset, 0);
            type = fInfo->Type;

            _type = fInfo->Type;
            Offset -= fInfo->Offset;
            //if (!Offset) return fInfo->Offset;
            continue;
        }
        if (kind == ikRecord)
        {
            ofs = GetRecordField(_type, Offset, _fname, _ftype);
            if (ofs == -1) return -1;
            if (name != "")
                name += ".";
            name += _fname;
            type = _ftype;

            _type = _ftype;
            Offset -= ofs;
            //if (!Offset) return ofs;
            continue;
        }
        if (kind == ikArray || kind == ikDynArray)
        {
            break;
        }
    }
    return Offset;
}
//---------------------------------------------------------------------------
String __fastcall GetRecordFields(int AOfs, String ARecType)
{
    String      _name, _typeName;

    if (ARecType == "" || GetField(ARecType, AOfs, _name, _typeName) < 0) return "";
    return _name + ":" + _typeName;
}
//---------------------------------------------------------------------------
String __fastcall GetAsmRegisterName(int Idx)
{
    assert(Idx >= 0 && Idx < 32);
    if (Idx >= 31) return "st(" + String(Idx - 31) + ")";
    if (Idx == 30) return "st";
    if (Idx >= 24) return String(SegRegTab[Idx - 24]);
    if (Idx >= 16) return String(Reg32Tab[Idx - 16]);
    if (Idx >= 8) return String(Reg16Tab[Idx - 8]);
    return String(Reg8Tab[Idx]);
}
//---------------------------------------------------------------------------
String __fastcall GetDecompilerRegisterName(int Idx)
{
    assert(Idx >= 0 && Idx < 32);
    if (Idx >= 16) return UpperCase(Reg32Tab[Idx - 16]);
    if (Idx >= 8) return UpperCase(Reg32Tab[Idx - 8]);
    return UpperCase(Reg32Tab[Idx]);
}
//---------------------------------------------------------------------------
bool __fastcall IsValidModuleName(int len, int pos)
{
    if (!len) return false;
    for (int i = pos; i < pos + len; i++)
    {
        BYTE b = *(Code + i);
        if (b < ' ' || b == ':' || (b & 0x80)) return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall IsValidName(int len, int pos)
{
    if (!len) return false;
    for (int i = pos; i < pos + len; i++)
    {
        //if (IsFlagSet(cfCode, i)) return false;

        BYTE b = *(Code + i);
        //first symbol may be letter or '_' or '.' or ':'
        if (i == pos)
        {
            if ((b >= 'A' && b <= 'z') || b == '.' || b == '_' || b == ':')
                continue;
            else
                return false;
        }
        if (b & 0x80) return false;
        //if ((b < '0' || b > 'z') && b != '.' && b != '_' && b != ':' && b != '$') return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall IsValidString(int len, int pos)
{
    if (len < 5) return false;
    for (int i = pos; i < pos + len; i++)
    {
        //if (IsFlagSet(cfCode, i)) return false;

        BYTE b = *(Code + i);
        if (b < ' ' && b != '\t' && b != '\n' && b != '\r') return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall IsValidCString(int pos)
{
    int len = 0;
    for (int i = pos; i < pos + 1024; i++)
    {
        BYTE b = *(Code + i);
        //if (IsFlagSet(cfCode, i)) break;
        if (!b) return (len >= 5);
        if (b < ' ' && b != '\t' && b != '\n' && b != '\r') break;
        len++;
    }           
    return false;
}
//---------------------------------------------------------------------------
DWORD __fastcall GetParentAdr(DWORD Adr)
{
    if (!IsValidImageAdr(Adr)) return 0;

    DWORD vmtAdr = Adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtParent;
    DWORD adr = *((DWORD*)(Code + pos));
    if (IsValidImageAdr(adr) && IsFlagSet(cfImport, Adr2Pos(adr)))
        return 0;

    if (DelphiVersion == 2 && adr) adr += VmtSelfPtr;
    return adr;
}
//---------------------------------------------------------------------------
DWORD __fastcall GetChildAdr(DWORD Adr)
{
    if (!IsValidImageAdr(Adr)) return 0;
    for (int m = 0; m < VmtList->Count; m++)
    {
        PVmtListRec recV = (PVmtListRec)VmtList->Items[m];
        if (recV->vmtAdr != Adr && IsInheritsByAdr(recV->vmtAdr, Adr))
            return recV->vmtAdr;
    }
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall GetClassSize(DWORD adr)
{
    if (!IsValidImageAdr(adr)) return 0;

    DWORD vmtAdr = adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtInstanceSize;
    int size = *((int*)(Code + pos));
    if (DelphiVersion >= 2009) return size - 4;
    return size;
}
//---------------------------------------------------------------------------
String __fastcall GetClsName(DWORD adr)
{
    if (!IsValidImageAdr(adr)) return "";

    DWORD vmtAdr = adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtClassName;
    if (IsFlagSet(cfImport, pos))
    {
        PInfoRec recN = GetInfoRec(vmtAdr + VmtClassName);
        return recN->GetName();
    }
    DWORD nameAdr = *((DWORD*)(Code + pos));
    if (!IsValidImageAdr(nameAdr))
        return "";

    pos = Adr2Pos(nameAdr);
    BYTE len = Code[pos]; pos++;
    return String((char*)&Code[pos], len);
}
//---------------------------------------------------------------------------
DWORD __fastcall GetClassAdr(const String& AName)
{
    String      name;

    if (AName.IsEmpty()) return 0;

    DWORD adr = FindClassAdrByName(AName);
    if (adr) return adr;

    int pos = AName.Pos(".");
    if (pos)
    {
        //type as .XX or array[XX..XX] of XXX - skip
        if (pos == 1 || AName[pos + 1] == '.') return 0;
        name = AName.SubString(pos + 1, AName.Length());
    }
    else
        name = AName;

    const int vmtCnt = VmtList->Count;
    for (int n = 0; n < vmtCnt; n++)
    {
        PVmtListRec recV = (PVmtListRec)VmtList->Items[n];
        if (SameText(recV->vmtName, name))
        {
            adr = recV->vmtAdr;
            AddClassAdr(adr, name);
            return adr;
        }
    }
    return 0;
}

//---------------------------------------------------------------------------
int __fastcall GetParentSize(DWORD Adr)
{
    return GetClassSize(GetParentAdr(Adr));
}
//---------------------------------------------------------------------------
String __fastcall GetParentName(DWORD Adr)
{
    DWORD adr = GetParentAdr(Adr);
    if (!adr) return "";
    return GetClsName(adr);
}
//---------------------------------------------------------------------------
String __fastcall GetParentName(const String& ClassName)
{
	return GetParentName(GetClassAdr(ClassName));
}
//---------------------------------------------------------------------------
//Adr1 inherits Adr2 (Adr1 >= Adr2)
bool __fastcall IsInheritsByAdr(const DWORD Adr1, const DWORD Adr2)
{
    DWORD adr = Adr1;
    while (adr)
    {
        if (adr == Adr2) return true;
        adr = GetParentAdr(adr);
    }
    return false;
}
//---------------------------------------------------------------------------
//Name1 >= Name2
bool __fastcall IsInheritsByClassName(const String& Name1, const String& Name2)
{
    DWORD adr = GetClassAdr(Name1);
    while (adr)
    {
        if (SameText(GetClsName(adr), Name2)) return true;
        adr = GetParentAdr(adr);
    }
    return false;
}
//---------------------------------------------------------------------------
bool __fastcall IsInheritsByProcName(const String& Name1, const String& Name2)
{
    return (IsInheritsByClassName(ExtractClassName(Name1), ExtractClassName(Name2)) &&
            SameText(ExtractProcName(Name1), ExtractProcName(Name2)));
}
//---------------------------------------------------------------------------
String __fastcall TransformString(char* str, int len)
{
    bool        s = true;//true - print string, false - print #XX
    BYTE        c, *p = str;
    String      res = "";

    for (int k = 0; k < len; k++)
    {
        c = *p; p++;
        if (!(c & 0x80) && c <= 13)
        {
            if (s)
            {
                if (k) res += "'+";
            }
            else
                res += "+";
            res += "#" + String((int)c);
            s = false;
        }
        else
        {
            if (s)
            {
                if (!k) res += "'";
            }
            else
                res += "+";
            s = true;
        }
        if (c == 0x22)
            res += "\"";
        else if (c == 0x27)
            res += "'";
        else if (c == 0x5C)
            res += "\\";
        else if (c > 13)
            res += (char)c;
    }
    if (s) res += "'";
    return res;
}
//---------------------------------------------------------------------------
String __fastcall TransformUString(WORD codePage, wchar_t* data, int len)
{
    if (!IsValidCodePage(codePage)) codePage = CP_ACP;
    int nChars = WideCharToMultiByte(codePage, 0, data, -1, 0, 0, 0, 0);
    if (!nChars) return "";
    char* tmpBuf = new char[nChars + 1];
    WideCharToMultiByte(codePage, 0, data, -1, tmpBuf, nChars, 0, 0);
    tmpBuf[nChars] = 0;
    String res = QuotedStr(tmpBuf);
    delete[] tmpBuf;
    return res;
}
//---------------------------------------------------------------------------
//Get stop address for analyzing virtual tables
DWORD __fastcall GetStopAt(DWORD VmtAdr)
{
    int     m;
    DWORD   pos, pointer, stopAt = CodeBase + TotalSize;

    if (DelphiVersion != 2)
    {
        pos = Adr2Pos(VmtAdr) + VmtIntfTable;
        for (m = VmtIntfTable; m != VmtInstanceSize; m += 4, pos += 4)
        {
            pointer = *((DWORD*)(Code + pos));
            if (pointer >= VmtAdr && pointer < stopAt) stopAt = pointer;
        }
    }
    else
    {
        pos = Adr2Pos(VmtAdr) + VmtInitTable;
        for (m = VmtInitTable; m != VmtInstanceSize; m += 4, pos += 4)
        {
            if (Adr2Pos(VmtAdr) < 0)
                return 0;
            pointer = *((DWORD*)(Code + pos));
            if (pointer >= VmtAdr && pointer < stopAt) stopAt = pointer;
        }
    }
    return stopAt;
}
//---------------------------------------------------------------------------
String __fastcall GetTypeName(DWORD adr)
{
	if (!IsValidImageAdr(adr)) return "?";
    if (IsFlagSet(cfImport, Adr2Pos(adr)))
    {
        PInfoRec recN = GetInfoRec(adr);
        return recN->GetName();
    }
    
    int pos = Adr2Pos(adr);
    if (IsFlagSet(cfRTTI, pos))
        pos += 4;
    //TypeKind
    BYTE kind = *(Code + pos); pos++;
    BYTE len = *(Code + pos); pos++;
    String Result = String((char*)(Code + pos), len);
    if (Result[1] == '.')
    {
        PUnitRec recU = FMain_11011981->GetUnit(adr);
        if (recU)
        {
            String prefix;
            switch (kind)
            {
            case ikEnumeration:
                prefix = "_Enum_";
                break;
            case ikArray:
                prefix = "_Arr_";
                break;
            case ikDynArray:
                prefix = "_DynArr_";
                break;
            default:
                prefix = FMain_11011981->GetUnitName(recU);
                break;
            }
            Result = prefix + String((int)recU->iniOrder) + "_" + Result.SubString(2, len);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
String __fastcall GetDynaInfo(DWORD adr, WORD id, DWORD* dynAdr)
{
	int			m;
	DWORD		classAdr = adr;
    PInfoRec 	recN;
    PMethodRec 	recM;

    *dynAdr = 0;

	if (!IsValidCodeAdr(adr)) return "";

    while (classAdr)
    {
    	recN = GetInfoRec(classAdr);
        if (recN && recN->vmtInfo && recN->vmtInfo->methods)
        {
        	for (m = 0; m < recN->vmtInfo->methods->Count; m++)
            {
            	PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[m];
                if (recM->kind == 'D' && recM->id == id)
                {
                	*dynAdr = recM->address;
                	if (recM->name != "") return recM->name;
                	return "";//GetDefaultProcName(recM->address);
                }
            }
        }
        classAdr = GetParentAdr(classAdr);
    }
    return "";
}
//---------------------------------------------------------------------------
String __fastcall GetDynArrayTypeName(DWORD adr)
{
    Byte    len;
    int     pos;

    pos = Adr2Pos(adr);
    pos += 4;
    pos++;//Kind
    len = Code[pos]; pos++;
    pos += len;//Name
    pos += 4;//elSize
    return GetTypeName(*((DWORD*)(Code + pos)));
}
//---------------------------------------------------------------------------
int __fastcall GetTypeSize(String AName)
{
    int         idx = -1;
    WORD*       uses;
    MTypeInfo   tInfo;

    uses = KnowledgeBase.GetTypeUses(AName.c_str());
    idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, AName.c_str());
    if (uses) delete[] uses;

    if (idx != -1)
    {
        idx = KnowledgeBase.TypeOffsets[idx].NamId;
        if (KnowledgeBase.GetTypeInfo(idx, INFO_DUMP, &tInfo))
        {
            if (tInfo.Size) return tInfo.Size;
        }
    }
    return 4;
}
//---------------------------------------------------------------------------
#define TypeKindsNum    22
String TypeKinds[TypeKindsNum] =
{
    "Unknown",
    "Integer",
    "Char",
    "Enumeration",
    "Float",
    "ShortString",
    "Set",
    "Class",
    "Method",
    "WChar",
    "AnsiString",
    "WideString",
    "Variant",
    "Array",
    "Record",
    "Interface",
    "Int64",
    "DynArray",
    "UString",
    "ClassRef",
    "Pointer",
    "Procedure"
};
//---------------------------------------------------------------------------
String __fastcall TypeKind2Name(BYTE kind)
{
    if (kind < TypeKindsNum) return TypeKinds[kind];
    else return "";
}
//---------------------------------------------------------------------------
DWORD __fastcall GetOwnTypeAdr(String AName)
{
    if (AName == "") return 0;
    PTypeRec recT = GetOwnTypeByName(AName);
    if (recT) return recT->adr;
    return 0;
}
//---------------------------------------------------------------------------
PTypeRec __fastcall GetOwnTypeByName(String AName)
{
    if (AName == "") return 0;
    for (int m = 0; m < OwnTypeList->Count; m++)
    {
        PTypeRec recT = (PTypeRec)OwnTypeList->Items[m];
        if (SameText(recT->name, AName)) return recT;
    }
    return 0;
}
//---------------------------------------------------------------------------
String __fastcall GetTypeDeref(String ATypeName)
{
    int         idx = -1;
    WORD*       uses;
    MTypeInfo   tInfo;

    if (ATypeName[1] == '^') return ATypeName.SubString(2, ATypeName.Length());

    //Scan knowledgeBase
    uses = KnowledgeBase.GetTypeUses(ATypeName.c_str());
    idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, ATypeName.c_str());
    if (uses) delete[] uses;

    if (idx != -1)
    {
        idx = KnowledgeBase.TypeOffsets[idx].NamId;
        if (KnowledgeBase.GetTypeInfo(idx, INFO_DUMP, &tInfo))
        {
            if (tInfo.Decl != "" && tInfo.Decl[1] == '^')
                return tInfo.Decl.SubString(2, tInfo.Decl.Length());
        }
    }
    return "";
}
//---------------------------------------------------------------------------
int __fastcall GetRTTIRecordSize(DWORD adr)
{
    BYTE        len;
    int         pos, kind, _ap;

    _ap = Adr2Pos(adr); pos = _ap;
    pos += 4;
    kind = Code[pos]; pos++;
    len = Code[pos]; pos += len + 1;

    if (kind == ikRecord)
        return *((int*)(Code + pos));
    else
        return 0;
}
//---------------------------------------------------------------------------
BYTE __fastcall GetTypeKind(String AName, int* size)
{
    BYTE        res;
    int         pos, idx = -1, kind;
    WORD*       uses;
    MTypeInfo   tInfo;
    String      name, typeName, str, sz;

    *size = 4;
    if (AName != "")
    {
        if (AName.Pos("array"))
        {
            if (AName.Pos("array of")) return ikDynArray;
            return ikArray;
        }

        pos = AName.LastDelimiter(".");
        if (pos > 1 && AName[pos + 1] != ':')
            name = AName.SubString(pos + 1, AName.Length());
        else
            name = AName;

        if (name[1] == '^' || SameText(name, "Pointer"))
            return ikPointer;

        if (SameText(name, "Boolean") ||
            SameText(name, "ByteBool") ||
            SameText(name, "WordBool") ||
            SameText(name, "LongBool"))
        {
            return ikEnumeration;
        }
        if (SameText(name, "ShortInt") ||
            SameText(name, "Byte")     ||
            SameText(name, "SmallInt") ||
            SameText(name, "Word")     ||
            SameText(name, "Dword")    ||
            SameText(name, "Integer")  ||
            SameText(name, "LongInt")  ||
            SameText(name, "LongWord") ||
            SameText(name, "Cardinal"))
        {
            return ikInteger;
        }
        if (SameText(name, "Char"))
        {
            return ikChar;
        }
        if (SameText(name, "Text") || SameText(name, "File"))
        {
            return ikRecord;
        }

        if (SameText(name, "Int64"))
        {
            *size = 8;
            return ikInt64;
        }
        if (SameText(name, "Single"))
        {
            return ikFloat;
        }
        if (SameText(name, "Real48")   ||
            SameText(name, "Real")     ||
            SameText(name, "Double")   ||
            SameText(name, "TDate")    ||
            SameText(name, "TTime")    ||
            SameText(name, "TDateTime")||
            SameText(name, "Comp")     ||
            SameText(name, "Currency"))
        {
            *size = 8;
            return ikFloat;
        }
        if (SameText(name, "Extended"))
        {
            *size = 12;
            return ikFloat;
        }
        if (SameText(name, "ShortString")) return ikString;
        if (SameText(name, "String") || SameText(name, "AnsiString")) return ikLString;
        if (SameText(name, "WideString")) return ikWString;
        if (SameText(name, "UnicodeString") || SameText(name, "UString")) return ikUString;
        if (SameText(name, "PChar") || SameText(name, "PAnsiChar")) return ikCString;
        if (SameText(name, "PWideChar")) return ikWCString;
        if (SameText(name, "Variant")) return ikVariant;
        //if (SameText(name, "Pointer")) return ikPointer;

        //File
        String recFileName = FMain_11011981->WrkDir + "\\types.idr";
        FILE* recFile = fopen(recFileName.c_str(), "rt");
        if (recFile)
        {
            while (1)
            {
                if (!fgets(StringBuf, 1024, recFile)) break;
                str = String(StringBuf);
                if (str.Pos(AName + "=") == 1)
                {
                    if (str.Pos("=record"))
                    {
                        *size = StrGetRecordSize(str);
                        fclose(recFile);
                        return ikRecord;
                    }
                }
            }
            fclose(recFile);
        }
        //RTTI
        PTypeRec recT = GetOwnTypeByName(name);
        if (recT)
        {
            *size = GetRTTIRecordSize(recT->adr);
            if (!*size) *size = 4;
            return recT->kind;
        }
        //Scan KB
        uses = KnowledgeBase.GetTypeUses(name.c_str());
        idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, name.c_str());
        if (uses) delete[] uses;

        if (idx != -1)
        {
            idx = KnowledgeBase.TypeOffsets[idx].NamId;
            if (KnowledgeBase.GetTypeInfo(idx, INFO_DUMP, &tInfo))
            {
                if (tInfo.Kind == 'Z')  //drAlias???
                    return ikUnknown;
                if (tInfo.Decl != "" && tInfo.Decl[1] == '^')
                {
                    return ikUnknown;
                    //res = GetTypeKind(tInfo.Decl.SubString(2, tInfo.Decl.Length()), size);
                    //if (res) return res;
                    //return 0;
                }
                *size = tInfo.Size;
                switch (tInfo.Kind)
                {
                case drRangeDef:
                    return ikEnumeration;
                case drPtrDef:
                    return ikMethod;
                case drProcTypeDef:
                    return ikMethod;
                case drSetDef:
                    return ikSet;
                case drRecDef:
                    return ikRecord;
                case drInterfaceDef:
                    return ikInterface;
                }
                if (tInfo.Decl != "")
                {
                    res = GetTypeKind(tInfo.Decl, size);
                    if (res) return res;
                }
            }
        }
        //May be Interface name
        if (AName[1] == 'I')
        {
            AName[1] = 'T';
            if (GetTypeKind(AName, size) == ikVMT) return ikInterface;
        }
        //Manual
    }
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall GetPackedTypeSize(String AName)
{
    int     _size;
    if (SameText(AName, "Boolean")  ||
        SameText(AName, "ShortInt") ||
        SameText(AName, "Byte")     ||
        SameText(AName, "Char"))
    {
        return 1;
    }
    if (SameText(AName, "SmallInt") ||
        SameText(AName, "Word"))
    {
        return 2;
    }
    if (SameText(AName, "Dword")    ||
        SameText(AName, "Integer")  ||
        SameText(AName, "LongInt")  ||
        SameText(AName, "LongWord") ||
        SameText(AName, "Cardinal") ||
        SameText(AName, "Single"))
    {
        return 4;
    }
    if (SameText(AName, "Real48"))
    {
        return 6;
    }
    if (SameText(AName, "Real")     ||
        SameText(AName, "Double")   ||
        SameText(AName, "Comp")     ||
        SameText(AName, "Currency") ||
        SameText(AName, "Int64"))
    {
        return 8;
    }
    if (SameText(AName, "Extended"))
    {
        return 10;
    }
    if (GetTypeKind(AName, &_size) == ikRecord)
    {
        return GetRecordSize(AName);
    }
    return 4;
}
//---------------------------------------------------------------------------
//return string representation of immediate value with comment
String __fastcall GetImmString(int Val)
{
    String _res;
    String _res0 = String((int)Val);
    if (Val > -16 && Val < 16) return _res0;
    _res = String("$") + Val2Str0(Val);
    if (!IsValidImageAdr(Val)) _res += "{" + _res0 + "}";
    return _res;
}
//---------------------------------------------------------------------------
String __fastcall GetImmString(String TypeName, int Val)
{
    int     _size;
    String  _str, _default = GetImmString(Val);
    BYTE _kind = GetTypeKind(TypeName, &_size);
    if (!Val && (_kind == ikString || _kind == ikLString || _kind == ikWString || _kind == ikUString)) return "''";
    if (!Val && (_kind == ikClass || _kind == ikVMT)) return "Nil";
    if (_kind == ikEnumeration)
    {
        _str = GetEnumerationString(TypeName, Val);
        if (_str != "") return _str;
        return _default;
    }
    if (_kind == ikChar) return Format("'%s'", ARRAYOFCONST(((Char)Val)));
    return _default;
}
//---------------------------------------------------------------------------
PInfoRec __fastcall GetInfoRec(DWORD adr)
{
    int pos = Adr2Pos(adr);
    if (pos >= 0)
        return Infos[pos];

    int _idx = BSSInfos->IndexOf(Val2Str8(adr));
    if (_idx != -1)
        return (PInfoRec)BSSInfos->Objects[_idx];

    return 0;
}
//---------------------------------------------------------------------------
String __fastcall GetEnumerationString(String TypeName, Variant Val)
{
    BYTE        len;
    int         n, pos, _val, idx;
    DWORD       adr, typeAdr, minValue, maxValue, minValueB, maxValueB;
    char        *p, *b, *e;
    WORD        *uses;
    MTypeInfo   tInfo;
    String      clsName;

    if (Val.Type() == varString) return String(Val);

    _val = Val;

    if (SameText(TypeName, "Boolean")  ||
        SameText(TypeName, "ByteBool") ||
        SameText(TypeName, "WordBool") ||
        SameText(TypeName, "LongBool"))
    {
        if (_val)
            return "True";
        else
            return "False";
    }

    adr = GetOwnTypeAdr(TypeName);
    //RTTI exists
    if (IsValidImageAdr(adr))
    {
        pos = Adr2Pos(adr);
        pos += 4;
        //typeKind
        pos++;
        len = Code[pos]; pos++;
        clsName = String((char*)(Code + pos), len); pos += len;
        //ordType
        pos++;
        minValue = *((DWORD*)(Code + pos)); pos += 4;
        maxValue = *((DWORD*)(Code + pos)); pos += 4;
        //BaseTypeAdr
        typeAdr = *((DWORD*)(Code + pos)); pos += 4;

        //If BaseTypeAdr != SelfAdr then fields extracted from BaseType
        if (typeAdr != adr)
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

        for (n = minValueB; n <= maxValueB; n++)
        {
            len = Code[pos]; pos++;
            if (n >= minValue && n <= maxValue && n == _val)
            {
                return String((char*)(Code + pos), len);
            }
            pos += len;
        }
    }
    //Try get from KB
    else
    {
        uses = KnowledgeBase.GetTypeUses(TypeName.c_str());
        idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, TypeName.c_str());
        if (uses) delete[] uses;

        if (idx != -1)
        {
            idx = KnowledgeBase.TypeOffsets[idx].NamId;
            if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS | INFO_PROPS | INFO_METHODS | INFO_DUMP, &tInfo))
            {
                if (tInfo.Kind == drRangeDef)
                    return String(Val);
                //if (SameText(TypeName, tInfo.TypeName) && tInfo.Decl != "")
                if (tInfo.Decl != "")
                {
                    p = tInfo.Decl.c_str();
                    e = p;
                    for (n = 0; n <= _val; n++)
                    {
                        b = e + 1;
                        e = strchr(b, ',');
                        if (!e) return "";
                    }
                    return tInfo.Decl.SubString(b - p + 1, e - b);
                }
            }
        }
    }
    return "";
}
//---------------------------------------------------------------------------
String __fastcall GetSetString(String TypeName, BYTE* ValAdr)
{
    int         n, m, idx, size;
    BYTE        b, *pVal;
    char        *pDecl, *p;
    WORD        *uses;
    MTypeInfo   tInfo;
    String      name, result = "";

    //Get from KB
    uses = KnowledgeBase.GetTypeUses(TypeName.c_str());
    idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, TypeName.c_str());
    if (uses) delete[] uses;

    if (idx != -1)
    {
        idx = KnowledgeBase.TypeOffsets[idx].NamId;
        if (KnowledgeBase.GetTypeInfo(idx, INFO_DUMP, &tInfo))
        {
            if (tInfo.Decl.Pos("set of "))
            {
                size = tInfo.Size;
                name = TrimTypeName(tInfo.Decl.SubString(8, TypeName.Length()));
                uses = KnowledgeBase.GetTypeUses(name.c_str());
                idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, name.c_str());
                if (uses) delete[] uses;

                if (idx != -1)
                {
                    idx = KnowledgeBase.TypeOffsets[idx].NamId;
                    if (KnowledgeBase.GetTypeInfo(idx, INFO_DUMP, &tInfo))
                    {
                        pVal = ValAdr;
                        pDecl = tInfo.Decl.c_str();
                        p = strtok(pDecl, ",()");
                        for (n = 0; n < size; n++)
                        {
                            b = *pVal;
                            for (m = 0; m < 8; m++)
                            {
                                if (b & ((DWORD)1 << m))
                                {
                                    if (result != "") result += ",";
                                    if (p)
                                        result += String(p);
                                    else
                                        result += "$" + Val2Str2(n * 8 + m);
                                }
                                if (p) p = strtok(0, ",)");
                            }
                            pVal++;
                        }
                    }
                }
            }
        }
    }
    if (result != "") result = "[" + result + "]";
    return result;
}
//---------------------------------------------------------------------------
void __fastcall OutputDecompilerHeader(FILE* f)
{
    int n = sprintf(StringBuf, "IDR home page: http://kpnc.org/idr32/en");
    int m = sprintf(StringBuf, "Decompiled by IDR v.%s", IDRVersion.c_str());
    if (n < m) n = m;
    
    memset(StringBuf, '*', n); StringBuf[n] = 0;
    fprintf(f, "//%s\n", StringBuf);

    fprintf(f, "//IDR home page: http://kpnc.org/idr32/en\n", StringBuf);

    sprintf(StringBuf, "Decompiled by IDR v.%s", IDRVersion.c_str());
    fprintf(f, "//%s\n", StringBuf);

    memset(StringBuf, '*', n); StringBuf[n] = 0;
    fprintf(f, "//%s\n", StringBuf);
}
//---------------------------------------------------------------------------
void __fastcall AddFieldXref(PFIELDINFO fInfo, DWORD ProcAdr, int ProcOfs, char type)
{
    PXrefRec    recX;

    if (!fInfo->xrefs) fInfo->xrefs = new TList;

    if (!fInfo->xrefs->Count)
    {
        recX = new XrefRec;
        recX->type = type;
        recX->adr = ProcAdr;
        recX->offset = ProcOfs;
        fInfo->xrefs->Add((void*)recX);
        return;
    }

    int F = 0;
    recX = (PXrefRec)fInfo->xrefs->Items[F];
    if (ProcAdr + ProcOfs < recX->adr + recX->offset)
    {
        recX = new XrefRec;
        recX->type = type;
        recX->adr = ProcAdr;
        recX->offset = ProcOfs;
        fInfo->xrefs->Insert(F, (void*)recX);
        return;
    }
    int L = fInfo->xrefs->Count - 1;
    recX = (PXrefRec)fInfo->xrefs->Items[L];
    if (ProcAdr + ProcOfs > recX->adr + recX->offset)
    {
        recX = new XrefRec;
        recX->type = type;
        recX->adr = ProcAdr;
        recX->offset = ProcOfs;
        fInfo->xrefs->Add((void*)recX);
        return;
    }
    while (F < L)
    {
        int M = (F + L)/2;
        recX = (PXrefRec)fInfo->xrefs->Items[M];
        if (ProcAdr + ProcOfs <= recX->adr + recX->offset)
            L = M;
        else
            F = M + 1;
    }
    recX = (PXrefRec)fInfo->xrefs->Items[L];
    if (ProcAdr + ProcOfs != recX->adr + recX->offset)
    {
        recX = new XrefRec;
        recX->type = type;
        recX->adr = ProcAdr;
        recX->offset = ProcOfs;
        fInfo->xrefs->Insert(L, (void*)recX);
    }
}
//---------------------------------------------------------------------------
int __fastcall ArgsCmpFunction(void *item1, void *item2)
{
    PARGINFO rec1 = (PARGINFO)item1;
    PARGINFO rec2 = (PARGINFO)item2;

    if (rec1->Ndx > rec2->Ndx) return 1;
    if (rec1->Ndx < rec2->Ndx) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall ExportsCmpFunction(void *item1, void *item2)
{
    PExportNameRec rec1 = (PExportNameRec)item1;
    PExportNameRec rec2 = (PExportNameRec)item2;
    if (rec1->address > rec2->address) return 1;
    if (rec1->address < rec2->address) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall ImportsCmpFunction(void *item1, void *item2)
{
    PImportNameRec rec1 = (PImportNameRec)item1;
    PImportNameRec rec2 = (PImportNameRec)item2;
    if (rec1->address > rec2->address) return 1;
    if (rec1->address < rec2->address) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall MethodsCmpFunction(void *item1, void *item2)
{
    PMethodRec rec1 = (PMethodRec)item1;
    PMethodRec rec2 = (PMethodRec)item2;

    if (rec1->kind > rec2->kind) return 1;
    if (rec1->kind < rec2->kind) return -1;
    if (rec1->id > rec2->id) return 1;
    if (rec1->id < rec2->id) return -1;
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall AddPicode(int Pos, BYTE Op, String Name, int Ofs)
{
    if (Name == "") return;

	PInfoRec recN = GetInfoRec(Pos2Adr(Pos));
    //if (recN && recN->picode) return;
    
    if (!recN)
        recN = new InfoRec(Pos, ikUnknown);
    if (!recN->picode)
        recN->picode = new PICODE;
    recN->picode->Op = Op;
    recN->picode->Name = Name;
    if (Op == OP_CALL)
    	recN->picode->Ofs.Address = Ofs;
    else
    	recN->picode->Ofs.Offset = Ofs;
}
//---------------------------------------------------------------------------
int __fastcall SortUnitsByAdr(void *item1, void* item2)
{
    PUnitRec recU1 = (PUnitRec)item1;
    PUnitRec recU2 = (PUnitRec)item2;
    if (recU1->toAdr > recU2->toAdr) return 1;
    if (recU1->toAdr < recU2->toAdr) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall SortUnitsByOrd(void *item1, void* item2)
{
    PUnitRec recU1 = (PUnitRec)item1;
    PUnitRec recU2 = (PUnitRec)item2;
    if (recU1->iniOrder > recU2->iniOrder) return 1;
    if (recU1->iniOrder < recU2->iniOrder) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall SortUnitsByNam(void *item1, void* item2)
{
    PUnitRec recU1 = (PUnitRec)item1;
    PUnitRec recU2 = (PUnitRec)item2;
    String name1 = "";
    for (int n = 0; n < recU1->names->Count; n++)
    {
        if (n) name1 += "+";
        name1 += recU1->names->Strings[n];
    }
    String name2 = "";
    for (int n = 0; n < recU2->names->Count; n++)
    {
        if (n) name2 += "+";
        name2 += recU2->names->Strings[n];
    }
    int result = CompareText(name1, name2);
    if (result) return result;
    if (recU1->toAdr > recU2->toAdr) return 1;
    if (recU1->toAdr < recU2->toAdr) return -1;
    return 0;
}
//---------------------------------------------------------------------------
String __fastcall GetArrayElementType(String arrType)
{
    DWORD adr = GetOwnTypeAdr(arrType);
    if (IsValidImageAdr(adr) && IsFlagSet(cfRTTI, Adr2Pos(adr)))
        return GetDynArrayTypeName(adr);

    int pos = arrType.Pos(" of ");
    if (!pos) return "";
    return Trim(arrType.SubString(pos + 4, arrType.Length()));
}
//---------------------------------------------------------------------------
int __fastcall GetArrayElementTypeSize(String arrType)
{
    String      _elType;

    _elType = GetArrayElementType(arrType);
    if (_elType == "") return 0;

    if (SameText(_elType, "procedure")) return 8;
    return GetTypeSize(_elType);
}
//---------------------------------------------------------------------------
//array [N1..M1,N2..M2,...,NK..MK] of Type
//A:array [N..M] of Size
//A[K]: [A + (K - N1)*Size] = [A - N1*Size + K*Size]
//A:array [N1..M1,N2..M2] of Size <=> array [N1..M1] of array [N2..M2] of Size
//A[K,L]: [A + (K - N1)*Dim2*Size + (L - N2)*Size]
//A:array [N1..M1,N2..M2,N3..M3] of Size <=> array [N1..M1] of array [N2..M2,N3..M3] of Size
//A[K,L,R]: [A + (K - N1)*Dim2*Dim3*Size + (L - N2)*Dim3*Size + (R - M3)*Size]
//A[I1,I2,...,IK]: [A + (I1 - N1L)*S/(N1H - N1L) + (I2 - N2L)*S/(N1H - N1L)*(N2H - N2L) + ... + (IK - NKL)*S/S] (*Size)
bool __fastcall GetArrayIndexes(String arrType, int ADim, int* LowIdx, int* HighIdx)
{
    char        c, *p, *b;
    int         _dim, _val, _pos;
    String      _item, _item1, _item2;

    *LowIdx = 1; *HighIdx = 1;//by default
    strcpy(StringBuf, arrType.c_str());
    p = strchr(StringBuf, '[');
    if (!p) return false;
    p++; b = p; _dim = 0;
    while (1)
    {
        c = *p;
        if (c == ',' || c == ']')
        {
            _dim++;
            if (_dim == ADim)
            {
                *p = 0;
                _item = String(b).Trim();
                _pos = _item.Pos("..");
                if (!_pos)
                    *LowIdx = 0;//Type
                else
                {
                    _item1 = _item.SubString(1, _pos - 1);
                    if (TryStrToInt(_item1, _val))
                        *LowIdx = _val;
                    else
                        *LowIdx = 0;//Type
                    _item2 = _item.SubString(_pos + 2, _item.Length());
                    if (TryStrToInt(_item2, _val))
                        *HighIdx = _val;
                    else
                        *HighIdx = 0;//Type
                }
                return true;
            }
            if (c == ']') break;
        }
        p++;
    }
    return false;
}
//---------------------------------------------------------------------------
int __fastcall GetArraySize(String arrType)
{
    char        c, *p, *b;
    int         _dim, _val, _pos, _result = 1, _lIdx, _hIdx, _elTypeSize;
    String      _item, _item1, _item2;

    _elTypeSize = GetArrayElementTypeSize(arrType);
    if (_elTypeSize == 0) return 0;

    strcpy(StringBuf, arrType.c_str());
    p = strchr(StringBuf, '[');
    if (!p) return 0;
    p++; b = p; _dim = 0;
    while (1)
    {
        c = *p;
        if (c == ',' || c == ']')
        {
            _dim++;
            *p = 0;
            _item = String(b).Trim();
            _pos = _item.Pos("..");
            if (!_pos)
                _lIdx = 0;//Type
            else
            {
                _item1 = _item.SubString(1, _pos - 1);
                if (TryStrToInt(_item1, _val))
                    _lIdx = _val;
                else
                    _lIdx = 0;//Type
                _item2 = _item.SubString(_pos + 2, _item.Length());
                if (TryStrToInt(_item2, _val))
                    _hIdx = _val;
                else
                    _hIdx = 0;//Type
            }
            if (_hIdx < _lIdx) return 0;
            _result *= (_hIdx - _lIdx + 1);
            if (c == ']')
            {
                _result *= _elTypeSize;
                break;
            }
            b = p + 1;
        }
        p++;
    }
    return _result;
}
//---------------------------------------------------------------------------
/*
String __fastcall GetArrayElement(String arrType, int offset)
{
    String  _result = "";
    int     _arrSize, _elTypeSize;

    _arrSize = GetArraySize(arrType);
    if (_arrSize == 0) return "";

    _elTypeSize = GetArrayElementTypeSize(arrType);

    strcpy(StringBuf, arrType.c_str());
    p = strchr(StringBuf, '[');
    if (!p) return "";
    p++; b = p; _dim = 0;
    while (1)
    {
        c = *p;
        if (c == ',' || c == ']')
        {
            _dim++;
            *p = 0;
            _item = String(b).Trim();
            _pos = _item.Pos("..");
            if (!_pos)
                _lIdx = 0;//Type
            else
            {
                _item1 = _item.SubString(1, _pos - 1);
                if (TryStrToInt(_item1, _val))
                    _lIdx = _val;
                else
                    _lIdx = 0;//Type
                _item2 = _item.SubString(_pos + 2, _item.Length());
                if (TryStrToInt(_item2, _val))
                    _hIdx = _val;
                else
                    _hIdx = 0;//Type
            }
            if (_hIdx - _lIdx + 1 <= 0) return "";
            if (offset > _arrSize / (_hIdx - _lIdx + 1))
            {

            }
            if (c == ']')
            {
                _result *= _elTypeSize;
                break;
            }
            b = p + 1;
        }
        p++;
    }
    return _result;
}
*/
//---------------------------------------------------------------------------
void __fastcall Copy2Clipboard(TStrings* items, int leftMargin, bool asmCode)
{
    int     n, bufLen = 0;
    String  line;

    Screen->Cursor = crHourGlass;

    for (n = 0; n < items->Count; n++)
    {
        line = items->Strings[n];
        bufLen += line.Length() + 2;
    }
    //Последний символ должен быть 0
    bufLen++;

    if (bufLen)
    {
        char* buf = new char[bufLen];
        if (buf)
        {
            Clipboard()->Open();
            //Запихиваем все данные в буфер
            char *p = buf;
            for (n = 0; n < items->Count; n++)
            {
                line = items->Strings[n];
                p += sprintf(p, "%s", line.c_str() + leftMargin);
                if (asmCode && n) p--;
                *p = '\r'; p++;
                *p = '\n'; p++;
            }

            *p = 0;
            Clipboard()->SetTextBuf(buf);
            Clipboard()->Close();

            delete[] buf;
        }
    }

    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
String __fastcall GetModuleVersion(const String& module)
{
    DWORD dwDummy;
    DWORD dwFVISize = GetFileVersionInfoSize(module.c_str(), &dwDummy);
    if (!dwFVISize) return "";

    String strVersion = ""; //empty means not found, etc - some error

    LPBYTE lpVersionInfo = new BYTE[dwFVISize];
    if (GetFileVersionInfo(module.c_str(), 0, dwFVISize, lpVersionInfo))
    {
        UINT uLen;
        VS_FIXEDFILEINFO *lpFfi;
        if (VerQueryValue(lpVersionInfo, _T("\\"), (LPVOID *)&lpFfi, &uLen))
        {
            DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
            DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
            DWORD dwLeftMost      = HIWORD(dwFileVersionMS);
            DWORD dwSecondLeft    = LOWORD(dwFileVersionMS);
            DWORD dwSecondRight   = HIWORD(dwFileVersionLS);
            DWORD dwRightMost     = LOWORD(dwFileVersionLS);

            strVersion.sprintf("%d.%d.%d.%d", dwLeftMost, dwSecondLeft, dwSecondRight, dwRightMost);
        }
    }
    delete[] lpVersionInfo;
    return strVersion;
}
//---------------------------------------------------------------------------
bool __fastcall IsBplByExport(const char* bpl)
{
	PIMAGE_NT_HEADERS			pHeader			= 0;
	PIMAGE_EXPORT_DIRECTORY		pExport			= 0;
	DWORD						*pFuncNames		= 0;
	WORD						*pFuncOrdinals	= 0;
	DWORD						*pFuncAddr		= 0;
	DWORD						pName			= 0;
	DWORD						imageBase		= 0;
	char*						szDll			= 0;
	bool						result			= 0;
    bool    haveInitializeFunc = false;
    bool    haveFinalizeFunc = false;
    bool    haveGetPackageInfoTableFunc = false;

	HMODULE hLib = LoadLibraryEx(bpl, 0, LOAD_LIBRARY_AS_DATAFILE);
	imageBase = (DWORD)(DWORD_PTR)hLib;

	if (hLib)
    {
		pHeader = ImageNtHeader(hLib);

		if (pHeader && pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress &&
			pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
		{
			pExport = (PIMAGE_EXPORT_DIRECTORY)(DWORD_PTR)
					(pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + imageBase);

			szDll = (char*)(imageBase + pExport->Name);

			pFuncOrdinals	=(WORD*) (imageBase + pExport->AddressOfNameOrdinals);
			pFuncNames		=(DWORD*)(imageBase + pExport->AddressOfNames);
			pFuncAddr		=(DWORD*)(imageBase + pExport->AddressOfFunctions);

			for (int i = 0; i < pExport->NumberOfFunctions; i++)
			{
				int index = -1;
				for (int j = 0; j < pExport->NumberOfNames; j++)
				{
					if (pFuncOrdinals[j] == i)
                    {
						index = j;
						break;
					}
				}
				if (index != -1)
				{
                    pName = pFuncNames[index];
                    String curFunc = String((char*)imageBase + pName);
                    //Every BPL has a function called @GetPackageInfoTable, Initialize and Finalize.
                    //lets catch it!
                    if (!haveInitializeFunc) haveInitializeFunc = (curFunc == "Initialize");
                    if (!haveFinalizeFunc) haveFinalizeFunc = (curFunc == "Finalize");
                    if (!haveGetPackageInfoTableFunc) haveGetPackageInfoTableFunc = (curFunc == "@GetPackageInfoTable");
				}
                if (haveInitializeFunc && haveFinalizeFunc && haveGetPackageInfoTableFunc) break;
			}

			result = haveInitializeFunc && haveFinalizeFunc;
		}
		FreeLibrary(hLib);
	}
	return result;
}
//---------------------------------------------------------------------------
//toAdr:dec reg
int __fastcall IsInitStackViaLoop(DWORD fromAdr, DWORD toAdr)
{
    int         stackSize = 0;
    DWORD       dd, curAdr;
    int         instrLen;
    DISINFO     _disInfo;

    curAdr = fromAdr;
    while (curAdr <= toAdr)
    {
        instrLen = Disasm.Disassemble(curAdr, &_disInfo, 0);
        //if (!instrLen) return 0;
        if (!instrLen)
        {
            curAdr++;
            continue;
        }
        dd = *((DWORD*)_disInfo.Mnem);
        //push ...
        if (dd == 'hsup')
        {
            stackSize += 4;
            curAdr += instrLen;
            continue;
        }
        //add esp, ...
        if (dd == 'dda' && _disInfo.OpType[0] == otREG && _disInfo.OpRegIdx[0] == 20 && _disInfo.OpType[1] == otIMM)
        {
            if ((int)_disInfo.Immediate < 0) stackSize -= (int)_disInfo.Immediate;
            curAdr += instrLen;
            continue;
        }
        //sub esp, ...
        if (dd == 'bus' && _disInfo.OpType[0] == otREG && _disInfo.OpRegIdx[0] == 20 && _disInfo.OpType[1] == otIMM)
        {
            if ((int)_disInfo.Immediate > 0) stackSize += (int)_disInfo.Immediate;
            curAdr += instrLen;
            continue;
        }
        //dec
        if (dd == 'ced')
        {
            curAdr += instrLen;
            if (curAdr == toAdr) return stackSize;
        }
        break;
    }
    return 0;
}
//---------------------------------------------------------------------------
int IdxToIdx32Tab[24] = {
16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18, 19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21, 22, 23
};
int __fastcall GetReg32Idx(int Idx)
{
    return IdxToIdx32Tab[Idx];
}
//---------------------------------------------------------------------------
bool __fastcall IsSameRegister(int Idx1, int Idx2)
{
    return (GetReg32Idx(Idx1) == GetReg32Idx(Idx2));
}
//---------------------------------------------------------------------------
//Is register al, ah, ax, eax, dl, dh, dx, edx, cl, ch, cx, ecx
bool __fastcall IsADC(int Idx)
{
    int     _idx = GetReg32Idx(Idx);
    return (_idx == 16 || _idx == 17 || _idx == 18);
}
//---------------------------------------------------------------------------
bool __fastcall IsAnalyzedAdr(DWORD Adr)
{
    bool    analyze = false;
    for (int n = 0; n < SegmentList->Count; n++)
    {
        PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
        if (segInfo->Start <= Adr && Adr < segInfo->Start + segInfo->Size)
        {
            if (!(segInfo->Flags & 0x80000)) analyze = true;
            break;
        }
    }
    return analyze;
}
//---------------------------------------------------------------------------
//Check that fromAdr is BoundErr sequence
int __fastcall IsBoundErr(DWORD fromAdr)
{
    int         _pos, _instrLen;
    DWORD       _adr;
    PInfoRec    _recN;
    DISINFO     _disInfo;

    _pos = Adr2Pos(fromAdr); _adr = fromAdr;
    while (IsFlagSet(cfSkip, _pos))
    {
        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
        _adr += _instrLen;
        if (_disInfo.Call && IsValidImageAdr(_disInfo.Immediate))
        {
            _recN = GetInfoRec(_disInfo.Immediate);
            if (_recN->SameName("@BoundErr")) return _adr - fromAdr;
        }
        _pos += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
bool __fastcall IsConnected(DWORD fromAdr, DWORD toAdr)
{
    int         n, _pos, _instrLen;
    DWORD       _adr;
    DISINFO     _disInfo;

    _pos = Adr2Pos(fromAdr); _adr = fromAdr;
    for (n = 0; n < 32; n++)
    {
        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
        if (_disInfo.Conditional && _disInfo.Immediate == toAdr) return true;
        _pos += _instrLen; _adr += _instrLen;
    }
    return false;
}
//---------------------------------------------------------------------------
//Check that fromAdr points to Exit
bool __fastcall IsExit(DWORD fromAdr)
{
    BYTE        _op;
    int         _pos, _instrLen;
    DWORD       _adr;
    DISINFO     _disInfo;

    if (!IsValidCodeAdr(fromAdr)) return 0;
    _pos = Adr2Pos(fromAdr); _adr = fromAdr;
    
    while (1)
    {
        if (!IsFlagSet(cfFinally | cfExcept, _pos)) break;
        _pos += 8; _adr += 8;
        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (_op == OP_PUSH || _op == OP_JMP)
        {
            _adr = _disInfo.Immediate;
            _pos = Adr2Pos(_adr);
        }
        else
        {
            return false;
        }
    }
    while (1)
    {
        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
        if (_disInfo.Ret) return true;
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (_op == OP_POP)
        {
            _pos += _instrLen;
            _adr += _instrLen;
            continue;
        }
        if (_op == OP_MOV && _disInfo.OpType[0] == otREG &&
           (IsSameRegister(_disInfo.OpRegIdx[0], 16) || IsSameRegister(_disInfo.OpRegIdx[0], 20)))
        {
            _pos += _instrLen;
            _adr += _instrLen;
            continue;
        }
        break;
    }
    return false;
}
//---------------------------------------------------------------------------
DWORD __fastcall IsGeneralCase(DWORD fromAdr, int retAdr)
{
    int         _regIdx = -1, _pos;
    DWORD       _dd;
    DWORD       _curAdr = fromAdr, _jmpAdr = 0;
    int         _curPos = Adr2Pos(fromAdr);
    int         _len, _num1 = 0;
    DISINFO     _disInfo;

    if (!IsValidCodeAdr(fromAdr)) return 0;

    while (1)
    {
        _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        //Switch at current address
        if (IsFlagSet(cfSwitch, _curPos))
        {
            Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'aj')
            {
                if (IsValidCodeAdr(_disInfo.Immediate))
                    return _disInfo.Immediate;
                else
                    return 0;
            }
        }
        //Switch at next address
        if (IsFlagSet(cfSwitch, _curPos + _len))
        {
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'aj')
            {
                if (IsValidCodeAdr(_disInfo.Immediate))
                    return _disInfo.Immediate;
                else
                    return 0;
            }
        }
        //cmp reg, imm
        if (_dd == 'pmc' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM)
        {
            _regIdx = _disInfo.OpRegIdx[0];
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bj' || _dd == 'gj' || _dd == 'egj')
            {
                if (IsGeneralCase(_disInfo.Immediate, retAdr))
                {
                    _curAdr += _len;
                    _curPos += _len;

                    _len = Disasm.Disassemble(Code + _curPos, (__int64)(_curAdr), &_disInfo, 0);
                    _dd = *((DWORD*)_disInfo.Mnem);
                    if (_dd == 'zj' || _dd == 'ej')
                    {
                        _curAdr += _len;
                        _curPos += _len;
                        //continue;
                    }
                    continue;
                }
                break;
            }
        }
        //sub reg, imm; dec reg
        if ((_dd == 'bus' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'ced' && _disInfo.OpType[0] == otREG))
        {
            _num1++;
            if (_regIdx == -1)
                _regIdx = _disInfo.OpRegIdx[0];
            else if (!IsSameRegister(_regIdx, _disInfo.OpRegIdx[0]))
                break;

            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bus' && IsSameRegister(_regIdx, _disInfo.OpRegIdx[0]))
            {
                _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
            }
            if (_dd == 'bj')
            {
                _curAdr += _len;
                _curPos += _len;
                _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'zj' || _dd == 'ej')
                {
                    _curAdr += _len;
                    _curPos += _len;
                }
                continue;
            }
            if (_dd == 'zj' || _dd == 'ej')
            {
                _pos = GetNearestUpInstruction(Adr2Pos(_disInfo.Immediate));
                Disasm.Disassemble(Code + _pos, (__int64)Pos2Adr(_pos), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'pmj')
                    _jmpAdr = _disInfo.Immediate;
                if (_disInfo.Ret)
                    _jmpAdr = Pos2Adr(GetLastLocPos(retAdr));
                _curAdr += _len;
                _curPos += _len;
                continue;
            }
            if (_dd == 'znj' || _dd == 'enj')
            {
                if (!_jmpAdr)
                {
                    //if only one dec or sub then it is simple if...else construction
                    if (_num1 == 1) return 0;
                    return _disInfo.Immediate;
                }
                if (_disInfo.Immediate == _jmpAdr) return _jmpAdr;
            }
            break;
        }
        //add reg, imm; inc reg
        if ((_dd == 'dda' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'cni' && _disInfo.OpType[0] == otREG))
        {
            _num1++;
            if (_regIdx == -1)
                _regIdx = _disInfo.OpRegIdx[0];
            else if (!IsSameRegister(_regIdx, _disInfo.OpRegIdx[0]))
                break;

            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bus')
            {
               _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
               _dd = *((DWORD*)_disInfo.Mnem);
               if (_dd == 'bj')
               {
                    _curAdr += _len;
                    _curPos += _len;
                    continue;
               }
            }
            if (_dd == 'zj' || _dd == 'ej')
            {
                _curAdr += _len;
                _curPos += _len;
                continue;
            }
            break;
        }
        if (_dd == 'pmj')
        {
            if (IsValidCodeAdr(_disInfo.Immediate))
                return _disInfo.Immediate;
            else
                return 0;
        }
        break;
    }
    return 0;
}
//---------------------------------------------------------------------------
//check
//xor reg, reg
//mov reg,...
bool __fastcall IsXorMayBeSkipped(DWORD fromAdr)
{
    DWORD       _curAdr = fromAdr, _dd;
    int         _instrlen, _regIdx;
    int         _curPos = Adr2Pos(fromAdr);
    DISINFO     _disInfo;

    _instrlen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _dd = *((DWORD*)_disInfo.Mnem);
    if (_dd == 'rox' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otREG && _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])
    {
        _regIdx = _disInfo.OpRegIdx[0];
        _curPos += _instrlen; _curAdr += _instrlen;
        Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'vom' && _disInfo.OpType[0] == otREG && IsSameRegister(_disInfo.OpRegIdx[0], _regIdx)) return true;
    }
    return false;
}
//---------------------------------------------------------------------------
String __fastcall UnmangleName(String Name)
{
    int     pos;
    //skip first '@'
    String  Result = Name.SubString(2, Name.Length());
    String  LeftPart = Result;
    String  RightPart = "";

    int breakPos = Result.Pos("$");
    if (breakPos)
    {
        if (breakPos == 1)//
        LeftPart = Result.SubString(1, breakPos - 1);
        RightPart = Result.SubString(breakPos + 1, Result.Length());
    }
    if (*LeftPart.AnsiLastChar() == '@')
        LeftPart.SetLength(LeftPart.Length() - 1);
    while (1)
    {
        pos = LeftPart.Pos("@@");
        if (!pos) break;
        LeftPart[pos + 1] = '_';
    }
    int num = 0;
    while (1)
    {
        pos = LeftPart.Pos("@");
        if (!pos) break;
        LeftPart[pos] = '.';
        num++;
    }
    while (1)
    {
        pos = LeftPart.Pos("._");
        if (!pos) break;
        LeftPart[pos + 1] = '@';
    }
}
//---------------------------------------------------------------------------
//Check construction (after cdq)
//xor eax, edx
//sub eax, edx
//return bytes to skip, if Abs, else return 0
int __fastcall IsAbs(DWORD fromAdr)
{
    int         _curPos = Adr2Pos(fromAdr), _instrLen;
    DWORD       _dd, _curAdr = fromAdr;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _dd = *((DWORD*)_disInfo.Mnem);
    if (_dd == 'rox' &&
        _disInfo.OpType[0] == otREG &&
        _disInfo.OpType[1] == otREG &&
        _disInfo.OpRegIdx[0] == 16 &&
        _disInfo.OpRegIdx[1] == 18)
    {
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'bus' &&
            _disInfo.OpType[0] == otREG &&
            _disInfo.OpType[1] == otREG &&
            _disInfo.OpRegIdx[0] == 16 &&
            _disInfo.OpRegIdx[1] == 18)
        {
            return (_curAdr + _instrLen) - fromAdr;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction
//jxx @1
//call @IntOver
//@1:
//return bytes to skip, if @IntOver, else return 0
int _fastcall IsIntOver(DWORD fromAdr)
{
    int         _instrLen, _curPos = Adr2Pos(fromAdr);
    DWORD       _curAdr = fromAdr;
    PInfoRec    _recN;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    if (_disInfo.Branch && _disInfo.Conditional)
    {
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        if (_disInfo.Call)
        {
            if (IsValidCodeAdr(_disInfo.Immediate))
            {
                _recN = GetInfoRec(_disInfo.Immediate);
                if (_recN && _recN->SameName("@IntOver")) return (_curAdr + _instrLen) - fromAdr;
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction (test reg, reg)
//test reg, reg
//jz @1
//mov reg, [reg-4]
//or-------------------------------------------------------------------------
//test reg, reg
//jz @1
//sub reg, 4
//mov reg, [reg]
int __fastcall IsInlineLengthTest(DWORD fromAdr)
{
    int         _curPos = Adr2Pos(fromAdr), _instrLen, _regIdx = -1;
    DWORD       _dd, _adr = 0, _curAdr = fromAdr;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _dd = *((DWORD*)_disInfo.Mnem);
    if (_dd == 'tset' &&
        _disInfo.OpType[0] == otREG &&
        _disInfo.OpType[1] == otREG &&
        _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])
    {
        _regIdx = _disInfo.OpRegIdx[0];
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'zj' || _dd == 'ej')
        {
            _adr = _disInfo.Immediate;
            _curPos += _instrLen; _curAdr += _instrLen;
            _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            //mov reg, [reg-4]
            if (_dd == 'vom' &&
                _disInfo.OpType[0] == otREG &&
                _disInfo.OpType[1] == otMEM &&
                _disInfo.BaseReg == _regIdx &&
                _disInfo.IndxReg == -1 &&
                _disInfo.Offset == -4)
            {
                if (_adr == _curAdr + _instrLen) return (_curAdr + _instrLen) - fromAdr;
            }
            //sub reg, 4
            if (_dd == 'bus' &&
                _disInfo.OpType[0] == otREG &&
                _disInfo.OpType[1] == otIMM &&
                _disInfo.Immediate == 4)
            {
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                //mov reg, [reg]
                if (_dd == 'vom' &&
                    _disInfo.OpType[0] == otREG &&
                    _disInfo.OpType[1] == otMEM &&
                    _disInfo.BaseReg == _regIdx &&
                    _disInfo.IndxReg == -1 &&
                    _disInfo.Offset == 0)
                {
                    if (_adr == _curAdr + _instrLen) return (_curAdr + _instrLen) - fromAdr;
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//cmp [lvar], 0
//jz @1
//mov reg, [lvar]
//sub reg, 4
//mov reg, [reg]
//mov [lvar], reg
int __fastcall IsInlineLengthCmp(DWORD fromAdr)
{
    BYTE        _op;
    int         _curPos = Adr2Pos(fromAdr), _instrLen, _regIdx = -1;
    int         _baseReg, _offset;
    DWORD       _dd, _adr = 0, _curAdr = fromAdr;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _op = Disasm.GetOp(_disInfo.Mnem);
    if (_op == OP_CMP &&
        _disInfo.OpType[0] == otMEM &&
        _disInfo.OpType[1] == otIMM &&
        _disInfo.Immediate == 0)
    {
        _baseReg = _disInfo.BaseReg;
        _offset = _disInfo.Offset;
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'zj' || _dd == 'ej')
        {
            _adr = _disInfo.Immediate;
            _curPos += _instrLen; _curAdr += _instrLen;
            _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _op = Disasm.GetOp(_disInfo.Mnem);
            //mov reg, [lvar]
            if (_op == OP_MOV &&
                _disInfo.OpType[0] == otREG &&
                _disInfo.OpType[1] == otMEM &&
                _disInfo.BaseReg == _baseReg &&
                _disInfo.Offset == _offset)
            {
                _regIdx = _disInfo.OpRegIdx[0];
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _op = Disasm.GetOp(_disInfo.Mnem);
                //sub reg, 4
                if (_op == OP_SUB &&
                    _disInfo.OpType[0] == otREG &&
                    _disInfo.OpType[1] == otIMM &&
                    _disInfo.Immediate == 4)
                {
                    _curPos += _instrLen; _curAdr += _instrLen;
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    //mov reg, [reg]
                    if (_op == OP_MOV &&
                        _disInfo.OpType[0] == otREG &&
                        _disInfo.OpType[1] == otMEM &&
                        _disInfo.BaseReg == _regIdx &&
                        _disInfo.IndxReg == -1 &&
                        _disInfo.Offset == 0)
                    {
                        _curPos += _instrLen; _curAdr += _instrLen;
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                        _op = Disasm.GetOp(_disInfo.Mnem);
                        //mov [lvar], reg
                        if (_op == OP_MOV &&
                            _disInfo.OpType[0] == otMEM &&
                            _disInfo.OpType[1] == otREG &&
                            _disInfo.BaseReg == _baseReg &&
                            _disInfo.Offset == _offset)
                        {
                            if (_adr == _curAdr + _instrLen) return (_curAdr + _instrLen) - fromAdr;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//test reg, reg
//jns @1
//add reg, (2^k - 1)
//sar reg, k
//@1
int __fastcall IsInlineDiv(DWORD fromAdr, int* div)
{
    BYTE        _op;
    int         _curPos = Adr2Pos(fromAdr), _instrLen, _regIdx = -1;
    DWORD       _dd, _adr, _curAdr = fromAdr, _imm;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _op = Disasm.GetOp(_disInfo.Mnem);
    if (_op == OP_TEST &&
        _disInfo.OpType[0] == otREG &&
        _disInfo.OpType[1] == otREG &&
        _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])
    {
        _regIdx = _disInfo.OpRegIdx[0];
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'snj')
        {
            _adr = _disInfo.Immediate;
            _curPos += _instrLen; _curAdr += _instrLen;
            _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _op = Disasm.GetOp(_disInfo.Mnem);
            if (_op == OP_ADD &&
                _disInfo.OpType[0] == otREG &&
                _disInfo.OpRegIdx[0] == _regIdx &&
                _disInfo.OpType[1] == otIMM)
            {
                _imm = _disInfo.Immediate + 1;
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _op = Disasm.GetOp(_disInfo.Mnem);
                if (_op == OP_SAR &&
                    _disInfo.OpType[0] == otREG &&
                    _disInfo.OpRegIdx[0] == _regIdx &&
                    _disInfo.OpType[1] == otIMM)
                {
                    if (((DWORD)1 << _disInfo.Immediate) == _imm)
                    {
                        *div = _imm;
                        return (_curAdr + _instrLen) - fromAdr;
                    }
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//and reg, imm
//jns @1
//dec reg
//or reg, imm
//inc reg
//@1
int __fastcall IsInlineMod(DWORD fromAdr, int* mod)
{
    BYTE        _op;
    int         _curPos = Adr2Pos(fromAdr), _instrLen, _regIdx = -1;
    DWORD       _dd, _adr = 0, _curAdr = fromAdr, _imm;
    DISINFO     _disInfo;

    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    _op = Disasm.GetOp(_disInfo.Mnem);
    if (_op == OP_AND &&
        _disInfo.OpType[0] == otREG &&
        _disInfo.OpType[1] == otIMM &&
        (_disInfo.Immediate & 0x80000000) != 0)
    {
        _regIdx = _disInfo.OpRegIdx[0];
        _imm = _disInfo.Immediate & 0x7FFFFFFF;
        _curPos += _instrLen; _curAdr += _instrLen;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'snj')
        {
            _adr = _disInfo.Immediate;
            _curPos += _instrLen; _curAdr += _instrLen;
            _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _op = Disasm.GetOp(_disInfo.Mnem);
            if (_op == OP_DEC &&
                _disInfo.OpType[0] == otREG &&
                _disInfo.OpRegIdx[0] == _regIdx)
            {
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _op = Disasm.GetOp(_disInfo.Mnem);
                if (_op == OP_OR &&
                    _disInfo.OpType[0] == otREG &&
                    _disInfo.OpType[1] == otIMM &&
                    _disInfo.OpRegIdx[0] == _regIdx &&
                    _disInfo.Immediate + _imm == 0xFFFFFFFF)
                {
                    _curPos += _instrLen; _curAdr += _instrLen;
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_op == OP_INC &&
                        _disInfo.OpType[0] == otREG &&
                        _disInfo.OpRegIdx[0] == _regIdx)
                    {
                        if (_adr == _curAdr + _instrLen)
                        {
                            *mod = _imm + 1;
                            return (_curAdr + _instrLen) - fromAdr;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall GetLastLocPos(int fromAdr)
{
    int     _pos = Adr2Pos(fromAdr);
    while (1)
    {
        if (IsFlagSet(cfLoc, _pos)) return _pos;
        _pos--;
    }
}
//---------------------------------------------------------------------------
bool __fastcall IsDefaultName(String AName)
{
    for (int Idx = 0; Idx < 8; Idx++)
    {
        if (SameText(AName, String(Reg32Tab[Idx]))) return true;
    }
    if (SameText(AName.SubString(1, 5), "lvar_")) return true;
    if (SameText(AName.SubString(1, 5), "gvar_")) return true;
    return false; 
}
//---------------------------------------------------------------------------
int __fastcall FloatNameToFloatType(String AName)
{
    if (SameText(AName, "Single")) return FT_SINGLE;
    if (SameText(AName, "Double")) return FT_DOUBLE;
    if (SameText(AName, "Extended")) return FT_EXTENDED;
    if (SameText(AName, "Real")) return FT_REAL;
    if (SameText(AName, "Comp")) return FT_COMP;
    if (SameText(AName, "Currency")) return FT_CURRENCY;
    return -1;
}
//---------------------------------------------------------------------------
String __fastcall InputDialogExec(String caption, String labelText, String text)
{
    String _result = "";

    FInputDlg_11011981->Caption = caption;
    FInputDlg_11011981->edtName->EditLabel->Caption = labelText;
    FInputDlg_11011981->edtName->Text = text;
    while (_result == "")
    {
        if (FInputDlg_11011981->ShowModal() == mrCancel) break;
        _result = FInputDlg_11011981->edtName->Text.Trim();
    }
    return _result;
}
//---------------------------------------------------------------------------
String __fastcall ManualInput(DWORD procAdr, DWORD curAdr, String caption, String labelText)
{
    String _result = "";

    FMain_11011981->ShowCode(procAdr, curAdr, FMain_11011981->lbCXrefs->ItemIndex, -1);
    FInputDlg_11011981->Caption = caption;
    FInputDlg_11011981->edtName->EditLabel->Caption = labelText;
    FInputDlg_11011981->edtName->Text = "";
    while (_result == "")
    {
        if (FInputDlg_11011981->ShowModal() == mrCancel) break;
        _result = FInputDlg_11011981->edtName->Text.Trim();
    }
    return _result;
}
//---------------------------------------------------------------------------
bool __fastcall MatchCode(BYTE* code, MProcInfo* pInfo)
{
	if (!code || !pInfo) return false;

    DWORD _dumpSz = pInfo->DumpSz;
    //ret
    if (_dumpSz < 2) return false;

    BYTE *_dump = pInfo->Dump;
    BYTE *_relocs = _dump + _dumpSz;
    //jmp XXXXXXXX
    if (_dumpSz == 5 && _dump[0] == 0xE9 && _relocs[1] == 0xFF) return false;
    //call XXXXXXXX ret
    if (_dumpSz == 6 && _dump[0] == 0xE8 && _relocs[1] == 0xFF && _dump[5] == 0xC3) return false;

    for (int n = 0; n < _dumpSz;)
    {
        //Relos skip
        if (_relocs[n] == 0xFF)
        {
            n += 4;
            continue;
        }
        if (code[n] != _dump[n])
            return false;

        n++;
    }

    return true;
}
//---------------------------------------------------------------------------
TColor SavedPenColor;
TColor SavedBrushColor;
TColor SavedFontColor;
TBrushStyle SavedBrushStyle;
//---------------------------------------------------------------------------
void __fastcall SaveCanvas(TCanvas* ACanvas)
{
    SavedPenColor = ACanvas->Pen->Color;
    SavedBrushColor = ACanvas->Brush->Color;
    SavedFontColor = ACanvas->Font->Color;
    SavedBrushStyle = ACanvas->Brush->Style;
}
//---------------------------------------------------------------------------
void __fastcall RestoreCanvas(TCanvas* ACanvas)
{
    ACanvas->Pen->Color = SavedPenColor;
    ACanvas->Brush->Color = SavedBrushColor;
    ACanvas->Font->Color = SavedFontColor;
    ACanvas->Brush->Style = SavedBrushStyle;
}
//---------------------------------------------------------------------------
void __fastcall DrawOneItem(String AItem, TCanvas* ACanvas, TRect &ARect, TColor AColor, int flags)
{
    SaveCanvas(ACanvas);
    ARect.Left = ARect.Right;
    ARect.Right += ACanvas->TextWidth(AItem);
    TRect R1 = Rect(ARect.Left -1, ARect.Top, ARect.Right, ARect.Bottom - 1);
    if (SameText(AItem, SelectedAsmItem))
    {
        ACanvas->Brush->Color = TColor(0x80DDFF);
        ACanvas->Brush->Style = bsSolid;
        ACanvas->FillRect(R1);
        ACanvas->Brush->Style = bsClear;
        ACanvas->Pen->Color = TColor(0x226DA8);;
        ACanvas->Rectangle(R1);
    }
    ACanvas->Font->Color = AColor;
    ACanvas->TextOut(ARect.Left, ARect.Top, AItem);
    RestoreCanvas(ACanvas);
}
//---------------------------------------------------------------------------
//Check try construction
//xor reg, reg
//push ebp
//push XXXXXXXX
//push fs:[reg]
//mov fs:[reg], esp
int __fastcall IsTryBegin(DWORD fromAdr, DWORD *endAdr)
{
    BYTE        _op;
    int         _instrLen, n;
    DWORD       _curAdr, _endAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr;
    for (n = 1; n <= 5; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_XOR && _disInfo.OpType[0] == _disInfo.OpType[1] && _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])) break;
        }
        else if (n == 2)
        {
            if (!(_op == OP_PUSH && _disInfo.OpRegIdx[0] == 21)) break;
        }
        else if (n == 3)
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otIMM)) break;
            _endAdr = _disInfo.Immediate;
        }
        else if (n == 4)
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg != -1)) break;
        }
        else if (n == 5)
        {
            if (!(_op == OP_MOV && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg != -1)) break;
            *endAdr = _endAdr;
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check try construction (Delphi3:Atlantis.exe at 0x50D676)
//push ebp
//push XXXXXXXX
//push fs:[0]
//mov fs:[0], esp
int __fastcall IsTryBegin0(DWORD fromAdr, DWORD *endAdr)
{
    BYTE        _op;
    int         _instrLen, n;
    DWORD       _curAdr, _endAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr;
    for (n = 1; n <= 4; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_PUSH && _disInfo.OpRegIdx[0] == 21)) break;
        }
        else if (n == 2)
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otIMM)) break;
            _endAdr = _disInfo.Immediate;
        }
        else if (n == 3)
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg == -1 && _disInfo.Offset == 0)) break;
        }
        else if (n == 4)
        {
            if (!(_op == OP_MOV && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg == -1 && _disInfo.Offset == 0)) break;
            *endAdr = _endAdr;
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check finally construction
//xor reg,reg
//pop reg
//pop reg
//pop reg
//mov fs:[reg],reg
//push XXXXXXXX
int __fastcall IsTryEndPush(DWORD fromAdr, DWORD* endAdr)
{
    BYTE        _op;
    int         _instrLen, n;
    DWORD       _curAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr;
    for (n = 1; n <= 6; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_XOR && _disInfo.OpType[0] == _disInfo.OpType[1] && _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])) break;
        }
        else if (n >= 2 && n <= 4)
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 5)
        {
            if (!(_op == OP_MOV && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg != -1)) break;
        }
        else if (n == 6)
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otIMM)) break;
            *endAdr = _disInfo.Immediate;
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check finally construction
//xor reg,reg
//pop reg
//pop reg
//pop reg
//mov fs:[reg],reg
//jmp XXXXXXXX
int __fastcall IsTryEndJump(DWORD fromAdr, DWORD* endAdr)
{
    BYTE        _op;
    int         _instrLen, n;
    DWORD       _curAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr;
    for (n = 1; n <= 6; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_XOR && _disInfo.OpType[0] == _disInfo.OpType[1] && _disInfo.OpRegIdx[0] == _disInfo.OpRegIdx[1])) break;
        }
        else if (n >= 2 && n <= 4)
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 5)
        {
            if (!(_op == OP_MOV && _disInfo.OpType[0] == otMEM && _disInfo.SegPrefix == 4 && _disInfo.BaseReg != -1)) break;
        }
        else if (n == 6)
        {
            if (!(_op == OP_JMP && _disInfo.OpType[0] == otIMM)) break;
            *endAdr = _disInfo.Immediate;
            return _curAdr - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction equality ((Int64)val = XXX)
//cmp XXX,XXX -> set cfSkip (_skipAdr1 = address of this instruction)
//jne @1 -> set cfSkip (_skipAdr2 = address of this instruction)
//cmp XXX,XXX
//jne @1
//...
//@1:
int __fastcall ProcessInt64Equality(DWORD fromAdr, DWORD* maxAdr)
{
    BYTE        _op, _b;
    int         _instrLen, n, _curPos;
    DWORD       _curAdr, _adr1, _maxAdr;
    DWORD       _skipAdr1, _skipAdr2;
    DISINFO     _disInfo;

    _curAdr = fromAdr; _curPos = Adr2Pos(_curAdr); _maxAdr = 0;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);

        _b = *(Code + _curPos);
        if (_b == 0xF) _b = *(Code + _curPos + 1);
        _b = (_b & 0xF) + 'A';

        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            _skipAdr1 = _curAdr;
        }
        else if (n == 2)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F')) break;
            _skipAdr2 = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
            if (_adr1 > _maxAdr) _maxAdr = _adr1;
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
        }
        else if (n == 4)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F' && _disInfo.Immediate == _adr1)) break;
            *maxAdr = _maxAdr;
            SetFlag(cfSkip, Adr2Pos(_skipAdr1));
            SetFlag(cfSkip, Adr2Pos(_skipAdr2));
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen; _curPos += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction not equality ((Int64)val <> XXX)
//cmp XXX,XXX -> set cfSkip (_skipAdr1 = address of this instruction)
//jne @1 -> set cfSkip (_skipAdr2 = address of this instruction)
//cmp XXX,XXX
//je @2
//@1:
int __fastcall ProcessInt64NotEquality(DWORD fromAdr, DWORD* maxAdr)
{
    BYTE        _op, _b;
    int         _instrLen, n, _curPos;
    DWORD       _curAdr, _adr1, _adr2, _maxAdr;
    DWORD       _skipAdr1, _skipAdr2;
    DISINFO     _disInfo;

    _curAdr = fromAdr; _curPos = Adr2Pos(_curAdr); _maxAdr = 0;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);

        _b = *(Code + _curPos);
        if (_b == 0xF) _b = *(Code + _curPos + 1);
        _b = (_b & 0xF) + 'A';

        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            _skipAdr1 = _curAdr;
        }
        else if (n == 2)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F')) break;
            _skipAdr2 = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
            if (_adr1 > _maxAdr) _maxAdr = _adr1;
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
        }
        else if (n == 4)//je @2
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'E' && _curAdr + _instrLen == _adr1)) break;
            _adr2 = _disInfo.Immediate;//@2
            if (_adr2 > _maxAdr) _maxAdr = _adr2;
            *maxAdr = _maxAdr;
            SetFlag(cfSkip, Adr2Pos(_skipAdr1));
            SetFlag(cfSkip, Adr2Pos(_skipAdr2));
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen; _curPos += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) XXX)
//cmp XXX,XXX -> set cfSkip (_skipAdr1 = address of this instruction)
//jxx @1 -> set cfSkip (_skipAdr2 = address of this instruction)
//cmp XXX,XXX
//jxx @@ -> set cfSkip (_skipAdr3 = address of this instruction)
//jmp @@ set cfSkip (_skipAdr4 = address of this instruction)
//@1:jxx @@
int __fastcall ProcessInt64Comparison(DWORD fromAdr, DWORD* maxAdr)
{
    BYTE        _op;
    int         _instrLen, n;
    DWORD       _curAdr, _adr, _adr1, _maxAdr;
    DWORD       _skipAdr1, _skipAdr2, _skipAdr3, _skipAdr4;
    DISINFO     _disInfo;

    _curAdr = fromAdr; _maxAdr = 0;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            _skipAdr1 = _curAdr;
        }
        else if (n == 2)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _skipAdr2 = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
            if (_adr1 > _maxAdr) _maxAdr = _adr1;
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
        }
        else if (n == 4)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _skipAdr3 = _curAdr;
            _adr = _disInfo.Immediate;//@@
            if (_adr > _maxAdr) _maxAdr = _adr;
        }
        else if (n == 5)//jmp @@
        {
            if (!(_disInfo.Branch && !_disInfo.Conditional)) break;
            _skipAdr4 = _curAdr;
            _adr = _disInfo.Immediate;//@@
            if (_adr > _maxAdr) _maxAdr = _adr;
        }
        else if (n == 6)////@1:jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _curAdr == _adr1)) break;
            _adr = _disInfo.Immediate;//@@
            if (_adr > _maxAdr) _maxAdr = _adr;
            *maxAdr = _maxAdr;
            SetFlag(cfSkip, Adr2Pos(_skipAdr1));
            SetFlag(cfSkip, Adr2Pos(_skipAdr2));
            SetFlag(cfSkip, Adr2Pos(_skipAdr3));
            SetFlag(cfSkip, Adr2Pos(_skipAdr4));
            return  _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) XXX)
//push reg
//push reg
//...
//cmp XXX,[esp+4] (m-th row) set cfSkip (_skipAdr1)
//jxx @1 ->set cfSkip (_skipAdr2)
//cmp XXX,[esp]
//@1:pop reg
//pop reg
//jxx @2
int __fastcall ProcessInt64ComparisonViaStack1(DWORD fromAdr, DWORD* maxAdr)
{
    BYTE        _op;
    int         _instrLen, n, m, _skip, _pos;
    DWORD       _curAdr, _adr1, _adr2, _adr3, _maxAdr, _pushAdr;
    DWORD       _skipAdr1, _skipAdr2;
    DISINFO     _disInfo;

    _curAdr = fromAdr; m = -1; _maxAdr = 0;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 2)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
            _pushAdr = _curAdr;
        }
        else if (n >= 3 && m == -1 && _op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 4)//cmp XXX,[esp+4]
        {
            //Find nearest up instruction "push reg"
            _pos = Adr2Pos(_curAdr);
            while (1)
            {
                _pos--;
                if (_pos == fromAdr) break;
                if (IsFlagSet(cfInstruction, _pos))
                {
                    Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_op == OP_PUSH) break;
                }
            }
            if (Pos2Adr(_pos) != _pushAdr) return 0;
            m = n;
            _skipAdr1 = _curAdr;
        }
        else if (m != -1 && n == m + 1)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _skipAdr2 = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
            if (_adr1 > _maxAdr) _maxAdr = _adr1;
        }
        else if (m != -1 && n == m + 2)//cmp XXX,[esp]
        {
            if (!(_op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 0)) break;
        }
        else if (m != -1 && n == m + 3)//@1:pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG && _curAdr == _adr1)) break;
        }
        else if (m != -1 && n == m + 4)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 5)//jxx @2
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            *maxAdr = _maxAdr;
            SetFlag(cfSkip, Adr2Pos(_skipAdr1));
            SetFlag(cfSkip, Adr2Pos(_skipAdr2));
            return  _curAdr + _instrLen - fromAdr;
        }
        if (m == -1 && (_disInfo.Ret || _disInfo.Branch)) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) XXX)
//push reg
//push reg
//...
//cmp XXX,[esp+4] (m-th row) set cfSkip (_skipAdr1)
//jxx @1 ->set cfSkip (_skipAdr2)
//cmp XXX,[esp]
//pop reg ->set cfSkip (_skipAdr3)
//pop reg ->set cfSkip (_skipAdr4)
//jxx @@ ->set cfSkip (_skipAdr5)
//jmp @@ ->set cfSkip (_skipAdr6)
//@1:
//pop reg
//pop reg
//jxx @2
int __fastcall ProcessInt64ComparisonViaStack2(DWORD fromAdr, DWORD* maxAdr)
{
    BYTE        _op;
    int         _instrLen, n, m, _skip, _pos;
    DWORD       _curAdr, _adr, _adr1, _adr2, _maxAdr, _pushAdr;
    DWORD       _skipAdr1, _skipAdr2, _skipAdr3, _skipAdr4, _skipAdr5, _skipAdr6;
    DISINFO     _disInfo;

    _curAdr = fromAdr; m = -1; _maxAdr = 0;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 2)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
            _pushAdr = _curAdr;
        }
        else if (n >= 3 && m == -1 && _op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 4)//cmp XXX,[esp+4]
        {
            //Find nearest up instruction "push reg"
            _pos = Adr2Pos(_curAdr);
            while (1)
            {
                _pos--;
                if (_pos == fromAdr) break;
                if (IsFlagSet(cfInstruction, _pos))
                {
                    Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_op == OP_PUSH) break;
                }
            }
            if (Pos2Adr(_pos) != _pushAdr) return 0;
            m = n;
            _skipAdr1 = _curAdr;
        }
        else if (m != -1 && n == m + 1)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _skipAdr2 = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
            if (_adr1 > _maxAdr) _maxAdr = _adr1;
        }
        else if (m != -1 && n == m + 2)//cmp XXX,[esp]
        {
            if (!(_op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 0)) break;
        }
        else if (m != -1 && n == m + 3)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
            _skipAdr3 = _curAdr;
        }
        else if (m != -1 && n == m + 4)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
            _skipAdr4 = _curAdr;
        }
        else if (m != -1 && n == m + 5)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _skipAdr5 = _curAdr;
            _adr = _disInfo.Immediate;//@3
            if (_adr > _maxAdr) _maxAdr = _adr;
        }
        else if (m != -1 && n == m + 6)//jmp @@
        {
            if (!(_disInfo.Branch && !_disInfo.Conditional)) break;
            _skipAdr6 = _curAdr;
            _adr = _disInfo.Immediate;//@2
            if (_adr > _maxAdr) _maxAdr = _adr;
        }
        else if (m != -1 && n == m + 7)//@1:pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG && _curAdr == _adr1)) break;
        }
        else if (m != -1 && n == m + 8)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 9)//jxx @2
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _adr2 = _disInfo.Immediate;
            if (_adr2 > _maxAdr) _maxAdr = _adr2;
            *maxAdr = _maxAdr;
            SetFlag(cfSkip, Adr2Pos(_skipAdr1));
            SetFlag(cfSkip, Adr2Pos(_skipAdr2));
            SetFlag(cfSkip, Adr2Pos(_skipAdr3));
            SetFlag(cfSkip, Adr2Pos(_skipAdr4));
            SetFlag(cfSkip, Adr2Pos(_skipAdr5));
            SetFlag(cfSkip, Adr2Pos(_skipAdr6));
            return  _curAdr + _instrLen - fromAdr;
        }
        if (m == -1 && (_disInfo.Ret || _disInfo.Branch)) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction equality ((Int64)val = XXX)
//cmp XXX,XXX
//jne @1 (_br1Adr = address of this instruction)
//cmp XXX,XXX ->skip1 up to this instruction
//jne @1 -> skip2 up to this instruction, Result = skip2
//...
//@1:... -> delete 1 xRef to this instruction (address = _adr1)
int __fastcall IsInt64Equality(DWORD fromAdr, int* skip1, int* skip2, bool *immVal, __int64* Val)
{
    bool        _imm;
    BYTE        _op, _b;
    int         _instrLen, n, _curPos, _skip;
    DWORD       _curAdr, _adr1, _br1Adr;
    DISINFO     _disInfo;
    __int64     _val1, _val2;
    //PInfoRec    _recN;

    _curAdr = fromAdr; _curPos = Adr2Pos(_curAdr); _imm = false;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);

        _b = *(Code + _curPos);
        if (_b == 0xF) _b = *(Code + _curPos + 1);
        _b = (_b & 0xF) + 'A';

        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val1 = _disInfo.Immediate;
            }
        }
        else if (n == 2)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F')) break;
            _br1Adr = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            *skip1 = _curAdr - fromAdr;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val2 = _disInfo.Immediate;
            }
        }
        else if (n == 4)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F' && _disInfo.Immediate == _adr1)) break;
            _skip = _curAdr - fromAdr;
            *skip2 = _skip;
            *immVal = _imm;
            if (_imm) *Val = (_val1 << 32) | _val2;
            return _skip;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen; _curPos += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction not equality ((Int64)val <> XXX)
//cmp XXX,XXX
//jne @1 (_br1Adr = address of this instruction)
//cmp XXX,XXX ->skip1 up to this instruction
//je @2 -> skip2 up to this instruction, Result = skip2
//@1:... -> delete 1 xRef to this instruction (address = _adr1)
int __fastcall IsInt64NotEquality(DWORD fromAdr, int* skip1, int* skip2, bool *immVal, __int64* Val)
{
    bool        _imm;
    BYTE        _op, _b;
    int         _instrLen, n, _curPos, _skip;
    DWORD       _curAdr, _adr1, _adr2, _br1Adr;
    DISINFO     _disInfo;
    __int64     _val1, _val2;
    //PInfoRec    _recN;

    _curAdr = fromAdr; _curPos = Adr2Pos(_curAdr); _imm = false;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);

        _b = *(Code + _curPos);
        if (_b == 0xF) _b = *(Code + _curPos + 1);
        _b = (_b & 0xF) + 'A';

        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val1 = _disInfo.Immediate;
            }
        }
        else if (n == 2)//jne @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'F')) break;
            _br1Adr = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            *skip1 = _curAdr - fromAdr;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val2 = _disInfo.Immediate;
            }
        }
        else if (n == 4)//je @2
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _b == 'E' && _curAdr + _instrLen == _adr1)) break;
            _skip = _curAdr - fromAdr;
            *skip2 = _skip;
            *immVal = _imm;
            if (_imm) *Val = (_val1 << 32) | _val2;
            return _skip;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen; _curPos += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) XXX)
//cmp XXX,XXX
//jxx @1 (_br1Adr = address of this instruction)
//cmp XXX,XXX ->skip1 up to this instruction
//jxx @@ (_br3Adr = address of this instruction)
//jmp @@ (_br2Adr = address of this instruction)
//@1:jxx @@ (skip2 up to this instruction, Result = skip2)
int __fastcall IsInt64Comparison(DWORD fromAdr, int* skip1, int* skip2, bool* immVal, __int64* Val)
{
    bool        _imm;
    BYTE        _op;
    int         _instrLen, n, m, _skip;
    __int64     _val1, _val2;
    DWORD       _curAdr, _adr1, _br1Adr, _br2Adr, _br3Adr;
    DISINFO     _disInfo;
    //PInfoRec    _recN;

    _curAdr = fromAdr; _imm = false;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val1 = _disInfo.Immediate;
            }
        }
        else if (n == 2)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _br1Adr = _curAdr;
            _adr1 = _disInfo.Immediate;//@1
        }
        else if (n == 3)//cmp XXX,XXX
        {
            if (!(_op == OP_CMP)) break;
            *skip1 = _curAdr - fromAdr;
            if (_disInfo.OpType[1] == otIMM)
            {
                _imm = true;
                _val2 = _disInfo.Immediate;
            }
        }
        else if (n == 4)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _br3Adr = _curAdr;
        }
        else if (n == 5)//jmp @@
        {
            if (!(_disInfo.Branch && !_disInfo.Conditional)) break;
            _br2Adr = _curAdr;
        }
        else if (n == 6)////@1:jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional && _curAdr == _adr1)) break;
            _skip = _curAdr - fromAdr;
            *skip2 = _skip;
            *immVal = _imm;
            if (_imm) *Val = (_val1 << 32) | _val2;
            return _skip;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) N)
//push reg
//push reg
//...
//cmp XXX,[esp+4] (m-th row) ->Simulate upto this address
//jxx @1
//cmp XXX,[esp] ->skip1=this position
//@1:pop reg
//pop reg
//jxx @@ ->Result
int __fastcall IsInt64ComparisonViaStack1(DWORD fromAdr, int* skip1, DWORD* simEnd)
{
    BYTE        _op;
    int         _instrLen, n, m, _pos;
    DWORD       _curAdr = fromAdr, _adr1, _pushAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr; *skip1 = 0; *simEnd = 0; m = -1;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 2)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
            _pushAdr = _curAdr;
        }
        else if (n >= 3 && m == -1 && _op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 4)//cmp XXX,[esp+4]
        {
            //Find nearest up instruction "push reg"
            _pos = Adr2Pos(_curAdr);
            while (1)
            {
                _pos--;
                if (_pos == fromAdr) break;
                if (IsFlagSet(cfInstruction, _pos))
                {
                    Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_op == OP_PUSH) break;
                }
            }
            if (Pos2Adr(_pos) != _pushAdr) return 0;
            m = n;
            *simEnd = _curAdr;
        }
        else if (m != -1 && n == m + 1)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _adr1 = _disInfo.Immediate;//@1
        }
        else if (m != -1 && n == m + 2)//cmp XXX,[esp]
        {
            if (!(_op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 0)) break;
            *skip1 = _curAdr - fromAdr;
        }
        else if (m != -1 && n == m + 3)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG && _curAdr == _adr1)) break;
        }
        else if (m != -1 && n == m + 4)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 5)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            return  _curAdr - fromAdr;
        }
        if (m == -1 && (_disInfo.Ret || _disInfo.Branch)) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction comparison ((Int64)val >(<) XXX)
//push reg
//push reg
//...
//cmp XXX,[esp+4] (m-th row) ->Simulate upto this address
//jxx @1
//cmp XXX,[esp] ->skip1=this position
//pop reg
//pop reg
//jxx @@ ->skip2
//jmp @@
//@1:
//pop reg
//pop reg
//jxx @@ ->Result
int __fastcall IsInt64ComparisonViaStack2(DWORD fromAdr, int* skip1, int* skip2, DWORD* simEnd)
{
    BYTE        _op;
    int         _instrLen, n, m, _pos;
    DWORD       _curAdr = fromAdr, _adr1, _pushAdr;
    DISINFO     _disInfo;

    _curAdr = fromAdr; *simEnd = 0; m = -1;
    for (n = 1; n <= 1024; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
        }
        else if (n == 2)//push reg
        {
            if (!(_op == OP_PUSH && _disInfo.OpType[0] == otREG)) break;
            _pushAdr = _curAdr;
        }
        else if (n >= 3 && m == -1 && _op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 4)//cmp XXX,[esp+4]
        {
            //Find nearest up instruction "push reg"
            _pos = Adr2Pos(_curAdr);
            while (1)
            {
                _pos--;
                if (_pos == fromAdr) break;
                if (IsFlagSet(cfInstruction, _pos))
                {
                    Disasm.Disassemble(Pos2Adr(_pos), &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_op == OP_PUSH) break;
                }
            }
            if (Pos2Adr(_pos) != _pushAdr) return 0;
            m = n;
            *simEnd = _curAdr;
        }
        else if (m != -1 && n == m + 1)//jxx @1
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            _adr1 = _disInfo.Immediate;//@1
        }
        else if (m != -1 && n == m + 2)//cmp XXX,[esp]
        {
            if (!(_op == OP_CMP && _disInfo.OpType[1] == otMEM && _disInfo.BaseReg == 20 && _disInfo.Offset == 0)) break;
            *skip1 = _curAdr - fromAdr;
        }
        else if (m != -1 && n == m + 3)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 4)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 5)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            *skip2 = _curAdr - fromAdr;
        }
        else if (m != -1 && n == m + 6)//jmp @@
        {
            if (!(_disInfo.Branch && !_disInfo.Conditional)) break;
        }
        else if (m != -1 && n == m + 7)//@1:pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG && _curAdr == _adr1)) break;
        }
        else if (m != -1 && n == m + 8)//pop reg
        {
            if (!(_op == OP_POP && _disInfo.OpType[0] == otREG)) break;
        }
        else if (m != -1 && n == m + 9)//jxx @@
        {
            if (!(_disInfo.Branch && _disInfo.Conditional)) break;
            return  _curAdr - fromAdr;
        }
        if (m == -1 && (_disInfo.Ret || _disInfo.Branch)) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction
//shrd reg1,reg2,N
//shr reg2,N
int __fastcall IsInt64Shr(DWORD fromAdr)
{
    BYTE        _op;
    int         _instrLen, n, _idx, _val;
    DWORD       _curAdr = fromAdr;
    DISINFO     _disInfo;

    for (n = 1; n <= 2; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_SHR && _disInfo.OpNum == 3 && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otREG && _disInfo.OpType[2] == otIMM)) break;
            _idx = _disInfo.OpRegIdx[1];
            _val = _disInfo.Immediate;
        }
        else if (n == 2)
        {
            if (!(_op == OP_SHR && _disInfo.OpNum == 2 && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM && _disInfo.OpRegIdx[0] == _idx && _disInfo.Immediate == _val)) break;
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------
//Check construction
//shld reg1,reg2,N
//shl reg2,N
int __fastcall IsInt64Shl(DWORD fromAdr)
{
    BYTE        _op;
    int         _instrLen, n, _idx, _val;
    DWORD       _curAdr = fromAdr;
    DISINFO     _disInfo;

    for (n = 1; n <= 2; n++)
    {
        _instrLen = Disasm.Disassemble(_curAdr, &_disInfo, 0);
        _op = Disasm.GetOp(_disInfo.Mnem);
        if (n == 1)
        {
            if (!(_op == OP_SHL && _disInfo.OpNum == 3 && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otREG && _disInfo.OpType[2] == otIMM)) break;
            _idx = _disInfo.OpRegIdx[1];
            _val = _disInfo.Immediate;
        }
        else if (n == 2)
        {
            if (!(_op == OP_SHL && _disInfo.OpNum == 2 && _disInfo.OpType[0] == otREG && _disInfo.OpType[2] == otIMM && _disInfo.OpRegIdx[0] == _idx &&_disInfo.Immediate == _val)) break;
            return _curAdr + _instrLen - fromAdr;
        }
        if (_disInfo.Ret) return 0;
        _curAdr += _instrLen;
    }
    return 0;
}
//---------------------------------------------------------------------------


