//---------------------------------------------------------------------------
#include <vcl.h> 
#include <StrUtils.hpp>
#pragma hdrstop

#include <assert>
#include "Decompiler.h"
#include "Main.h"
#include "Misc.h"
#include "TypeInfo.h"
#include "InputDlg.h"
//---------------------------------------------------------------------------
extern  int         DelphiVersion; 
extern  MDisasm     Disasm;
extern  DWORD       CodeBase;
extern  DWORD       CurProcAdr;               
extern  DWORD       EP;
extern  DWORD       ImageBase;
extern  BYTE        *Image;
extern  BYTE        *Code;
extern  PInfoRec    *Infos;
extern  TStringList *BSSInfos;
extern  DWORD       *Flags;
extern  int         VmtSelfPtr;
extern  char        StringBuf[MAXSTRBUFFER];
extern  MKnowledgeBase  KnowledgeBase;
//---------------------------------------------------------------------------
String __fastcall GetString(PITEM item, BYTE precedence)
{
    String  _val = item->Value;

    if (_val == "")
    {
        if (item->Flags & IF_INTVAL)
            return GetImmString(item->Type, item->IntValue);
        if (item->Name != "")
            _val = item->Name;
    }
    if (item->Precedence && item->Precedence < precedence) return "(" + _val + ")";
    return _val;
}
//---------------------------------------------------------------------------
String __fastcall GetFieldName(PFIELDINFO fInfo)
{
    if (fInfo->Name == "") return String("?f") + Val2Str0(fInfo->Offset);
    return fInfo->Name;
}
//---------------------------------------------------------------------------
String __fastcall GetArgName(PARGINFO argInfo)
{
    if (argInfo->Name == "") return String("arg_") + Val2Str0(argInfo->Ndx);
    return argInfo->Name;
}
//---------------------------------------------------------------------------
String __fastcall GetGvarName(DWORD adr)
{
    if (!IsValidImageAdr(adr)) return "?";
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->HasName()) return recN->GetName();
    return MakeGvarName(adr);
}
//---------------------------------------------------------------------------
//'A'-JO;'B'-JNO;'C'-JB;'D'-JNB;'E'-JZ;'F'-JNZ;'G'-JBE;'H'-JA;
//'I'-JS;'J'-JNS;'K'-JP;'L'-JNP;'M'-JL;'N'-JGE;'O'-JLE;'P'-JG
//'Q'-in;'R'-not in;'S'-is
String DirectConditions[19] = {"", "", "<", ">=", "=", "<>", "<=", ">", "", "", "", "", "<", ">=", "<=", ">", "not in", "in", "is"};
String __fastcall GetDirectCondition(char c)
{
    if (c >= 'A')
        return DirectConditions[c - 'A'];
    else
        return "?";
}
//---------------------------------------------------------------------------
String InvertConditions[19] = {"", "", ">=", "<", "<>", "=", ">", "<=", "", "", "", "", ">=", "<", ">", "<=", "in", "not in", "is not"};
String __fastcall GetInvertCondition(char c)
{
    if (c >= 'A')
        return InvertConditions[c - 'A'];
    else
        return "?";
}
//---------------------------------------------------------------------------
void __fastcall InitItem(PITEM Item)
{
    Item->Flags = 0;
    Item->Precedence = PRECEDENCE_ATOM;
    Item->Size = 0;
    Item->Offset = 0;
    Item->IntValue = 0;
    Item->Value = "";
    Item->Value1 = "";
    Item->Type = "";
    Item->Name = "";
}
//---------------------------------------------------------------------------
void __fastcall AssignItem(PITEM DstItem, PITEM SrcItem)
{
    DstItem->Flags = SrcItem->Flags;
    DstItem->Precedence = SrcItem->Precedence;
    DstItem->Size = SrcItem->Size;
    DstItem->IntValue = SrcItem->IntValue;
    DstItem->Value = SrcItem->Value;
    DstItem->Value1 = SrcItem->Value1;
    DstItem->Type = SrcItem->Type;
    DstItem->Name = SrcItem->Name;
}
//---------------------------------------------------------------------------
__fastcall TNamer::TNamer()
{
    MaxIdx = -1;
    Names = new TStringList;
    Names->Sorted = true;
}
//---------------------------------------------------------------------------
__fastcall TNamer::~TNamer()
{
    delete Names;
}
//---------------------------------------------------------------------------
String __fastcall TNamer::MakeName(String shablon)
{
    char    name[1024];

    if (shablon[1] == 'T')
    {
        int idx = -1;
        int len = sprintf(name, "_%s_", shablon.c_str() + 1);
        for (int n = 0; n <= MaxIdx; n++)
        {
            sprintf(name + len, "%d", n);
            if (Names->IndexOf(String(name)) != -1) idx = n;
        }
        if (idx == -1)
        {
            Names->Add(String(name));
            if (MaxIdx == -1) MaxIdx = 0;
        }
        else
        {
            sprintf(name + len, "%d", idx + 1);
            Names->Add(String(name));
            if (idx + 1 > MaxIdx) MaxIdx = idx + 1; 
        }
    }
    return String(name);
}
//---------------------------------------------------------------------------
__fastcall TForInfo::TForInfo(bool ANoVar, bool ADown, int AStopAdr, String AFrom, String ATo, BYTE AVarType, int AVarIdx, BYTE ACntType, int ACntIdx)
{
    NoVar = ANoVar;
    Down = ADown;
    StopAdr = AStopAdr;
    From = AFrom;
    To = ATo;
    VarInfo.IdxType = AVarType;
    VarInfo.IdxValue = AVarIdx;
    CntInfo.IdxType = ACntType;
    CntInfo.IdxValue = ACntIdx;
}
//---------------------------------------------------------------------------
__fastcall TWhileInfo::TWhileInfo(bool ANoCond)
{
    NoCondition = ANoCond;
}
//---------------------------------------------------------------------------
__fastcall TLoopInfo::TLoopInfo(BYTE AKind, DWORD AContAdr, DWORD ABreakAdr, DWORD ALastAdr)
{
    Kind = AKind;
    ContAdr = AContAdr;
    BreakAdr = ABreakAdr;
    LastAdr = ALastAdr;
    forInfo = 0;
    whileInfo = 0;
}
//---------------------------------------------------------------------------
__fastcall TLoopInfo::~TLoopInfo()
{
}
//---------------------------------------------------------------------------
__fastcall TDecompileEnv::TDecompileEnv(DWORD AStartAdr, int ASize, PInfoRec recN)
{
    StartAdr = AStartAdr;
    Size = ASize;

    StackSize = recN->procInfo->stackSize;
    if (!StackSize) StackSize = 0x8000;
    Stack = new ITEM[StackSize];

    ErrAdr = 0;
    Body = new TStringList;
    Namer = new TNamer;
    SavedContext = new TList;
    BJLseq = new TList;
    bjllist = new TList;
    CmpStack = new TList;
    Embedded = (recN->procInfo->flags & PF_EMBED);
    EmbeddedList = new TStringList;

    BpBased = recN->procInfo->flags & PF_BPBASED;
    LocBase = 0;
    if (!BpBased) LocBase = StackSize;
    LastResString = "";
}
//---------------------------------------------------------------------------
__fastcall TDecompileEnv::~TDecompileEnv()
{
    if (Stack) delete[] Stack;
    if (Body) delete Body;
    if (Namer) delete Namer;
    if (SavedContext) delete SavedContext;
    if (BJLseq) delete BJLseq;
    if (bjllist) delete bjllist;
    if (CmpStack) delete CmpStack;
    if (EmbeddedList) delete EmbeddedList;
}
//---------------------------------------------------------------------------
String __fastcall TDecompileEnv::GetLvarName(int Ofs, String Type)
{
    PLOCALINFO  locInfo;
    String  _defaultName = String("lvar_") + Val2Str0(LocBase - Ofs);

    //Insert by ZGL
    PInfoRec _recN = GetInfoRec(StartAdr);
    if (_recN && _recN->procInfo)
    {
        if (_recN->procInfo->locals)
        {
            for (int n = 0; n < _recN->procInfo->locals->Count; n++)
            {
                locInfo = PLOCALINFO(_recN->procInfo->locals->Items[n]);
                if (locInfo->Ofs == Ofs && locInfo->Name != "")//LocBase - Ofs
                     return locInfo->Name;
            }
        }
        locInfo = _recN->procInfo->AddLocal(Ofs, 1, _defaultName, Type);
    }
    ////////////////
    return _defaultName;
}
//---------------------------------------------------------------------------
PDCONTEXT __fastcall TDecompileEnv::GetContext(DWORD Adr)
{
    int         n;
    PDCONTEXT   ctx;

    for (n = 0; n < SavedContext->Count; n++)
    {
        ctx = (PDCONTEXT)SavedContext->Items[n];
        if (ctx->adr == Adr) return ctx;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::SaveContext(DWORD Adr)
{
    int         n;
    PDCONTEXT   ctx;

    if (!GetContext(Adr))
    {
        ctx = new DCONTEXT;
        ctx->adr = Adr;

        for (n = 0; n < 8; n++)
        {
            ctx->gregs[n] = RegInfo[n];
            ctx->fregs[n] = FStack[n];
        }
        SavedContext->Add((void*)ctx);
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::RestoreContext(DWORD Adr)
{
    int         n;
    PDCONTEXT   ctx;

    ctx = GetContext(Adr);
    if (ctx)
    {
        for (n = 0; n < 8; n++)
        {
            RegInfo[n] = ctx->gregs[n];
            FStack[n] = ctx->fregs[n];
        }
    }
}
//---------------------------------------------------------------------------
__fastcall TDecompiler::TDecompiler(TDecompileEnv* AEnv)
{
    WasRet = false;
    Env = AEnv;
    Stack = 0;
    DeFlags = new BYTE[AEnv->Size + 1];
}
//---------------------------------------------------------------------------
__fastcall TDecompiler::~TDecompiler()
{
    if (Stack) delete[] Stack;
    if (DeFlags) delete DeFlags;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SetDeFlags(BYTE* ASrc)
{
    memmove(DeFlags, ASrc, Env->Size + 1);
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SetStop(DWORD Adr)
{
    DeFlags[Adr - Env->StartAdr] = 1;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::InitFlags()
{
    memset(DeFlags, 0, Env->Size + 1);
    int _ap = Adr2Pos(Env->StartAdr);
    for (int n = _ap; n < _ap + Env->Size + 1; n++)
    {
        if (IsFlagSet(cfLoc, n))
        {
            PInfoRec recN = GetInfoRec(Pos2Adr(n));
            if (recN) recN->counter = 0;
        }
        ClearFlag(cfPass, n);
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::ClearStop(DWORD Adr)
{
    DeFlags[Adr - Env->StartAdr] = 0;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SetStackPointers(TDecompiler* ASrc)
{
    _TOP_ = ASrc->_TOP_;
    _ESP_ = ASrc->_ESP_;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SetRegItem(int Idx, PITEM Val)
{
    int     _idx;

    if (Idx >= 16)
    {
        _idx = Idx - 16;
        Env->RegInfo[_idx].Size = 4;
    }
    else if (Idx >= 8)
    {
        _idx = Idx - 8;
        Env->RegInfo[_idx].Size = 2;
    }
    else if (Idx >= 4)
    {
        _idx = Idx - 4;
        Env->RegInfo[_idx].Size = 1;
    }
    else
    {
        _idx = Idx;
        Env->RegInfo[_idx].Size = 1;
    }

    Env->RegInfo[_idx].Flags = Val->Flags;
    Env->RegInfo[_idx].Precedence = Val->Precedence;
    Env->RegInfo[_idx].Offset = Val->Offset;
    Env->RegInfo[_idx].IntValue = Val->IntValue;
    Env->RegInfo[_idx].Value = Val->Value;
    Env->RegInfo[_idx].Value1 = Val->Value1;
    Env->RegInfo[_idx].Type = Val->Type;
    Env->RegInfo[_idx].Name = Val->Name;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::GetRegItem(int Idx, PITEM Dst)
{
    int     _idx;

    assert(Idx >= 0 && Idx < 24);

    if (Idx >= 16)
        _idx = Idx - 16;
    else if (Idx >= 8)
        _idx = Idx - 8;
    else if (Idx >= 4)
        _idx = Idx - 4;
    else
        _idx = Idx;

    PITEM _src = &Env->RegInfo[_idx];
    Dst->Flags = _src->Flags;
    Dst->Precedence = _src->Precedence;
    Dst->Size = _src->Size;
    Dst->Offset = _src->Offset;
    Dst->IntValue = _src->IntValue;
    Dst->Value = _src->Value;
    Dst->Value1 = _src->Value1;
    Dst->Type = _src->Type;
    Dst->Name = _src->Name;
}
//---------------------------------------------------------------------------
String __fastcall TDecompiler::GetRegType(int Idx)
{
    if (Idx >= 16) return Env->RegInfo[Idx - 16].Type;
    if (Idx >= 8) return Env->RegInfo[Idx - 8].Type;
    if (Idx >= 4) return Env->RegInfo[Idx - 4].Type;
    return Env->RegInfo[Idx].Type;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::Push(PITEM Item)
{
    if (_ESP_ < 4)
    {
        Env->ErrAdr = CurProcAdr;
        throw Exception("Stack!");
    }
    _ESP_ -= 4;
    Env->Stack[_ESP_] = *Item;
    /*
    Env->Stack[_ESP_].Flags = Item->Flags;
    Env->Stack[_ESP_].Precedence = Item->Precedence;
    Env->Stack[_ESP_].Size = Item->Size;
    Env->Stack[_ESP_].Offset = Item->Offset;
    Env->Stack[_ESP_].IntValue = Item->IntValue;
    Env->Stack[_ESP_].Value = Item->Value;
    Env->Stack[_ESP_].Type = Item->Type;
    Env->Stack[_ESP_].Name = Item->Name;
    */
}
//---------------------------------------------------------------------------
PITEM __fastcall TDecompiler::Pop()
{
    if (_ESP_ == Env->StackSize)
    {
        Env->ErrAdr = CurProcAdr;
        throw Exception("Stack!");
    }
    PITEM _item = &Env->Stack[_ESP_];
    _ESP_ += 4;
    return _item;
}
//---------------------------------------------------------------------------
//Get val from ST(idx)
PITEM __fastcall TDecompiler::FGet(int idx)
{
    PITEM _item = &Env->FStack[(_TOP_ + idx) & 7];
    return _item;
}
//---------------------------------------------------------------------------
//Save val into ST(idx)
void __fastcall TDecompiler::FSet(int idx, PITEM val)
{
    int     n = (_TOP_ + idx) & 7;
    Env->FStack[n] = *val;
    /*
    Env->FStack[n].Flags = val->Flags;
    Env->FStack[n].Precedence = val->Precedence;
    Env->FStack[n].Size = val->Size;
    Env->FStack[n].Offset = val->Offset;
    Env->FStack[n].IntValue = val->IntValue;
    Env->FStack[n].Value = val->Value;
    Env->FStack[n].Type = val->Type;
    Env->FStack[n].Name = val->Name;
    */
}
//---------------------------------------------------------------------------
//Xchange ST(idx1) and ST(idx2)
void __fastcall TDecompiler::FXch(int idx1, int idx2)
{
    ITEM    _tmp;
    PITEM   _item1 = FGet(idx1);
    PITEM   _item2 = FGet(idx2);

    _tmp.Flags = _item1->Flags;
    _tmp.Precedence = _item1->Precedence;
    _tmp.Size = _item1->Size;
    _tmp.Offset = _item1->Offset;
    _tmp.IntValue = _item1->IntValue;
    _tmp.Value = _item1->Value;
    _tmp.Type = _item1->Type;
    _tmp.Name = _item1->Name;

    _item1->Flags = _item2->Flags;
    _item1->Precedence = _item2->Precedence;
    _item1->Size = _item2->Size;
    _item1->Offset = _item2->Offset;
    _item1->IntValue = _item2->IntValue;
    _item1->Value = _item2->Value;
    _item1->Type = _item2->Type;
    _item1->Name = _item2->Name;

    _item2->Flags = _tmp.Flags;
    _item2->Precedence = _tmp.Precedence;
    _item2->Size = _tmp.Size;
    _item2->Offset = _tmp.Offset;
    _item2->IntValue = _tmp.IntValue;
    _item2->Value = _tmp.Value;
    _item2->Type = _tmp.Type;
    _item2->Name = _tmp.Name;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::FPush(PITEM val)
{
    _TOP_--;
    _TOP_ &= 7;
    FSet(0, val);
}
//---------------------------------------------------------------------------
PITEM __fastcall TDecompiler::FPop()
{
    PITEM _item = FGet(0);
    _TOP_++;
    _TOP_ &= 7;
    return _item;
}
//---------------------------------------------------------------------------
bool __fastcall TDecompiler::CheckPrototype(PInfoRec ARec)
{
    int         n, _argsNum;
    PARGINFO    _argInfo;

    _argsNum = (ARec->procInfo->args) ? ARec->procInfo->args->Count : 0;
    for (n = 0; n < _argsNum; n++)
    {
        _argInfo = (PARGINFO)ARec->procInfo->args->Items[n];
        if (_argInfo->TypeDef == "") return false;
    }
    if (ARec->kind == ikFunc && ARec->type == "") return false;
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TDecompiler::Init(DWORD fromAdr)
{
    BYTE        _kind, _retKind = 0, _callKind;
    int         n, _pos, _argsNum, _ndx, _rn, _size;
    int         _fromPos;
    PInfoRec    _recN, _recN1;
    PARGINFO    _argInfo;
    ITEM        _item;
    String      _retType, _typeDef;

    _fromPos = Adr2Pos(fromAdr); assert(_fromPos >= 0);
    //Imports not decompile
    if (IsFlagSet(cfImport, _fromPos)) return true;
    _recN = GetInfoRec(fromAdr);
    if (!CheckPrototype(_recN)) return false;
    _retType = _recN->type;
    //Check that function return type is given
    if (_recN->kind == ikFunc) _retKind = GetTypeKind(_retType, &_size);
    //Init registers
    InitItem(&_item);
    for (n = 16; n < 24; n++)
    {
        SetRegItem(n, &_item);
    }

    //Init Env->Stack
    _ESP_ = Env->StackSize;
    //Init floating registers stack
    _TOP_ = 0;
    for (n = 0; n < 8; n++)
    {
        FSet(n, &_item);
    }

    _callKind = _recN->procInfo->flags & 7;

    //Arguments
    _argsNum = (_recN->procInfo->args) ? _recN->procInfo->args->Count : 0;
    if (_callKind == 0)//fastcall
    {
        _ndx = 0;
        for (n = 0; n < _argsNum; n++)
        {
            _argInfo = (PARGINFO)_recN->procInfo->args->Items[n];
            InitItem(&_item);
            _item.Flags = IF_ARG;
            if (_argInfo->Tag == 0x22) _item.Flags |= IF_VAR;
            _kind = GetTypeKind(_argInfo->TypeDef, &_size);
            _item.Type = _argInfo->TypeDef;
            _item.Name = GetArgName(_argInfo);
            _item.Value = _item.Name;

            if (_kind == ikFloat && _argInfo->Tag != 0x22)
            {
                _size = _argInfo->Size;
                while (_size)
                {
                    Push(&_item);
                    _size -= 4;
                }
                continue;
            }
            //eax, edx, ecx
            if (_ndx >= 0 && _ndx <= 2)
            {
                if (_ndx)
                    _rn = 19 - _ndx;
                else
                    _rn = 16;

                SetRegItem(_rn, &_item);
                _ndx++;
            }
            else
                Push(&_item);
        }
        //ret value
        if (_retKind == ikLString || _retKind == ikRecord)
        {
            InitItem(&_item);
            _item.Flags = IF_ARG | IF_VAR;
            _item.Type = _retType;
            _item.Name = "Result";
            _item.Value = _item.Name;

            //eax, edx, ecx
            if (_ndx >= 0 && _ndx <= 2)
            {
                if (_ndx)
                    _rn = 19 - _ndx;
                else
                    _rn = 16;

                SetRegItem(_rn, &_item);
                _ndx++;
            }
            else
                Push(&_item);
        }
    }
    else if (_callKind == 3 || _callKind == 1)//stdcall, cdecl
    {
        //Arguments in reverse order
        for (n = _argsNum - 1; n >= 0; n--)
        {
            _argInfo = (PARGINFO)_recN->procInfo->args->Items[n];
            InitItem(&_item);
            _item.Flags = IF_ARG;
            if (_argInfo->Tag == 0x22) _item.Flags |= IF_VAR;
            _item.Type = _argInfo->TypeDef;
            _item.Name = GetArgName(_argInfo);
            _item.Value = _item.Name;
            Push(&_item);
        }
    }
    else if (_callKind == 2)//pascal
    {
        for (n = 0; n < _argsNum; n++)
        {
            _argInfo = (PARGINFO)_recN->procInfo->args->Items[n];
            InitItem(&_item);
            _item.Flags = IF_ARG;
            if (_argInfo->Tag == 0x22) _item.Flags |= IF_VAR;
            _item.Type = _argInfo->TypeDef;
            _item.Name = GetArgName(_argInfo);
            _item.Value = _item.Name;
            Push(&_item);
        }
    }
    //Push ret address
    InitItem(&_item);
    Push(&_item);
    //Env->BpBased = _recN->procInfo->flags & PF_BPBASED;
    //Env->LocBase = 0;
    //if (!Env->BpBased) Env->LocBase = _ESP_;
    //Env->LastResString = "";
    return true;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::OutputSourceCodeLine(String line)
{
    int spaceNum = TAB_SIZE * Indent;
    memset(StringBuf, ' ', spaceNum);
    int len = sprintf(StringBuf + spaceNum, "%s", line.c_str());

    FMain_11011981->lbSourceCode->Items->Add(String(StringBuf));
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::OutputSourceCode()
{
    bool    _end;
    String  line, nextline;

    Alarm = False;
    FMain_11011981->lbSourceCode->Clear(); Indent = 0;
    for (int n = 0; n < Body->Count; n++)
    {
        line = Body->Strings[n];
        if (n < Body->Count - 1)
            nextline = Body->Strings[n + 1];
        else
            nextline = "";

        if (line != "" && line[1] == '/' && line.Pos("??? And ???")) Alarm = True;

        if (SameText(line, "begin") || SameText(line, "try") || SameText(line, "repeat") || line.Pos("case ") > 0)
        {
            if (SameText(line, "begin")) line += "//" + String(Indent);
            OutputSourceCodeLine(line);
            Indent++;
            continue;
        }

        if (SameText(line, "finally") || SameText(line, "except"))
        {
            Indent--;
            if (Indent < 0) Indent = 0;
            line += "//" + String(Indent);
            OutputSourceCodeLine(line);
            Indent++;
            continue;
        }

        if (SameText(line, "end") || SameText(line, "until"))
        {
            _end = SameText(line, "end");
            Indent--;
            if (Indent < 0)Indent = 0;
            if (!SameText(nextline, "else")) line += ";";
            if (_end) line += "//" + String(Indent);
            OutputSourceCodeLine(line);
            continue;
        }
        OutputSourceCodeLine(line);
    }
}
//---------------------------------------------------------------------------
//Embedded???
void __fastcall TDecompileEnv::DecompileProc()
{
    EmbeddedList->Clear();
    TDecompiler* De = new TDecompiler(this);
    if (!De->Init(StartAdr))
    {
        De->Env->ErrAdr = StartAdr;
        delete De;
        throw Exception("Prototype is not completed");
    }
    De->InitFlags();
    De->SetStop(StartAdr + Size);

    //add prototype
    PInfoRec _recN = GetInfoRec(StartAdr);
    ProcName = _recN->GetName();
    AddToBody(_recN->MakePrototype(StartAdr, true, false, false, true, false));

    //add vars -- Insert by ZGL
    if (_recN->procInfo->locals && _recN->procInfo->locals->Count > 0)
    {
        for (int n = 0; n < _recN->procInfo->locals->Count; n++)
        {
            PLOCALINFO locInfo = PLOCALINFO(_recN->procInfo->locals->Items[n]);
            GetLvarName(locInfo->Ofs, locInfo->TypeDef);
        }
    }
    if (_recN->procInfo->locals && _recN->procInfo->locals->Count > 0)
    {
        AddToBody("var");
        for (int n = 0; n < _recN->procInfo->locals->Count; n++)
        {
            PLOCALINFO locInfo = PLOCALINFO(_recN->procInfo->locals->Items[n]);
            String line = "  " + GetLvarName(locInfo->Ofs, locInfo->TypeDef) + ":" + locInfo->TypeDef + ";";
            AddToBody(line);
        }
    }
    ///////////////////////////
    
    AddToBody("begin");
    if (StartAdr != EP)
    {
        if (_recN->kind == ikConstructor)
            De->Decompile(StartAdr, CF_CONSTRUCTOR, 0);
        else if (_recN->kind == ikDestructor)
            De->Decompile(StartAdr, CF_DESTRUCTOR, 0);
        else
            De->Decompile(StartAdr, 0, 0);
    }
    else
    {
        De->Decompile(StartAdr, 0, 0);
    }
    AddToBody("end");
    delete De;
}
//---------------------------------------------------------------------------
//BJL
bool __fastcall TDecompileEnv::GetBJLRange(DWORD fromAdr, DWORD* bodyBegAdr, DWORD* bodyEndAdr, DWORD* jmpAdr, PLoopInfo loopInfo)
{
    BYTE        _op;
    int         _curPos, _instrLen, _brType;
    DWORD       _curAdr, _jmpAdr;
    DISINFO     _disInfo;

    *bodyEndAdr = 0;
    *jmpAdr = 0;
    _curPos = Adr2Pos(fromAdr);
    assert(_curPos >= 0);
    _curAdr = fromAdr;

    while (1)
    {
        if (_curAdr == *bodyBegAdr) break;
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
       _curPos += _instrLen; _curAdr += _instrLen;
        if (_disInfo.Branch)
        {
            if (IsFlagSet(cfSkip, _curPos - _instrLen)) continue;
            if (!_disInfo.Conditional) return false;
            _brType = BranchGetPrevInstructionType(_disInfo.Immediate, &_jmpAdr, loopInfo);
            //jcc down
            if (_brType == 1)
            {
                if (_disInfo.Immediate > *bodyBegAdr)
                {
                    *bodyBegAdr = _disInfo.Immediate;
                    continue;
                }
            }
            //simple if or jmp down
            if (_brType == 0 || _brType == 3)
            {
                if (_disInfo.Immediate > *bodyEndAdr)
                {
                    *bodyEndAdr = _disInfo.Immediate;
                    if (_brType == 3) *jmpAdr = _jmpAdr;
                    continue;
                }
            }
            //jcc up
            if (_brType == 2)
            {
                //if (_disInfo.Immediate > fromAdr) return false;
                if (*bodyEndAdr == 0)
                    *bodyEndAdr = _disInfo.Immediate;
                else if (*bodyEndAdr != _disInfo.Immediate)
                    throw Exception("GetBJLRange: unknown situation");
            }
            //jmp up
            if (_brType == 4) return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::CreateBJLSequence(DWORD fromAdr, DWORD bodyBegAdr, DWORD bodyEndAdr)
{
    bool        found;
    BYTE        _b;
    int         i, j, m, _curPos, _instrLen;
    DWORD       _curAdr, _adr;
    PInfoRec    _recN;
    TBJL        *_bjl, *_bjl1, *_bjl2;
    TBJLInfo    *_bjlInfo, *_bjlInfo1, *_bjlInfo2;
    DISINFO     _disInfo;

    BJLnum = 0;

    bjllist->Clear();
    BJLseq->Clear();
    _curPos = Adr2Pos(fromAdr); assert(_curPos >= 0);
    _curAdr = fromAdr;

    while (1)
    {
        if (_curAdr == bodyBegAdr)
        {
            _bjl = new TBJL;
            _bjl->branch = false;
            _bjl->loc = true;
            _bjl->type = BJL_LOC;
            _bjl->address = bodyBegAdr;
            _bjl->idx = 0;
            bjllist->Add((void*)_bjl);
            _bjlInfo = new TBJLInfo;
            _bjlInfo->state = 'L';
            _bjlInfo->bcnt = 0;
            _bjlInfo->address = bodyBegAdr;
            _bjlInfo->dExpr = "";
            _bjlInfo->iExpr = "";
            BJLseq->Add((void*)_bjlInfo);

            _bjl = new TBJL;
            _bjl->branch = false;
            _bjl->loc = true;
            _bjl->type = BJL_LOC;
            _bjl->address = bodyEndAdr;
            _bjl->idx = 0;
            bjllist->Add((void*)_bjl);
            _bjlInfo = new TBJLInfo;
            _bjlInfo->state = 'L';
            _bjlInfo->bcnt = 0;
            _bjlInfo->address = bodyEndAdr;
            _bjlInfo->dExpr = "";
            _bjlInfo->iExpr = "";
            BJLseq->Add((void*)_bjlInfo);
            break;
        }
        if (IsFlagSet(cfLoc, _curPos))
        {
            _bjl = new TBJL;
            _bjl->branch = false;
            _bjl->loc = true;
            _bjl->type = BJL_LOC;
            _bjl->address = _curAdr;
            _bjl->idx = 0;
            bjllist->Add((void*)_bjl);
            _bjlInfo = new TBJLInfo;
            _bjlInfo->state = 'L';
            _bjlInfo->bcnt = 0;
            _bjlInfo->address = _curAdr;
            _bjlInfo->dExpr = "";
            _bjlInfo->iExpr = "";
            BJLseq->Add((void*)_bjlInfo);
        }
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        if (_disInfo.Branch)
        {
            _bjl = new TBJL;
            _bjl->branch = true;
            _bjl->loc = false;
            if (IsFlagSet(cfSkip, _curPos))
                _bjl->type = BJL_SKIP_BRANCH;
            else
                _bjl->type = BJL_BRANCH;
            _bjl->address = _disInfo.Immediate;
            _bjl->idx = 0;
            bjllist->Add((void*)_bjl);
            _bjlInfo = new TBJLInfo;
            _bjlInfo->state = 'B';
            _bjlInfo->bcnt = 0;
            _bjlInfo->address = _disInfo.Immediate;

            _b = *(Code + _curPos);
            if (_b == 0xF) _b = *(Code + _curPos + 1);
            _bjlInfo->dExpr = GetDirectCondition((_b & 0xF) + 'A');
            _bjlInfo->iExpr = GetInvertCondition((_b & 0xF) + 'A');
            BJLseq->Add((void*)_bjlInfo);
        }
       _curPos += _instrLen; _curAdr += _instrLen;
    }

    for (i = 0, BJLmaxbcnt = 0; i < bjllist->Count; i++)
    {
        _bjl1 = (TBJL*)bjllist->Items[i];
        if (_bjl1->type == BJL_BRANCH || _bjl1->type == BJL_SKIP_BRANCH)
        {
            for (j = 0; j < bjllist->Count; j++)
            {
                _bjl2 = (TBJL*)bjllist->Items[j];
                if (_bjl2->type == BJL_LOC && _bjl1->address == _bjl2->address)
                {
                    _bjlInfo = (TBJLInfo*)BJLseq->Items[j];
                    _bjlInfo->bcnt++;
                    if (_bjlInfo->bcnt > BJLmaxbcnt)
                        BJLmaxbcnt = _bjlInfo->bcnt;
                }
            }
        }
    }

    while (1)
    {
        found = false;
        for (i = 0; i < bjllist->Count; i++)
        {
            _bjl1 = (TBJL*)bjllist->Items[i];
            if (_bjl1->type == BJL_SKIP_BRANCH)
            {
                _adr = _bjl1->address;
                bjllist->Delete(i);
                delete _bjl1;

                _bjlInfo1 = (TBJLInfo*)BJLseq->Items[i];
                BJLseq->Delete(i);
                delete _bjlInfo1;

                for (j = 0; j < bjllist->Count; j++)
                {
                    _bjl2 = (TBJL*)bjllist->Items[j];
                    if (_bjl2->type == BJL_LOC && _adr == _bjl2->address)
                    {
                        _bjlInfo2 = (TBJLInfo*)BJLseq->Items[j];
                        _bjlInfo2->bcnt--;

                        if (!_bjlInfo2->bcnt)
                        {
                            bjllist->Delete(j);
                            delete _bjl2;
                            BJLseq->Delete(j);
                            delete _bjlInfo2;
                        }
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }
        if (!found) break;
    }

    for (i = 0, m = 1, BJLmaxbcnt = 0; i < bjllist->Count; i++)
    {
        _bjl1 = (TBJL*)bjllist->Items[i];
        _bjl1->idx = i;
        if (_bjl1->type == BJL_BRANCH)
        {
            _bjlInfo = (TBJLInfo*)BJLseq->Items[i];
            _bjlInfo->dExpr = "#" + String(m) + "#" + _bjlInfo->dExpr + "#" + String(m + 1) + "#";
            _bjlInfo->iExpr = "#" + String(m) + "#" + _bjlInfo->iExpr + "#" + String(m + 1) + "#";
            m += 2;

            for (j = 0; j < bjllist->Count; j++)
            {
                _bjl2 = (TBJL*)bjllist->Items[j];
                if (_bjl2->type == BJL_LOC && _bjl1->address == _bjl2->address)
                {
                    _bjlInfo = (TBJLInfo*)BJLseq->Items[j];
                    if (_bjlInfo->bcnt > BJLmaxbcnt)
                        BJLmaxbcnt = _bjlInfo->bcnt;
                }
            }
        }
    }
    BJLnum = bjllist->Count;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::UpdateBJLList()
{
    bool        found;
    int         i, j;
    TBJL        *_bjl, *_bjl1, *_bjl2;
    TBJLInfo    *_bjlInfo;

    while (1)
    {
        found = false;
        for (i = 0; i < bjllist->Count; i++)
        {
            _bjl = (TBJL*)bjllist->Items[i];
            if (_bjl->type == BJL_USED)
            {
                bjllist->Delete(i);
                found = true;
                break;
            }
        }
        if (!found) break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::BJLAnalyze()
{
    bool        found = true;
    int         i, j, k, indx, no;
    TBJL        *_bjl;
    TBJLInfo    *_bjlInfo;
    String      sd, si;
    int         idx[MAXSEQNUM];
    char        pattern[MAXSEQNUM];

    UpdateBJLList();

    while (found)
    {
        found = false;
//BM BM ... BM <=> BM (|| ... ||)
//BM	0
//BM	1
//...
//BM	k-1
        for (k = BJLmaxbcnt; k >= 2; k--)
        {
            for (i = 0; i < bjllist->Count; i++)
            {
                if (!BJLGetIdx(idx, i, k)) continue;
                for (j = 0; j < k; j++) pattern[j] = 'B';
                pattern[k] = 0;
                if (!BJLCheckPattern1(pattern, i)) continue;
                for (j = 0; j < k; j++) pattern[j] = '1';
                pattern[k] = 0;
                if (!BJLCheckPattern2(pattern, i)) continue;
                _bjl = (TBJL*)bjllist->Items[i];
                if ((indx = BJLFindLabel(_bjl->address, &no)) == -1) continue;

                BJLSeqSetStateU(idx, k - 1);
                BJLListSetUsed(i, k - 1);
                sd = ""; si = "";

                for (j = 0; j < k; j++)
                {
                    _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[j]];
                    ExprMerge(sd, _bjlInfo->dExpr, '|');
                    ExprMerge(si, _bjlInfo->iExpr, '&');
                }

                _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[k - 1]];
                _bjlInfo->dExpr = sd;
                _bjlInfo->iExpr = si;

                _bjlInfo = (TBJLInfo*)BJLseq->Items[indx];
                _bjl = (TBJL*)bjllist->Items[no];
                _bjlInfo->bcnt -= k - 1;
                if (!_bjlInfo->bcnt && _bjl->type == BJL_LOC) _bjl->type = BJL_USED;

                UpdateBJLList();
                found = true;
                break;
            }
            if (found) break;
        }
        if (found) continue;
//BN BM @N <=> BM (&&)
//BN	0
//BM	1
//@N    2
        for (i = 0; i < bjllist->Count; i++)
        {
            if (!BJLGetIdx(idx, i, 3)) continue;
            if (!BJLCheckPattern1("BBL", i)) continue;
            if (!BJLCheckPattern2("101", i)) continue;
            if (BJLCheckPattern2("111", i)) continue; //check that N != M

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[0]];
            _bjlInfo->state = 'U';
            _bjl = (TBJL*)bjllist->Items[i];
            _bjl->type = BJL_USED;

            sd = ""; si = "";
            ExprMerge(sd, _bjlInfo->iExpr, '&');
            ExprMerge(si, _bjlInfo->dExpr, '|');
            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[1]];
            ExprMerge(sd, _bjlInfo->dExpr, '&');
            ExprMerge(si, _bjlInfo->iExpr, '|');

            _bjlInfo->dExpr = sd;
            _bjlInfo->iExpr = si;

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[2]];
            _bjl = (TBJL*)bjllist->Items[i + 2];
            _bjlInfo->bcnt--;
            if (!_bjlInfo->bcnt && _bjl->type == BJL_LOC) _bjl->type = BJL_USED;

            UpdateBJLList();
            found = true;
            break;
        }
        if (found) continue;
//BN @N <=> if (~BN)
//BN	0   if {
//@N	1   }
        for (i = 0; i < bjllist->Count; i++)
        {
            if (!BJLGetIdx(idx, i, 2)) continue;
            if (!BJLCheckPattern1("BL", i)) continue;
            if (!BJLCheckPattern2("11", i)) continue;

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[0]];
            _bjlInfo->state = 'I';
            _bjlInfo->result = _bjlInfo->iExpr;
            _bjl = (TBJL*)bjllist->Items[i];
            _bjl->branch = false;
            _bjl->type = BJL_USED;

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[1]];
            _bjl = (TBJL*)bjllist->Items[i + 1];
            _bjlInfo->bcnt--;
            if (!_bjlInfo->bcnt && _bjl->type == BJL_LOC) _bjl->type = BJL_USED;

            UpdateBJLList();
            found = true;
            break;
        }
        if (found) continue;
//BN BM @N @M <=> if (BN || ~BM)
//BN    0   if {
//BM    1
//@N    2
//@M    3   }                  k = 1
        for (i = 0; i < bjllist->Count; i++)
        {
            if (!BJLGetIdx(idx, i, 4)) continue;
            if (!BJLCheckPattern1("BBLL", i)) continue;
            if (!BJLCheckPattern2("1010", i)) continue;
            if (!BJLCheckPattern2("0101", i)) continue;

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[0]];
            _bjlInfo->state = 'I';
            sd = _bjlInfo->dExpr;
            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[1]];
            ExprMerge(sd, _bjlInfo->iExpr, '|');
            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[0]];
            _bjlInfo->result = sd;

            BJLSeqSetStateU(idx, 2);
            BJLListSetUsed(i, 2);

            _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[2]];
            _bjl = (TBJL*)bjllist->Items[i + 2];
            _bjlInfo->bcnt--;
            if (!_bjlInfo->bcnt && _bjl->type == BJL_LOC) _bjl->type = BJL_USED;
            _bjl = (TBJL*)bjllist->Items[i + 3];
            _bjlInfo->bcnt--;
            if (!_bjlInfo->bcnt && _bjl->type == BJL_LOC) _bjl->type = BJL_USED;

            UpdateBJLList();
            found = true;
            break;
        }
        if (found) continue;
    }
}
//---------------------------------------------------------------------------
bool __fastcall TDecompileEnv::BJLGetIdx(int* idx, int from, int num)
{
    TBJL    *_bjl;

    for (int k = 0; k < num; k++)
    {
        if (from + k < bjllist->Count)
        {
            _bjl = (TBJL*)bjllist->Items[from + k];
            idx[k] = _bjl->idx;
        }
        else
        {
            return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TDecompileEnv::BJLCheckPattern1(char* t, int from)
{
    TBJL    *_bjl;

    for (int k = 0; k < strlen(t); k++)
    {
        if (from + k >= bjllist->Count) return false;
        _bjl = (TBJL*)bjllist->Items[from + k];
        if (t[k] == 'B' && !_bjl->branch) return false;
        if (t[k] == 'L' && !_bjl->loc) return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TDecompileEnv::BJLCheckPattern2(char* t, int from)
{
    int     _address = -1;
    TBJL    *_bjl;

    for (int k = 0; k < strlen(t); k++)
    {
        if (from + k >= bjllist->Count) return false;
        if (t[k] == '1')
        {
            _bjl = (TBJL*)bjllist->Items[from + k];
            if (_address == -1)
            {
                _address = _bjl->address;
                continue;
            }
            if (_bjl->address != _address) return false;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
int __fastcall TDecompileEnv::BJLFindLabel(int address, int* no)
{
    TBJL    *_bjl;

    *no = -1;
    for (int k = 0; k < bjllist->Count; k++)
    {
        _bjl = (TBJL*)bjllist->Items[k];
        if (_bjl->loc && _bjl->address == address)
        {
            *no = k;
            return _bjl->idx;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::BJLSeqSetStateU(int* idx, int num)
{
    TBJLInfo    *_bjlInfo;

    for (int k = 0; k < num; k++)
    {
        if (idx[k] < 0 || idx[k] >= BJLseq->Count) break;
        _bjlInfo = (TBJLInfo*)BJLseq->Items[idx[k]];
        _bjlInfo->state = 'U';
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::BJLListSetUsed(int from, int num)
{
    TBJL    *_bjl;

    for (int k = 0; k < num; k++)
    {
        if (from + k >= bjllist->Count) break;
        _bjl = (TBJL*)bjllist->Items[from + k];
        _bjl->type = BJL_USED;
    }
}
//---------------------------------------------------------------------------
char __fastcall TDecompileEnv::ExprGetOperation(String s)
{
    char    c;
    int     n = 0;

    for (int k = 1; k <= s.Length(); k++)
    {
        c = s[k];
        if (c == '(')
        {
            n++;
            continue;
        }
        if (c == ')')
        {
            n--;
            continue;
        }
        if ((c == '&' || c == '|') && !n) return c;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::ExprMerge(String& dst, String src, char op)//dst = dst op src, op = '|' or '&'
{
    char    op1, op2;

    if (dst == "") {dst = src; return;}

    op1 = ExprGetOperation(dst);
    op2 = ExprGetOperation(src);

    if (op == '|')
    {
        if (op1 == 0)
        {
            if (op2 == 0)   {dst = dst + " || " + src; return;}
            if (op2 == '|') {dst = dst + " || " + src; return;}
            if (op2 == '&') {dst = dst + " || (" + src + ")"; return;}
        }
        if (op1 == '|')
        {
            if (op2 == 0)   {dst = dst + " || " + src; return;}
            if (op2 == '|') {dst = dst + " || " + src; return;}
            if (op2 == '&') {dst = dst + " || (" + src + ")"; return;}
        }
        if (op1 == '&')
        {
            if (op2 == 0)   {dst = "(" + dst + ") || " + src; return;}
            if (op2 == '|') {dst = "(" + dst + ") || " + src; return;}
            if (op2 == '&') {dst = "(" + dst + ") || (" + src + ")"; return;}
        }
    }
    if (op == '&')
    {
        if (op1 == 0)
        {
            if (op2 == 0)   {dst = dst + " && " + src; return;}
            if (op2 == '|') {dst = dst + " && (" + src + ")"; return;}
            if (op2 == '&') {dst = dst + " && " + src; return;}
        }
        if (op1 == '|')
        {
            if (op2 == 0)   {dst = "(" + dst + ") && " + src; return;}
            if (op2 == '|') {dst = "(" + dst + ") && (" + src + ")"; return;}
            if (op2 == '&') {dst = "(" + dst + ") && " + src; return;}
        }
        if (op1 == '&')
        {
            if (op2 == 0)   {dst = dst + " && " + src; return;}
            if (op2 == '|') {dst = dst + " && (" + src + ")"; return;}
            if (op2 == '&') {dst = dst + " && " + src; return;}
        }
    }
}
//---------------------------------------------------------------------------
String __fastcall TDecompileEnv::PrintBJL()
{
    int         n, m, k;
    String      _result = "";
    CMPITEM     *_cmpItem;
    TBJLInfo    *_bjlInfo;

    for (n = 0; n < BJLseq->Count; n++)
    {
        _bjlInfo = (TBJLInfo*)BJLseq->Items[n];
        if (_bjlInfo->state == 'I')
        {
            _result = _bjlInfo->result;
            for (m = 0, k = 1; m < CmpStack->Count; m++)
            {
                _cmpItem = (CMPITEM*)CmpStack->Items[m];
                _result = AnsiReplaceText(_result, "#" + String(k) + "#", "(" + _cmpItem->L + " "); k++;
                _result = AnsiReplaceText(_result, "#" + String(k) + "#", " " + _cmpItem->R + ")"); k++;
            }
            break;
        }
    }
    _result = AnsiReplaceText(_result, "||", "Or");
    _result = AnsiReplaceText(_result, "&&", "And");
    return _result;
}
//---------------------------------------------------------------------------
DWORD __fastcall TDecompiler::Decompile(DWORD fromAdr, DWORD flags, PLoopInfo loopInfo)
{
    bool            _cmp, _immInt64Val, _fullSim;
    BYTE            _op;
    DWORD           _dd, _curAdr, _branchAdr, _sAdr, _jmpAdr, _endAdr, _adr;
    DWORD           _begAdr;
    int             n, _kind, _skip1, _skip2, _size, _elSize, _procSize;
    int             _fromPos, _curPos, _endPos, _instrLen, _instrLen1, _num, _regIdx, _pos, _sPos;
    int             _decPos, _cmpRes, _varidx, _brType, _mod, _div;
    int             _bytesToSkip, _bytesToSkip1, _bytesToSkip2, _bytesToSkip3;
    __int64         _int64Val;
    PInfoRec        _recN, _recN1;
    PLoopInfo       _loopInfo;
    PXrefRec 	    _recX;
    ITEM            _item, _item1, _item2;
    String          _line, _comment, _name, _typeName;
    DISINFO         _disInfo;
    TDecompiler     *de;

    _fromPos = Adr2Pos(fromAdr);

    _line = "//" + Val2Str8(fromAdr);
    if (IsFlagSet(cfPass, _fromPos))
    {
        _line += "??? And ???";
        //return fromAdr;
    }
    Env->AddToBody(_line);

    _curPos = _fromPos; _curAdr = fromAdr;
    _procSize = GetProcSize(Env->StartAdr);

    while (1)
    {
//!!!
if (_curAdr == 0x0068CFA5)
_curAdr = _curAdr;
        //End of decompilation
        if (DeFlags[_curAdr - Env->StartAdr] == 1)
        {
            SetFlag(cfPass, _fromPos);
            break;
        }
        //@TryFinallyExit
        if (IsFlagSet(cfFinallyExit, _curPos))
        {
            _pos = _curPos; _adr = _curAdr; _num = 0;
            while (IsFlagSet(cfFinallyExit, _pos))
            {
                ClearFlag(cfFinallyExit, _pos);//to avoid infinity recursion
                _pos++; _adr++; _num++;
            }

            de = new TDecompiler(Env);
            de->SetStackPointers(this);
            de->SetDeFlags(DeFlags);
            de->SetStop(_adr);
            try
            {
                _curAdr = de->Decompile(_curAdr, 0, 0);
                //Env->AddToBody("Exit;");
            }
            catch(Exception &exception)
            {
                delete de;
                throw Exception("FinallyExit->" + exception.Message);
            }
            delete de;

            SetFlags(cfFinallyExit, _curPos, _num);//restore flags
            _curPos = _pos; _curAdr = _adr;
            continue;
        }
        //Try
        if (IsFlagSet(cfTry, _curPos))
        {
            try
            {
                _curAdr = DecompileTry(_curAdr, flags, loopInfo);
            }
            catch(Exception &exception)
            {
                throw Exception("Decompile->" + exception.Message);
            }
            _curPos = Adr2Pos(_curAdr);
            continue;
        }

        if (IsFlagSet(cfLoop, _curPos))
        {
            _recN = GetInfoRec(_curAdr);
            if (IsFlagSet(cfFrame, _curPos))
            {
                if (_recN && _recN->xrefs->Count == 1)
                {
                    _recX = (PXrefRec)_recN->xrefs->Items[0];
                    _endPos = GetNearestUpInstruction(Adr2Pos(_recX->adr + _recX->offset)); _endAdr = Pos2Adr(_endPos);
                    //Check instructions between _curAdr and _endAdr (must be push, add or sub to full simulation)
                    _fullSim = true; _pos = _curPos; _adr = _curAdr;
                    while (1)
                    {
                        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
                        _op = Disasm.GetOp(_disInfo.Mnem);
                        if (_op != OP_PUSH && _op != OP_ADD && _op != OP_SUB)
                        {
                            _fullSim = false;
                            break;
                        }
                        _pos += _instrLen; _adr += _instrLen;
                        if (_pos >= _endPos) break;
                    }
                    //Full simulation
                    if (_fullSim)
                    {
                        //Get instruction at _endAdr
                        _pos = _endPos; _adr = _endAdr;
                        _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
                        _op = Disasm.GetOp(_disInfo.Mnem);
                        //dec reg in frame - full simulate
                        if (_op == OP_DEC && _disInfo.OpType[0] == otREG)
                        {
                            _regIdx = _disInfo.OpRegIdx[0];
                            GetRegItem(_regIdx, &_item);
                            //Save dec position
                            _decPos = _pos;

                            _num = _item.IntValue;
                            //next instruction is jne
                            _pos += _instrLen;
                            _adr += _instrLen;
                            _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &_disInfo, 0);
                            _branchAdr = _disInfo.Immediate;
                            //Save position and address
                            _sPos = _pos + _instrLen;
                            _sAdr = _adr + _instrLen;

                            for (n = 0; n < _num; n++)
                            {
                                _adr = _branchAdr;
                                _pos = Adr2Pos(_adr);
                                while (1)
                                {
                                    if (_pos == _decPos) break;
                                    _instrLen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &DisInfo, 0);
                                    _op = Disasm.GetOp(DisInfo.Mnem);
                                    if (_op == OP_PUSH) SimulatePush(_adr, true);
                                    if (_op == OP_ADD || _op == OP_SUB) SimulateInstr2(_adr, _op);
                                    _pos += _instrLen;
                                    _adr += _instrLen;
                                }
                            }
                            //reg = 0 after cycle
                            InitItem(&_item);
                            _item.Flags |= IF_INTVAL;
                            SetRegItem(_regIdx, &_item);
                            _curPos = _sPos;
                            _curAdr = _sAdr;
                            continue;
                        }
                    }
                }
            }
            else//loop
            {
                //Count xrefs from above
                for (n = 0, _num = 0; n < _recN->xrefs->Count; n++)
                {
                    _recX = (PXrefRec)_recN->xrefs->Items[n];
                    if (_recX->adr + _recX->offset < _curAdr) _num++;
                }
                if (_recN && _recN->counter < _recN->xrefs->Count - _num)
                {
                    _recN->counter++;
                    _loopInfo = GetLoopInfo(_curAdr);
                    if (!_loopInfo)
                    {
                        Env->ErrAdr = _curAdr;
                        throw Exception("Control flow under construction");
                    }
                    //for
                    if (_loopInfo->Kind == 'F')
                    {
                        _line = "for ";
                        if (_loopInfo->forInfo->NoVar)
                        {
                            _varidx = _loopInfo->forInfo->CntInfo.IdxValue;
                            //register
                            if (_loopInfo->forInfo->CntInfo.IdxType == itREG)
                                _line += GetDecompilerRegisterName(_varidx);
                            //local var
                            if (_loopInfo->forInfo->CntInfo.IdxType == itLVAR)
                                _line += Env->GetLvarName((int)_varidx, "Integer");
                        }
                        else
                        {
                            _varidx = _loopInfo->forInfo->VarInfo.IdxValue;
                            //register
                            if (_loopInfo->forInfo->VarInfo.IdxType == itREG)
                                _line += GetDecompilerRegisterName(_varidx);
                            //local var
                            if (_loopInfo->forInfo->VarInfo.IdxType == itLVAR)
                                _line += Env->GetLvarName((int)_varidx, "Integer");
                        }
                        _line += " := " + _loopInfo->forInfo->From + " ";
                        if (_loopInfo->forInfo->Down) _line += "down";
                        _line += "to " + _loopInfo->forInfo->To + " do";
                        Env->AddToBody(_line);

                        de = new TDecompiler(Env);
                        de->SetStackPointers(this);
                        de->SetDeFlags(DeFlags);
                        de->SetStop(_loopInfo->forInfo->StopAdr);
                        try
                        {
                            Env->AddToBody("begin");
                            _curAdr = de->Decompile(_curAdr, 0, _loopInfo);
                            Env->AddToBody("end");
                        }
                        catch(Exception &exception)
                        {
                            delete de;
                            throw Exception("Loop->" + exception.Message);
                        }
                        delete de;
                        if (_curAdr > _loopInfo->BreakAdr)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Loop->desynchronization");
                        }
                        _curAdr = _loopInfo->BreakAdr;
                        _curPos = Adr2Pos(_curAdr);
                        delete _loopInfo;
                        continue;
                    }
                    //while, repeat
                    else
                    {
                        de = new TDecompiler(Env);
                        de->SetStackPointers(this);
                        de->SetDeFlags(DeFlags);
                        de->SetStop(_loopInfo->BreakAdr);
                        try
                        {
                            if (_loopInfo->Kind == 'R')
                                Env->AddToBody("repeat");
                            else
                            {
                                Env->AddToBody("while () do");
                                Env->AddToBody("begin");
                            }
                            _curAdr = de->Decompile(_curAdr, 0, _loopInfo);
                            if (_loopInfo->Kind == 'R')
                                Env->AddToBody("until");
                            else
                                Env->AddToBody("end");
                        }
                        catch(Exception &exception)
                        {
                            delete de;
                            throw Exception("Loop->" + exception.Message);
                        }
                        delete de;
                        if (_curAdr > _loopInfo->BreakAdr)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Loop->desynchronization");
                        }
                        _curAdr = _loopInfo->BreakAdr;
                        _curPos = Adr2Pos(_curAdr);
                        delete _loopInfo;
                        continue;
                    }             
                }
            }
        }
        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
        //if (!_instrLen)
        //{
        //    Env->ErrAdr = _curAdr;
        //    throw Exception("Unknown instruction");
        //}
        if (!_instrLen)
        {
            Env->AddToBody("???");
            _curPos++; _curAdr++;
            continue;
        }

        if (IsFlagSet(cfDSkip, _curPos))
        {
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        _dd = *((DWORD*)DisInfo.Mnem);
        //skip wait, sahf
        if (_dd == 'tiaw' || _dd == 'fhas')
        {
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        _op = Disasm.GetOp(DisInfo.Mnem);
        //cfSkip - skip instructions
        if (IsFlagSet(cfSkip, _curPos))
        {
            //Constructor or Destructor
            if ((_op == OP_TEST || _op == OP_CMP || DisInfo.Call) && (flags & (CF_CONSTRUCTOR | CF_DESTRUCTOR)))
            {
                while (1)
                {
                    //If instruction test or cmp - skip until loc (instruction at loc position need to be executed)
                    if ((_op == OP_TEST || _op == OP_CMP) && IsFlagSet(cfLoc, _curPos)) break;
                    _curPos += _instrLen; _curAdr += _instrLen;
                    //Skip @BeforeDestruction
                    if (DisInfo.Call && !IsFlagSet(cfSkip, _curPos)) break;
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                    if (!_instrLen)
                    {
                        Env->AddToBody("???");
                        _curPos++; _curAdr++;
                        continue;
                    }
                }
                continue;
            }
            if ((flags & (CF_FINALLY | CF_EXCEPT)))
            {
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            if (DisInfo.Call)
            {
                if (flags & CF_EXCEPT)
                {
                    _recN = GetInfoRec(DisInfo.Immediate);
                    if (_recN->SameName("@DoneExcept"))
                    {
                        _curPos += _instrLen; _curAdr += _instrLen;
                        break;
                    }
                }
            }
        }

        if (DisInfo.Branch)
        {
            _pos = Adr2Pos(DisInfo.Immediate);
            if (DisInfo.Conditional)
            {
                _bytesToSkip = IsIntOver(_curAdr);
                if (_bytesToSkip)
                {
                    _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                    continue;
                }
                //Skip jns
                if (_dd == 'snj')
                {
                    _curAdr = DisInfo.Immediate;
                    _curPos = Adr2Pos(_curAdr);
                    continue;
                }
            }
            //skip jmp
            else
            {
                //Case without cmp
                if (IsFlagSet(cfSwitch, _curPos))
                {
                    GetRegItem(DisInfo.OpRegIdx[0], &_item);
                    if (_item.Value != "")
                        Env->AddToBody("case " + _item.Value + " of");
                    else
                        Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                    _curAdr = DecompileCaseEnum(_curAdr, 0, loopInfo);
                    Env->AddToBody("end");
                    _curPos = Adr2Pos(_curAdr);
                    continue;
                }
                //Some deoptimization!
                if (0)//IsFlagSet(cfLoop, _pos)
                {
                    _recN = GetInfoRec(DisInfo.Immediate);
                    if (_recN->xrefs)
                    {
                        for (n = 0; n < _recN->xrefs->Count; n++)
                        {
                            _recX = (PXrefRec)_recN->xrefs->Items[n];
                            if (_recX->adr + _recX->offset == _curAdr && _recX->type == 'J')
                            {
                                SetFlag(cfLoop | cfLoc, _curPos);
                                _recN1 = GetInfoRec(_curAdr);
                                if (!_recN1) _recN1 = new InfoRec(_curPos, ikUnknown);
                                continue;
                            }
                        }
                    }
                }
                //jmp XXXXXXXX - End Of Decompilation?
                if (DeFlags[DisInfo.Immediate - Env->StartAdr] == 1)
                {
                    //SetFlag(cfPass, _fromPos);
                    //check Exit
                    if (IsExit(DisInfo.Immediate))
                        Env->AddToBody("Exit;");
                    //if jmp BreakAdr
                    if (loopInfo && loopInfo->BreakAdr == DisInfo.Immediate)
                        Env->AddToBody("Break;");
                    _curPos += _instrLen; _curAdr += _instrLen;
                    break;
                }
                _curPos += _instrLen; _curAdr += _instrLen;
                if (DeFlags[_curAdr - Env->StartAdr] == 1)
                {
                    //if stop flag at this point - check Exit
                    if (IsExit(DisInfo.Immediate))
                        Env->AddToBody("Exit;");
                    //if jmp BreakAdr
                    if (loopInfo && loopInfo->BreakAdr == DisInfo.Immediate)
                        Env->AddToBody("Break;");
                }
                continue;
            }
        }

        if (DisInfo.Call)
        {
            _recN = GetInfoRec(DisInfo.Immediate);
            if (_recN)
            {
                //Inherited
                if ((flags & (CF_CONSTRUCTOR | CF_DESTRUCTOR)) && IsValidCodeAdr(DisInfo.Immediate))
                {
                    GetRegItem(16, &_item);
                    if (SameText(_item.Value, "Self"))  //eax=Self
                    {
                        if (_recN->kind == ikConstructor || _recN->kind == ikDestructor)
                        {
                            SimulateInherited(DisInfo.Immediate);
                            Env->AddToBody("inherited;");
                            _curPos += _instrLen; _curAdr += _instrLen;
                            continue;
                        }
                    }
                }
                //Other case (not constructor and destructor)
                if (IsInheritsByProcName(Env->ProcName, _recN->GetName()))
                {
                    SimulateInherited(DisInfo.Immediate);
                    InitItem(&_item);
                    _item.Precedence = PRECEDENCE_ATOM;
                    _item.Name = "EAX";
                    _item.Type = _recN->type;
                    SetRegItem(16, &_item);
                    Env->AddToBody("EAX := inherited " + ExtractProcName(Env->ProcName) + ";");
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
            }
            //true - CMP
            _cmp = SimulateCall(_curAdr, DisInfo.Immediate, _instrLen, 0, 0);
            if (_cmp)
            {
                _sPos = _curPos; _sAdr = _curAdr;
                _curPos += _instrLen; _curAdr += _instrLen;
                _cmpRes = GetCmpInfo(_curAdr);
                CmpInfo.O = CmpOp;

                if (_cmpRes == CMP_FAILED) continue;

                if (flags & CF_BJL)
                {
                    CMPITEM* _cmpItem = new CMPITEM;
                    _cmpItem->L = CmpInfo.L;
                    _cmpItem->O = CmpInfo.O;
                    _cmpItem->R = CmpInfo.R;
                    Env->CmpStack->Add((void*)_cmpItem);
                    //skip jcc
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;//???
                }

                if (_cmpRes == CMP_BRANCH)
                {
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                    //jcc up
                    if (_disInfo.Immediate < _curAdr)
                    {
                        _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                        Env->AddToBody(_line);
                        _curPos += _instrLen; _curAdr += _instrLen;
                        continue;
                    }
                     _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                    //Skip conditional branch
                    _curAdr += _instrLen;

                    _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                    _curPos = Adr2Pos(_curAdr);
                    continue;
                }
                if (_cmpRes == CMP_SET)
                {
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                    SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
            }
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }

        if (_op == OP_MOV)
        {
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (DisInfo.Ret)
        {
            _ESP_ += 4;
            _curPos += _instrLen; _curAdr += _instrLen;
            //End of proc
            if (_procSize && _curPos - _fromPos < _procSize)
                Env->AddToBody("Exit;");
            WasRet = true;
            SetFlag(cfPass, Adr2Pos(fromAdr));
            break;
            //continue;
        }
        if (_op == OP_PUSH)
        {
            _bytesToSkip1 = IsInt64ComparisonViaStack1(_curAdr, &_skip1, &_endAdr);
            _bytesToSkip2 = IsInt64ComparisonViaStack2(_curAdr, &_skip1, &_skip2, &_endAdr);
            if (_bytesToSkip1 + _bytesToSkip2 == 0)
            {
                SimulatePush(_curAdr, !IsFlagSet(cfFrame, _curPos));
                _curPos += _instrLen; _curAdr += _instrLen;
            }
            else
            {
                //Save position and address
                _sPos = _curPos; _sAdr = _curAdr;

                //Simulate push
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulatePush(_curAdr, true);
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulatePush(_curAdr, true);
                _curPos += _instrLen; _curAdr += _instrLen;

                //Decompile until _skip1
                SetStop(_endAdr);
                Decompile(_curAdr, flags, 0);

                if (_bytesToSkip1)
                    _cmpRes = GetCmpInfo(_sAdr + _bytesToSkip1);
                else
                    _cmpRes = GetCmpInfo(_sAdr + _bytesToSkip2);

                //Simulate 2nd cmp instruction
                _curPos = _sPos + _skip1; _curAdr = _sAdr + _skip1;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr2(_curAdr, OP_CMP);
                _curPos += _instrLen; _curAdr += _instrLen;
                //Simulate pop
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulatePop(_curAdr);
                _curPos += _instrLen; _curAdr += _instrLen;
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulatePop(_curAdr);

                if (_bytesToSkip1)
                {
                    _curPos = _sPos + _bytesToSkip1; _curAdr = _sAdr + _bytesToSkip1;
                }
                else
                {
                    _curPos = _sPos + _bytesToSkip2; _curAdr = _sAdr + _bytesToSkip2;
                }

                if (_cmpRes == CMP_FAILED) continue;

                if (flags & CF_BJL)
                {
                    CMPITEM* _cmpItem = new CMPITEM;
                    _cmpItem->L = CmpInfo.L;
                    _cmpItem->O = CmpInfo.O;
                    _cmpItem->R = CmpInfo.R;
                    Env->CmpStack->Add((void*)_cmpItem);
                    //skip jcc
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                if (_cmpRes == CMP_BRANCH)
                {
                    _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                    //jcc up
                    if (_disInfo.Immediate < _curAdr)
                    {
                        _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                        Env->AddToBody(_line);
                        _curPos += _instrLen; _curAdr += _instrLen;
                        continue;
                    }
                     _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                    //Skip conditional branch
                    _curAdr += _instrLen;

                    _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                    _curPos = Adr2Pos(_curAdr);
                }
            }
            continue;
        }
        if (_op == OP_POP)
        {
            SimulatePop(_curAdr);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_XOR)
        {
            if (!IsXorMayBeSkipped(_curAdr)) SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_CMP || (DisInfo.Float && _dd == 'mocf'))
        {
            //Save position and address
            _sPos = _curPos; _sAdr = _curAdr;
            _bytesToSkip = IsBoundErr(_curAdr);
            if (_bytesToSkip)
            {
                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }
            if (IsFlagSet(cfSwitch, _curPos))
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileCaseEnum(_curAdr, 0, loopInfo);
                Env->AddToBody("end");
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            _bytesToSkip = IsInlineLengthCmp(_curAdr);
            if (_bytesToSkip)
            {
                GetMemItem(_curAdr, &_item, 0);
                _line = _item.Name + " := Length(";
                if (_item.Name != "")
                    _line += _item.Name;
                else
                    _line += _item.Value;
                _line += ");";
                Env->AddToBody(_line);

                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }
            _endAdr = IsGeneralCase(_curAdr, Env->StartAdr + Env->Size);
            if (_endAdr)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileGeneralCase(_curAdr, _curAdr, loopInfo, 0);
                Env->AddToBody("end");
                //_curAdr = _endAdr;
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            //skip current instruction (cmp)
            _curPos += _instrLen; _curAdr += _instrLen;

            if (!DisInfo.Float)
            {
                _bytesToSkip1 = 0;//IsInt64Equality(_sAdr, &_skip1, &_skip2, &_immInt64Val, &_int64Val);
                _bytesToSkip2 = 0;//IsInt64NotEquality(_sAdr, &_skip1, &_skip2, &_immInt64Val, &_int64Val);
                _bytesToSkip3 = IsInt64Comparison(_sAdr, &_skip1, &_skip2, &_immInt64Val, &_int64Val);
                if (_bytesToSkip1 + _bytesToSkip2 + _bytesToSkip3 == 0)
                {
                    _cmpRes = GetCmpInfo(_curAdr);
                    SimulateInstr2(_sAdr, _op);

                    if (_cmpRes == CMP_FAILED) continue;

                    if (flags & CF_BJL)
                    {
                        CMPITEM* _cmpItem = new CMPITEM;
                        _cmpItem->L = CmpInfo.L;
                        _cmpItem->O = CmpInfo.O;
                        _cmpItem->R = CmpInfo.R;
                        Env->CmpStack->Add((void*)_cmpItem);
                        //skip jcc
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                        _curPos += _instrLen; _curAdr += _instrLen;
                        continue;
                    }
                }
                //int64 comparison
                else
                {
                    if (_bytesToSkip1)
                    {
                        _cmpRes = GetCmpInfo(_sAdr + _skip2);
                        _curPos = _sPos + _skip1; _curAdr = _sAdr + _skip1;
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                        SimulateInstr2(_curAdr, _op);
                        if (_immInt64Val) CmpInfo.R = IntToStr(_int64Val) + "{" + IntToHex(_int64Val, 0) + "}";
                        _curPos = _sPos + _bytesToSkip1; _curAdr = _sAdr + _bytesToSkip1;
                    }
                    else if (_bytesToSkip2)
                    {
                        _cmpRes = GetCmpInfo(_sAdr + _skip2);
                        _curPos = _sPos + _skip1; _curAdr = _sAdr + _skip1;
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                        SimulateInstr2(_curAdr, _op);
                        if (_immInt64Val) CmpInfo.R = IntToStr(_int64Val) + "{" + IntToHex(_int64Val, 0) + "}";
                        _curPos = _sPos + _bytesToSkip2; _curAdr = _sAdr + _bytesToSkip2;
                    }
                    else//_bytesToSkip3
                    {
                        _cmpRes = GetCmpInfo(_sAdr + _skip2);
                        _curPos = _sPos + _skip1; _curAdr = _sAdr + _skip1;
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                        SimulateInstr2(_curAdr, _op);
                        if (_immInt64Val) CmpInfo.R = IntToStr(_int64Val) + "{" + IntToHex(_int64Val, 0) + "}";
                        _curPos = _sPos + _bytesToSkip3; _curAdr = _sAdr + _bytesToSkip3;
                    }
                    if (flags & CF_BJL)
                    {
                        CMPITEM* _cmpItem = new CMPITEM;
                        _cmpItem->L = CmpInfo.L;
                        _cmpItem->O = CmpInfo.O;
                        _cmpItem->R = CmpInfo.R;
                        Env->CmpStack->Add((void*)_cmpItem);
                        //skip jcc
                        _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                        _curPos += _instrLen; _curAdr += _instrLen;
                        continue;
                    }
                }
            }
            else
            {
                //skip until branch or set
                while (1)
                {
                    _instrLen1 = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                    _op = Disasm.GetOp(_disInfo.Mnem);
                    if (_disInfo.Branch || _op == OP_SET) break;
                    _curPos += _instrLen1; _curAdr += _instrLen1;
                }

                _cmpRes = GetCmpInfo(_curAdr);
                SimulateFloatInstruction(_sAdr);

                if (flags & CF_BJL)
                {
                    SimulateFloatInstruction(_sAdr);
                    CMPITEM* _cmpItem = new CMPITEM;
                    _cmpItem->L = CmpInfo.L;
                    _cmpItem->O = CmpInfo.O;
                    _cmpItem->R = CmpInfo.R;
                    Env->CmpStack->Add((void*)_cmpItem);
                    _curPos += _instrLen1; _curAdr += _instrLen1;
                    continue;
                }
            }
            if (_cmpRes == CMP_FAILED) continue;

            if (_cmpRes == CMP_BRANCH)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                //Exit
                if (IsExit(_disInfo.Immediate))
                {
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Exit;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                //jcc up
                if (_disInfo.Immediate < _curAdr)
                {
                    if (DisInfo.Float) SimulateFloatInstruction(_sAdr);
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                //jcc at BreakAdr
                if (loopInfo && loopInfo->BreakAdr == _disInfo.Immediate)
                {
                    if (DisInfo.Float) SimulateFloatInstruction(_sAdr);
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Break;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                //jcc at forInfo->StopAdr
                if (loopInfo && loopInfo->forInfo && loopInfo->forInfo->StopAdr == _disInfo.Immediate)
                {
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _curAdr += _instrLen;
                
                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
        }
        if (_op == OP_TEST || _op == OP_BT)
        {
            //Save address
            _sAdr = _curAdr;
            _bytesToSkip = IsInlineLengthTest(_curAdr);
            if (_bytesToSkip)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                _item.Precedence = PRECEDENCE_ATOM;
                _item.Value = "Length(" + _item.Value + ")";
                _item.Type = "Integer";
                SetRegItem(DisInfo.OpRegIdx[0], &_item);

                _line = GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " := Length(" + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + ");";
                Env->AddToBody(_line);

                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }
            _bytesToSkip = IsInlineDiv(_curAdr, &_div);
            if (_bytesToSkip)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                _item.Precedence = PRECEDENCE_MULT;
                _item.Value = GetString(&_item, PRECEDENCE_MULT) + " Div " + String(_div);
                _item.Type = "Integer";
                SetRegItem(DisInfo.OpRegIdx[0], &_item);

                _line = GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " := " + _item.Value + ";";
                Env->AddToBody(_line);

                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }

            _cmpRes = GetCmpInfo(_curAdr + _instrLen);
            SimulateInstr2(_sAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;

            if (_cmpRes == CMP_FAILED) continue;

            if (flags & CF_BJL)
            {
                CMPITEM* _cmpItem = new CMPITEM;
                _cmpItem->L = CmpInfo.L;
                _cmpItem->O = CmpInfo.O;
                _cmpItem->R = CmpInfo.R;
                Env->CmpStack->Add((void*)_cmpItem);
                //skip jcc
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }

            if (_cmpRes == CMP_BRANCH)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                //jcc up
                if (_disInfo.Immediate < _curAdr)
                {
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                //jcc at BreakAdr
                if (loopInfo && loopInfo->BreakAdr == _disInfo.Immediate)
                {
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Break;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                //jcc at forInfo->StopAdr
                if (loopInfo && loopInfo->forInfo && loopInfo->forInfo->StopAdr == _disInfo.Immediate)
                {
                    _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Continue;";
                    Env->AddToBody(_line);
                    _curPos += _instrLen; _curAdr += _instrLen;
                    continue;
                }
                 _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _curAdr += _instrLen;

                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
        }
        if (_op == OP_LEA)
        {
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_ADD)
        {
            _bytesToSkip = IsIntOver(_curAdr + _instrLen);

            _endAdr = IsGeneralCase(_curAdr, Env->StartAdr + Env->Size);
            if (_endAdr)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileGeneralCase(_curAdr, _curAdr, loopInfo, 0);
                Env->AddToBody("end");
                //_curAdr = _endAdr;
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            //Next switch
            if (IsFlagSet(cfSwitch, _curPos + _instrLen))
            {
                n = -DisInfo.Immediate;
                _curPos += _instrLen; _curAdr += _instrLen;
                Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                GetRegItem(_disInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(_disInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileCaseEnum(_curAdr, n, loopInfo);
                Env->AddToBody("end");
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen + _bytesToSkip; _curAdr += _instrLen + _bytesToSkip;
            continue;
        }
        if (_op == OP_SUB)
        {
            //Save address
            _sAdr = _curAdr;
            _bytesToSkip = IsIntOver(_curAdr + _instrLen);

            _endAdr = IsGeneralCase(_curAdr, Env->StartAdr + Env->Size);
            if (_endAdr)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileGeneralCase(_curAdr, _curAdr, loopInfo, 0);
                Env->AddToBody("end");
                //_curAdr = _endAdr;
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            //Next switch
            if (IsFlagSet(cfSwitch, _curPos + _instrLen))
            {
                n = DisInfo.Immediate;
                _curPos += _instrLen; _curAdr += _instrLen;
                Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                GetRegItem(_disInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(_disInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileCaseEnum(_curAdr, n, loopInfo);
                Env->AddToBody("end");
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            _cmpRes = GetCmpInfo(_curAdr + _instrLen);
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;

            if (_bytesToSkip)
            {
                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }

            if (_cmpRes == CMP_FAILED) continue;

            if (flags & CF_BJL)
            {
                CMPITEM* _cmpItem = new CMPITEM;
                _cmpItem->L = CmpInfo.L;
                _cmpItem->O = CmpInfo.O;
                _cmpItem->R = CmpInfo.R;
                Env->CmpStack->Add((void*)_cmpItem);
                //skip jcc
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }

            if (_cmpRes == CMP_BRANCH)
            {
                _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curAdr += _instrLen;

                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            continue;
        }
        if (_op == OP_AND)
        {
            _bytesToSkip = IsInlineMod(_curAdr, &_mod);
            if (_bytesToSkip)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                _item.Precedence = PRECEDENCE_ATOM;
                _item.Value = _item.Value + " mod " + String(_mod);
                _item.Type = "Integer";
                SetRegItem(DisInfo.OpRegIdx[0], &_item);

                _line = GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " := " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " mod " + String(_mod);
                Env->AddToBody(_line);

                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
                continue;
            }
            _cmpRes = GetCmpInfo(_curAdr + _instrLen);
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;

            if (_cmpRes == CMP_FAILED) continue;

            if (flags & CF_BJL)
            {
                CMPITEM* _cmpItem = new CMPITEM;
                _cmpItem->L = CmpInfo.L;
                _cmpItem->O = CmpInfo.O;
                _cmpItem->R = CmpInfo.R;
                Env->CmpStack->Add((void*)_cmpItem);
                //skip jcc
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            if (_cmpRes == CMP_BRANCH)
            {
                _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curAdr += _instrLen;

                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            continue;
        }
        if (_op == OP_OR)
        {
            _cmpRes = GetCmpInfo(_curAdr + _instrLen);
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;

            if (_cmpRes == CMP_FAILED) continue;

            if (flags & CF_BJL)
            {
                CMPITEM* _cmpItem = new CMPITEM;
                _cmpItem->L = CmpInfo.L;
                _cmpItem->O = CmpInfo.O;
                _cmpItem->R = CmpInfo.R;
                Env->CmpStack->Add((void*)_cmpItem);
                //skip jcc
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            if (_cmpRes == CMP_BRANCH)
            {
                _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curAdr += _instrLen;

                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            continue;
        }
        if (_op == OP_ADC || _op == OP_SBB)
        {
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_SAL || _op == OP_SHL)
        {
            _bytesToSkip = IsInt64Shl(_curAdr);
            if (_bytesToSkip)
            {
                DisInfo.OpNum = 2;
                DisInfo.OpType[1] = otIMM;
            }
            else
            {
                _bytesToSkip = _instrLen;
            }
            SimulateInstr2(_curAdr, _op);
            _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
            continue;
        }
        if (_op == OP_SAR || _op == OP_SHR)
        {
            _bytesToSkip = IsInt64Shr(_curAdr);
            if (_bytesToSkip)
            {
                DisInfo.OpNum = 2;
                DisInfo.OpType[1] = otIMM;
            }
            else
            {
                _bytesToSkip = _instrLen;
            }
            SimulateInstr2(_curAdr, _op);
            _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
            continue;
        }
        if (_op == OP_NEG)
        {
            SimulateInstr1(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_NOT)
        {
            SimulateInstr1(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_XCHG)
        {
            SimulateInstr2(_curAdr, _op);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (_op == OP_INC || _op == OP_DEC)
        {
            //Save address
            _sAdr = _curAdr;
            _endAdr = IsGeneralCase(_curAdr, Env->StartAdr + Env->Size);
            if (_endAdr)
            {
                GetRegItem(DisInfo.OpRegIdx[0], &_item);
                if (_item.Value != "")
                    Env->AddToBody("case " + _item.Value + " of");
                else
                    Env->AddToBody("case " + GetDecompilerRegisterName(DisInfo.OpRegIdx[0]) + " of");
                _curAdr = DecompileGeneralCase(_curAdr, _curAdr, loopInfo, 0);
                Env->AddToBody("end");
                //_curAdr = _endAdr;
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            //simulate dec as sub
            DisInfo.OpNum = 2;
            DisInfo.OpType[1] = otIMM;
            DisInfo.Immediate = 1;
            _cmpRes = GetCmpInfo(_curAdr + _instrLen);
            if (_op == OP_DEC)
                SimulateInstr2(_curAdr, OP_SUB);
            else
                SimulateInstr2(_curAdr, OP_ADD);
            _curPos += _instrLen; _curAdr += _instrLen;

            if (_cmpRes == CMP_FAILED) continue;

            if (flags & CF_BJL)
            {
                CMPITEM* _cmpItem = new CMPITEM;
                _cmpItem->L = CmpInfo.L;
                _cmpItem->O = CmpInfo.O;
                _cmpItem->R = CmpInfo.R;
                Env->CmpStack->Add((void*)_cmpItem);
                //skip jcc
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }

            if (_cmpRes == CMP_BRANCH)
            {
                _brType = BranchGetPrevInstructionType(CmpAdr, &_jmpAdr, loopInfo);
                //Skip conditional branch
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, 0, 0);
                _curAdr += _instrLen;

                _curAdr = AnalyzeConditions(_brType, _curAdr, _sAdr, _jmpAdr, loopInfo, DisInfo.Float);
                _curPos = Adr2Pos(_curAdr);
                continue;
            }
            if (_cmpRes == CMP_SET)
            {
                _instrLen = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &DisInfo, 0);
                SimulateInstr1(_curAdr, Disasm.GetOp(DisInfo.Mnem));
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            continue;
        }
        if (_op == OP_DIV || _op == OP_IDIV)
        {
            if (DisInfo.OpNum == 2)
            {
                SimulateInstr2(_curAdr, _op);
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
        }
        if (_op == OP_MUL || _op == OP_IMUL)
        {
            _bytesToSkip = IsIntOver(_curAdr + _instrLen);

            if (DisInfo.OpNum == 1)
            {
                SimulateInstr1(_curAdr, _op);
                _curPos += _instrLen + _bytesToSkip; _curAdr += _instrLen + _bytesToSkip;
                continue;
            }
            if (DisInfo.OpNum == 2)
            {
                SimulateInstr2(_curAdr, _op);
                _curPos += _instrLen + _bytesToSkip; _curAdr += _instrLen + _bytesToSkip;
                continue;
            }
            if (DisInfo.OpNum == 3)
            {
                SimulateInstr3(_curAdr, _op);
                _curPos += _instrLen + _bytesToSkip; _curAdr += _instrLen + _bytesToSkip;
                continue;
            }
        }
        if (_op == OP_CDQ)
        {
            _curPos += _instrLen; _curAdr += _instrLen;
            GetRegItem(16, &_item);
            _bytesToSkip = IsAbs(_curAdr);
            if (_bytesToSkip)
            {
                _item.Flags = IF_CALL_RESULT;
                _item.Precedence = PRECEDENCE_ATOM;
                _item.Value = "Abs(" + _item.Value + ")";
                _item.Type = "Integer";
                SetRegItem(16, &_item);

                _line = "EAX := Abs(EAX)";
                _comment = _item.Value;
                Env->AddToBody(_line + ";//" + _comment);

                _curPos += _bytesToSkip; _curAdr += _bytesToSkip;
            }
            SetRegItem(18, &_item);//Set edx to eax
            continue;
        }
        if (_op == OP_MOVS)
        {
            GetRegItem(23, &_item1);//edi
            GetRegItem(22, &_item2);//esi
            //->lvar
            if (_item1.Flags & IF_STACK_PTR)
            {
                if (_item1.Type != "")
                    _typeName = _item1.Type;
                else
                    _typeName = _item2.Type;
                _kind = GetTypeKind(_typeName, &_size);
                if (_kind == ikRecord)
                {
                    _size = GetRecordSize(_typeName);
                    for (int r = 0; r < _size; r++)
                    {
                        if (_item1.IntValue + r >= Env->StackSize)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Possibly incorrect RecordSize (or incorrect type of record)");
                        }
                        _item = Env->Stack[_item1.IntValue + r];
                        _item.Flags = IF_FIELD;
                        _item.Offset = r;
                        _item.Type = "";
                        if (r == 0) _item.Type = _typeName;
                        Env->Stack[_item1.IntValue + r] = _item;
                    }
                }
                else if (_kind == ikArray)
                {
                    _size = GetArraySize(_typeName);
                    _elSize = GetArrayElementTypeSize(_typeName);
                    _typeName = GetArrayElementType(_typeName);
                    if (!_size || !_elSize)
                    {
                        Env->ErrAdr = _curAdr;
                        throw Exception("Possibly incorrect array definition");
                    }
                    for (int r = 0; r < _size; r += _elSize)
                    {
                        if (_item1.IntValue + r >= Env->StackSize)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Possibly incorrect array definition");
                        }
                        Env->Stack[_item1.IntValue + r].Type = _typeName;
                    }
                }
                Env->AddToBody(Env->GetLvarName(_item1.IntValue, _typeName) + " := " + _item2.Value + ";");
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            //lvar->
            if (_item2.Flags & IF_STACK_PTR)
            {
                if (_item2.Type != "")
                    _typeName = _item2.Type;
                else
                    _typeName = _item1.Type;
                _kind = GetTypeKind(_typeName, &_size);
                if (_kind == ikRecord)
                {
                    _size = GetRecordSize(_typeName);
                    for (int r = 0; r < _size; r++)
                    {
                        if (_item2.IntValue + r >= Env->StackSize)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Possibly incorrect RecordSize (or incorrect type of record)");
                        }
                        _item = Env->Stack[_item2.IntValue + r];
                        _item.Flags = IF_FIELD;
                        _item.Offset = r;
                        _item.Type = "";
                        if (r == 0) _item.Type = _typeName;
                        Env->Stack[_item2.IntValue + r] = _item;
                    }
                }
                else if (_kind == ikArray)
                {
                    _size = GetArraySize(_typeName);
                    _elSize = GetArrayElementTypeSize(_typeName);
                    _typeName = GetArrayElementType(_typeName);
                    if (!_size || !_elSize)
                    {
                        Env->ErrAdr = _curAdr;
                        throw Exception("Possibly incorrect array definition");
                    }
                    for (int r = 0; r < _size; r += _elSize)
                    {
                        if (_item1.IntValue + r >= Env->StackSize)
                        {
                            Env->ErrAdr = _curAdr;
                            throw Exception("Possibly incorrect array definition");
                        }
                        Env->Stack[_item1.IntValue + r].Type = _typeName;
                    }
                }
                Env->AddToBody(_item1.Value + " := " + Env->GetLvarName(_item2.IntValue, _typeName) + ";");
                _curPos += _instrLen; _curAdr += _instrLen;
                continue;
            }
            if (_item1.Value != "")
                _line = _item1.Value;
            else
                _line = "_edi_";
            _line += " := ";
            if (_item2.Value != "")
                _line += _item2.Value;
            else
                _line += "_esi_";
            _line += ";";
            Env->AddToBody(_line);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }
        if (DisInfo.Float)
        {
            SimulateFloatInstruction(_curAdr);
            _curPos += _instrLen; _curAdr += _instrLen;
            continue;
        }

        Env->ErrAdr = _curAdr;
        throw Exception("Still unknown");
    }
    return _curAdr;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInherited(DWORD procAdr)
{
    PInfoRec _recN = GetInfoRec(procAdr);
    _ESP_ += _recN->procInfo->retBytes;
}
//---------------------------------------------------------------------------
bool __fastcall TDecompiler::SimulateCall(DWORD curAdr, DWORD callAdr, int instrLen, PMethodRec recM, DWORD AClassAdr)
{
    bool        _sep, _fromKB, _vmt;
    BYTE        _kind, _callKind, _retKind, _methodKind, *p, *pp;
    int         _res = 0;
    int         _argsNum, _retBytes, _retBytesCalc, _len, _val, _esp;
    int         _idx = -1, _rn, _ndx, ss, _pos, _size, _recsize;
    WORD*       _uses;
    DWORD       _classAdr, _adr, _dynAdr;
    char        *tmpBuf;
    ITEM        _item, _item1;
    ARGINFO     _aInfo, *_argInfo = &_aInfo;
    PFIELDINFO  _fInfo;
    MProcInfo   _pInfo;
    PMethodRec  _recM;
    PInfoRec    _recN, _recN1;
    MTypeInfo   _tInfo;
    TDecompiler *de;
    String      _name, _type, _alias, _line, _retType, _value, _iname, _embAdr;
    String      _typeName, _comment, _regName, _propName, _lvarName;

    pp = 0; ss = 0; _propName = "";
    //call imm
    if (IsValidCodeAdr(callAdr))
    {
        _recN = GetInfoRec(callAdr);
        _name = _recN->GetName();
        //Is it property function (Set, Get, Stored)?
        if (_name.Pos("."))
        {
            _propName = KnowledgeBase.IsPropFunction(ExtractClassName(_name), ExtractProcName(_name));
        }
        if (SameText(_name, "@AbstractError"))
        {
            Env->ErrAdr = curAdr;
            throw Exception("Pure Virtual Call");
        }
        //Import can have no prototype
        if (IsFlagSet(cfImport, Adr2Pos(callAdr)))
        {
            if (!CheckPrototype(_recN))
            {
                Env->AddToBody(_recN->GetName() + "(...);//Import");
                _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
                if (_value == "")
                {
                    Env->ErrAdr = curAdr;
                    throw Exception("Bye!");
                }
                sscanf(_value.c_str(), "%lX", &_retBytes);
                _ESP_ += _retBytes;
                return false;
            }
        }
        //@DispInvoke
        if (SameText(_name, "@DispInvoke"))
        {
            Env->AddToBody("DispInvoke(...);");
            _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
            if (_value == "")
            {
                Env->ErrAdr = curAdr;
                throw Exception("Bye!");
            }
            sscanf(_value.c_str(), "%lX", &_retBytes);
            _ESP_ += _retBytes;
            return false;
        }
        if (!recM)
        {
            _name = _recN->GetName();
            _callKind = _recN->procInfo->flags & 7;
            _methodKind = _recN->kind;
            _argsNum = (_recN->procInfo->args) ? _recN->procInfo->args->Count : 0;
            _retType = _recN->type;
            _fromKB = (_recN->kbIdx != -1);
            _retBytes = _recN->procInfo->retBytes;
            //stdcall, pascal, cdecl - return bytes = 4 * ArgsNum
            if ((_callKind == 3 || _callKind == 2 || _callKind == 1) && !_retBytes)
                _retBytes = _argsNum * 4;
            if (_recN->procInfo->flags & PF_EMBED)
            {
                _embAdr = Val2Str8(callAdr);
                if (Env->EmbeddedList->IndexOf(_embAdr) == -1)
                {
                    Env->EmbeddedList->Add(_embAdr);
                    int _savedIdx = FMain_11011981->lbCode->ItemIndex;
                    FMain_11011981->lbCode->ItemIndex = -1;
                    if (Application->MessageBox(String("Decompile embedded procedure at address " + _embAdr + "?").c_str(), "Confirmation", MB_YESNO) == IDYES)
                    {
                        Env->AddToBody("//BEGIN_EMBEDDED_" + _embAdr);
                        Env->AddToBody(_recN->MakePrototype(callAdr, true, false, false, true, false));
                        DWORD _savedStartAdr = Env->StartAdr;
                        bool _savedBpBased = Env->BpBased;
                        int _savedLocBase = Env->LocBase;
                        int _savedSize = Env->Size;
                        Env->StartAdr = callAdr;
                        _size = GetProcSize(callAdr);
                        Env->Size = _size;
                        de = new TDecompiler(Env);
                        _ESP_ -= 4;//ret address
                        de->SetStackPointers(this);
                        de->SetStop(callAdr + _size);
                        try
                        {
                            Env->AddToBody("begin");
                            de->Decompile(callAdr, 0, 0);
                            Env->AddToBody("end");
                            Env->AddToBody("//END_EMBEDDED_" + _embAdr);
                        }
                        catch(Exception &exception)
                        {
                            delete de;
                            throw Exception("Embedded->" + exception.Message);
                        }
                        _ESP_ += 4;
                        delete de;
                        Env->StartAdr = _savedStartAdr;
                        Env->Size = _savedSize;
                        Env->BpBased = _savedBpBased;
                        Env->LocBase = _savedLocBase;
                    }
                    FMain_11011981->lbCode->ItemIndex = _savedIdx;
                }
            }
        }
        else
        {
            _name = recM->name;
            _res = (int)KnowledgeBase.GetProcInfo(recM->name.c_str(), INFO_DUMP | INFO_ARGS, &_pInfo, &_idx);
            if (_res && _res != -1)
            {
                _callKind = _pInfo.CallKind;
                switch (_pInfo.MethodKind)
                {
                case 'C':
                    _methodKind = ikConstructor;
                    break;
                case 'D':
                    _methodKind = ikDestructor;
                    break;
                case 'F':
                    _methodKind = ikFunc;
                    break;
                case 'P':
                    _methodKind = ikProc;
                    break;
                }
                _argsNum = (_pInfo.Args) ? _pInfo.ArgsNum : 0;
                _retType = _pInfo.TypeDef;
                pp = _pInfo.Args;
                _fromKB = true;
                _retBytes = GetProcRetBytes(&_pInfo);
            }
            else
            {
                _name = _recN->GetName();
                _callKind = _recN->procInfo->flags & 7;
                _methodKind = _recN->kind;
                _argsNum = (_recN->procInfo->args) ? _recN->procInfo->args->Count : 0;
                _retType = _recN->type;
                _fromKB = false;
                _retBytes = _recN->procInfo->retBytes;
            }
        }
        //Check prototype
        if (!_fromKB)
        {
            if (!CheckPrototype(_recN))
            {
                Env->ErrAdr = curAdr;
                if (_name != "")
                    throw Exception("Prototype of " + _name + " is not completed");
                else
                    throw Exception("Prototype of " + GetDefaultProcName(callAdr) + " is not completed");
            }
        }
        if (_name != "")
        {
            if (_name[1] == '@')
            {
                if (SameText(_name, "@CallDynaInst") ||
                    SameText(_name, "@CallDynaClass"))
                {
                    _recN = GetInfoRec(curAdr);
                    if (_recN)
                    {
                        PICODE* _picode = _recN->picode;
                        if (_picode && _picode->Op == OP_CALL)
                        {
                            SimulateCall(curAdr, _picode->Ofs.Address, instrLen, 0, 0);
                            return false;
                        }
                    }
                    else
                    {
                        GetRegItem(16, &_item);
                        if (DelphiVersion <= 5)
                        {
                            GetRegItem(11, &_item1);
                            _comment = GetDynaInfo(GetClassAdr(_item.Type), _item1.IntValue, &_dynAdr);	//bx
                        }
                        else
                        {
                            GetRegItem(14, &_item1);
                            _comment = GetDynaInfo(GetClassAdr(_item.Type), _item1.IntValue, &_dynAdr);	//si
                        }
                        AddPicode(Adr2Pos(curAdr), OP_CALL, _comment, _dynAdr);
                        SimulateCall(curAdr, _dynAdr, instrLen, 0, 0);
                        return false;
                    }
                }
                _alias = GetSysCallAlias(_name);
                if (_alias == "")
                {
                    return SimulateSysCall(_name, curAdr, instrLen);
                }
                _name = _alias;
            }
            //Some special functions
            if (SameText(_name, "Format"))
            {
                SimulateFormatCall();
                return false;
            }
            if (_name.Pos("."))
            {
                _line = ExtractProcName(_name);
            }
            else
                _line = _name;
        }
        else
            _line = GetDefaultProcName(callAdr);

        if (_propName != "")
            _line += "{" + _propName + "}";

        if (_methodKind == ikFunc)
        {
            if (SameText(_name, "TList.Get"))
            {
                while (1)
                {
                    _retType = ManualInput(Env->StartAdr, curAdr, "Define type of function at " + IntToHex((int)curAdr, 8), "Type:");
                    if (_retType == "")
                    {
                        Env->ErrAdr = curAdr;
                        throw Exception("You need to define type of function later");
                    }
                    //if (_retType[1] == '^')
                    //    _retType = GetTypeDeref(_retType);
                    _retKind = GetTypeKind(_retType, &_size);
                    if (_retKind) break;
                }
            }
            else
            {
                while (1)
                {
                    if (_retType[1] == '^')
                        _retType = GetTypeDeref(_retType);

                    _retKind = GetTypeKind(_retType, &_size);
                    if (_retKind) break;
                    
                    _retType = ManualInput(Env->StartAdr, curAdr, "Define type of function at " + IntToHex((int)curAdr, 8), "Type:");
                    if (_retType == "")
                    {
                        Env->ErrAdr = curAdr;
                        throw Exception("You need to define type of function later");
                    }
                }
            }
        }

        _retBytesCalc = 0; _ndx = 0;
        if (_argsNum)
        {
            _line += "(";
            if (_callKind == 0 || _callKind == 3 || _callKind == 2 || _callKind == 1)//fastcall, stdcall, pascal, cdecl
            {
                 _sep = false; _esp = _ESP_;
                _ESP_ += _retBytes;
                //fastcall, pascal - reverse order of arguments
                if (_callKind == 0 || _callKind == 2) _esp = _ESP_;
                //cdecl - stack restored by caller
                if (_callKind == 1) _ESP_ -= _retBytes;

                for (int n = 0; n < _argsNum; n++)
                {
                    if (pp)
                        FillArgInfo(n, _callKind, _argInfo, &pp, &ss);
                    else
                        _argInfo = (PARGINFO)_recN->procInfo->args->Items[n];

                    _rn = -1; _regName = "";
                    _kind = GetTypeKind(_argInfo->TypeDef, &_size);
                    if (_kind == ikFloat || _kind == ikInt64)
                        _size = _argInfo->Size;
                    else
                        _size = 4;
                    if (_argInfo->Tag == 0x22) _size = 4;

                    if (_callKind == 0)//fastcall
                    {
                        if (_methodKind == ikConstructor)
                        {
                            if (_ndx <= 1)
                            {
                                if (!_ndx)
                                {
                                    GetRegItem(16, &_item);
                                    _retType = _item.Type;
                                    _retKind = GetTypeKind(_retType, &_size);
                                    _line = _retType + ".Create(";
                                }
                                _ndx++;
                                continue;
                            }
                        }
                        if ((_kind == ikFloat || _kind == ikInt64) && _argInfo->Tag != 0x22)
                        {
                            _esp -= _size;
                            _item = Env->Stack[_esp];
                            _item1 = Env->Stack[_esp + 4];
                            _retBytesCalc += _size;
                        }
                        else
                        {
                            //fastcall
                            if (_ndx >= 0 && _ndx <= 2)
                            {
                                //eax
                                if (_ndx == 0)
                                {
                                    _rn = 16;
                                    _regName = "EAX";
                                }
                                //edx
                                else if (_ndx == 1)
                                {
                                    _rn = 18;
                                    _regName = "EDX";
                                }
                                //ecx
                                else
                                {
                                    _rn = 17;
                                    _regName = "ECX";
                                }
                                GetRegItem(_rn, &_item);
                                _ndx++;
                            }
                            //stack args
                            else
                            {
                                _esp -= _argInfo->Size;
                                _item = Env->Stack[_esp];
                                _retBytesCalc += _argInfo->Size;
                            }
                        }
                    }
                    else if (_callKind == 3 || _callKind == 1)//stdcall, cdecl
                    {
                        if (_size >= 8)
                        {
                            _item = Env->Stack[_esp];
                            _item1 = Env->Stack[_esp + 4];
                            _esp += _size;
                            _retBytesCalc += _size;
                        }
                        else
                        {
                            _item = Env->Stack[_esp];
                            _esp += _argInfo->Size;
                            _retBytesCalc += _argInfo->Size;
                        }
                    }
                    if (SameText(_argInfo->Name, "Self"))
                    {
                        if (!SameText(_item.Value, "Self")) _line = _item.Value + "." + _line;
                        _sep = false;
                        continue;
                    }
                    if (SameText(_argInfo->Name, "_Dv__"))
                    {
                        _sep = false;
                        continue;
                    }
                    if (_sep) _line += ", "; _sep = true;
                    if (_item.Flags & IF_STACK_PTR)
                    {
                        _item1 = Env->Stack[_item.IntValue];
                        if (_kind == ikInteger)
                        {
                            _lvarName = Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                            _line += _lvarName;
                            Env->Stack[_item.IntValue].Value = _lvarName;
                            continue;
                        }
                        if (_kind == ikEnumeration)
                        {
                            _lvarName = Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                            _line += _lvarName;
                            Env->Stack[_item.IntValue].Value = _lvarName;
                            continue;
                        }
                        if (_kind == ikLString || _kind == ikVariant)
                        {
                            _lvarName = Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                            _line += _lvarName;
                            Env->Stack[_item.IntValue].Value = _lvarName;;
                            Env->Stack[_item.IntValue].Type = _argInfo->TypeDef;
                            continue;
                        }
                        if (_kind == ikVMT || _kind == ikClass)
                        {
                            _line += _item1.Value;
                            continue;
                        }
                        if (_kind == ikArray)
                        {
                            _line += Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                            continue;
                        }
                        if (_kind == ikRecord)
                        {
                            _line += Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                            _recsize = GetRecordSize(_argInfo->TypeDef);
                            for (int r = 0; r < _recsize; r++)
                            {
                                if (_item.IntValue + r >= Env->StackSize)
                                {
                                    Env->ErrAdr = curAdr;
                                    throw Exception("Possibly incorrect RecordSize (or incorrect type of record)");
                                }
                                _item1 = Env->Stack[_item.IntValue + r];
                                _item1.Flags = IF_FIELD;
                                _item1.Offset = r;
                                _item1.Type = "";
                                if (r == 0) _item1.Type = _argInfo->TypeDef;
                                Env->Stack[_item.IntValue + r] = _item1;
                            }
                            continue;
                        }
                        //Type not found
                        _lvarName = Env->GetLvarName(_item.IntValue, _argInfo->TypeDef);
                        _line += _lvarName;
                        Env->Stack[_item.IntValue].Value = _lvarName;
                        Env->Stack[_item.IntValue].Type = _argInfo->TypeDef;
                        continue;
                    }
                    if (_kind == ikInteger)
                    {
                        _line += _item.Value;
                        if (_item.Flags & IF_INTVAL) _line += "{" + GetImmString(_item.IntValue) + "}";
                        continue;
                    }
                    if (_kind == ikChar)
                    {
                        _line += _item.Value;
                        if (_item.Flags & IF_INTVAL) _line += "{'" + String(_item.IntValue) + "'}";
                        continue;
                    }
                    if (_kind == ikLString || _kind == ikWString || _kind == ikUString || _kind == ikCString || _kind == ikWCString)
                    {
                        if (_item.Value != "")
                            _line += _item.Value;
                        else
                        {
                            if (_item.Flags & IF_INTVAL)
                            {
                                if (!_item.IntValue)
                                    _line += "''";
                                else
                                {
                                    _recN1 = GetInfoRec(_item.IntValue);
                                    if (_recN1)
                                        _line += _recN1->GetName();
                                    else
                                    {
                                        if (_kind == ikCString)
                                        {
                                            _line += "'" + String((char*)(Code + Adr2Pos(_item.IntValue))) + "'";
                                        }
                                        else if (_kind == ikLString)
                                            _line += TransformString(Code + Adr2Pos(_item.IntValue), -1);
                                        else
                                            _line += TransformUString(CP_ACP, (wchar_t*)(Code + Adr2Pos(_item.IntValue)), -1);
                                    }
                                }
                            }
                        }
                        continue;
                    }
                    if (_kind == ikVMT || _kind == ikClass)
                    {
                        if (_item.Value != "")
                            _line += _item.Value;
                        else
                        {
                            if ((_item.Flags & IF_INTVAL) && (!_item.IntValue))
                                _line += "Nil";
                            else
                                _line += "?";
                        }
                        continue;
                    }
                    if (_kind == ikEnumeration)
                    {
                        if (_rn != -1) _line += _regName + "{";
                        if (_item.Flags & IF_INTVAL)
                        {
                            _line += GetEnumerationString(_argInfo->TypeDef, _item.IntValue);
                        }
                        else if (_item.Value != "")
                        {
                            _line += _item.Value;
                        }
                        if (_rn != -1) _line += "}";
                        continue;
                    }
                    if (_kind == ikSet)
                    {
                        if (_item.Flags & IF_INTVAL)
                        {
                            if (IsValidImageAdr(_item.IntValue))
                                _line += GetSetString(_argInfo->TypeDef, Code + Adr2Pos(_item.IntValue));
                            else
                                _line += GetSetString(_argInfo->TypeDef, (BYTE*)&_item.IntValue);
                        }
                        else
                            _line += _item.Value;
                        continue;
                    }
                    if (_kind == ikRecord)
                    {
                        if (_size < 8)
                            _line += _item.Value;
                        else
                            _line += _item1.Value;
                        continue;
                    }
                    if (_kind == ikFloat)
                    {
                        if (_item.Flags & IF_INTVAL)
                        {
                            GetFloatItemFromStack(_esp, &_item, FloatNameToFloatType(_argInfo->TypeDef));
                            _line += _item.Value;
                        }
                        else
                            _line += _item.Value;
                        continue;
                    }
                    if (_kind == ikInt64)
                    {
                        if (_item.Flags & IF_INTVAL)
                        {
                            GetInt64ItemFromStack(_esp, &_item);
                            _line += _item.Value;
                        }
                        else
                            _line += _item.Value;
                        continue;
                    }
                    if (_kind == ikClassRef)
                    {
                        _line += _item.Type;
                        continue;
                    }
                    if (_kind == ikResString)
                    {
                        _line += _item.Value;
                        continue;
                    }
                    if (_kind == ikPointer)
                    {
                        if ((_item.Flags & IF_INTVAL) && IsValidImageAdr(_item.IntValue))
                        {
                            _recN1 = GetInfoRec(_item.IntValue);
                            if (_recN1 && _recN1->HasName())
                                _line += _recN1->GetName();
                            else
                                _line += "sub_" + Val2Str8(_item.IntValue);
                            continue;
                        }
                    }
                    //var
                    if (_argInfo->Tag == 0x22)
                    {
                        _adr = _item.IntValue;
                        if (IsValidImageAdr(_adr))
                        {
                            _recN1 = GetInfoRec(_adr);
                            if (_recN1 && _recN1->HasName())
                                _line += _recN1->GetName();
                            else
                                _line += MakeGvarName(_adr);
                        }
                        else
                            _line += _item.Value;
                        continue;
                    }
                    if (_argInfo->Size == 8)
                    {
                        if (SameText(_item1.Value, "Self"))
                        {
                            if ((_item.Flags & IF_INTVAL) && IsValidImageAdr(_item.IntValue))
                            {
                                _recN1 = GetInfoRec(_item.IntValue);
                                if (_recN1 && _recN1->HasName())
                                    _line += _recN1->GetName();
                                else
                                    _line += "sub_" + Val2Str8(_item.IntValue);
                                continue;
                            }
                        }
                        _line += _item.Value;
                        continue;
                    }
                    if (_item.Value != "")
                        _line += _item.Value;
                    else if (_item.Flags & IF_INTVAL)
                        _line += String(_item.IntValue);
                    else
                        _line += "?";
                    continue;
                }
            }
            _len = _line.Length();
            if (_line[_len] != '(')
                _line += ")";
            else
                _line.SetLength(_len - 1);
        }
        if (_methodKind != ikFunc && _methodKind != ikConstructor)
        {
            _line += ";";
            if (Env->LastResString != "")
            {
                _line += "//" + QuotedStr(Env->LastResString);
                Env->LastResString = "";
            }
            Env->AddToBody(_line);
        }
        else
        {
            if (_callKind == 0)//fastcall
            {
                if (_retKind == ikLString ||
                    _retKind == ikUString ||
                    _retKind == ikRecord  ||
                    _retKind == ikVariant ||
                    _retKind == ikArray)
                {
                    //eax
                    if (_ndx == 0)
                        GetRegItem(16, &_item);
                    //edx
                    else if (_ndx == 1)
                        GetRegItem(18, &_item);
                    //ecx
                    else if (_ndx == 2)
                        GetRegItem(17, &_item);
                    //last pushed
                    else
                    {
                        _esp -= 4;
                        _item = Env->Stack[_esp];
                    }

                    if (_item.Flags & IF_STACK_PTR)
                    {
                        if (_item.Name != "")
                            _line = _item.Name + " := " + _line;
                        else if (_item.Value != "")
                            _line = _item.Value + " := " + _line;
                        else
                            _line = Env->GetLvarName(_item.IntValue, _retType) + " := " + _line;
                        if (_retKind == ikRecord)
                        {
                            /*
                            _size = GetRecordSize(_retType);
                            for (int r = 0; r < _size; r++)
                            {
                                if (_item.IntValue + r >= Env->StackSize)
                                {
                                    Env->ErrAdr = curAdr;
                                    throw Exception("Possibly incorrect RecordSize (or incorrect type of record)");
                                }
                                _item1 = Env->Stack[_item.IntValue + r];
                                _item1.Flags = IF_FIELD;
                                _item1.Offset = r;
                                _item1.Type = "";
                                if (r == 0) _item1.Type = _retType;
                                Env->Stack[_item.IntValue + r] = _item1;
                            }
                            */
                        }
                        Env->Stack[_item.IntValue].Flags = 0;
                        Env->Stack[_item.IntValue].Type = _retType;
                    }
                    else
                        _line = _item.Value + " := " + _line;

                    _line += ";";
                    if (Env->LastResString != "")
                    {
                        _line += "//" + QuotedStr(Env->LastResString);
                        Env->LastResString = "";
                    }
                    Env->AddToBody(_line);
                }
                else if (_retKind == ikFloat)
                {
                    InitItem(&_item);
                    _item.Value = _line;
                    _item.Type = _retType;
                    FPush(&_item);
                }
                else
                {
                    //???__int64
                    InitItem(&_item);
                    _item.Precedence = PRECEDENCE_ATOM;
                    _item.Flags = IF_CALL_RESULT;
                    _item.Value = _line;
                    _item.Type = _retType;
                    SetRegItem(16, &_item);
                    _line += ";";
                    if (Env->LastResString != "")
                    {
                        _line += "//" + QuotedStr(Env->LastResString);
                        Env->LastResString = "";
                    }
                    Env->AddToBody("EAX := " + _line);
                }
            }
            else if (_callKind == 3 || _callKind == 1)//stdcall, cdecl
            {
                InitItem(&_item);
                _item.Precedence = PRECEDENCE_ATOM;
                _item.Flags = IF_CALL_RESULT;
                _item.Value = _line;
                _item.Type = _retType;
                SetRegItem(16, &_item);
                _line += ";";
                Env->AddToBody("EAX := " + _line);
            }
        }
        if (_callKind == 3 && _retBytesCalc != _retBytes)
        {
            Env->ErrAdr = curAdr;
            throw Exception("Incorrect number of return bytes!");
        }
        //_ESP_ += _retBytes;
        return false;
    }
    //call [reg+N]
    if (DisInfo.BaseReg != -1)
    {
        //esp
        if (DisInfo.BaseReg == 20)
        {
            _item = Env->Stack[_ESP_ + DisInfo.Offset];
            _line = Env->GetLvarName(_ESP_ + DisInfo.Offset, "") + "(...);";
            Env->AddToBody(_line);
            _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
            if (_value == "")
            {
                Env->ErrAdr = curAdr;
                throw Exception("Bye!");
            }
            sscanf(_value.c_str(), "%lX", &_retBytes);
            _ESP_ += _retBytes;
            return false;
        }

        GetRegItem(DisInfo.BaseReg, &_item);
        _classAdr = 0;
        //if (SameText(_item.Value, "Self"))
        //{
        //    Env->ErrAdr = curAdr;
        //    throw Exception("Under construction");
        //}
        if (_item.Type != "")
            _classAdr = GetClassAdr(_item.Type);
        if (_item.Flags & IF_VMT_ADR)
            _classAdr = _item.IntValue;
        if (IsValidImageAdr(_classAdr))
        {
            //Interface
            if (_item.Flags & IF_INTERFACE)
            {
                Env->AddToBody(_item.Value + ".I" + String(DisInfo.Offset) + "(...);");
                _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
                if (_value == "")
                {
                    Env->ErrAdr = curAdr;
                    throw Exception("Bye!");
                }
                sscanf(_value.c_str(), "%lX", &_retBytes);
                _ESP_ += _retBytes;
                return false;
            }
            //Method
            _recM = FMain_11011981->GetMethodInfo(_classAdr, 'V', DisInfo.Offset);
            if (_recM)
            {
                callAdr = *((DWORD*)(Code + Adr2Pos(_classAdr) - VmtSelfPtr + DisInfo.Offset));
                if (_recM->abstract)
                {
                    _classAdr = GetChildAdr(_classAdr);
                    callAdr = *((DWORD*)(Code + Adr2Pos(_classAdr) - VmtSelfPtr + DisInfo.Offset));
                }
                if (_recM->name != "")
                    return SimulateCall(curAdr, callAdr, instrLen, _recM, _classAdr);
                else
                    return SimulateCall(curAdr, callAdr, instrLen, 0, _classAdr);
            }
            //Field
            if (GetField(_item.Type, DisInfo.Offset, _name, _type) < 0)
            {
                while (!_recM)
                {
                    _typeName = ManualInput(CurProcAdr, curAdr, "Class (" + _item.Type + ") has no such virtual method. Give correct class name", "Name:");
                    if (_typeName == "")
                    {
                        Env->ErrAdr = curAdr;
                        throw Exception("Possibly incorrect class (has no such virtual method)");
                    }
                    _classAdr = GetClassAdr(_typeName);
                    _recM = FMain_11011981->GetMethodInfo(_classAdr, 'V', DisInfo.Offset);
                }
                callAdr = *((DWORD*)(Code + Adr2Pos(_classAdr) - VmtSelfPtr + DisInfo.Offset));
                if (_recM->abstract)
                {
                    _classAdr = GetChildAdr(_classAdr);
                    callAdr = *((DWORD*)(Code + Adr2Pos(_classAdr) - VmtSelfPtr + DisInfo.Offset));
                }
                if (_recM->name != "")
                    return SimulateCall(curAdr, callAdr, instrLen, _recM, _classAdr);
                else
                    return SimulateCall(curAdr, callAdr, instrLen, 0, _classAdr);
            }
            else
            {
                if (_name != "")
                    Env->AddToBody(_name + "(...);");
                else
                    Env->AddToBody("f" + Val2Str0(DisInfo.Offset) + "(...);");
                _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
                if (_value == "")
                {
                    Env->ErrAdr = curAdr;
                    throw Exception("Bye!");
                }
                sscanf(_value.c_str(), "%lX", &_retBytes);
                _ESP_ += _retBytes;
                return false;
            }
        }
        if (_item.Flags & IF_STACK_PTR)
        {
            _item = Env->Stack[_item.IntValue + DisInfo.Offset];
            _line = _item.Value + ";";
            Env->AddToBody(_line);
            _value = ManualInput(CurProcAdr, curAdr, "Input return bytes number (in hex) of procedure at " + Val2Str8(curAdr), "Bytes:");
            if (_value == "")
            {
                Env->ErrAdr = curAdr;
                throw Exception("Bye!");
            }
            sscanf(_value.c_str(), "%lX", &_retBytes);
            _ESP_ += _retBytes;
            return false;
        }
    }
    //call reg
    if (DisInfo.OpNum == 1 && DisInfo.OpType[0] == otREG)
    {
        //GetRegItem(DisInfo.OpRegIdx[0], &_item);
        //_line = _item.Value + ";";
        Env->AddToBody("call...;");
        _value = ManualInput(CurProcAdr, curAdr, "Input stack arguments number of procedure at " + Val2Str8(curAdr), "ArgsNum:");
        if (_value == "")
        {
            Env->ErrAdr = curAdr;
            throw Exception("Bye!");
        }
        sscanf(_value.c_str(), "%d", &_argsNum);
        _ESP_ += _argsNum * 4;
        return false;
    }
    //call [reg+N]
    if (DisInfo.OpNum == 1 && DisInfo.OpType[0] == otMEM)
    {
        Env->AddToBody("call(");
        _value = ManualInput(CurProcAdr, curAdr, "Input stack arguments number of procedure at " + Val2Str8(curAdr), "ArgsNum:");
        if (_value == "")
        {
            Env->ErrAdr = curAdr;
            throw Exception("Bye!");
        }
        sscanf(_value.c_str(), "%d", &_argsNum);

        while (_argsNum)
        {
            _item = Env->Stack[_ESP_];
            if (_item.Flags & IF_INTVAL)
                Env->AddToBody(String(_item.IntValue));
            else
                Env->AddToBody(_item.Value);
            _ESP_ += 4;
            _argsNum--;
        }
        Env->AddToBody(");");
        return false;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
int __fastcall TDecompiler::GetCmpInfo(DWORD fromAdr)
{
    BYTE        _b, _op;
    int         _curPos;
    DWORD       _curAdr = fromAdr;
    DISINFO     _disInfo;

    _curPos = Adr2Pos(_curAdr);
    CmpAdr = 0;

    Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
    if (_disInfo.Conditional && IsValidCodeAdr(_disInfo.Immediate))
    {
        _b = *(Code + _curPos);
        if (_b == 0xF) _b = *(Code + _curPos + 1);
        _b = (_b & 0xF) + 'A';
        if (_b == 'A' || _b == 'B') return CMP_FAILED;
        CmpAdr = _disInfo.Immediate;
        CmpOp = _b;
        return CMP_BRANCH;
    }
    _op = Disasm.GetOp(_disInfo.Mnem);
    if (_op == OP_SET)
    {
        CmpAdr = 0;
        _b = *(Code + _curPos + 1);
        CmpOp = (_b & 0xF) + 'A';
        return CMP_SET;
    }
    return CMP_FAILED;
}
//---------------------------------------------------------------------------
//"for" cycle types
//1 - for I := C1 to C2
//2 - for I := 0 to N
//3 - for I := 1 to N
//4 - for I := C to N
//5 - for I := N to C
//6 - for I ;= N1 to N2
//7 - for I := C1 downto C2
//8 - for I := 0 downto N
//9 - for I := C downto N
//10 - for I := N downto C
//11 - for I := N1 downto N2
PLoopInfo __fastcall TDecompiler::GetLoopInfo(int fromAdr)
{
    bool        noVar = true;
    bool        down = false;
    bool        bWhile = false;
    BYTE        _op;
    int         instrLen, pos, pos1, fromPos, num, intTo, idx, idxVal;
    DWORD       maxAdr, brkAdr, lastAdr, stopAdr, _dd, _dd1;
    PInfoRec    recN;
    PLoopInfo   res;
    DISINFO     _disInfo;
    String      from, to, cnt;
    ITEM        item;
    IDXINFO     varIdxInfo, cntIdxInfo;

    fromPos = Adr2Pos(fromAdr);
    pos1 = GetNearestUpInstruction(fromPos);
    Disasm.Disassemble(Code + pos1, (__int64)Pos2Adr(pos1), &_disInfo, 0);
    _dd = *((DWORD*)_disInfo.Mnem);
    if (_dd == 'pmj') bWhile = true;

    recN = GetInfoRec(fromAdr);
    if (recN && recN->xrefs)
    {
        maxAdr = 0;
        for (int n = 0; n < recN->xrefs->Count; n++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[n];
            if (recX->adr + recX->offset > maxAdr) maxAdr = recX->adr + recX->offset;
        }
        //Instruction at maxAdr
        instrLen = Disasm.Disassemble(Code + Adr2Pos(maxAdr), (__int64)maxAdr, &_disInfo, 0);
        brkAdr = maxAdr + instrLen;
        lastAdr = maxAdr;
        if (bWhile)
        {
            res = new TLoopInfo('W', fromAdr, brkAdr, lastAdr);//while
            return res;
        }
        _dd = *((DWORD*)_disInfo.Mnem);
        if (_dd == 'pmj')
        {
            res = new TLoopInfo('T', fromAdr, brkAdr, lastAdr);//while true
            res->whileInfo = new TWhileInfo(true);
            return res;
        }
        //First instruction before maxAdr
        pos1 = GetNearestUpInstruction(Adr2Pos(maxAdr), fromPos);
        Disasm.Disassemble(Code + pos1, (__int64)Pos2Adr(pos1), &_disInfo, 0);
        _dd1 = *((DWORD*)_disInfo.Mnem);
        //cmp reg/mem, imm
        if (_dd1 == 'pmc' && _disInfo.OpType[1] == otIMM)
        {
            noVar = false;
            GetCycleIdx(&varIdxInfo, &_disInfo);
            intTo = _disInfo.Immediate;
            //Find mov reg/mem,...
            pos = fromPos;
            while (1)
            {
                pos = GetNearestUpInstruction(pos);
                instrLen = Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                if (_disInfo.Branch || IsFlagSet(cfProcStart, pos))
                {
                    res = new TLoopInfo('R', fromAdr, brkAdr, lastAdr);//repeat
                    return res;
                }
                _op = Disasm.GetOp(_disInfo.Mnem);
                if (_op == OP_MOV || _op == OP_XOR)
                {
                    if (varIdxInfo.IdxType == itREG && _disInfo.OpType[0] == otREG && IsSameRegister(_disInfo.OpRegIdx[0], varIdxInfo.IdxValue))
                    {
                        GetRegItem(varIdxInfo.IdxValue, &item);
                        if (item.Flags & IF_INTVAL)
                        {
                            from = String(item.IntValue);
                            item.Flags &= ~IF_INTVAL;
                        }
                        else
                            from = item.Value;
                        item.Value = GetDecompilerRegisterName(varIdxInfo.IdxValue);
                        SetRegItem(varIdxInfo.IdxValue, &item);
                        break;
                    }
                    if (varIdxInfo.IdxType == itLVAR && _disInfo.OpType[0] == otMEM)
                    {
                        if (_disInfo.BaseReg == 21)//[ebp-N]
                        {
                            GetRegItem(_disInfo.BaseReg, &item);
                            if (item.IntValue + _disInfo.Offset == varIdxInfo.IdxValue)
                            {
                                if (Env->Stack[varIdxInfo.IdxValue].Flags & IF_INTVAL)
                                    from = String(Env->Stack[varIdxInfo.IdxValue].IntValue);
                                else
                                    from = Env->Stack[varIdxInfo.IdxValue].Value;
                                Env->Stack[varIdxInfo.IdxValue].Value = Env->GetLvarName(varIdxInfo.IdxValue, "Integer");
                                break;
                            }
                        }
                        if (_disInfo.BaseReg == 20)//[esp+N]
                        {
                            if (_ESP_ + _disInfo.Offset == varIdxInfo.IdxValue)
                            {
                                if (Env->Stack[varIdxInfo.IdxValue].Flags & IF_INTVAL)
                                    from = String(Env->Stack[varIdxInfo.IdxValue].IntValue);
                                else
                                    from = Env->Stack[varIdxInfo.IdxValue].Value;
                                Env->Stack[varIdxInfo.IdxValue].Value = Env->GetLvarName(varIdxInfo.IdxValue, "Integer");
                                break;
                            }
                        }
                    }
                }
            }
            //Find array elements
            pos += instrLen;
            while (1)
            {
                if (pos == fromPos) break;

                instrLen = Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                _op = Disasm.GetOp(_disInfo.Mnem);
                //lea reg, mem
                if (_op == OP_LEA && _disInfo.OpType[1] == otMEM)
                {
                    idx = _disInfo.OpRegIdx[0];
                    GetRegItem(idx, &item);
                    item.Flags |= IF_ARRAY_PTR;
                    SetRegItem(idx, &item);
                    pos += instrLen;
                    continue;
                }
                //mov reg, esp
                if (_op == OP_MOV && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otREG && _disInfo.OpRegIdx[1] == 20)
                {
                    idx = _disInfo.OpRegIdx[0];
                    GetRegItem(idx, &item);
                    item.Flags |= IF_ARRAY_PTR;
                    SetRegItem(idx, &item);
                    pos += instrLen;
                    continue;
                }
                //mov
                if (_op == OP_MOV)
                {
                    if (_disInfo.OpType[1] == otIMM && IsValidImageAdr(_disInfo.Immediate))
                    {
                        if (_disInfo.OpType[0] == otREG)
                        {
                            GetRegItem(_disInfo.OpRegIdx[0], &item);
                            item.Flags |= IF_ARRAY_PTR;
                            SetRegItem(_disInfo.OpRegIdx[0], &item);
                        }
                        if (_disInfo.OpType[0] == otMEM)
                        {
                            if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                            {
                                if (_disInfo.BaseReg == 21)//[ebp-N]
                                {
                                    GetRegItem(_disInfo.BaseReg, &item);
                                    idxVal = item.IntValue + _disInfo.Offset;
                                }
                                if (_disInfo.BaseReg == 20)//[esp-N]
                                {
                                    idxVal = _ESP_ + _disInfo.Offset;
                                }
                                Env->Stack[idxVal].Flags |= IF_ARRAY_PTR;
                            }
                        }
                    }
                    if (_disInfo.OpType[1] == otREG)
                    {
                        GetRegItem(_disInfo.OpRegIdx[1], &item);
                        if (item.Flags & IF_ARRAY_PTR)
                        {
                            if (_disInfo.OpType[0] == otREG)
                            {
                                GetRegItem(_disInfo.OpRegIdx[0], &item);
                                item.Flags |= IF_ARRAY_PTR;
                                SetRegItem(_disInfo.OpRegIdx[0], &item);
                            }
                            if (_disInfo.OpType[0] == otMEM)
                            {
                                if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                                {
                                    if (_disInfo.BaseReg == 21)//[ebp-N]
                                    {
                                        GetRegItem(_disInfo.BaseReg, &item);
                                        idxVal = item.IntValue + _disInfo.Offset;
                                    }
                                    if (_disInfo.BaseReg == 20)//[esp-N]
                                    {
                                        idxVal = _ESP_ + _disInfo.Offset;
                                    }
                                    Env->Stack[idxVal].Flags |= IF_ARRAY_PTR;
                                }
                            }
                        }
                    }
                    if (_disInfo.OpType[1] == otMEM)
                    {
                        if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                        {
                            if (_disInfo.BaseReg == 21)//[ebp-N]
                            {
                                GetRegItem(_disInfo.BaseReg, &item);
                                idxVal = item.IntValue + _disInfo.Offset;
                            }
                            if (_disInfo.BaseReg == 20)//[esp-N]
                            {
                                idxVal = _ESP_ + _disInfo.Offset;
                            }
                            if (Env->Stack[idxVal].Flags & IF_ARRAY_PTR)
                            {
                                GetRegItem(_disInfo.OpRegIdx[0], &item);
                                item.Flags |= IF_ARRAY_PTR;
                                SetRegItem(_disInfo.OpRegIdx[0], &item);
                            }
                        }
                    }
                }
                pos += instrLen;
            }
            //4
            pos = Adr2Pos(maxAdr); stopAdr = brkAdr;
            pos = GetNearestUpInstruction(pos);
            while (1)
            {
                pos = GetNearestUpInstruction(pos);
                Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'cni' || _dd == 'ced' || _dd == 'dda' || _dd == 'bus')
                {
                    if (_disInfo.OpType[0] == otREG)
                    {
                        GetRegItem(_disInfo.OpRegIdx[0], &item);
                        if (item.Flags & IF_ARRAY_PTR)
                        {
                            stopAdr = Pos2Adr(pos);
                            continue;
                        }
                        if (varIdxInfo.IdxType == itREG && IsSameRegister(_disInfo.OpRegIdx[0], varIdxInfo.IdxValue))
                        {
                            if (_dd == 'ced') down = true;
                            stopAdr = Pos2Adr(pos);
                        }
                        break;
                    }
                    if (_disInfo.OpType[0] == otMEM)
                    {
                        if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                        {
                            if (_disInfo.BaseReg == 21)//[ebp-N]
                            {
                                GetRegItem(_disInfo.BaseReg, &item);
                                idxVal = item.IntValue + _disInfo.Offset;
                            }
                            if (_disInfo.BaseReg == 20)//[esp-N]
                            {
                                idxVal = _ESP_ + _disInfo.Offset;
                            }
                            if (Env->Stack[idxVal].Flags & IF_ARRAY_PTR)
                            {
                                stopAdr = Pos2Adr(pos);
                                continue;
                            }
                            if (varIdxInfo.IdxType == itLVAR && varIdxInfo.IdxValue == idxVal)
                            {
                                if (_dd == 'ced') down = true;
                                stopAdr = Pos2Adr(pos);
                            }
                        }
                        break;
                    }
                }
                break;
            }
            res = new TLoopInfo('F', fromAdr, brkAdr, lastAdr);//for
            if (!down)
                to = String(intTo - 1);
            else
                to = String(intTo + 1);
            res->forInfo = new TForInfo(noVar, down, stopAdr, from, to, varIdxInfo.IdxType, varIdxInfo.IdxValue, -1, -1);
            return res;
        }
        if (_dd1 == 'cni' || _dd1 == 'ced')
        {
            from = "1";
            //1
            GetCycleIdx(&cntIdxInfo, &_disInfo);
            if (_disInfo.OpType[0] == otREG)
            {
                GetRegItem(_disInfo.OpRegIdx[0], &item);
                if (item.Flags & IF_INTVAL)
                    cnt = String(item.IntValue);
                else if (item.Value1 != "")
                    cnt = item.Value1;
                else
                    cnt = item.Value;
            }
            if (_disInfo.OpType[0] == otMEM)
            {
                GetRegItem(_disInfo.BaseReg, &item);
                if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                {
                    if (_disInfo.BaseReg == 21)//[ebp-N]
                    {
                        item = Env->Stack[item.IntValue + _disInfo.Offset];
                    }
                    if (_disInfo.BaseReg == 20)//[esp-N]
                    {
                        item = Env->Stack[_ESP_ + _disInfo.Offset];
                    }
                    if (item.Flags & IF_INTVAL)
                        cnt = String(item.IntValue);
                    else
                        cnt = item.Value;
                }
            }
            //2
            pos = fromPos;
            while (1)
            {
                pos = GetNearestUpInstruction(pos);
                instrLen = Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                if (SameText(cntIdxInfo.IdxStr, String(_disInfo.Op1)))
                    break;
            }
            //3
            pos += instrLen;
            while (1)
            {
                if (pos == fromPos) break;

                instrLen = Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                _op = Disasm.GetOp(_disInfo.Mnem);
                //lea reg1, mem
                if (_op == OP_LEA && _disInfo.OpType[1] == otMEM)
                {
                    idx = _disInfo.OpRegIdx[0];
                    GetRegItem(idx, &item);
                    item.Flags |= IF_ARRAY_PTR;
                    SetRegItem(idx, &item);
                    pos += instrLen;
                    continue;
                }
                //mov reg, esp
                if (_op == OP_MOV && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otREG && _disInfo.OpRegIdx[1] == 20)
                {
                    idx = _disInfo.OpRegIdx[0];
                    GetRegItem(idx, &item);
                    item.Flags |= IF_ARRAY_PTR;
                    SetRegItem(idx, &item);
                    pos += instrLen;
                    continue;
                }
                if (_op == OP_MOV)
                {
                    if (_disInfo.OpType[1] == otIMM)
                    {
                        if (!IsValidImageAdr(_disInfo.Immediate))
                        {
                            GetCycleIdx(&varIdxInfo, &_disInfo);
                            noVar = false;
                        }
                        else
                        {
                            if (_disInfo.OpType[0] == otREG)
                            {
                                GetRegItem(_disInfo.OpRegIdx[0], &item);
                                item.Flags |= IF_ARRAY_PTR;
                                SetRegItem(_disInfo.OpRegIdx[0], &item);
                            }
                            else//otMEM
                            {
                                if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                                {
                                    if (_disInfo.BaseReg == 21)//[ebp-N]
                                    {
                                        GetRegItem(_disInfo.BaseReg, &item);
                                        idxVal = item.IntValue + _disInfo.Offset;
                                    }
                                    if (_disInfo.BaseReg == 20)//[esp-N]
                                    {
                                        idxVal = _ESP_ + _disInfo.Offset;
                                    }
                                    Env->Stack[idxVal].Flags |= IF_ARRAY_PTR;
                                }
                            }
                        }
                    }
                    if (_disInfo.OpType[1] == otREG)
                    {
                        GetRegItem(_disInfo.OpRegIdx[1], &item);
                        int _size;
                        int _kind = GetTypeKind(item.Type, &_size);
                        if ((item.Flags & IF_ARRAY_PTR) || _kind == ikArray || _kind == ikDynArray)
                        {
                            if (_disInfo.OpType[0] == otREG)
                            {
                                GetRegItem(_disInfo.OpRegIdx[0], &item);
                                item.Flags |= IF_ARRAY_PTR;
                                SetRegItem(_disInfo.OpRegIdx[0], &item);
                            }
                            else//otMEM
                            {
                                if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                                {
                                    if (_disInfo.BaseReg == 21)//[ebp-N]
                                    {
                                        GetRegItem(_disInfo.BaseReg, &item);
                                        idxVal = item.IntValue + _disInfo.Offset;
                                    }
                                    if (_disInfo.BaseReg == 20)//[esp-N]
                                    {
                                        idxVal = _ESP_ + _disInfo.Offset;
                                    }
                                    Env->Stack[idxVal].Flags |= IF_ARRAY_PTR;
                                }
                            }
                        }
                        else
                        {
                            GetCycleIdx(&varIdxInfo, &_disInfo);
                            noVar = false;
                        }
                    }
                    if (_disInfo.OpType[1] == otMEM)
                    {
                    //??????!!!!!!
                        if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                        {
                            if (_disInfo.BaseReg == 21)//[ebp-N]
                            {
                                GetRegItem(_disInfo.BaseReg, &item);
                                idxVal = item.IntValue + _disInfo.Offset;
                            }
                            if (_disInfo.BaseReg == 20)//[esp-N]
                            {
                                idxVal = _ESP_ + _disInfo.Offset;
                            }
                            item = Env->Stack[idxVal];
                            if (item.Flags & IF_VAR)
                            {
                                GetRegItem(_disInfo.OpRegIdx[0], &item);
                                item.Flags |= IF_ARRAY_PTR;
                                SetRegItem(_disInfo.OpRegIdx[0], &item);
                                pos += instrLen;
                                continue;
                            }
                            if (!(item.Flags & IF_ARRAY_PTR))
                            {
                                GetCycleIdx(&varIdxInfo, &_disInfo);
                                noVar = false;
                            }
                        }
                    }
                }
                pos += instrLen;
            }
            //4
            pos = GetNearestUpInstruction(Adr2Pos(maxAdr)); stopAdr = Pos2Adr(pos);
            while (1)
            {
                pos = GetNearestUpInstruction(pos);
                Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'cni' || _dd == 'ced' || _dd == 'dda' || _dd == 'bus')
                {
                    if (_disInfo.OpType[0] == otREG)
                    {
                        GetRegItem(_disInfo.OpRegIdx[0], &item);
                        if (item.Flags & IF_ARRAY_PTR)
                        {
                            stopAdr = Pos2Adr(pos);
                            continue;
                        }
                        if (noVar)
                        {
                            GetCycleIdx(&varIdxInfo, &_disInfo);
                            if (item.Flags & IF_INTVAL)
                            {
                                from = String(item.IntValue);
                                item.Flags &= ~IF_INTVAL;
                            }
                            else
                                from = item.Value;
                            stopAdr = Pos2Adr(pos);
                        }
                        else
                        {
                            if (varIdxInfo.IdxType == itREG && IsSameRegister(_disInfo.OpRegIdx[0], varIdxInfo.IdxValue))
                            {
                                if (_dd == 'ced') down = true;
                                stopAdr = Pos2Adr(pos);
                            }
                        }
                        break;
                    }
                    if (_disInfo.OpType[0] == otMEM)
                    {
                        if (_disInfo.BaseReg == 21 || _disInfo.BaseReg == 20)
                        {
                            if (_disInfo.BaseReg == 21)//[ebp-N]
                            {
                                GetRegItem(_disInfo.BaseReg, &item);
                                idxVal = item.IntValue + _disInfo.Offset;
                            }
                            if (_disInfo.BaseReg == 20)//[esp-N]
                            {
                                idxVal = _ESP_ + _disInfo.Offset;
                            }
                            item = Env->Stack[idxVal];
                            if (item.Flags & IF_ARRAY_PTR)
                            {
                                stopAdr = Pos2Adr(pos);
                                continue;
                            }
                            if (!noVar && varIdxInfo.IdxType == itLVAR && varIdxInfo.IdxValue == idxVal)
                            {
                                if (_dd == 'ced') down = true;
                                stopAdr = Pos2Adr(pos);
                            }
                            break;
                        }
                    }
                }
                break;
            }
            //from, to
            if (noVar)
            {
                //from = "1";
                if (cntIdxInfo.IdxType == itREG)
                {
                    if (SameText(from, "1"))
                        to = cnt;
                    else
                        to = cnt + " + " + from + " - 1";
                    GetRegItem(cntIdxInfo.IdxValue, &item);
                    if (item.Flags & IF_INTVAL)
                    {
                        //to = String(item.IntValue);
                        item.Flags &= ~IF_INTVAL;
                    }
                    //else
                    //    to = item.Value;
                    item.Value = GetDecompilerRegisterName(cntIdxInfo.IdxValue);
                    SetRegItem(cntIdxInfo.IdxValue, &item);
                }
                else if (cntIdxInfo.IdxType == itLVAR)
                {
                    item = Env->Stack[cntIdxInfo.IdxValue];
                    if (item.Flags & IF_INTVAL)
                    {
                        //to = String(item.IntValue);
                        item.Flags &= ~IF_INTVAL;
                    }
                    //else
                    //    to = item.Value;
                    item.Value = Env->GetLvarName(cntIdxInfo.IdxValue, "Integer");
                    Env->Stack[cntIdxInfo.IdxValue] = item;
                }
            }
            else
            {
                if (varIdxInfo.IdxType == itREG)
                {
                    GetRegItem(varIdxInfo.IdxValue, &item);
                    if (item.Flags & IF_INTVAL)
                    {
                        if (from == "") from = String(item.IntValue);
                        item.Flags &= ~IF_INTVAL;
                    }
                    else
                        if (from == "") from = item.Value;
                    item.Value = GetDecompilerRegisterName(varIdxInfo.IdxValue);
                    item.Flags |= IF_CYCLE_VAR;
                    SetRegItem(varIdxInfo.IdxValue, &item);
                }
                else if (varIdxInfo.IdxType == itLVAR)
                {
                    item = Env->Stack[varIdxInfo.IdxValue];
                    from = item.Value1;
                    item.Value = Env->GetLvarName(varIdxInfo.IdxValue, "Integer");
                    item.Flags |= IF_CYCLE_VAR;
                    Env->Stack[varIdxInfo.IdxValue] = item;
                }
                if (cntIdxInfo.IdxType == itREG)
                {
                    GetRegItem(cntIdxInfo.IdxValue, &item);
                    if (item.Flags & IF_INTVAL)
                    {
                        if (cnt == "") cnt = String(item.IntValue);
                        item.Flags &= ~IF_INTVAL;
                    }
                    else
                        if (cnt == "") cnt = item.Value;
                    item.Value = GetDecompilerRegisterName(cntIdxInfo.IdxValue);
                    SetRegItem(cntIdxInfo.IdxValue, &item);
                }
                else if (cntIdxInfo.IdxType == itLVAR)
                {
                    cnt = Env->Stack[cntIdxInfo.IdxValue].Value;
                    Env->Stack[cntIdxInfo.IdxValue].Value = Env->GetLvarName(cntIdxInfo.IdxValue, "Integer");
                }
                if (SameText(from, "1"))
                    to = cnt;
                else if (SameText(from, "0"))
                    to = cnt + " - 1";
                else
                {
                    to = cnt + " + " + from + " - 1";
                }
            }
            res = new TLoopInfo('F', fromAdr, brkAdr, lastAdr);//for
            res->forInfo = new TForInfo(noVar, false, stopAdr, from, to, varIdxInfo.IdxType, varIdxInfo.IdxValue, cntIdxInfo.IdxType, cntIdxInfo.IdxValue);
            return res;
        }
    }
    res = new TLoopInfo('R', fromAdr, brkAdr, lastAdr);//repeat
    return res;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulatePush(DWORD curAdr, bool bShowComment)
{
    bool        _vmt;
    int         _offset, _idx;
    BYTE        *_pdi, _b;
    ITEM        _item, _item1;
    PInfoRec    _recN;
    String      _name, _type, _typeName, _value, _regname;

    //push imm
    if (DisInfo.OpType[0] == otIMM)
    {
        if (IsValidImageAdr(DisInfo.Immediate))
        {
            _recN = GetInfoRec(DisInfo.Immediate);
            if (_recN && (_recN->kind == ikLString || _recN->kind == ikWString || _recN->kind == ikUString))
            {
                InitItem(&_item);
                _item.Value = _recN->GetName();
                _item.Type = "String";
                Push(&_item);
                return;
            }
        }

        InitItem(&_item);
        _item.Flags = IF_INTVAL;
        /*
        _pdi = (BYTE*)&DisInfo.Immediate; _b = *_pdi;
        if (DisInfo.ImmSize == 1)//byte
        {
            if (_b & 0x80)
                _imm = 0xFFFFFF00 | _b;
            else
                _imm = _b;
        }
        else if (DisInfo.ImmSize == 2)//word
        {
            if (_b & 0x80)
                _imm = 0xFFFF0000 | _b;
            else
                _imm = _b;
        }
        else
        {
            _imm = DisInfo.Immediate;
        }
        */
        _item.IntValue = DisInfo.Immediate;
        Push(&_item);
        return;
    }
    //push reg
    if (DisInfo.OpType[0] == otREG)
    {
        _idx = DisInfo.OpRegIdx[0];
        //push esp
        if (_idx == 20)
        {
            InitItem(&_item);
            _item.Flags = IF_STACK_PTR;
            _item.IntValue = _ESP_;
            Push(&_item);
            return;
        }
        GetRegItem(_idx, &_item);

        _regname = GetDecompilerRegisterName(_idx);
        if (_item.Value != "" && !SameText(_regname, _item.Value)) _value = _item.Value + "{" + _regname + "}";
        _item.Value = _value;

        //push eax - clear flag IF_CALL_RESULT
        if (_item.Flags & IF_CALL_RESULT)
        {
            _item.Flags &= ~IF_CALL_RESULT;
            SetRegItem(_idx, &_item);
        }
        //if (_item.Flags & IF_ARG)
        //{
        //    _item.Flags &= ~IF_ARG;
        //    _item.Name = "";
        //}

        Push(&_item);
        if (bShowComment) Env->AddToBody("//push " + _regname);
        return;
    }
    //push mem
    if (DisInfo.OpType[0] == otMEM)
    {
        GetMemItem(curAdr, &_item, OP_PUSH);
        Push(&_item);
        return;
        _offset = DisInfo.Offset;
        //push [BaseReg + IndxReg*Scale + Offset]
        if (DisInfo.BaseReg != -1)
        {
            if (DisInfo.BaseReg == 20)
            {
                _item = Env->Stack[_ESP_ + _offset];
                Push(&_item);
                return;
            }

            GetRegItem(DisInfo.BaseReg, &_item1);
            //cop reg, [BaseReg + Offset]
            if (DisInfo.IndxReg == -1)
            {
                //push [ebp-N]
                if (_item1.Flags & IF_STACK_PTR)
                {
                    _name = Env->GetLvarName(_item1.IntValue + _offset, "");
                    _item = Env->Stack[_item1.IntValue + _offset];
                    _item.Value = _name;
                    Push(&_item);
                    return;
                }
                //push [reg]
                if (!_offset)
                {
                    //var
                    if (_item1.Flags & IF_VAR)
                    {
                        InitItem(&_item);
                        _item.Value = _item1.Value;
                        _item.Type = _item1.Type;
                        Push(&_item);
                        return;
                    }
                    if (IsValidImageAdr(_item1.IntValue))
                    {
                        _recN = GetInfoRec(_item1.IntValue);
                        if (_recN)
                        {
                            InitItem(&_item);
                            _item.Value = _recN->GetName();
                            _item.Type = _recN->type;
                            Push(&_item);
                            return;
                        }
                    }
                }
                _typeName = TrimTypeName(GetRegType(DisInfo.BaseReg));
                if (_typeName != "")
                {
                    if (_typeName[1] == '^')   //Pointer to gvar (from other unit)
                    {
                        InitItem(&_item);
                        _item.Value = _item1.Value;
                        _item.Type = GetTypeDeref(_typeName);
                        Push(&_item);
                        return;
                    }
                    //push [reg+N]
                    if (GetField(_typeName, _offset, _name, _type) >= 0)
                    {
                        InitItem(&_item);

                        if (SameText(_item1.Value, "Self"))
                            _item.Value = _name;
                        else
                            _item.Value = _item1.Value + "." + _name;

                        _item.Type = _type;
                        Push(&_item);
                        return;
                    }
                }
            }
        }
        //[Offset]
        if (IsValidImageAdr(_offset))
        {
            _recN = GetInfoRec(_offset);
            if (_recN)
            {
                InitItem(&_item);
                _item.Value = _recN->GetName();
                _item.Type = _recN->type;
                Push(&_item);
                return;
            }
            InitItem(&_item);
            Push(&_item);
            return;
        }
        else
        {
            Env->ErrAdr = curAdr;
            throw Exception("Address is outside program image");
        }
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulatePop(DWORD curAdr)
{
    String  _regname;
    PITEM   _item;

    //pop reg
    if (DisInfo.OpType[0] == otREG)
    {
        _item = Pop();
        if (!IsFlagSet(cfFrame, Adr2Pos(curAdr)))
        {
            _regname = GetDecompilerRegisterName(DisInfo.OpRegIdx[0]);
            //_line = _regname + " := ";
            //if (_item->Flags & IF_ARG)
            //    _line += _item->Name;
            //else
            //    _line += _item->Value;
            Env->AddToBody("//pop " + _regname);
        }
        _item->Precedence = PRECEDENCE_NONE;
        _item->Value = _regname;
        SetRegItem(DisInfo.OpRegIdx[0], _item);
        return;
    }
    //pop mem
    if (DisInfo.OpType[0] == otMEM)
    {
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
//Simulate instruction with 1 operand
void __fastcall TDecompiler::SimulateInstr1(DWORD curAdr, BYTE Op)
{
    int         _regIdx, _offset;
    ITEM        _item, _item1, _item2, _itemBase, _itemSrc;
    String      _name, _value, _line, _comment;

    //op reg
    if (DisInfo.OpType[0] == otREG)
    {
        _regIdx = DisInfo.OpRegIdx[0];
        if (Op == OP_INC)
        {
            GetRegItem(_regIdx, &_item);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Value = _item.Value + " + 1";
            SetRegItem(_regIdx, &_item);
            _line = GetDecompilerRegisterName(_regIdx) + " := " + GetDecompilerRegisterName(_regIdx) + " + 1;";
            if (_item.Value != "") _line += "//" + _item.Value;
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_DEC)
        {
            GetRegItem(_regIdx, &_item);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Value = _item.Value + " - 1";
            SetRegItem(_regIdx, &_item);
            _line = GetDecompilerRegisterName(_regIdx) + " := " + GetDecompilerRegisterName(_regIdx) + " - 1;";
            if (_item.Value != "") _line += "//" + _item.Value;
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_IMUL)
        {
            GetRegItem(_regIdx, &_item1);
            GetRegItem(16, &_item2);
            InitItem(&_item);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Value = _item1.Value + " * " + _item2.Value;
            _item.Type = "Int64";
            SetRegItem(16, &_item);
            SetRegItem(18, &_item);
            _line = "EDX_EAX := " + GetDecompilerRegisterName(_regIdx) + " * " + "EAX;";
            if (_item.Value != "") _line += "//" + _item.Value;
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_NEG)
        {
            GetRegItem(_regIdx, &_item);
            _item.Precedence = PRECEDENCE_ATOM;
            _item.Value = "-" + _item.Value;
            SetRegItem(_regIdx, &_item);
            _line = GetDecompilerRegisterName(_regIdx) + " := -" + GetDecompilerRegisterName(_regIdx) + ";";
            if (_item.Value != "") _line += "//" + _item.Value;
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_NOT)
        {
            GetRegItem(_regIdx, &_item);
            _item.Precedence = PRECEDENCE_ATOM;
            _item.Value = "not " + _item.Value;
            SetRegItem(_regIdx, &_item);
            _line = GetDecompilerRegisterName(_regIdx) + " := not " + GetDecompilerRegisterName(_regIdx) + ";";
            if (_item.Value != "") _line += "//" + _item.Value;
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_SET)
        {
            InitItem(&_item);
            _item.Value = CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R;
            _item.Type = "Boolean";
            SetRegItem(_regIdx, &_item);
            _line = GetDecompilerRegisterName(_regIdx) + " := " + "(" + _item.Value + ");";
            Env->AddToBody(_line);
            return;
        }
        Env->ErrAdr = curAdr;
        throw Exception("Under construction");
    }
    //op mem
    if (DisInfo.OpType[0] == otMEM)
    {
        GetMemItem(curAdr, &_itemSrc, Op);
        if (_itemSrc.Name != "")
            _value = _itemSrc.Name;
        else
            _value = _itemSrc.Value;
        if (Op == OP_IMUL)
        {
            GetRegItem(16, &_item1);
            InitItem(&_item);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Value = _value + " * " + _item1.Value;;
            _item.Type = "Int64";
            SetRegItem(16, &_item);
            SetRegItem(18, &_item);
            _line = "EDX_EAX := " + _item.Value + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_SET)
        {
            _line = _value + " := (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ");";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_NEG)
        {
            _line = _value + " := -" + _value + ";";
            Env->AddToBody(_line);
            return;
        }
        Env->ErrAdr = curAdr;
        throw Exception("Under construction");
        
        _offset = DisInfo.Offset;
        if (DisInfo.BaseReg != -1)
        {
            if (DisInfo.IndxReg == -1)
            {
                GetRegItem(DisInfo.BaseReg, &_itemBase);
                //op [esp+N]
                if (DisInfo.BaseReg == 20)
                {
                    if (Op == OP_IMUL)
                    {
                        _item1 = Env->Stack[_ESP_ + _offset];
                        GetRegItem(16, &_item2);
                        InitItem(&_item);
                        _item.Precedence = PRECEDENCE_MULT;
                        _item.Value = _item1.Value + " * " + _item2.Value;
                        _item.Type = "Integer";
                        SetRegItem(16, &_item);
                        SetRegItem(18, &_item);
                        _line = "EDX_EAX := EAX * " + Env->GetLvarName(_ESP_ + _offset, "Integer") + ";//" + _item1.Value;
                        Env->AddToBody(_line);
                        return;
                    }
                }
                //op [ebp-N]
                if (DisInfo.BaseReg == 21 && (_itemBase.Flags & IF_STACK_PTR))
                {
                    if (Op == OP_IMUL)
                    {
                        _item1 = Env->Stack[_itemBase.IntValue + _offset];
                        GetRegItem(16, &_item2);
                        if (_item1.Value != "")
                            _name = _item1.Value;
                        else
                            _name = Env->GetLvarName(_itemBase.IntValue + _offset, "Integer");

                        InitItem(&_item);
                        _item.Precedence = PRECEDENCE_MULT;
                        _item.Value = _name + " * " + _item2.Value;
                        _item.Type = "Integer";
                        SetRegItem(16, &_item);
                        SetRegItem(18, &_item);
                        _line = "EDX_EAX := EAX * " + Env->GetLvarName(_ESP_ + _offset, "Integer") + ";//" + _item.Value;
                        Env->AddToBody(_line);
                        return;
                    }
                }
                if (_itemBase.Type[1] == '^')   //Pointer to gvar (from other unit)
                {
                    if (Op == OP_IMUL)
                    {
                        GetRegItem(16, &_item2);

                        InitItem(&_item);
                        _item.Precedence = PRECEDENCE_MULT;
                        _item.Value = GetString(&_item2, PRECEDENCE_MULT) + " * " + GetString(&_itemBase, PRECEDENCE_MULT + 1);
                        _item.Type = "Integer";
                        SetRegItem(16, &_item);
                        SetRegItem(18, &_item);
                        _line = "EDX_EAX := EAX * " + _itemBase.Value + ";//" + _item.Value;
                        Env->AddToBody(_line);
                        return;
                    }
                }
            }
        }
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInstr2RegImm(DWORD curAdr, BYTE Op)
{
    bool        _vmt, _ptr;
    BYTE        _kind, _kind1, _kind2;
    char        *_tmpBuf;
    int         _reg1Idx, _reg2Idx, _offset, _foffset, _pow2, _mod, _size;
    int         _idx, _classSize, _dotpos, _len, _ap;
    DWORD       _adr;
    ITEM        _itemSrc, _itemDst, _itemBase, _itemIndx;
    ITEM        _item, _item1, _item2;
    PInfoRec    _recN, _recN1;
    PLOCALINFO  _locInfo;
    String      _name, _type, _value, _typeName, _line, _comment, _imm, _iname, _fname, _text;
    WideString  _wStr;

    _reg1Idx = DisInfo.OpRegIdx[0];
    _imm = GetImmString(DisInfo.Immediate);
    if (Op == OP_MOV)
    {
        InitItem(&_item);
        _item.Flags = IF_INTVAL;
        _item.IntValue = DisInfo.Immediate;
        SetRegItem(_reg1Idx, &_item);
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _imm + ";";
        Env->AddToBody(_line);
        return;
        
        if (IsValidImageAdr(DisInfo.Immediate))
        {
            _ap = Adr2Pos(DisInfo.Immediate);
            if (_ap >= 0)
            {
                _recN = GetInfoRec(DisInfo.Immediate);
                if (_recN)
                {
                    _kind1 = _recN->kind;
                    if (_kind1 == ikPointer)
                    {
                        _item.Flags = IF_INTVAL;
                        _item.IntValue = DisInfo.Immediate;
                        SetRegItem(_reg1Idx, &_item);
                        return;
                    }
                    if (_kind1 == ikUnknown || _kind1 == ikData) _kind1 = GetTypeKind(_recN->type, &_size);

                    switch (_kind1)
                    {
                    case ikSet:
                        _item.Flags = IF_INTVAL;
                        _item.IntValue = DisInfo.Immediate;
                        _item.Value = GetDecompilerRegisterName(_reg1Idx);
                        _item.Type = _recN->type;
                        break;
                    case ikString:
                        _item.Value = _recN->GetName();
                        _item.Type = "ShortString";
                        break;
                    case ikLString:
                        _item.Value = _recN->GetName();
                        _item.Type = "String";
                        break;
                    case ikWString:
                        _item.Value = _recN->GetName();
                        _item.Type = "WideString";
                        break;
                    case ikUString:
                        _item.Value = _recN->GetName();
                        _item.Type = "UnicodeString";
                        break;
                    case ikCString:
                        _item.Value = _recN->GetName();
                        _item.Type = "PChar";
                        break;
                    case ikWCString:
                        _len = wcslen((wchar_t*)(Code + _ap));
                        _wStr = WideString((wchar_t*)(Code + _ap));
                        _size = WideCharToMultiByte(CP_ACP, 0, _wStr, _len, 0, 0, 0, 0);
                        if (_size)
                        {
                            _tmpBuf = new char[_size + 1];
                            WideCharToMultiByte(CP_ACP, 0, _wStr, _len, (LPSTR)_tmpBuf, _size, 0, 0);
                            _recN->SetName(TransformString(_tmpBuf, _size));
                            delete[] _tmpBuf;
                        }
                        _item.Value = _recN->GetName();
                        _item.Type = "PWideChar";
                        break;
                    case ikResString:
                        Env->LastResString = _recN->rsInfo->value;
                        _item.Value = _recN->GetName();
                        _item.Type = "PResStringRec";
                        break;
                    case ikArray:
                        _item.Value = _recN->GetName();
                        _item.Type = _recN->type;
                        break;
                    case ikDynArray:
                        _item.Value = _recN->GetName();
                        _item.Type = _recN->type;
                        break;
                    default:
                        Env->ErrAdr = curAdr;
                        throw Exception("Under construction");
                    }
                    SetRegItem(_reg1Idx, &_item);
                    return;
                }
            }
            else
            {
                _idx = BSSInfos->IndexOf(Val2Str8(DisInfo.Immediate));
                if (_idx != -1)
                {
                    _recN = (PInfoRec)BSSInfos->Objects[_idx];
                    _item.Value = _recN->GetName();
                    _item.Type = _recN->type;
                }
                else
                {
                    _item.Value = MakeGvarName(DisInfo.Immediate);
                    AddToBSSInfos(DisInfo.Immediate, _item.Value, "");
                }
                SetRegItem(_reg1Idx, &_item);
                return;
            }
        }
        _item.Flags = IF_INTVAL;
        _item.IntValue = DisInfo.Immediate;
        SetRegItem(_reg1Idx, &_item);
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _imm + ";";
        Env->AddToBody(_line);
        return;
    }
    //cmp reg, imm
    if (Op == OP_CMP)
    {
        GetRegItem(_reg1Idx, &_item);
        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
        if (_item.Value != "" && !SameText(_item.Value, CmpInfo.L))
            CmpInfo.L += "{" + _item.Value + "}";
        CmpInfo.O = CmpOp;
        CmpInfo.R = GetImmString(_item.Type, DisInfo.Immediate);
        return;
    }
    //test reg, imm
    if (Op == OP_TEST)
    {
        GetRegItem(_reg1Idx, &_item);
        _kind = GetTypeKind(_item.Type, &_size);
        if (_kind == ikSet)
        {
            CmpInfo.L = GetSetString(_item.Type, (BYTE*)&DisInfo.Immediate);
            CmpInfo.O = CmpOp + 12;//look GetDirectCondition (in)
            CmpInfo.R = GetDecompilerRegisterName(_reg1Idx);
            return;
        }
        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx) + " And " + _imm;
        CmpInfo.O = CmpOp;
        CmpInfo.R = "0";
        return;
    }
    //add reg, imm (points to class field)
    if (Op == OP_ADD)
    {
        //add esp, imm
        if (_reg1Idx == 20)
        {
            _ESP_ += (int)DisInfo.Immediate;
            return;
        }
        //add reg, imm
        GetRegItem(_reg1Idx, &_item1);
        //If stack ptr
        if (_item1.Flags & IF_STACK_PTR)
        {
            _item1.IntValue += (int)DisInfo.Immediate;
            SetRegItem(_reg1Idx, &_item1);
            return;
        }
        if (_item1.Type != "")
        {
            _typeName = _item1.Type;
            _ptr = (_item1.Type[1] == '^');
            if (_ptr) _typeName = GetTypeDeref(_item1.Type);
            _kind = GetTypeKind(_typeName, &_size);
            if (_kind == ikRecord)
            {
                InitItem(&_item);
                _value = _item1.Value;

                if (_ptr) _value += "^";
                if (GetField(_typeName, DisInfo.Immediate, _name, _type) >= 0)
                {
                    _value += "." + _name;
                    _typeName = _type;
                }
                else
                {
                    _value += ".f" + Val2Str0(DisInfo.Immediate);
                    _typeName = _type;
                }
                _item.Value = _value;
                _item.Type = _typeName;
                SetRegItem(_reg1Idx, &_item);
                _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _item.Value;
                Env->AddToBody(_line);
                return;
            }
            if (_kind == ikVMT)
            {
                if (GetField(_item1.Type, DisInfo.Immediate, _name, _type) >= 0)
                {
                    InitItem(&_item);

                    if (SameText(_item1.Value, "Self"))
                        _item.Value = _name;
                    else
                        _item.Value = _item1.Value + "." + _name;

                    _item.Type = _type;

                    SetRegItem(_reg1Idx, &_item);
                    _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _item.Value;
                    Env->AddToBody(_line);
                    return;
                }
            }
        }
        if (_item1.Value != "")
            _value = GetString(&_item1, PRECEDENCE_ADD) + " + " + _imm;
        else
            _value = GetDecompilerRegisterName(_reg1Idx) + " + " + _imm;
        //-1 and +1 for cycles
        if (DisInfo.Immediate == 1 && _item1.Value != "")
        {
            _len = _item1.Value.Length();
            if (_len > 4)
            {
                if (SameText(_item1.Value.SubString(_len - 3, 4), " - 1"))
                {
                    _item1.Value = _item1.Value.SubString(1, _len - 4);
                    _item1.Precedence = PRECEDENCE_NONE;
                    SetRegItem(_reg1Idx, &_item1);
                    return;
                }
            }
        }

        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
        CmpInfo.O = CmpOp;
        CmpInfo.R = 0;

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ADD;
        _item.Value = GetDecompilerRegisterName(_reg1Idx);
        SetRegItem(_reg1Idx, &_item);

        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " + " + _imm;
        _comment = _item.Value;
        Env->AddToBody(_line + ";//" + _comment);
        return;
    }
    //sub reg, imm
    if (Op == OP_SUB)
    {
        //sub esp, imm
        if (_reg1Idx == 20)
        {
            _ESP_ -= (int)DisInfo.Immediate;
            return;
        }
        GetRegItem(_reg1Idx, &_item1);
        //If reg point to VMT - return (may be operation with Interface)
        if (GetTypeKind(_item1.Type, &_size) == ikVMT) return;

        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
        CmpInfo.O = CmpOp;
        CmpInfo.R = 0;

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ADD;
        _item.Value = GetDecompilerRegisterName(_reg1Idx);
        SetRegItem(_reg1Idx, &_item);

        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " - " + _imm;
        _comment = _item.Value;
        Env->AddToBody(_line + ";//" + _comment);
        return;
    }
    //and reg, imm
    if (Op == OP_AND)
    {
        GetRegItem(_reg1Idx, &_item1);
        if (DisInfo.Immediate == 0xFF && SameText(_item1.Type, "Byte")) return;
        
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " And " + _imm;
        SetRegItem(_reg1Idx, &_item);

        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " And " + _imm;
        _comment = _item.Value;
        Env->AddToBody(_line + ";//" + _comment);
        return;
    }
    //or reg, imm
    if (Op == OP_OR)
    {
        GetRegItem(_reg1Idx, &_item1);
        InitItem(&_item);
        if (DisInfo.Immediate == -1)
        {
            _item.Flags = IF_INTVAL;
            _item.IntValue = -1;
            _item.Type = "Cardinal";
        }
        else
        {
            _item.Precedence = PRECEDENCE_ADD;
            _item.Value = GetString(&_item1, PRECEDENCE_ADD) + " Or " + _imm;
            _item.Type = "Cardinal";
        }
        SetRegItem(_reg1Idx, &_item);

        _line = GetDecompilerRegisterName(_reg1Idx) + " := ";
        if (DisInfo.Immediate != -1)
            _line += GetDecompilerRegisterName(_reg1Idx) + " Or ";
        _line += _imm;
        Env->AddToBody(_line + ";");
        return;
    }
    //sal(shl) reg, imm
    if (Op == OP_SAL || Op == OP_SHL)
    {
        _pow2 = 1;
        for (int n = 0; n < DisInfo.Immediate; n++) _pow2 *= 2;
        GetRegItem(_reg1Idx, &_item1);
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        if (Op == OP_SHL)
        {
            _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " Shl " + String(DisInfo.Immediate);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Shl " + String(DisInfo.Immediate);
            _comment = GetString(&_item1, PRECEDENCE_MULT) + " * " + String(_pow2);
        }
        else
        {
            _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " * " + String(_pow2);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " * " + String(_pow2);
            _comment = "";
        }
        SetRegItem(_reg1Idx, &_item);
        if (_comment != "") _line += ";//" + _comment;
        Env->AddToBody(_line);
        return;
    }
    //sar(shr) reg, imm
    if (Op == OP_SAR || Op == OP_SHR)
    {
        _pow2 = 1;
        for (int n = 0; n < DisInfo.Immediate; n++) _pow2 *= 2;
        GetRegItem(_reg1Idx, &_item1);
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        if (Op == OP_SHR)
        {
            _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " Shr " + String(DisInfo.Immediate);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Shr " + String(DisInfo.Immediate);
            _comment = GetString(&_item1, PRECEDENCE_MULT) + " Div " + String(_pow2);
        }
        else
        {
            _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " Div " + String(_pow2);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Div " + String(_pow2);
            _comment = "";
        }
        SetRegItem(_reg1Idx, &_item);
        if (_comment != "") _line += ";//" + _comment;
        Env->AddToBody(_line);
        return;
    }
    //xor reg, imm
    if (Op == OP_XOR)
    {
        GetRegItem(_reg1Idx, &_item1);
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ADD;
        _item.Value = GetString(&_item1, PRECEDENCE_ADD) + " Xor " + _imm;
        SetRegItem(_reg1Idx, &_item);

        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Xor " + _imm;
        _comment = _item.Value;
        Env->AddToBody(_line + ";//" + _comment);
        return;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInstr2RegReg(DWORD curAdr, BYTE Op)
{
    bool        _vmt;
    BYTE        _kind;
    int         _reg1Idx, _reg2Idx, _offset, _foffset, _pow2, _mod, _size, _idx, _classSize, _dotpos;
    DWORD       _adr;
    ITEM        _itemSrc, _itemDst, _itemBase, _itemIndx;
    ITEM        _item, _item1, _item2;
    PInfoRec    _recN, _recN1;
    PLOCALINFO  _locInfo;
    PFIELDINFO  _fInfo;
    String      _name, _value, _typeName, _line, _comment, _imm, _iname, _fname, _op;

    _reg1Idx = DisInfo.OpRegIdx[0];
    _reg2Idx = DisInfo.OpRegIdx[1];
    GetRegItem(_reg1Idx, &_item1);
    GetRegItem(_reg2Idx, &_item2);

    if (Op == OP_MOV)
    {
        if (IsSameRegister(_reg1Idx, _reg2Idx)) return;

        //mov esp, reg
        if (_reg1Idx == 20)
        {
            SetRegItem(20, &_item2);
            return;
        }
        //mov reg, esp
        if (_reg2Idx == 20)
        {
            InitItem(&_item);
            _item.Flags = IF_STACK_PTR;
            _item.IntValue = _ESP_;
            SetRegItem(_reg1Idx, &_item);
            //mov ebp, esp
            if (Env->BpBased && _reg1Idx == 21 && !Env->LocBase) Env->LocBase = _ESP_;
            return;
        }
        //mov reg, reg
        if (_item2.Flags & IF_ARG)
        {
            if (_reg1Idx != _reg2Idx)
            {
                _item2.Flags &= ~IF_ARG;
                _item2.Name = "";
                SetRegItem(_reg1Idx, &_item2);
                _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _item2.Value + ";";
                Env->AddToBody(_line);
            }
            return;
        }
        //mov reg, eax (eax - call result) -> set eax to reg!!!!
        if (_item2.Flags & IF_CALL_RESULT)
        {
            _item2.Flags &= ~IF_CALL_RESULT;
            _item2.Value1 = _item2.Value;
            _item2.Value = GetDecompilerRegisterName(_reg1Idx);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _item2.Value1 + ";";
            SetRegItem(_reg1Idx, &_item2);
            Env->AddToBody(_line);
            return;
        }
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg2Idx) + ";";
        if (_item2.Value != "")
            _line += "//" + _item2.Value;
        _item2.Flags = 0;
        if (_item2.Value == "")
            _item2.Value = GetDecompilerRegisterName(_reg2Idx);
        SetRegItem(_reg1Idx, &_item2);
        Env->AddToBody(_line);
        return;
    }
    //cmp reg, reg
    if (Op == OP_CMP)
    {
        //_kind1 = GetTypeKind(_item1.Type, &_size);
        //_kind2 = GetTypeKind(_item2.Type, &_size);
        //!!! if kind1 <> kind2 ???
        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
        if (_item1.Value != "" && !SameText(_item1.Value, CmpInfo.L))
            CmpInfo.L += "{" + _item1.Value + "}";
        CmpInfo.O = CmpOp;
        CmpInfo.R = GetDecompilerRegisterName(_reg2Idx);
        if (_item2.Value != "" && !SameText(_item2.Value, CmpInfo.R))
            CmpInfo.R += "{" + _item2.Value + "}";
        return;
    }
    if (Op == OP_TEST)
    {
        if (_reg1Idx == _reg2Idx)
        {
            //GetRegItem(_reg1Idx, &_item);
            CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
            if (_item1.Value != "" && !SameText(_item1.Value, CmpInfo.L))
                CmpInfo.L += "{" + _item1.Value + "}";
            CmpInfo.O = CmpOp;
            CmpInfo.R = GetImmString(_item1.Type, (Variant)0);
            return;
        }
        else
        {
            CmpInfo.L = GetDecompilerRegisterName(_reg1Idx) + " And " + GetDecompilerRegisterName(_reg2Idx);
            CmpInfo.O = CmpOp;
            CmpInfo.R = "0";
            return;
        }
    }
    if (Op == OP_ADD)
        _op = " + ";
    else if (Op == OP_SUB)
        _op = " - ";
    else if (Op == OP_OR)
        _op = " Or ";
    else if (Op == OP_XOR)
        _op = " Xor ";
    else if (Op == OP_MUL || Op == OP_IMUL)
        _op = " * ";
    else if (Op == OP_AND)
        _op = " And ";
    else if (Op == OP_SHR)
        _op = " Shr ";
    else if (Op == OP_SHL)
        _op = " Shl ";

    if (Op == OP_ADD || Op == OP_SUB || Op == OP_OR || Op == OP_XOR)
    {
        if (Op == OP_XOR && _reg1Idx == _reg2Idx)//xor reg,reg
        {
            Env->AddToBody(GetDecompilerRegisterName(_reg1Idx) + " := 0;");
            InitItem(&_item);
            _item.Flags = IF_INTVAL;
            _item.IntValue = 0;
            SetRegItem(_reg1Idx, &_item);
            return;
        }
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + GetDecompilerRegisterName(_reg2Idx);
        _comment = GetString(&_item1, PRECEDENCE_ADD) + _op + GetString(&_item2, PRECEDENCE_ADD + 1);
        Env->AddToBody(_line + ";//" + _comment);

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ADD;
        _item.Value = GetDecompilerRegisterName(_reg1Idx);
        SetRegItem(_reg1Idx, &_item);

        if (Op == OP_SUB)
        {
            CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
            CmpInfo.O = CmpOp;
            CmpInfo.R = GetDecompilerRegisterName(_reg2Idx);
        }
        return;
    }
    if (Op == OP_MUL || Op == OP_IMUL || Op == OP_AND || Op == OP_SHR || Op == OP_SHL)
    {
        _line += GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + GetDecompilerRegisterName(_reg2Idx);
        _comment = GetString(&_item1, PRECEDENCE_MULT) + _op + GetString(&_item2, PRECEDENCE_MULT + 1);
        Env->AddToBody(_line + ";//" + _comment);

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = _comment;
        _item.Type = "Integer";
        SetRegItem(_reg1Idx, &_item);
        return;
    }
    if (Op == OP_DIV || Op == OP_IDIV)
    {
        _line = "EAX := " + GetDecompilerRegisterName(_reg1Idx) + " Div " + GetDecompilerRegisterName(_reg2Idx);
        _comment = GetString(&_item1, PRECEDENCE_MULT) + " Div " + GetString(&_item2, PRECEDENCE_MULT + 1);
        Env->AddToBody(_line + ";//" + _comment);

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = _comment;
        _item.Type = "Integer";
        SetRegItem(16, &_item);
        _item.Value = GetString(&_item1, PRECEDENCE_MULT) + " Mod " + GetString(&_item2, PRECEDENCE_MULT + 1);
        SetRegItem(18, &_item);
        return;
    }
    if (Op == OP_XCHG)
    {
        SetRegItem(_reg1Idx, &_item2);
        SetRegItem(_reg2Idx, &_item1);
        return;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInstr2RegMem(DWORD curAdr, BYTE Op)
{
    bool        _vmt;
    BYTE        _kind, _kind1, _kind2;
    int         _reg1Idx, _reg2Idx, _offset, _foffset, _pow2, _mod, _size, _idx, _idx1, _classSize, _ap;
    DWORD       _adr;
    ITEM        _itemSrc, _itemDst, _itemBase, _itemIndx;
    ITEM        _item, _item1, _item2;
    PInfoRec    _recN, _recN1;
    PLOCALINFO  _locInfo;
    PFIELDINFO  _fInfo;
    String      _name, _value, _type, _line, _comment, _imm, _iname, _fname, _op;

     _reg1Idx = DisInfo.OpRegIdx[0];
    GetRegItem(_reg1Idx, &_itemDst);
    
    GetMemItem(curAdr, &_itemSrc, Op);
    if ((_itemSrc.Flags & IF_VMT_ADR) || (_itemSrc.Flags & IF_EXTERN_VAR))
    {
        SetRegItem(_reg1Idx, &_itemSrc);
        return;
    }
    _op = "?";
    if (Op == OP_ADD || Op == OP_SUB || Op == OP_MUL || Op == OP_IMUL || Op == OP_OR || Op == OP_AND || Op == OP_XOR)
    {
        if (Op == OP_ADD)
            _op = " + ";
        else if (Op == OP_SUB)
            _op = " - ";
        else if (Op == OP_MUL || Op == OP_IMUL)
            _op = " * ";
        else if (Op == OP_OR)
            _op = " Or ";
        else if (Op == OP_AND)
            _op = " And ";
        else if (Op == OP_XOR)
            _op = " Xor ";
    }
    if (_itemSrc.Flags & IF_STACK_PTR)
    {
        _item = Env->Stack[_itemSrc.IntValue];
        if (Op == OP_MOV)
        {
            //Arg
            if (_item.Flags & IF_ARG)
            {
                //_item.Flags &= ~IF_VAR;
                _item.Value = _item.Name;
                SetRegItem(_reg1Idx, &_item);
                Env->AddToBody(GetDecompilerRegisterName(_reg1Idx) + " := " + _item.Value + ";");
                return;
            }
            //Var
            if (_item.Flags & IF_VAR)
            {
                //_item.Flags &= ~IF_VAR;
                SetRegItem(_reg1Idx, &_item);
                return;
            }
            //Field
            if (_item.Flags & IF_FIELD)
            {
                _foffset = _item.Offset;
                _fname = GetRecordFields(_foffset, Env->Stack[_itemSrc.IntValue - _foffset].Type);
                _name = Env->Stack[_itemSrc.IntValue - _foffset].Value;
                _type = ExtractType(_fname);
                if (_name == "")
                    _name = Env->GetLvarName(_itemSrc.IntValue - _foffset, _type);
                InitItem(&_itemDst);
                if (_fname.Pos(":"))
                {
                    _itemDst.Value = _name + "." + ExtractName(_fname);
                    _itemDst.Type = _type;
                }
                else
                {
                    _itemDst.Value = _name + ".f" + Val2Str0(_foffset);
                }
                SetRegItem(_reg1Idx, &_itemDst);
                Env->AddToBody(GetDecompilerRegisterName(_reg1Idx) + " := " + _itemDst.Value + ";");
                return;
            }

            if (_item.Name != "")
                _value = _item.Name;
            else
            {
                _value = Env->GetLvarName(_itemSrc.IntValue, "");
                if (_item.Value != "")
                    _value += "{" + _item.Value + "}";
            }
            _item.Value = _value;

            SetRegItem(_reg1Idx, &_item);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _value + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_LEA)
        {
            SetRegItem(_reg1Idx, &_itemSrc);
            return;
        }
        if (Op == OP_ADD || Op == OP_SUB || Op == OP_MUL || Op == OP_IMUL || Op == OP_OR || Op == OP_AND)
        {
            //Field
            if (_item.Flags & IF_FIELD)
            {
                _foffset = _item.Offset;
                _fname = GetRecordFields(_foffset, Env->Stack[_itemSrc.IntValue - _foffset].Type);
                _name = Env->Stack[_itemSrc.IntValue - _foffset].Value;

                _itemDst.Flags = 0;
                _itemDst.Precedence = PRECEDENCE_ADD;
                if (_fname.Pos(":"))
                {
                    _itemDst.Value += _op + _name + "." + ExtractName(_fname);
                    _itemDst.Type = ExtractType(_fname);
                }
                else
                {
                    _itemDst.Value += _op + _name + ".f" + Val2Str0(_foffset);
                }
                SetRegItem(_reg1Idx, &_itemDst);
                Env->AddToBody(GetDecompilerRegisterName(_reg1Idx) + " := " + _itemDst.Value + ";");
                return;
            }
            if (_itemSrc.Name != "")
                _name = _itemSrc.Name;
            else
                _name = _itemSrc.Value;

            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + _name + ";";
            _itemDst.Flags = 0;
            if (Op == OP_ADD || Op == OP_SUB || Op == OP_OR)
                _itemDst.Precedence = PRECEDENCE_ADD;
            else if (Op == OP_MUL || Op == OP_IMUL || Op == OP_AND)
                _itemDst.Precedence = PRECEDENCE_MULT;
            _itemDst.Value = GetDecompilerRegisterName(_reg1Idx);
            SetRegItem(_reg1Idx, &_itemDst);
            Env->AddToBody(_line);
            if (Op == OP_OR || Op == OP_AND)
            {
                CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
                CmpInfo.O = CmpOp;
                CmpInfo.R = "0";
            }
            return;
        }
        if (Op == OP_CMP)
        {
            CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
            if (_itemDst.Value != "" && !SameText(_itemDst.Value, CmpInfo.L))
                CmpInfo.L += "{" + _itemDst.Value + "}";
            CmpInfo.O = CmpOp;
            if (_item.Flags & IF_ARG)
                CmpInfo.R = _item.Name;
            else
                CmpInfo.R = _itemSrc.Value;
            return;
        }
        if (Op == OP_XCHG)
        {
            SetRegItem(_reg1Idx, &_item);
            Env->Stack[_itemSrc.IntValue] = _itemDst;
            return;
        }
    }
    if (_itemSrc.Flags & IF_INTVAL)
    {
        _offset = _itemSrc.IntValue;
        if (Op == OP_MOV || Op == OP_ADD || Op == OP_SUB || Op == OP_MUL || Op == OP_IMUL || Op == OP_DIV || Op == OP_IDIV)
        {
            _name = "";
            _type = "";
            _ap = Adr2Pos(_offset); _recN = GetInfoRec(_offset);
            if (_recN)
            {
                //VMT
                if (_recN->kind == ikVMT || _recN->kind == ikDynArray)
                {
                    InitItem(&_item);
                    _item.Flags = IF_INTVAL;
                    _item.IntValue = _offset;
                    _item.Value = _recN->GetName();
                    _item.Type = _recN->GetName();
                    SetRegItem(_reg1Idx, &_item);
                    return;
                }
                MakeGvar(_recN, _offset, curAdr);
                _name = _recN->GetName();
                _type = _recN->type;

                if (_ap >= 0)
                {
                    _adr = *(DWORD*)(Code + _ap);
                    //May be pointer to var
                    if (IsValidImageAdr(_adr))
                    {
                        _recN1 = GetInfoRec(_adr);
                        if (_recN1)
                        {
                            MakeGvar(_recN1, _offset, curAdr);
                            InitItem(&_item);
                            _item.Value = _recN1->GetName();
                            _item.Type = "^" + _recN1->type;
                            SetRegItem(_reg1Idx, &_item);
                            return;
                        }
                    }
                }
            }
            //Just value
            else
            {
                InitItem(&_item);
                _item.Flags = IF_INTVAL;
                _item.IntValue = _offset;
                SetRegItem(_reg1Idx, &_item);
                _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _offset + ";";
                Env->AddToBody(_line);
                return;
            }

            if (Op == OP_MOV)
            {
                InitItem(&_item);
                _item.Value = _name;
                _item.Type = _type;
                SetRegItem(_reg1Idx, &_item);
                _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _name + ";";
                Env->AddToBody(_line);
                return;
            }
            if (Op == OP_ADD || Op == OP_SUB || Op == OP_MUL || Op == OP_IMUL)
            {
                _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + _name + ";";
                Env->AddToBody(_line);
                return;
            }
            if (Op == OP_DIV || Op == OP_IDIV)
            {
                _line = GetDecompilerRegisterName(16) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Div " + _name + ";";
                Env->AddToBody(_line);
                _line = GetDecompilerRegisterName(18) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Mod " + _name + ";";
                Env->AddToBody(_line);
                return;
            }
            Env->ErrAdr = curAdr;
            throw Exception("Under construction");
        }
        if (Op == OP_CMP)
        {
            CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
            if (_itemDst.Value != "" && !SameText(_itemDst.Value, CmpInfo.L))
                CmpInfo.L += "{" + _itemDst.Value + "}";
            CmpInfo.O = CmpOp;
            _recN = GetInfoRec(_offset);
            if (_recN)
                CmpInfo.R = _recN->GetName();
            else
                CmpInfo.R = MakeGvarName(_offset);
            return;
        }
    }
    if (Op == OP_MOV || Op == OP_LEA)
    {
        SetRegItem(_reg1Idx, &_itemSrc);
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _itemSrc.Value;
        if (_itemSrc.Flags & IF_RECORD_FOFS)
        {
            _line += "." + GetRecordFields(_itemSrc.Offset, _itemSrc.Type);
        }
        _line += ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_CMP)
    {
        CmpInfo.L = GetDecompilerRegisterName(_reg1Idx);
        if (_itemDst.Value != "" && !SameText(_itemDst.Value, CmpInfo.L))
            CmpInfo.L += "{" + _itemDst.Value + "}";
        CmpInfo.O = CmpOp;
        CmpInfo.R = _itemSrc.Value;
        return;
    }
    if (Op == OP_ADD || Op == OP_SUB || Op == OP_XOR)
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ADD;
        _item.Value = GetString(&_itemDst, PRECEDENCE_ADD) + _op + GetString(&_itemSrc, PRECEDENCE_ADD + 1);
        _item.Type = _itemSrc.Type;
        SetRegItem(_reg1Idx, &_item);
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + _itemSrc.Value + ";//" + _item.Value;
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_MUL || Op == OP_IMUL || Op == OP_AND)
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = GetString(&_itemDst, PRECEDENCE_MULT) + _op + GetString(&_itemSrc, PRECEDENCE_MULT + 1);
        _item.Type = _itemSrc.Type;
        SetRegItem(_reg1Idx, &_item);
        _line = GetDecompilerRegisterName(_reg1Idx) + " := " + GetDecompilerRegisterName(_reg1Idx) + _op + _itemSrc.Value + ";//" + _item.Value;
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_DIV || Op == OP_IDIV)
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = GetString(&_itemDst, PRECEDENCE_MULT) + " Div " + GetString(&_itemSrc, PRECEDENCE_MULT + 1);
        _item.Type = _itemSrc.Type;
        SetRegItem(16, &_item);

        InitItem(&_item);
        _item.Precedence = PRECEDENCE_MULT;
        _item.Value = GetString(&_itemDst, PRECEDENCE_MULT) + " Mod " + GetString(&_itemSrc, PRECEDENCE_MULT + 1);
        _itemDst.Type = _itemSrc.Type;
        SetRegItem(18, &_item);

        _line = GetDecompilerRegisterName(16) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Div " + _itemSrc.Value + ";//" + _itemDst.Value + " Div " + _itemSrc.Value;
        Env->AddToBody(_line);
        _line = GetDecompilerRegisterName(18) + " := " + GetDecompilerRegisterName(_reg1Idx) + " Mod " + _itemSrc.Value + ";//" + _itemDst.Value + " Div " + _itemSrc.Value;
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_OR)
    {
        //Dst - Set
        if (GetTypeKind(_itemDst.Type, &_size) == ikSet)
        {
            AssignItem(&_item1, &_itemDst);
            AssignItem(&_item2, &_itemSrc);
        }
        //Src - Set
        if (GetTypeKind(_itemSrc.Type, &_size) == ikSet)
        {
            AssignItem(&_item1, &_itemSrc);
            AssignItem(&_item2, &_itemDst);
        }
        _line = _item1.Value + " := " + _item1.Value + " + ";
        if (_item2.Flags & IF_INTVAL)
        {
            if (IsValidImageAdr(_item2.IntValue))
                _line += GetSetString(_item1.Type, Code + Adr2Pos(_item2.IntValue));
            else
                _line += GetSetString(_item1.Type, (BYTE*)&_item2.IntValue);
        }
        else
            _line += _item2.Value;
        _line += ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_XCHG)
    {
        SetRegItem(_reg1Idx, &_itemSrc);
        Env->Stack[_itemSrc.IntValue] = _itemDst;
        return;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInstr2MemImm(DWORD curAdr, BYTE Op)
{
    bool        _vmt;
    BYTE        _kind, _kind1, _kind2;
    int         _reg1Idx, _reg2Idx, _offset, _foffset, _pow2, _mod, _size, _idx, _idx1, _classSize, _ap;
    DWORD       _adr;
    ITEM        _itemSrc, _itemDst, _itemBase, _itemIndx;
    ITEM        _item, _item1, _item2;
    PInfoRec    _recN, _recN1;
    PLOCALINFO  _locInfo;
    PFIELDINFO  _fInfo;
    String      _name, _value, _typeName, _line, _comment, _imm, _iname, _fname, _key, _type, _op;

     _imm = GetImmString(DisInfo.Immediate);
    GetMemItem(curAdr, &_itemDst, Op);
    if (_itemDst.Flags & IF_STACK_PTR)
    {
        if (_itemDst.Name != "")
            _name = _itemDst.Name;
        else
            _name = _itemDst.Value;
        _item = Env->Stack[_itemDst.IntValue];
        if (_item.Flags & IF_ARG)
            _name = _item.Value;
        if (Op == OP_MOV)
        {
            Env->Stack[_itemDst.IntValue].Value1 = _imm;
            _line = _name + " := " + _imm + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_CMP)
        {
            CmpInfo.L = _name;
            CmpInfo.O = CmpOp;
            CmpInfo.R = GetImmString(_item.Type, DisInfo.Immediate);
            return;
        }
        if (Op == OP_TEST)
        {
            _typeName = TrimTypeName(Env->Stack[_itemDst.IntValue].Type);
            _kind = GetTypeKind(_typeName, &_size);
            if (_kind == ikSet)
            {
                CmpInfo.L = GetSetString(_typeName, (BYTE*)&DisInfo.Immediate);
                CmpInfo.O = CmpOp + 12;
                CmpInfo.R = _name;
                return;
            }

            CmpInfo.L = _name + " And " + _imm;
            CmpInfo.O = CmpOp;
            CmpInfo.R = "0";
            return;
        }
        if (Op == OP_ADD || Op == OP_SUB || Op == OP_AND || Op == OP_OR || Op == OP_XOR || Op == OP_SHL || Op == OP_SHR)
        {
            if (Op == OP_ADD)
                _op = " + ";
            else if (Op == OP_SUB)
                _op = " - ";
            else if (Op == OP_MUL || Op == OP_IMUL)
                _op = " * ";
            else if (Op == OP_AND)
                _op = " And ";
            else if (Op == OP_OR)
                _op = " Or ";
            else if (Op == OP_XOR)
                _op = " Xor ";
            else if (Op == OP_SHL)
                _op = " Shl ";
            else if (Op == OP_SHR)
                _op = " Shr ";
            else
                _op = " ? ";

            Env->Stack[_itemDst.IntValue].Value = _name;
            _line = _name + " := " + _name + _op + _imm + ";";
            Env->AddToBody(_line);
            return;
        }
        Env->ErrAdr = curAdr;
        throw Exception("Under construction");
    }
    if (_itemDst.Flags & IF_INTVAL)
    {
        if (IsValidImageAdr(_itemDst.IntValue))
        {
            _name = MakeGvarName(_itemDst.IntValue);
            _ap = Adr2Pos(_itemDst.IntValue);
            if (_ap >= 0)
            {
                _recN = GetInfoRec(_itemDst.IntValue);
                if (_recN)
                {
                    if (_recN->HasName()) _name = _recN->GetName();
                    if (_recN->type != "") _imm = GetImmString(_recN->type, DisInfo.Immediate);
                }
            }
            else
            {
                _idx = BSSInfos->IndexOf(Val2Str8(_itemDst.IntValue));
                if (_idx != -1)
                {
                    _recN = (PInfoRec)BSSInfos->Objects[_idx];
                    _name = _recN->GetName();
                    _type = _recN->type;
                    if (_type != "") _imm = GetImmString(_type, DisInfo.Immediate);
                }
                else
                {
                    AddToBSSInfos(_itemDst.IntValue, _name, "");
                }
            }
            if (Op == OP_MOV)
            {
                _line = _name + " := " + _imm + ";";
                Env->AddToBody(_line);
                return;
            }
            if (Op == OP_CMP)
            {
                CmpInfo.L = _name;
                CmpInfo.O = CmpOp;
                CmpInfo.R = _imm;
                return;
            }
            if (Op == OP_ADD)
            {
                _line = _name + " := " + _name + " + " + _imm + ";";
                Env->AddToBody(_line);
                return;
            }
            if (Op == OP_SUB)
            {
                _line = _name + " := " + _name + " - " + _imm + ";";
                Env->AddToBody(_line);
                return;
            }
        }
        Env->ErrAdr = curAdr;
        throw Exception("Under construction");
    }
    if (_itemDst.Name != "" && !IsDefaultName(_itemDst.Name))
        _name = _itemDst.Name;
    else
        _name = _itemDst.Value;
    _typeName = TrimTypeName(_itemDst.Type);
    if (_typeName == "")
    {
        if (Op == OP_MOV)
        {
            _line = _name + " := " + _imm + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_CMP)
        {
            CmpInfo.L = _name;
            CmpInfo.O = CmpOp;
            CmpInfo.R = _imm;
            return;
        }
        if (Op == OP_ADD)
        {
            _line = _name + " := " + _name + " + " + _imm + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_SUB)
        {
            _line = _name + " := " + _name + " - " + _imm + ";";
            Env->AddToBody(_line);
            return;
        }
        Env->ErrAdr = curAdr;
        throw Exception("Under construction");
    }
    _kind = GetTypeKind(_typeName, &_size);
    if (Op == OP_MOV)
    {
        if (_kind == ikMethod)
        {
            if (IsValidImageAdr(DisInfo.Immediate))
            {
                _ap = Adr2Pos(DisInfo.Immediate);
                if (_ap >= 0)
                {
                    _recN = GetInfoRec(DisInfo.Immediate);
                    if (_recN)
                    {
                        if (_recN->HasName())
                            _line = _name + " := " + _recN->GetName() + ";";
                        else
                            _line = _name + " := " + GetDefaultProcName(DisInfo.Immediate);
                    }
                    if (IsFlagSet(cfProcStart, _ap))
                    {
                        _line = _name + " := " + GetDefaultProcName(DisInfo.Immediate);
                    }
                }
                else
                {
                    _idx = BSSInfos->IndexOfName(Val2Str8(DisInfo.Immediate));
                    if (_idx != -1)
                    {
                        _recN = (PInfoRec)BSSInfos->Objects[_idx];
                        _line = _name + " := " + _recN->GetName() + ";";
                    }
                    else
                    {
                        AddToBSSInfos(DisInfo.Immediate, MakeGvarName(DisInfo.Immediate), "");
                        _line = _name + " := " + MakeGvarName(DisInfo.Immediate);
                    }
                }
                Env->AddToBody(_line);
                return;
            }
        }
        _line = _name + " := " + GetImmString(_typeName, DisInfo.Immediate) + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_CMP)
    {
        CmpInfo.L = _name;
        CmpInfo.O = CmpOp;
        CmpInfo.R = GetImmString(_typeName, DisInfo.Immediate);
        return;
    }
    if (Op == OP_TEST)
    {
        if (_kind == ikSet)
        {
            CmpInfo.L = GetSetString(_typeName, (BYTE*)&DisInfo.Immediate);
            CmpInfo.O = CmpOp + 12;
            CmpInfo.R = _name;
            return;
        }
        if (_kind == ikInteger)
        {
            CmpInfo.L = GetImmString(_typeName, DisInfo.Immediate);
            CmpInfo.O = CmpOp;
            CmpInfo.R = _name;
            return;
        }
        _line = _name + " And " + GetImmString(_typeName, DisInfo.Immediate) + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_ADD || Op == OP_SUB || Op == OP_AND || Op == OP_OR || Op == OP_XOR || Op == OP_SHL || Op == OP_SHR)
    {
        if (Op == OP_ADD)
            _op = " + ";
        else if (Op == OP_SUB)
            _op = " - ";
        else if (Op == OP_MUL || Op == OP_IMUL)
            _op = " * ";
        else if (Op == OP_AND)
            _op = " And ";
        else if (Op == OP_OR)
            _op = " Or ";
        else if (Op == OP_XOR)
            _op = " Xor ";
        else if (Op == OP_SHL)
            _op = " Shl ";
        else if (Op == OP_SHR)
            _op = " Shr ";
        else
            _op = " ? ";

        _line = _name + " := " + _name + _op + GetImmString(_typeName, DisInfo.Immediate) + ";";
        Env->AddToBody(_line);
        return;
    }

    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateInstr2MemReg(DWORD curAdr, BYTE Op)
{
    bool        _vmt;
    BYTE        _kind, _kind1, _kind2;
    int         _reg2Idx, _offset, _foffset, _pow2, _mod, _size, _idx, _idx1, _classSize, _ap;
    DWORD       _adr;
    ITEM        _itemSrc, _itemDst, _itemBase, _itemIndx;
    ITEM        _item, _item1, _item2;
    PInfoRec    _recN, _recN1;
    PLOCALINFO  _locInfo;
    PFIELDINFO  _fInfo;
    String      _name, _value, _typeName, _line, _comment, _imm, _iname, _fname, _key;

    _reg2Idx = DisInfo.OpRegIdx[1];
    GetRegItem(_reg2Idx, &_itemSrc);

    _value = GetDecompilerRegisterName(_reg2Idx);
    if (_itemSrc.Value != "") _value = _itemSrc.Value;

    GetMemItem(curAdr, &_itemDst, Op);
    _name = "?";
    if (_itemDst.Value != "")
    {
        _name = _itemDst.Value;
        if (_itemDst.Name != "" && !SameText(_name, _itemDst.Name))
            _name += "{" + _itemDst.Name + "}";
    }
    else if (_itemDst.Name != "")
    {
        _name = _itemDst.Name;
    }

    if (_itemDst.Flags & IF_STACK_PTR)
    {
        if (Op == OP_MOV)
        {
            if (_itemSrc.Flags & IF_CALL_RESULT)
            {
                _itemSrc.Flags &= ~IF_CALL_RESULT;
                _itemSrc.Value = Env->GetLvarName(_itemDst.IntValue, "");
                SetRegItem(_reg2Idx, &_itemSrc);
            }
            else
            {
                if (!(_itemSrc.Flags & IF_ARG))
                    _itemSrc.Name = Env->GetLvarName(_itemDst.IntValue, "");
                if (_itemSrc.Value != "")
                    _itemSrc.Value = Env->GetLvarName(_itemDst.IntValue, "");
            }

            Env->Stack[_itemDst.IntValue] = _itemSrc;
            _line = _name + " := " + _value + ";";
            Env->AddToBody(_line);
            return;
        }
        if (Op == OP_ADD)
        {
            _line = _name + " := " + _name + " + " + _value + ";";
            Env->AddToBody(_line);
            Env->Stack[_itemDst.IntValue].Value = _name + " + " + _value;
            return;
        }
        if (Op == OP_SUB)
        {
            _line = _name + " := " + _name + " - " + _value + ";";
            Env->AddToBody(_line);
            Env->Stack[_itemDst.IntValue].Value = _name + " - " + _value;
            return;
        }
        if (Op == OP_TEST)
        {
            CmpInfo.L = _name + " And " + _value;
            CmpInfo.O = CmpOp;
            CmpInfo.R = 0;
            return;
        }
        if (Op == OP_XOR)
        {
            _line = _name + " := " + _name + " Xor " + _value + ";";
            Env->AddToBody(_line);
            Env->Stack[_itemDst.IntValue].Value = _name + " Xor " + _value;
            return;
        }
        if (Op == OP_BT)
        {
            CmpInfo.L = _name + "[" + _value + "]";
            CmpInfo.O = CmpOp;
            CmpInfo.R = "True";
            return;
        }
    }
    if (_itemDst.Flags & IF_INTVAL)
    {
        _offset = _itemDst.IntValue;
        _ap = Adr2Pos(_offset);
        _recN = GetInfoRec(_offset);
        if (_recN) MakeGvar(_recN, _offset, curAdr);
        if (_ap >= 0)
        {
            _adr = *(DWORD*)(Code + _ap);
            //May be pointer to var
            if (IsValidImageAdr(_adr))
            {
                _recN = GetInfoRec(_adr);
                if (_recN)
                {
                    MakeGvar(_recN, _offset, _adr);
                    _line = "^";
                }
            }
        }
        if (_recN)
        {
            if (_itemSrc.Type != "") _recN->type = _itemSrc.Type;
            _name = _recN->GetName();
        }
/*
        if (Op == OP_MOV)
        {
            _ap = Adr2Pos(_offset); _recN = GetInfoRec(_offset);
            if (_ap >= 0)
            {
                _adr = *(DWORD*)(Code + _ap);
                //May be pointer to var
                if (IsValidImageAdr(_adr))
                {
                    _recN1 = GetInfoRec(_adr);
                    if (_recN1)
                    {
                        MakeGvar(_recN1, _offset, curAdr);
                        if (_itemSrc.Type != "") _recN1->type = _itemSrc.Type;
                        _line = "^" + _recN1->GetName() + " := " + _value + ";";
                        Env->AddToBody(_line);
                        return;
                    }
                }
            }
            if (_recN)
            {
                MakeGvar(_recN, _offset, curAdr);
                if (_itemSrc.Type != "") _recN->type = _itemSrc.Type;
                _line = _recN->GetName() + " := " + _value + ";";
                Env->AddToBody(_line);
                return;
            }
            Env->ErrAdr = curAdr;
            throw Exception("Under construction");
        }
*/
    }
    if (Op == OP_MOV)
    {
        _line = _name + " := ";
        _typeName = _itemDst.Type;
        if (_typeName != "")
        {
            _kind = GetTypeKind(_typeName, &_size);
            if (_kind == ikSet)
            {
                _line += GetSetString(_typeName, (BYTE*)&_itemSrc.IntValue) + ";";
                Env->AddToBody(_line);
                return;
            }
        }
        _line += GetDecompilerRegisterName(_reg2Idx) + ";";
        if (_value != "") _line += "//" + _value;
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_ADD)
    {
        _line = _name + " := " + _name + " + " + _value + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_SUB)
    {
        _line = _name + " := " + _name + " - " + _value + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_OR)
    {
        _line = _name + " := " + _name + " Or " + _value + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_XOR)
    {
        _line = _name + " := " + _name + " Xor " + _value + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_TEST)
    {
        CmpInfo.L = _name + " And " + _value;
        CmpInfo.O = CmpOp;
        CmpInfo.R = 0;
        _line = _name + " := " + _name + " And " + _value + ";";
        Env->AddToBody(_line);
        return;
    }
    if (Op == OP_BT)
    {
        CmpInfo.L = _value;
        CmpInfo.O = 'Q';
        CmpInfo.R = _name;
        return;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
//Simulate instruction with 2 operands
void __fastcall TDecompiler::SimulateInstr2(DWORD curAdr, BYTE Op)
{
    if (DisInfo.OpType[0] == otREG)
    {
        if (DisInfo.OpType[1] == otIMM)
        {
            SimulateInstr2RegImm(curAdr, Op);
            return;
        }
        if (DisInfo.OpType[1] == otREG)
        {
            SimulateInstr2RegReg(curAdr, Op);
            return;
        }
        if (DisInfo.OpType[1] == otMEM)
        {
            SimulateInstr2RegMem(curAdr, Op);
            return;
        }
    }
    if (DisInfo.OpType[0] == otMEM)
    {
        if (DisInfo.OpType[1] == otIMM)
        {
            SimulateInstr2MemImm(curAdr, Op);
            return;
        }
        if (DisInfo.OpType[1] == otREG)
        {
            SimulateInstr2MemReg(curAdr, Op);
            return;
        }
    }
}
//---------------------------------------------------------------------------
//Simulate instruction with 3 operands
void __fastcall TDecompiler::SimulateInstr3(DWORD curAdr, BYTE Op)
{
    int         _reg1Idx, _reg2Idx, _imm;
    ITEM        _itemDst, _itemSrc;
    String      _value, _line, _comment;

    _reg1Idx = DisInfo.OpRegIdx[0];
    _reg2Idx = DisInfo.OpRegIdx[1];
    _imm = DisInfo.Immediate;
    
    InitItem(&_itemDst);
    _itemDst.Type = "Integer";

    if (Op == OP_IMUL)
    {
        //mul reg, reg, imm
        if (DisInfo.OpType[1] == otREG)
        {
            _value = String(_imm) + " * " + GetDecompilerRegisterName(_reg2Idx);
            _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _value;
            GetRegItem(_reg2Idx, &_itemSrc);
            _comment = String(_imm) +  " * " + GetString(&_itemSrc, PRECEDENCE_MULT);
            _itemDst.Precedence = PRECEDENCE_MULT;
            _itemDst.Value = _comment;
            SetRegItem(_reg1Idx, &_itemDst);
            Env->AddToBody(_line + ";//" + _comment);
            return;
        }
        //mul reg, [Mem], imm
        if (DisInfo.OpType[1] == otMEM)
        {
            GetMemItem(curAdr, &_itemSrc, Op);
            _value = String(_imm) + " * " + _itemSrc.Name;
           _line = GetDecompilerRegisterName(_reg1Idx) + " := " + _value;
            _comment = String(_imm) +  " * " + GetString(&_itemSrc, PRECEDENCE_MULT);
            _itemDst.Precedence = PRECEDENCE_MULT;
            _itemDst.Value = _comment;
            SetRegItem(_reg1Idx, &_itemDst);
            Env->AddToBody(_line + ";//" + _comment);
            return;
        }
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
DWORD __fastcall TDecompiler::DecompileTry(DWORD fromAdr, DWORD flags, PLoopInfo loopInfo)
{
    BYTE            op;
    int             pos, tpos, pos1, instrLen, skipNum;
    DWORD           startTryAdr, endTryAdr, startFinallyAdr, endFinallyAdr, endExceptAdr, adr, endAdr, hAdr;
    PInfoRec        recN;
    ITEM            item;
    TDecompiler     *de;

    skipNum = IsTryBegin(fromAdr, &startTryAdr) + IsTryBegin0(fromAdr, &startTryAdr);
    adr = startTryAdr; pos = Adr2Pos(adr);

    if (IsFlagSet(cfFinally, pos))
    {
        //jmp @HandleFinally
        instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, 0, 0);
        adr += instrLen; pos += instrLen;
        //jmp @2
        Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
        startFinallyAdr = DisInfo.Immediate;
        //Get prev push
        pos1 = GetNearestUpInstruction(Adr2Pos(DisInfo.Immediate));
        Disasm.Disassemble(Code + pos1, (__int64)Pos2Adr(pos1), &DisInfo, 0);
        endFinallyAdr = DisInfo.Immediate;
        //Find flag cfFinally
        while (1)
        {
            pos1--;
            if (IsFlagSet(cfFinally, pos1)) break;
        }
        if (pos1) endTryAdr = Pos2Adr(pos1);
        if (!endTryAdr)
        {
            Env->ErrAdr = fromAdr;
            throw Exception("Invalid end address of try section");
        }
        //Decompile try
        Env->AddToBody("try");
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(endTryAdr);
        try
        {
            endAdr = de->Decompile(fromAdr + skipNum, flags, loopInfo);
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("Try->" + exception.Message);
        }
        delete de;

        Env->AddToBody("finally");
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(endFinallyAdr);
        try
        {
            endAdr = de->Decompile(startFinallyAdr, CF_FINALLY, loopInfo);
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("Try->" + exception.Message);
        }
        Env->AddToBody("end");
        delete de;
        return endAdr;
    }
    else if (IsFlagSet(cfExcept, pos))
    {
        //Prev jmp
        pos1 = GetNearestUpInstruction(pos);
        Disasm.Disassemble(Code + pos1, (__int64)Pos2Adr(pos1), &DisInfo, 0);
        endExceptAdr = DisInfo.Immediate;
        //Next
        pos += Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &DisInfo, 0);
        //Find prev flag cfExcept
        while (1)
        {
            if (IsFlagSet(cfExcept, pos1)) break;
            pos1--;
        }
        if (pos1) endTryAdr = Pos2Adr(pos1);
        if (!endTryAdr)
        {
            Env->ErrAdr = fromAdr;
            throw Exception("Invalid end address of try section");
        }
        //Decompile try
        Env->AddToBody("try");
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(endTryAdr);
        try
        {
            endAdr = de->Decompile(fromAdr + skipNum, 0, loopInfo);
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("Try->" + exception.Message);
        }
        delete de;
        //on except
        if (IsFlagSet(cfETable, pos))
        {
            Env->AddToBody("except");
            int num = *((DWORD*)(Code + pos)); pos += 4;
            //Table pos
            tpos = pos;
            for (int n = 0; n < num; n++)
            {
                adr = *((DWORD*)(Code + pos)); pos += 4;
                if (IsValidCodeAdr(adr))
                {
                    recN = GetInfoRec(adr);
                    InitItem(&item);
                    item.Value = "E";
                    item.Type = recN->GetName();
                    SetRegItem(16, &item);  //eax -> exception info
                    Env->AddToBody("on E:" + recN->GetName() + " do");
                }
                else
                {
                    Env->AddToBody("else");
                }
                hAdr = *((DWORD*)(Code + pos)); pos += 4;
                if (IsValidCodeAdr(hAdr))
                {
                    de = new TDecompiler(Env);
                    de->SetStackPointers(this);
                    de->SetDeFlags(DeFlags); pos1 = tpos;
                    for (int m = 0; m < num; m++)
                    {
                        pos1 += 4;
                        adr = *((DWORD*)(Code + pos1)); pos1 += 4;
                        de->SetStop(adr);
                    }
                    de->ClearStop(hAdr);
                    de->SetStop(endExceptAdr);
                    try
                    {
                        Env->AddToBody("begin");
                        endAdr = de->Decompile(hAdr, 0, loopInfo);
                        Env->AddToBody("end");
                    }
                    catch(Exception &exception)
                    {
                        delete de;
                        throw Exception("Try->" + exception.Message);
                    }
                    delete de;
                }
            }
            Env->AddToBody("end");
        }
        //except
        else
        {
            Env->AddToBody("except");
            de = new TDecompiler(Env);
            de->SetStackPointers(this);
            de->SetDeFlags(DeFlags);
            de->SetStop(endExceptAdr);
            try
            {
                endAdr = de->Decompile(Pos2Adr(pos), CF_EXCEPT, loopInfo);
            }
            catch(Exception &exception)
            {
                delete de;
                throw Exception("Try->" + exception.Message);
            }
            Env->AddToBody("end");
            delete de;
        }
        return endAdr;//endExceptAdr;
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::MarkCaseEnum(DWORD fromAdr)
{
    BYTE            b;
    DWORD           adr = fromAdr, jAdr;
    int             n, pos = Adr2Pos(fromAdr), cTblPos, jTblPos;

    //cmp reg, Imm
    //Imm -
    int instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    int caseNum = DisInfo.Immediate + 1;
    pos += instrLen;
    adr += instrLen;
    //ja EndOfCaseAdr
    instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    DWORD endOfCaseAdr = DisInfo.Immediate;

    pos += instrLen;
    adr += instrLen;
    //mov???
    instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    BYTE op = Disasm.GetOp(DisInfo.Mnem);
    DWORD   cTblAdr = 0, jTblAdr = 0;
    if (op == OP_MOV)
    {
        cTblAdr = DisInfo.Offset;
        pos += instrLen;
        adr += instrLen;
        instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    }
    jTblAdr = DisInfo.Offset;
    int maxN = 0;
    if (cTblAdr)
    {
        cTblPos = Adr2Pos(cTblAdr);
        int cNum = jTblAdr - cTblAdr;
        for (n = 0; n < cNum; n++)
        {
            b = *(Code + cTblPos);
            if (b > maxN) maxN = b;
            cTblPos++;
        }
        jTblPos = Adr2Pos(jTblAdr);
        for (n = 0; n <= maxN; n++)
        {
            jAdr = *((DWORD*)(Code + jTblPos));
            SetStop(jAdr);
            jTblPos += 4;
        }
    }
    else
    {
        jTblPos = Adr2Pos(jTblAdr);
        for (n = 0; n < caseNum; n++)
        {
            if (IsFlagSet(cfCode | cfLoc, jTblPos)) break;
            jAdr = *((DWORD*)(Code + jTblPos));
            SetStop(jAdr);
            jTblPos += 4;
        }
    }
}
//---------------------------------------------------------------------------
DWORD __fastcall TDecompiler::DecompileCaseEnum(DWORD fromAdr, int N, PLoopInfo loopInfo)
{
    bool            skip;
    BYTE            b;
    DWORD           adr = fromAdr, jAdr, jAdr1, _adr, _endAdr = 0;
    int             n, m, pos = Adr2Pos(fromAdr), cTblPos, cTblPos1, jTblPos, jTblPos1;
    ITEM            item, item1;
    String          line = "";
    TDecompiler     *de;

    //cmp reg, Imm
    //Imm -
    int instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    int caseNum = DisInfo.Immediate + 1;
    pos += instrLen;
    adr += instrLen;
    //ja EndOfCaseAdr
    instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    DWORD endOfCaseAdr = DisInfo.Immediate;

    pos += instrLen;
    adr += instrLen;
    //mov???
    instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    BYTE op = Disasm.GetOp(DisInfo.Mnem);
    DWORD   cTblAdr = 0, jTblAdr = 0;
    if (op == OP_MOV)
    {
        cTblAdr = DisInfo.Offset;
        pos += instrLen;
        adr += instrLen;
        instrLen = Disasm.Disassemble(Code + pos, (__int64)adr, &DisInfo, 0);
    }
    jTblAdr = DisInfo.Offset;
    int maxN = 0;
    if (cTblAdr)
    {
        cTblPos = Adr2Pos(cTblAdr);
        int cNum = jTblAdr - cTblAdr;
        for (n = 0; n < cNum; n++)
        {
            b = *(Code + cTblPos);
            if (b > maxN) maxN = b;
            cTblPos++;
        }
        jTblPos = Adr2Pos(jTblAdr);
        for (m = 0; m <= maxN; m++)
        {
            line = "";
            skip = false;
            cTblPos = Adr2Pos(cTblAdr);
            for (n = 0; n < cNum; n++)
            {
                b = *(Code + cTblPos);
                jAdr = *((DWORD*)(Code + jTblPos));
                if (b == m)
                {
                    if (jAdr == endOfCaseAdr)
                    {
                        skip = true;
                        break;
                    }
                    if (line != "") line += ",";
                    line += String(n + N);
                }
                cTblPos++;
            }
            if (!skip)
            {
                Env->AddToBody(line + ":");
                Env->SaveContext(endOfCaseAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->SetStop(endOfCaseAdr);
                de->MarkCaseEnum(fromAdr);
                de->ClearStop(jAdr);

                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(jAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("CaseEnum->" + exception.Message);
                }
                Env->RestoreContext(endOfCaseAdr);
                delete de;
            }
            jTblPos += 4;
        }
        return _endAdr;//endOfCaseAdr;
    }
    jTblPos = Adr2Pos(jTblAdr);
    for (n = 0; n < caseNum; n++)
    {
        if (IsFlagSet(cfCode | cfLoc, jTblPos)) break;
        jAdr = *((DWORD*)(Code + jTblPos));
        if (jAdr != endOfCaseAdr)
        {
            skip = false;
            //If case already decompiled?
            jTblPos1 = Adr2Pos(jTblAdr);
            for (m = 0; m < n; m++)
            {
                jAdr1 = *((DWORD*)(Code + jTblPos1));
                if (jAdr1 == jAdr)
                {
                    skip = true;
                    break;
                }
                jTblPos1 += 4;
            }

            if (!skip)
            {
                line = "";
                jTblPos1 = Adr2Pos(jTblAdr);
                for (m = 0; m < caseNum; m++)
                {
                    if (IsFlagSet(cfCode | cfLoc, jTblPos1)) break;
                    jAdr1 = *((DWORD*)(Code + jTblPos1));
                    if (jAdr1 == jAdr)
                    {
                        if (line != "") line += ",";
                        line += String(m + N);
                    }
                    jTblPos1 += 4;
                }
                Env->AddToBody(line + ":");
                Env->SaveContext(endOfCaseAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->SetStop(endOfCaseAdr);
                de->MarkCaseEnum(fromAdr);
                de->ClearStop(jAdr);

                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(jAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("CaseEnum->" + exception.Message);
                }
                Env->RestoreContext(endOfCaseAdr);
                delete de;
            }
        }
        jTblPos += 4;
    }
    return _endAdr;//endOfCaseAdr;
}
//---------------------------------------------------------------------------
String __fastcall TDecompiler::GetSysCallAlias(String AName)
{
    if (SameText(AName, "@Assign") ||
        SameText(AName, "@AssignText")) return "AssignFile";
    if (SameText(AName, "@Append")) return "Append";
    if (SameText(AName, "@Assert")) return "Assert";
    if (SameText(AName, "@BlockRead")) return "Read";
    if (SameText(AName, "@BlockWrite")) return "Write";
    if (SameText(AName, "@ChDir")) return "ChDir";
    if (SameText(AName, "@Close")) return "CloseFile";
    if (SameText(AName, "@DynArrayHigh")) return "High";
    if (SameText(AName, "@EofText")) return "Eof";
    if (SameText(AName, "@FillChar")) return "FillChar";
    if (SameText(AName, "@Flush")) return "Flush";
    if (SameText(AName, "@Copy")     ||
        SameText(AName, "@LStrCopy") ||
        SameText(AName, "@WStrCopy") ||
        SameText(AName, "@UStrCopy")) return "Copy";
    if (SameText(AName, "@LStrDelete") ||
        SameText(AName, "@UStrDelete")) return "Delete";
    if (SameText(AName, "@LStrFromPCharLen")) return "SetString";
    if (SameText(AName, "@LStrInsert")) return "Insert";
    if (SameText(AName, "@PCharLen") ||
        SameText(AName, "@LStrLen") ||
        SameText(AName, "@WStrLen") ||
        SameText(AName, "@UStrLen")) return "Length";
    if (SameText(AName, "@LStrOfChar")) return "StringOfChar";
    if (SameText(AName, "@Pos") ||
        SameText(AName, "@LStrPos") ||
        SameText(AName, "@WStrPos") ||
        SameText(AName, "@UStrPos")) return "Pos";
    if (SameText(AName, "@LStrSetLength") ||
        SameText(AName, "@UStrSetLength")) return "SetLength";
    if (SameText(AName, "@MkDir")) return "MkDir";
    if (SameText(AName, "@New")) return "New";
    if (SameText(AName, "@RaiseAgain")) return "Raise";
    if (SameText(AName, "@RandInt")) return "Random";
    if (SameText(AName, "@ReadLn")) return "ReadLn";
    if (SameText(AName, "@ReadLong") ||
        SameText(AName, "@ReadLString") ||
        SameText(AName, "@ReadRec") ||
        SameText(AName, "@ReadUString")) return "Read";
    if (SameText(AName, "@ReallocMem")) return "ReallocMem";
    if (SameText(AName, "@ResetFile") ||
        SameText(AName, "@ResetText")) return "Reset";
    if (SameText(AName, "@ValLong") ||
        SameText(AName, "@ValInt64")) return "Val";
    if (SameText(AName, "@WriteLn")) return "WriteLn";
    return "";
}
//---------------------------------------------------------------------------
bool __fastcall TDecompiler::SimulateSysCall(String name, DWORD procAdr, int instrLen)
{
    int         _cnt, _esp, _cmpRes, _size;
    DWORD       _adr;
    ITEM        _item, _item1, _item2, _item3, _item4;
    PInfoRec    _recN;
    String      _line, _value, _value1, _value2, _typeName, _op;
    __int64     _int64Val;

    if (SameText(name, "@AsClass"))
    {
        //type of edx -> type of eax
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        _item1.Value = _item2.Type + "(" + _item1.Value + ")";
        _item1.Type = _item2.Type;
        SetRegItem(16, &_item1);
        return false;
    }
    if (SameText(name, "@IsClass"))
    {
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        _adr = _item2.IntValue;
        if (!IsValidImageAdr(_adr))
        {
            throw Exception("Invalid address");
        }
        _recN = GetInfoRec(_adr);
        InitItem(&_item);
        _item.Value = "(" + _item1.Value + " is " + _recN->GetName() + ")";
        _item.Type = "Boolean";
        SetRegItem(16, &_item);
        return false;
    }
    if (SameText(name, "@CopyRecord"))
    {
        //dest:Pointer
        GetRegItem(16, &_item1);
        //source:Pointer
        GetRegItem(18, &_item2);
        //typeInfo:Pointer
        GetRegItem(17, &_item3);
        _recN = GetInfoRec(_item3.IntValue);
        _typeName = _recN->GetName();
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, _typeName);
            _item1 = Env->Stack[_item1.IntValue];
        }
        Env->Stack[_item1.IntValue].Type = _typeName;
        
        _line = _item1.Value + " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@DynArrayAsg"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item1.IntValue].Value == "") Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, "array of");
            _item1 = Env->Stack[_item1.IntValue];
        }
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item2.IntValue].Value == "") Env->Stack[_item2.IntValue].Value = Env->GetLvarName(_item2.IntValue, "array of");
            _item2 = Env->Stack[_item2.IntValue];
        }
        _line = _item1.Value + " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@DynArrayClear"))
    {
        //eax - var
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item1.IntValue].Value == "") Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, "array of");
            _item1 = Env->Stack[_item1.IntValue];
        }
        _line = _item1.Value + " := Nil;";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@DynArrayLength"))
    {
        //eax - ptr
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item1.IntValue].Value == "") Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, "array of");
            _item1 = Env->Stack[_item1.IntValue];
        }
        _value = "Length(" + _item1.Value + ")";
        _line = "EAX := " + _value;
        Env->AddToBody(_line);
        InitItem(&_item);
        _item.Flags |= IF_CALL_RESULT;
        _item.Value = _value;
        _item.Type = "Integer";
        SetRegItem(16, &_item);
        return false;
    }
    if (SameText(name, "@DynArraySetLength"))//stdcall
    {
        //eax - dst
        GetRegItem(16, &_item1); _item = _item1;
        //edx - type of DynArray
        GetRegItem(18, &_item2);
        _typeName = GetTypeName(_item2.IntValue);
        if (_item1.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item1.IntValue].Value == "") Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, _typeName);
            _item = Env->Stack[_item1.IntValue];
            Env->Stack[_item1.IntValue].Type = _typeName;
            _line = "SetLength(" + _item.Value;
        }
        else if (_item1.Flags & IF_INTVAL)
        {
            _line = "SetLength(" + MakeGvarName(_item1.IntValue);
        }
        //ecx - dims cnt
        GetRegItem(17, &_item3); _cnt = _item3.IntValue;
        _esp = _ESP_;
        for (int n = 0; n < _cnt; n++)
        {
            _line += ", ";
            _item = Env->Stack[_esp];
            if (_item.Flags & IF_INTVAL)
                _line += String(_item.IntValue);
            else
                _line += _item.Value;
            _esp += 4;
        }
        _line += ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@FreeMem"))
    {
        _line = "FreeMem(";
        GetRegItem(16, &_item);
        if (_item.Value != "")
            _line += _item.Value;
        else
            _line += "EAX";
        _line += ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@GetMem"))
    {
        _value = "GetMem(";
        //eax-Bytes
        GetRegItem(16, &_item);
        if (_item.Flags & IF_INTVAL)
            _value += String(_item.IntValue);
        else
            _value += _item.Value;
        _value += ")";
        _line = _value + ";";
        Env->AddToBody(_line);

        _typeName = ManualInput(CurProcAdr, procAdr, "Define type of function GetMem", "Type:");
        if (_typeName == "")
        {
            Env->ErrAdr = procAdr;
            throw Exception("Bye!");
        }

        InitItem(&_item);
        _item.Flags |= IF_CALL_RESULT;
        _item.Value = _value;
        _item.Type = _typeName;
        SetRegItem(16, &_item);
        return false;
    }
    if (SameText(name, "@Halt0"))
    {
        //_line = "Exit;";
        //Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@InitializeRecord") ||
        SameText(name, "@FinalizeRecord")   ||
        SameText(name, "@AddRefRecord"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        //edx - TypeInfo
        GetRegItem(18, &_item2);
        if (_item1.Flags & IF_STACK_PTR)
        {
            _typeName = GetTypeName(_item2.IntValue);
            _size = GetRecordSize(_typeName);
            for (int r = 0; r < _size; r++)
            {
                if (_item1.IntValue + r >= Env->StackSize)
                {
                    Env->ErrAdr = procAdr;
                    throw Exception("Possibly incorrect RecordSize (or incorrect type of record)");
                }
                _item = Env->Stack[_item1.IntValue + r];
                _item.Flags = IF_FIELD;
                _item.Offset = r;
                _item.Type = "";
                if (r == 0) _item.Type = _typeName;
                Env->Stack[_item1.IntValue + r] = _item;
            }
            if (Env->Stack[_item1.IntValue].Value == "") Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, _typeName);
            Env->Stack[_item1.IntValue].Type = _typeName;
        }
        return false;
    }
    if (SameText(name, "@InitializeArray") ||
        SameText(name, "@FinalizeArray"))
    {
        //eax - dst
        GetRegItem(16, &_item1); _item = _item1;
        //edx - type of DynArray
        GetRegItem(18, &_item2);
        //ecx - dims cnt
        GetRegItem(17, &_item3); _cnt = _item3.IntValue;
        _typeName = "array [1.." + String(_cnt) + "] of " + GetTypeName(_item2.IntValue);
        if (_item1.Flags & IF_STACK_PTR)
        {
            if (Env->Stack[_item1.IntValue].Value == "")
                Env->Stack[_item1.IntValue].Value = Env->GetLvarName(_item1.IntValue, _typeName);
            _item = Env->Stack[_item1.IntValue];
        }
        Env->Stack[_item1.IntValue].Type = _typeName;
        return false;
    }
    if (SameText(name, "@IntfCast"))
    {
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        _line = "(" + _item1.Value + " as " + _item2.Value1 + ")";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@IntfClear"))
    {
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "IInterface";

        _line = _item1.Value + " := Nil;";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@IntfCopy"))
    {
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        _line = _item1.Value + " := " + _item2.Value1;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrAsg")  ||
        SameText(name, "@LStrLAsg") ||
        SameText(name, "@WStrAsg")  ||
        SameText(name, "@WStrLAsg")  ||
        SameText(name, "@UStrAsg")  ||
        SameText(name, "@UStrLAsg"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        //edx - src
        GetRegItem(18, &_item2);
        _line = GetStringArgument(&_item1) + " := " + GetStringArgument(&_item2) + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrCat") ||
        SameText(name, "@WStrCat") ||
        SameText(name, "@UStrCat"))
    {
        //eax - dst
        GetRegItem(16, &_item);
        _line = GetStringArgument(&_item) + " := " + GetStringArgument(&_item) + " + ";
        //edx - src
        GetRegItem(18, &_item);
        _line += GetStringArgument(&_item) + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrCat3") ||
        SameText(name, "@WStrCat3") ||
        SameText(name, "@UStrCat3"))
    {
        //eax - dst
        GetRegItem(16, &_item);
        _line = GetStringArgument(&_item) + " := ";
        //edx - str1
        GetRegItem(18, &_item);
        _line += GetStringArgument(&_item) + " + ";
        //ecx - str2
        GetRegItem(17, &_item);
        _line += GetStringArgument(&_item) + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrCatN") ||
        SameText(name, "@WStrCatN") ||
        SameText(name, "@UStrCatN"))
    {
        //eax - dst
        GetRegItem(16, &_item);
        _line = GetStringArgument(&_item) + " := ";
        if (_item.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item.IntValue].Flags = 0;
            Env->Stack[_item.IntValue].Value = Env->GetLvarName(_item.IntValue, "String");
            if (name[2] == 'L')
                Env->Stack[_item.IntValue].Type = "AnsiString";
            else if (name[2] == 'W')
                Env->Stack[_item.IntValue].Type = "WideString";
            else
                Env->Stack[_item.IntValue].Type = "UnicodeString";
        }
        //edx - argCnt
        GetRegItem(18, &_item2); _cnt = _item2.IntValue;
        _ESP_ += 4*_cnt; _esp = _ESP_;
        for (int n = 0; n < _cnt; n++)
        {
            if (n) _line += " + ";
            _esp -= 4;
            _item = Env->Stack[_esp];
            _line += GetStringArgument(&_item);
        }
        _line += ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrClr") ||
        SameText(name, "@WStrClr") ||
        SameText(name, "@UStrClr"))
    {
        GetRegItem(16, &_item);
        _line = GetStringArgument(&_item) + " := '';";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@AStrCmp") ||
        SameText(name, "@LStrCmp") ||
        SameText(name, "@WStrCmp") ||
        SameText(name, "@UStrCmp") ||
        SameText(name, "@UStrEqual"))
    {
        GetCmpInfo(procAdr + instrLen);
        CmpInfo.O = 'F';//By default (<>)

        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        CmpInfo.L = _item1.Value;

        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            _item2 = Env->Stack[_item2.IntValue];
            CmpInfo.R = _item2.Value;
        }
        else if (_item2.Flags & IF_INTVAL)
        {
            if (!_item2.IntValue)
                CmpInfo.R = "''";
            else
            {
                _recN = GetInfoRec(_item2.IntValue);
                if (_recN)
                    CmpInfo.R = _recN->GetName();
                else
//Fix it!!!
//Add analyze of String type
                    CmpInfo.R = TransformString(Code + Adr2Pos(_item2.IntValue), -1);
            }
        }
        else
        {
            CmpInfo.R = _item2.Value;
        }
        return true;
    }
    if (SameText(name, "@LStrFromArray")  ||
        SameText(name, "@LStrFromChar")   ||
        SameText(name, "@LStrFromPChar")  ||
        SameText(name, "@LStrFromString") ||
        SameText(name, "@LStrFromWStr")   ||
        SameText(name, "@LStrFromUStr"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "String";
        }
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            if (name[10] == 'A')
                _typeName = "PAnsiChar";
            else if (name[10] == 'C')
                _typeName = "Char";
            else if (name[10] == 'P')
                _typeName = "PChar";
            else if (name[10] == 'S')
                _typeName = "ShortString";
            else if (name[10] == 'W')
                _typeName = "WideString";
            else if (name[10] == 'U')
                _typeName = "UnicodeString";

            Env->Stack[_item2.IntValue].Type = _typeName;
        }
        _line = _item1.Value + " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrToPChar"))
    {
        //eax - src
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "String";
        _item1.Value = "PChar(" + _item1.Value + ")";
        SetRegItem(16, &_item1);
        _line = "EAX := " + _item1.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@LStrToString"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_INTVAL)
        {
            _line = MakeGvarName(_item1.IntValue);
        }
        else if (_item1.Flags & IF_STACK_PTR)
        {
            _line = _item1.Value;
            Env->Stack[_item1.IntValue].Type = "ShortString";
        }
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
            Env->Stack[_item2.IntValue].Type = "String";
        _line += " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@PStrNCat") ||
        SameText(name, "@PStrCpy")  ||
        SameText(name, "@PStrNCpy"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "ShortString";
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
            Env->Stack[_item2.IntValue].Type = "ShortString";
        _line = _item1.Value + " := ";
        if (SameText(name, "@PStrNCat")) _line += _item1.Value + " + ";
        _line += _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@RaiseExcept"))
    {
        //eax - Exception
        GetRegItem(16, &_item1);
        _line = "raise " + _item1.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@RewritFile") ||
        SameText(name, "@RewritText"))
    {
        //File
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        _line = "Rewrite(" + ExtractClassName(_item1.Value) + ");";
        Env->AddToBody(_line);
        return false;
    }

    if (SameText(name, "@Sin") ||
        SameText(name, "@Cos") ||
        SameText(name, "@Exp") ||
        SameText(name, "@Int"))
    {
        _value = name.SubString(2, 5);
        InitItem(&_item1);
        _item1.Value = _value + "(" + FGet(0)->Value + ")";
        _item1.Type = "Extended";
        FSet(0, &_item1);
        return false;
    }
    if (SameText(name, "@Trunc") ||
        SameText(name, "@Round"))
    {
        _value = name.SubString(2, 5) + "(" + FGet(0)->Value + ")";
        InitItem(&_item);
        _item.Value = _value;
        _item.Type = "Int64";
        SetRegItem(16, &_item);
        SetRegItem(18, &_item);
        _line = "//EAX := " + _value + ";";
        Env->AddToBody(_line);
        FPop();
        return false;
    }
    if (SameText(name.SubString(1, name.Length() - 1), "@UniqueString"))
    {
        GetRegItem(16, &_item1);
        InitItem(&_item);
        _item.Value = "EAX";
        _item.Type = "String";
        SetRegItem(16, &_item);
        if (_item1.Flags & IF_STACK_PTR)
        {
            _value = "UniqueString(" + Env->Stack[_item1.IntValue].Value + ")";
            Env->Stack[_item1.IntValue].Type = "String";
            _line = "//EAX := " + _value;
            Env->AddToBody(_line);
            return false;
        }
        _value = "UniqueString(" + _item1.Value + ")";
        _line = "//EAX := " + _value;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrFromChar") ||
        SameText(name, "@UStrFromWChar"))
    {
        //eax-Dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        //edx-Src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            if (name[11] == 'C')
                Env->Stack[_item2.IntValue].Type = "Char";
            else
                Env->Stack[_item2.IntValue].Type = "WideChar";
        }
        _line = _item1.Value + " := " + _item2.Value;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrFromLStr"))
    {
        //eax-Dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        //edx-Src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "AnsiString";
        }
        _line = _item1.Value + " := " + _item2.Value;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrFromString"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "ShortString";
        }
        _line = _item1.Value + " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrFromWStr"))
    {
        //eax-Dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        //edx-Src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "WideString";
        }
        _line = _item1.Value + " := " + _item2.Value;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrFromPWChar"))
    {
        //eax - Dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        //edx-Src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "PWideChar";
        }
        _line = _item1.Value + " := " + _item2.Value;
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@WStrToPWChar"))
    {
        //eax - src
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "WideString";
        }
        _item1.Value = "PWideChar(" + _item1.Value + ")";
        _item1.Type = "PWideChar";
        SetRegItem(16, &_item1);
        _line = "EAX := " + _item1.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrToPWChar"))
    {
        //eax - src
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        }
        _item1.Value = "PWideChar(" + _item1.Value + ")";
        _item1.Type = "PWideChar";
        SetRegItem(16, &_item1);
        _line = "EAX := " + _item1.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@UStrToString"))
    {
        //eax - dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "UnicodeString";
        //edx - src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
            Env->Stack[_item2.IntValue].Type = "String";
        _line = _item1.Value + " := " + _item2.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarClear"))
    {
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "Variant";
        _line = _item1.Value + " := 0;";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarAdd") ||
        SameText(name, "@VarSub") ||
        SameText(name, "@VarMul") ||
        SameText(name, "@VarDiv") ||
        SameText(name, "@VarMod") ||
        SameText(name, "@VarAnd") ||
        SameText(name, "@VarOr")  ||
        SameText(name, "@VarXor") ||
        SameText(name, "@VarShl") ||
        SameText(name, "@VarShr") ||
        SameText(name, "@VarRDiv"))
    {
        _op = name.SubString(5, name.Length());

        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "Variant";
            _item1 = Env->Stack[_item1.IntValue];
        }
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "Variant";
            _item2 = Env->Stack[_item2.IntValue];
        }
        _line = _item1.Name + " := " + _item1.Name + " " + _op + " " + _item2.Name + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarNeg") ||
        SameText(name, "@VarNot"))
    {
        _op = name.SubString(5, name.Length());
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "Variant";
            _item1 = Env->Stack[_item1.IntValue];
        }
        _line = _item1.Name + " := " + _op + " " + _item1.Name + ";";
        Env->AddToBody(_line);
        return false;
    }

    if (SameText(name.SubString(1, 7), "@VarCmp"))
    {
        GetCmpInfo(procAdr + instrLen);
        if (name[8] == 'E' && name[9] == 'Q')
            CmpInfo.O = 'E';//JZ
        else if (name[8] == 'N' && name[9] == 'E')
            CmpInfo.O = 'F';//JNZ
        else if (name[8] == 'L')
        {
            if (name[9] == 'E')
                CmpInfo.O = 'O';//JLE
            else if (name[9] == 'T')
                CmpInfo.O = 'M';//JL
        }
        else if (name[8] == 'G')
        {
            if (name[9] == 'E')
                CmpInfo.O = 'N';//JGE
            else if (name[9] == 'T')
                CmpInfo.O = 'P';//JG
        }

        GetRegItem(16, &_item1);//eax - Left argument
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "Variant";
            _item1 = Env->Stack[_item1.IntValue];
        }
        CmpInfo.L = _item1.Name;

        GetRegItem(18, &_item2);//edx - Right argument
        if (_item2.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item2.IntValue].Type = "Variant";
            _item2 = Env->Stack[_item2.IntValue];
        }
        CmpInfo.R = _item2.Name;
        return true;
    }
    //Cast to Variant
    if (SameText(name, "@VarFromBool")  ||
        SameText(name, "@VarFromInt")   ||
        SameText(name, "@VarFromPStr")  ||
        SameText(name, "@VarFromLStr")  ||
        SameText(name, "@VarFromWStr")  ||
        SameText(name, "@VarFromUStr")  ||
        SameText(name, "@VarFromDisp"))
    {
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "Variant";
            _item1 = Env->Stack[_item1.IntValue];
        }
        GetRegItem(18, &_item2);
        _line = _item1.Name + " := Variant(" + _item2.Name + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarFromTDateTime") ||
        SameText(name, "@VarFromCurr"))
    {
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            Env->Stack[_item1.IntValue].Type = "Variant";
            _item1 = Env->Stack[_item1.IntValue];
        }
        _line = _item1.Name + " := Variant(" + FPop()->Value + ")";//FGet(0)
        Env->AddToBody(_line);
        //FPop();
        return false;
    }
    if (SameText(name, "@VarFromReal"))
    {
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            _line = Env->GetLvarName(_item1.IntValue, "Variant");
        _line += " := Variant(" + FPop()->Value + ")";//???
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarToInt"))
    {
        //eax=Variant, return Integer
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "Variant";
        InitItem(&_item);
        _item.Value = "Integer(" + Env->GetLvarName(_item1.IntValue, "Variant") + ")";
        SetRegItem(16, &_item);
        _line = "EAX := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarToInteger"))
    {
        //edx=Variant, eax=Integer
        GetRegItem(18, &_item1);
        GetRegItem(16, &_item2);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "Variant";
        InitItem(&_item);
        _item.Value = "Integer(" + _item1.Value + ")";
        SetRegItem(16, &_item);
        _line = Env->GetLvarName(_item2.IntValue, "Variant") + " := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarToLStr"))
    {
        //edx=Variant, eax=String
        GetRegItem(18, &_item1);
        GetRegItem(16, &_item2);

        if (_item2.Flags & IF_INTVAL)//!!!Use it for other cases!!!
        {
            _line = MakeGvarName(_item2.IntValue);
        }
        else if (_item2.Flags & IF_STACK_PTR)
        {
            _line = Env->GetLvarName(_item2.IntValue, "String");
            Env->Stack[_item2.IntValue].Type = "String";
        }
        else
        {
            _line = _item2.Value;
        }

        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "Variant";
        InitItem(&_item);
        _item.Value = "String(" + _item1.Value + ")";
        SetRegItem(16, &_item);
        _line += " := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@VarToReal"))
    {
        //eax=Variant
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
            Env->Stack[_item1.IntValue].Type = "Variant";
        InitItem(&_item);
        _item.Value = "Real(" + _item1.Value + ")";
        _item.Type = "Extended";
        FSet(0, &_item);
        return false;
    }
    if (SameText(name, "@Write0Ext"))
    {
        //File
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        //Value (Extended)
        GetFloatItemFromStack(_ESP_, &_item2, FT_EXTENDED); _ESP_ += 12;
        _line = "Write(" + ExtractClassName(_item1.Value) + ", " + _item2.Value + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Str0Ext"))
    {
        //Value (Extended)
        GetFloatItemFromStack(_ESP_, &_item1, FT_EXTENDED); _ESP_ += 12;
        //Destination - eax
        GetRegItem(16, &_item2);
        _line = "Str(" + _item1.Value + ", " + _item2.Value + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Str1Ext"))
    {
        //Value (Extended)
        GetFloatItemFromStack(_ESP_, &_item1, FT_EXTENDED); _ESP_ += 12;
        //Width - eax
        GetRegItem(16, &_item2);
        //Destination - edx
        GetRegItem(18, &_item3);
        _line = "Str(" + _item1.Value + ":" + String(_item2.IntValue) + ", " + _item3.Value + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Str2Ext"))
    {
        //Value (Extended)
        GetFloatItemFromStack(_ESP_, &_item1, FT_EXTENDED); _ESP_ += 12;
        //Width - eax
        GetRegItem(16, &_item2);
        //Precision - edx
        GetRegItem(18, &_item3);
        //Destination - ecx
        GetRegItem(17, &_item4);
        _line = "Str(" + _item1.Value + ":" + String(_item2.IntValue) + ":" + String(_item3.IntValue) + ", " + _item4.Value + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Write2Ext"))
    {
        //File
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        //Value (Extended)
        GetFloatItemFromStack(_ESP_, &_item2, FT_EXTENDED); _ESP_ += 12;
        //edx
        GetRegItem(18, &_item3);
        //ecx
        GetRegItem(17, &_item4);
        _line = "Write(" + ExtractClassName(_item1.Value) + ", " + _item2.Value + ":" + String(_item3.IntValue) + ":" + String(_item4.IntValue) + ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Write0Char")  ||
        SameText(name, "@Write0WChar") ||
        SameText(name, "@WriteLong")   ||
        SameText(name, "@Write0Long"))
    {
        //File
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        _line = "Write(" + ExtractClassName(_item1.Value) + ", ";
        //edx
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_INTVAL)
            _line += GetImmString(_item2.IntValue);
        else
            _line += _item2.Value;
        _line += ");";
        Env->AddToBody(_line);
        return false;
    }
    if (SameText(name, "@Write0LString") ||
        SameText(name, "@Write0UString"))
    {
        //File
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR) _item1 = Env->Stack[_item1.IntValue];
        //edx
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR) _item2 = Env->Stack[_item2.IntValue];
        _line = "Write(" + ExtractClassName(_item1.Value) + ", " + _item2.Value + ");";
        Env->AddToBody(_line);
        return false;
    }

    if (SameText(name, "@WStrFromUStr") ||
        SameText(name, "@WStrFromLStr"))
    {
        //eax-Dst
        GetRegItem(16, &_item1);
        if (_item1.Flags & IF_STACK_PTR)
        {
            _item = Env->Stack[_item1.IntValue];
            _item.Value = Env->GetLvarName(_item1.IntValue, "String");
            _item.Type = "WideString";
            Env->Stack[_item1.IntValue] = _item;
            _item1 = _item;
        }
        //edx-Src
        GetRegItem(18, &_item2);
        if (_item2.Flags & IF_STACK_PTR)
        {
            _item2 = Env->Stack[_item2.IntValue];
            if (SameText(name, "@WStrFromUStr"))
                Env->Stack[_item2.IntValue].Type = "UnicodeString";
            else
                Env->Stack[_item2.IntValue].Type = "String";
        }
        _line = _item1.Value + " := " + _item2.Value;
        Env->AddToBody(_line);
        return false;
    }
    if (name.Pos("@_lldiv") == 1 ||
        SameText(name, "@__lludiv"))
    {
        //Argument (Int64) in edx:eax
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        if (_item1.Flags & IF_INTVAL)
        {
            _int64Val = (_item2.IntValue << 32) | _item1.IntValue;
            _value1 = IntToStr(_int64Val);
        }
        else if (_item1.Flags & IF_STACK_PTR)
        {
            _item1 = Env->Stack[_item1.IntValue];
            _value1 = _item1.Value;
        }
        else
            _value1 = _item1.Value;

        //Argument (Int64) in stack
        _item3 = Env->Stack[_ESP_]; _ESP_ += 4;
        _item4 = Env->Stack[_ESP_]; _ESP_ += 4;
        if (_item3.Flags & IF_INTVAL)
        {
            _int64Val = (_item4.IntValue << 32) | _item3.IntValue;
            _value2 = IntToStr(_int64Val);
        }
        else if (_item3.Flags & IF_STACK_PTR)
        {
            _item3 = Env->Stack[_item3.IntValue];
            _value2 = _item3.Value;
        }
        else
            _value2 = _item3.Value;

        InitItem(&_item);
        _item.Value = _value1 + " Div " + _value2;
        _item.Type = "Int64";
        SetRegItem(16, &_item);
        SetRegItem(18, &_item);
        _line = "EDX_EAX := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (name.Pos("@_llmod") == 1 ||
        SameText(name, "@__llumod"))
    {
        //Argument (Int64) in edx:eax
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        if (_item1.Flags & IF_INTVAL)
        {
            _int64Val = (_item2.IntValue << 32) | _item1.IntValue;
            _value1 = IntToStr(_int64Val);
        }
        else if (_item1.Flags & IF_STACK_PTR)
        {
            _item1 = Env->Stack[_item1.IntValue];
            _value1 = _item1.Value;
        }
        else
            _value1 = _item1.Value;

        //Argument (Int64) in stack
        _item3 = Env->Stack[_ESP_]; _ESP_ += 4;
        _item4 = Env->Stack[_ESP_]; _ESP_ += 4;
        if (_item3.Flags & IF_INTVAL)
        {
            _int64Val = (_item4.IntValue << 32) | _item3.IntValue;
            _value2 = IntToStr(_int64Val);
        }
        else if (_item3.Flags & IF_STACK_PTR)
        {
            _item3 = Env->Stack[_item3.IntValue];
            _value2 = _item3.Value;
        }
        else
            _value2 = _item3.Value;

        InitItem(&_item);
        _item.Value = _value1 + " Mod " + _value2;
        _item.Type = "Int64";
        SetRegItem(16, &_item);
        SetRegItem(18, &_item);
        _line = "EDX_EAX := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    if (name.Pos("@_llmul") == 1)
    {
        //Argument (Int64) in edx:eax
        GetRegItem(16, &_item1);
        GetRegItem(18, &_item2);
        if (_item1.Flags & IF_INTVAL)
        {
            _int64Val = (_item2.IntValue << 32) | _item1.IntValue;
            _value1 = IntToStr(_int64Val);
        }
        else if (_item1.Flags & IF_STACK_PTR)
        {
            _item1 = Env->Stack[_item1.IntValue];
            _value1 = _item1.Value;
        }
        else
            _value1 = _item1.Value;

        //Argument (Int64) in stack
        _item3 = Env->Stack[_ESP_]; _ESP_ += 4;
        _item4 = Env->Stack[_ESP_]; _ESP_ += 4;
        if (_item3.Flags & IF_INTVAL)
        {
            _int64Val = (_item4.IntValue << 32) | _item3.IntValue;
            _value2 = IntToStr(_int64Val);
        }
        else if (_item3.Flags & IF_STACK_PTR)
        {
            _item3 = Env->Stack[_item3.IntValue];
            _value2 = _item3.Value;
        }
        else
            _value2 = _item3.Value;

        InitItem(&_item);
        _item.Value = _value1 + " * " + _value2;
        _item.Type = "Int64";
        SetRegItem(16, &_item);
        SetRegItem(18, &_item);
        _line = "EDX_EAX := " + _item.Value + ";";
        Env->AddToBody(_line);
        return false;
    }
    //No action
    if (SameText(name, "@DoneExcept")           ||
        SameText(name, "@LStrAddRef")           ||
        SameText(name, "@WStrAddRef")           ||
        SameText(name, "@UStrAddRef")           ||
        SameText(name, "@LStrArrayClr")         ||
        SameText(name, "@WStrArrayClr")         ||
        SameText(name, "@UStrArrayClr")         ||
        SameText(name, "@VarClr")               ||
        SameText(name, "@DynArrayAddRef")       ||
        SameText(name, "@EnsureAnsiString")     ||
        SameText(name, "@EnsureUnicodeString")  ||
        SameText(name, "@_IOTest")              ||
        SameText(name, "@CheckAutoResult")      ||
        SameText(name, "@InitExe")              ||
        SameText(name, "@InitLib")              ||
        SameText(name, "@IntfAddRef")           ||
        SameText(name, "@TryFinallyExit"))
    {
        return false;
    }
    Env->ErrAdr = procAdr;
    throw Exception("Under construction");
    return false;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateFormatCall()
{
    int     n, _num = 0, _ofs, _esp;
    String  _line = "Format(";
    ITEM    _item;

    //eax - format
    GetRegItem(16, &_item);
    _line += _item.Value + ", [";

    //edx - pointer to args
    GetRegItem(18, &_item);
    _ofs = _item.IntValue;

    //ecx - args num
    GetRegItem(17, &_item);
    _num = _item.IntValue + 1;

    for (n = 0; n < _num; n++)
    {
        if (n) _line += ", ";
        _line += Env->Stack[_ofs].Value;
        _ofs += 5;
    }
    _line += "]);";
    //Result
    _esp = _ESP_; _ESP_ += 4;
    _item = Env->Stack[_esp];
    _line = _item.Value + " := " + _line;
    
    Env->AddToBody(_line);
    Env->LastResString = "";
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::SimulateFloatInstruction(DWORD curAdr)
{
    bool        _vmt, _r, _p, _pp;
    int         _reg1Idx, _reg2Idx, _offset, _cmpRes, _ea, _sz, _ofs;
    char        *_pos;
    DWORD       _dd;
    ITEM        _item, _itemBase, _itemSrc;
    PInfoRec    _recN;
    PFIELDINFO  _fInfo;
    String      _name, _typeName, _val, _line, _lvarName;

    _r = false; _p = false; _pp = false;
    _dd = *((DWORD*)(DisInfo.Mnem + 1));
    //fild, fld
    if (_dd == 'dli' || _dd == 'dl')
    {
        //op Mem
        if (DisInfo.OpType[0] == otMEM)
        {
            GetMemItem(curAdr, &_itemSrc, 0);
            if (_itemSrc.Flags & IF_STACK_PTR)
            {
                _item = Env->Stack[_itemSrc.IntValue];
                if (_item.Value == "")
                {
                    if (_itemSrc.Value != "")
                        _item.Value = _itemSrc.Value;
                    if (_itemSrc.Name != "")
                        _item.Value = _itemSrc.Name;
                }
                _item.Precedence = PRECEDENCE_NONE;
                FPush(&_item);
                return;
            }
            if (_itemSrc.Flags & IF_INTVAL)
            {
                _recN = GetInfoRec(_itemSrc.IntValue);
                if (_recN && _recN->HasName())
                {
                    InitItem(&_item);
                    _item.Value = _recN->GetName();
                    _item.Type = _recN->type;
                    FPush(&_item);
                    return;
                }
            }
            InitItem(&_item);
            if (_itemSrc.Name != "" && !IsDefaultName(_itemSrc.Name))
                _item.Value = _itemSrc.Name;
            else
                _item.Value = _itemSrc.Value;
            _item.Type = _itemSrc.Type;
            FPush(&_item);
            return;
        }
    }
    //fst, fstp
    if ((_pos = strstr(DisInfo.Mnem + 1, "st")) != 0)
    {
        if (_pos[2] == 'p') _p = True;
        //op Mem
        if (DisInfo.OpType[0] == otMEM)
        {
            //fst(p) [esp]
            if (DisInfo.BaseReg == 20 && !DisInfo.Offset)
            {
                _item = Env->FStack[_TOP_];
                if (_p) FPop();
                _ofs = _ESP_; _sz = DisInfo.OpSize;
                Env->Stack[_ofs] = _item; _ofs += 4; _sz -= 4;

                InitItem(&_item);
                while (_sz > 0)
                {
                    Env->Stack[_ofs] = _item; _ofs += 4; _sz -= 4;
                }
                return;
            }
            GetMemItem(curAdr, &_itemSrc, 0);
            if (_itemSrc.Flags & IF_STACK_PTR)
            {
                _item = Env->FStack[_TOP_];
                if (_p) FPop();

                _lvarName = Env->GetLvarName(_itemSrc.IntValue, "Double");
                _line = _lvarName + " := " + _item.Value + ";";
                Env->AddToBody(_line);
                _item.Precedence = PRECEDENCE_NONE;
                _item.Value = _lvarName;
                _ofs = _itemSrc.IntValue; _sz = DisInfo.OpSize;
                Env->Stack[_ofs] = _item; _ofs += 4; _sz -= 4;

                InitItem(&_item);
                while (_sz > 0)
                {
                    Env->Stack[_ofs] = _item; _ofs += 4; _sz -= 4;
                }
                return;
            }
            if (_itemSrc.Flags & IF_INTVAL)
            {
                _recN = GetInfoRec(_itemSrc.IntValue);
                if (_recN && _recN->HasName())
                {
                    _item = Env->FStack[_TOP_];
                    if (_p) FPop();
                    _line = _recN->GetName() + " := " + _item.Value + ";";
                    Env->AddToBody(_line);
                    return;
                }
            }
            _item = Env->FStack[_TOP_];
            if (_p) FPop();

            if (_itemSrc.Name != "" && !IsDefaultName(_itemSrc.Name))
                _line = _itemSrc.Name;
            else
                _line = _itemSrc.Value;

            _line += " := " + _item.Value + ";";
            Env->AddToBody(_line);
            return;
        }
        //fstp - do nothing
        if (DisInfo.OpType[0] == otFST)
        {
            return;
        }
    }
    //fcom, fcomp, fcompp
    if ((_pos = strstr(DisInfo.Mnem + 1, "com")) != 0)
    {
        if (_pos[3] == 'p') _p = True;
        if (_pos[4] == 'p') _pp = True;
        if (DisInfo.OpNum == 0)
        {
            CmpInfo.L = FGet(0)->Value;
            CmpInfo.O = CmpOp;
            CmpInfo.R = FGet(1)->Value;
            if (_p)
            {
                FPop();
                //fcompp
                if (_pp) FPop();
            }
            return;
        }
        if (DisInfo.OpNum == 1)
        {
            if (DisInfo.OpType[0] == otMEM)
            {
                GetMemItem(curAdr, &_itemSrc, 0);
                if (_itemSrc.Flags & IF_STACK_PTR)
                {
                    CmpInfo.L = FGet(0)->Value;
                    CmpInfo.O = CmpOp;
                    _item = Env->Stack[_itemSrc.IntValue];
                    if (_item.Value != "")
                        CmpInfo.R = _item.Value;
                    else
                        CmpInfo.R = Env->GetLvarName(_itemSrc.IntValue, "Double");
                    if (_p)
                    {
                        FPop();
                        //fcompp
                        if (_pp) FPop();
                    }
                    return;
                }
                if (_itemSrc.Flags & IF_INTVAL)
                {
                    _recN = GetInfoRec(_itemSrc.IntValue);
                    if (_recN && _recN->HasName())
                    {
                        CmpInfo.L = FGet(0)->Value;
                        CmpInfo.O = CmpOp;
                        CmpInfo.R = _recN->GetName();
                        if (_p)
                        {
                            FPop();
                            //fcompp
                            if (_pp) FPop();
                        }
                        return;
                    }
                }
                CmpInfo.L = FGet(0)->Value;
                CmpInfo.O = CmpOp;
                CmpInfo.R = _itemSrc.Value;
                if (_p)
                {
                    FPop();
                    //fcompp
                    if (_pp) FPop();
                }
                return;
            }
        }
    }
    //fadd, fiadd, faddp
    if ((_pos = strstr(DisInfo.Mnem + 1, "add")) != 0)
    {
        if (_pos[3] == 'p') _p = True;
        //faddp (ST(1) = ST(0) + ST(1) and pop)
        if (DisInfo.OpNum == 0)
        {
            InitItem(&_item);
            _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " + " + GetString(FGet(1), PRECEDENCE_ADD + 1);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Type = "Extended";
            FSet(1, &_item);
            FPop();
            return;
        }
        //fadd r/m (ST(0) = ST(0) + r/m)
        if (DisInfo.OpNum == 1)
        {
            if (DisInfo.OpType[0] == otMEM)
            {
                GetMemItem(curAdr, &_itemSrc, 0);
                if (_itemSrc.Flags & IF_STACK_PTR)
                {
                    _item = Env->Stack[_itemSrc.IntValue];
                    _name = _item.Name;
                    if (_item.Value != "")
                        _val = GetString(&_item, PRECEDENCE_ADD + 1);
                    else
                        _val = Env->GetLvarName(_itemSrc.IntValue, "Extended");

                    InitItem(&_item);
                    _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " + " + _name + "{" + _val + "}";
                    _item.Precedence = PRECEDENCE_ADD;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                if (_itemSrc.Flags & IF_INTVAL)
                {
                    _recN = GetInfoRec(_itemSrc.IntValue);
                    if (_recN && _recN->HasName())
                        _name = _recN->GetName();
                    else
                        _name = GetGvarName(_itemSrc.IntValue);
                    InitItem(&_item);
                    _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " + " + _name;
                    _item.Precedence = PRECEDENCE_ADD;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                InitItem(&_item);
                _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " + " + _itemSrc.Value;
                _item.Precedence = PRECEDENCE_ADD;
                _item.Type = "Extended";
                FSet(0, &_item);
                return;
            }
        }
        //fadd ST(X), ST(Y) (ST(X) = ST(X)+ST(Y))
        if (DisInfo.OpNum == 2)
        {
            _reg1Idx = DisInfo.OpRegIdx[0] - 30;
            _reg2Idx = DisInfo.OpRegIdx[1] - 30;
            InitItem(&_item);
            _item.Value = GetString(FGet(_reg1Idx), PRECEDENCE_ADD) + " + " + GetString(FGet(_reg2Idx), PRECEDENCE_ADD + 1);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Type = "Extended";
            FSet(_reg1Idx, &_item);
            //faddp
            if (_p) FPop();
            return;
        }
    }
    //fsub, fisub, fsubp, fsubr, fisubr, fsubrp
    if ((_pos = strstr(DisInfo.Mnem + 1, "sub")) != 0)
    {
        if (_pos[3] == 'r')
        {
            _r = true;
            if (_pos[4] == 'p') _p = True;
        }
        if (_pos[3] == 'p') _p = True;

        //fsubp, fsubrp (ST(1) = ST(1) - ST(0) and pop)
        if (DisInfo.OpNum == 0)
        {
            InitItem(&_item);
            if (_r)
                _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " - " + GetString(FGet(1), PRECEDENCE_ADD + 1);
            else
                _item.Value = GetString(FGet(1), PRECEDENCE_ADD) + " - " + GetString(FGet(0), PRECEDENCE_ADD + 1);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Type = "Extended";
            FSet(1, &_item);
            FPop();
            return;
        }
        //fsub r/m (ST(0) = ST(0) - r/m)
        if (DisInfo.OpNum == 1)
        {
            if (DisInfo.OpType[0] == otMEM)
            {
                GetMemItem(curAdr, &_itemSrc, 0);
                if (_itemSrc.Flags & IF_STACK_PTR)
                {
                    _item = Env->Stack[_itemSrc.IntValue];
                    _name = _item.Name;
                    if (_item.Value != "")
                        _val = GetString(&_item, PRECEDENCE_ADD);
                    else
                        _val = Env->GetLvarName(_itemSrc.IntValue, "Extended");

                    InitItem(&_item);
                    if (_r)
                        _item.Value = _name + "{" + _val + "}" + " - " + GetString(FGet(0), PRECEDENCE_ADD + 1);
                    else
                        _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " - " + _name + "{" + _val + "}";
                    _item.Precedence = PRECEDENCE_ADD;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                if (_itemSrc.Flags & IF_INTVAL)
                {
                    _recN = GetInfoRec(_itemSrc.IntValue);
                    if (_recN && _recN->HasName())
                        _name = _recN->GetName();
                    else
                        _name = GetGvarName(_itemSrc.IntValue);
                    InitItem(&_item);
                    if (_r)
                        _item.Value = _name + " - " + GetString(FGet(0), PRECEDENCE_ADD + 1);
                    else
                        _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " - " + _name;
                    _item.Precedence = PRECEDENCE_ADD;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                InitItem(&_item);
                if (_r)
                    _item.Value = _itemSrc.Value + " - " + GetString(FGet(0), PRECEDENCE_ADD + 1);
                else
                    _item.Value = GetString(FGet(0), PRECEDENCE_ADD) + " - " + _itemSrc.Value;
                _item.Precedence = PRECEDENCE_ADD;
                _item.Type = "Extended";
                FSet(0, &_item);
                return;
            }
        }
        //fsub ST(X), ST(Y) (ST(X) = ST(X)-ST(Y))
        if (DisInfo.OpNum == 2)
        {
            _reg1Idx = DisInfo.OpRegIdx[0] - 30;
            _reg2Idx = DisInfo.OpRegIdx[1] - 30;
            InitItem(&_item);
            if (_r)
                _item.Value = GetString(FGet(_reg2Idx), PRECEDENCE_ADD) + " - " + GetString(FGet(_reg1Idx), PRECEDENCE_ADD + 1);
            else
                _item.Value = GetString(FGet(_reg1Idx), PRECEDENCE_ADD) + " - " + GetString(FGet(_reg2Idx), PRECEDENCE_ADD + 1);
            _item.Precedence = PRECEDENCE_ADD;
            _item.Type = "Extended";
            FSet(_reg1Idx, &_item);
            //fsubp
            if (_p) FPop();
            return;
        }
    }
    //fmul, fimul, fmulp
    if ((_pos = strstr(DisInfo.Mnem + 1, "mul")) != 0)
    {
        if (_pos[3] == 'p') _p = True;
        //fmulp (ST(1) = ST(0) * ST(1) and pop)
        if (DisInfo.OpNum == 0)
        {
            InitItem(&_item);
            _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " * " + GetString(FGet(1), PRECEDENCE_MULT + 1);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Type = "Extended";
            FSet(1, &_item);
            FPop();
            return;
        }
        //fmul r/m (ST(0) = r/m * ST(0))
        if (DisInfo.OpNum == 1)
        {
            if (DisInfo.OpType[0] == otMEM)
            {
                GetMemItem(curAdr, &_itemSrc, 0);
                if (_itemSrc.Flags & IF_STACK_PTR)
                {
                    _item = Env->Stack[_itemSrc.IntValue];
                    _name = _item.Name;
                    if (_item.Value != "")
                        _val = GetString(&_item, PRECEDENCE_MULT);
                    else
                        _val = Env->GetLvarName(_itemSrc.IntValue, "Extended");

                    InitItem(&_item);
                    _item.Value = GetString(FGet(0), PRECEDENCE_MULT + 1) + " * " + _name + "{" + _val + "}";
                    _item.Precedence = PRECEDENCE_MULT;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                if (_itemSrc.Flags & IF_INTVAL)
                {
                    _recN = GetInfoRec(_itemSrc.IntValue);
                    if (_recN && _recN->HasName())
                        _name = _recN->GetName();
                    else
                        _name = GetGvarName(_itemSrc.IntValue);
                    InitItem(&_item);
                    _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " * " + _name;
                    _item.Precedence = PRECEDENCE_MULT;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                InitItem(&_item);
                _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " * " + _itemSrc.Value;
                _item.Precedence = PRECEDENCE_ADD;
                _item.Type = "Extended";
                FSet(0, &_item);
                return;
            }
        }
        //fmul ST(X), ST(Y) (ST(X) = ST(X)*ST(Y))
        if (DisInfo.OpNum == 2)
        {
            _reg1Idx = DisInfo.OpRegIdx[0] - 30;
            _reg2Idx = DisInfo.OpRegIdx[1] - 30;
            InitItem(&_item);
            _item.Value = GetString(FGet(_reg1Idx), PRECEDENCE_MULT) + " * " + GetString(FGet(_reg2Idx), PRECEDENCE_MULT + 1);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Type = "Extended";
            FSet(_reg1Idx, &_item);
            //fmulp
            if (_p) FPop();
            return;
        }
    }
    //fdiv, fdivr, fidiv, fidivr, fdivp, fdivrp
    if ((_pos = strstr(DisInfo.Mnem + 1, "div")) != 0)
    {
        if (_pos[3] == 'r')
        {
            _r = true;
            if (_pos[4] == 'p') _p = True;
        }
        if (_pos[3] == 'p') _p = True;

        //fdivp (ST(1) = ST(1) / ST(0) and pop)
        //fdivrp (ST(1) = ST(0) / ST(1) and pop)
        if (DisInfo.OpNum == 0)
        {
            InitItem(&_item);
            if (_r)
                _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " / " + GetString(FGet(1), PRECEDENCE_MULT + 1);
            else
                _item.Value = GetString(FGet(1), PRECEDENCE_MULT) + " / " + GetString(FGet(0), PRECEDENCE_MULT + 1);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Type = "Extended";
            FSet(1, &_item);
            FPop();
            return;
        }
        //fdiv r/m (ST(0) = ST(0) / r/m)
        //fdivr r/m (ST(0) = r/m / ST(0))
        if (DisInfo.OpNum == 1)
        {
            if (DisInfo.OpType[0] == otMEM)
            {
                GetMemItem(curAdr, &_itemSrc, 0);
                if (_itemSrc.Flags & IF_STACK_PTR)
                {
                    _item = Env->Stack[_itemSrc.IntValue];
                    _name = _item.Name;
                    if (_item.Value != "")
                        _val = GetString(&_item, PRECEDENCE_MULT);
                    else
                        _val = Env->GetLvarName(_itemSrc.IntValue, "Extended");

                    InitItem(&_item);
                    if (_r)
                        _item.Value = _name + "{" + _val + "}" + " / " + GetString(FGet(0), PRECEDENCE_MULT + 1);
                    else
                        _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " / " + _name + "{" + _val + "}";
                    _item.Precedence = PRECEDENCE_MULT;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                if (_itemSrc.Flags & IF_INTVAL)
                {
                    _recN = GetInfoRec(_itemSrc.IntValue);
                    if (_recN && _recN->HasName())
                        _name = _recN->GetName();
                    else
                        _name = GetGvarName(_itemSrc.IntValue);
                    InitItem(&_item);
                    if (_r)
                        _item.Value = _name + " / " + GetString(FGet(0), PRECEDENCE_MULT + 1);
                    else
                        _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " / " + _name;
                    _item.Precedence = PRECEDENCE_MULT;
                    _item.Type = "Extended";
                    FSet(0, &_item);
                    return;
                }
                InitItem(&_item);
                if (_r)
                    _item.Value = _itemSrc.Value + " / " + GetString(FGet(0), PRECEDENCE_MULT + 1);
                else
                    _item.Value = GetString(FGet(0), PRECEDENCE_MULT) + " / " + _itemSrc.Value;
                _item.Precedence = PRECEDENCE_MULT;
                _item.Type = "Extended";
                FSet(0, &_item);
                return;
            }
        }
        //fdiv(p) ST(X), ST(Y) (ST(X) = ST(X)/ST(Y))
        //fdivr(p) ST(X), ST(Y) (ST(X) = ST(X)/ST(Y))
        if (DisInfo.OpNum == 2)
        {
            _reg1Idx = DisInfo.OpRegIdx[0] - 30;
            _reg2Idx = DisInfo.OpRegIdx[1] - 30;
            InitItem(&_item);
            if (_r)
                _item.Value = GetString(FGet(_reg2Idx), PRECEDENCE_MULT) + " / " + GetString(FGet(_reg1Idx), PRECEDENCE_MULT + 1);
            else
                _item.Value = GetString(FGet(_reg1Idx), PRECEDENCE_MULT) + " / " + GetString(FGet(_reg2Idx), PRECEDENCE_MULT + 1);
            _item.Precedence = PRECEDENCE_MULT;
            _item.Type = "Extended";
            FSet(_reg1Idx, &_item);
            //fdivp
            if (_p) FPop();
            return;
        }
    }
    //fabs
    if (_dd == 'sba')
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ATOM;
        _item.Value = "Abs(" + FGet(0)->Value + ")";
        _item.Type = "Extended";
        FSet(0, &_item);
        return;
    }
    //fchs
    if (_dd == 'shc')
    {
        _item = Env->FStack[_TOP_];
        _item.Value = "-" + GetString(&_item, PRECEDENCE_ATOM);
        _item.Precedence = PRECEDENCE_UNARY;
        //_item.Value = "-" + _item.Value;
        FSet(0, &_item);
        return;
    }
    //fld1
    if (_dd == '1dl')
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ATOM;
        _item.Value = "1.0";
        _item.Type = "Extended";
        FPush(&_item);
        return;
    }
    //fldln2
    if (_dd == 'nldl')
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ATOM;
        _item.Value = "Ln(2)";
        _item.Type = "Extended";
        FPush(&_item);
        return;
    }
    //fpatan
    if (_dd == 'atap')
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ATOM;
        if (SameText(FGet(0)->Value, "1.0"))
            _item.Value = "ArcTan(" + FGet(1)->Value + ")";
        else
            _item.Value = "ArcTan(" + FGet(1)->Value + " / " + FGet(0)->Value + ")";
        _item.Type = "Extended";
        FSet(1, &_item);
        FPop();
        return;
    }
    //fsqrt
    if (_dd == 'trqs')
    {
        InitItem(&_item);
        _item.Precedence = PRECEDENCE_ATOM;
        _item.Value = "Sqrt(" + FGet(0)->Value + ")";
        _item.Type = "Extended";
        FSet(0, &_item);
        return;
    }
    //fxch
    if (_dd == 'hcx')
    {
        _reg1Idx = 1;
        //fxch st(i)
        if (DisInfo.OpNum == 1)
        {
            _reg1Idx = DisInfo.OpRegIdx[0] - 30;
            //fxch st(0) == fxcg
            if (!_reg1Idx) _reg1Idx = 1;
        }
        FXch(0, _reg1Idx);
        return;
    }
    //fyl2x
    if (_dd == 'x2ly')
    {
        InitItem(&_item);
        _item.Value = GetString(FGet(1), PRECEDENCE_MULT) + " * Log2(" + GetString(FGet(0), PRECEDENCE_NONE) + ")";
        _item.Type = "Extended";
        _item.Precedence = PRECEDENCE_MULT;
        FSet(1, &_item);
        FPop();
        return;
    }
    Env->ErrAdr = curAdr;
    throw Exception("Under construction");
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::AddToBody(String src)
{
    Body->Add(src);
}
//---------------------------------------------------------------------------
void __fastcall TDecompileEnv::AddToBody(TStringList* src)
{
    for (int n = 0; n < src->Count; n++)
    {
        Body->Add(src->Strings[n]);
    }
}
//---------------------------------------------------------------------------
bool __fastcall TDecompileEnv::IsExitAtBodyEnd()
{
    return SameText(Body->Strings[Body->Count - 1], "Exit;");
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::GetCycleIdx(PIDXINFO IdxInfo, DISINFO* ADisInfo)
{
    ITEM        _item;

    IdxInfo->IdxType = itUNK;
    IdxInfo->IdxValue = 0;
    IdxInfo->IdxStr = ADisInfo->Op1;

    if (ADisInfo->OpType[0] == otREG)
    {
        IdxInfo->IdxType = itREG;
        IdxInfo->IdxValue = ADisInfo->OpRegIdx[0];
        return;
    }
    if (ADisInfo->OpType[0] == otMEM)
    {
        //Offset
        if (ADisInfo->BaseReg == -1)
        {
            IdxInfo->IdxType = itGVAR;
            IdxInfo->IdxValue = ADisInfo->Offset;
            IdxInfo->IdxStr = ADisInfo->Op1;
            return;
        }
        //[ebp-N]
        if (ADisInfo->BaseReg == 21)
        {
            GetRegItem(ADisInfo->BaseReg, &_item);
            IdxInfo->IdxType = itLVAR;
            IdxInfo->IdxValue = _item.IntValue + ADisInfo->Offset;//LocBase - ((int)_item.Value + ADisInfo->Offset);
            return;
        }
        //[esp-N]
        if (ADisInfo->BaseReg == 20)
        {
            IdxInfo->IdxType = itLVAR;
            IdxInfo->IdxValue = _ESP_ + ADisInfo->Offset;//LocBase - (_ESP_ + ADisInfo->Offset);
            return;
        }
    }
}
//---------------------------------------------------------------------------
String __fastcall TDecompiler::GetCycleFrom()
{
    ITEM        _item;

    if (DisInfo.OpType[1] == otIMM) return String(DisInfo.Immediate);
    if (DisInfo.OpType[1] == otREG)
    {
        GetRegItem(DisInfo.OpRegIdx[1], &_item);
        return _item.Value;
    }
    if (DisInfo.OpType[1] == otMEM)
    {
        //Offset
        if (DisInfo.BaseReg == -1) return GetGvarName(DisInfo.Offset);
        //[ebp-N]
        if (DisInfo.BaseReg == 21)
        {
            GetRegItem(DisInfo.BaseReg, &_item);
            _item = Env->Stack[_item.IntValue + DisInfo.Offset];
            return _item.Value;
        }
        //[esp-N]
        if (DisInfo.BaseReg == 20)
        {
            GetRegItem(DisInfo.BaseReg, &_item);
            _item = Env->Stack[_ESP_ + DisInfo.Offset];
            return _item.Value;
        }
    }
    return "?";
}
//---------------------------------------------------------------------------
String __fastcall TDecompiler::GetCycleTo()
{
    ITEM        _item;

    if (DisInfo.OpType[0] == otREG)
    {
        GetRegItem(DisInfo.OpRegIdx[0], &_item);
        return _item.Value;
    }
    if (DisInfo.OpType[0] == otMEM)
    {
        //Offset
        if (DisInfo.BaseReg == -1) return GetGvarName(DisInfo.Offset);
        //[ebp-N]
        if (DisInfo.BaseReg == 21)
        {
            GetRegItem(DisInfo.BaseReg, &_item);
            _item = Env->Stack[_item.IntValue + DisInfo.Offset];
            return _item.Value;
        }
        //[esp-N]
        if (DisInfo.BaseReg == 20)
        {
            GetRegItem(DisInfo.BaseReg, &_item);
            _item = Env->Stack[_ESP_ + DisInfo.Offset];
            return _item.Value;
        }
    }
    return "?";
}
//---------------------------------------------------------------------------
DWORD __fastcall TDecompiler::DecompileGeneralCase(DWORD fromAdr, DWORD markAdr, PLoopInfo loopInfo, int N)
{
    //bool        _changed = false;
    int         _N, _N1 = N, _N2;
    DWORD       _dd;
    DWORD       _curAdr = fromAdr, _adr, _endAdr = 0, _begAdr;
    int         _curPos = Adr2Pos(fromAdr);
    int         _len, _instrLen;
    TDecompiler *de;
    DISINFO     _disInfo;

    while (1)
    {
        _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        //Switch at current address
        if (IsFlagSet(cfSwitch, _curPos))
        {
            de = new TDecompiler(Env);
            _begAdr = _curAdr;
            Env->SaveContext(_begAdr);
            de->SetStackPointers(this);
            de->SetDeFlags(DeFlags);
            de->MarkGeneralCase(markAdr);
            de->ClearStop(_curAdr);
            try
            {
                _adr = de->DecompileCaseEnum(_begAdr, 0, loopInfo);
                if (_adr > _endAdr) _endAdr = _adr;
            }
            catch(Exception &exception)
            {
                delete de;
                throw Exception("GeneralCase->" + exception.Message);
            }
            Env->RestoreContext(_begAdr);
            delete de;
            return _endAdr;
        }
        //Switch at next address
        if (IsFlagSet(cfSwitch, _curPos + _len))
        {
            _N = _disInfo.Immediate;
            //add or sub
            if (_dd == 'dda') _N = -_N;
            _curAdr += _len; _curPos += _len;
            _begAdr = _curAdr;
            Env->SaveContext(_begAdr);
            de = new TDecompiler(Env);
            de->SetStackPointers(this);
            de->SetDeFlags(DeFlags);
            de->MarkGeneralCase(_curAdr);//markAdr
            de->ClearStop(_curAdr);
            try
            {
                _adr = de->DecompileCaseEnum(_begAdr, _N, loopInfo);
                if (_adr > _endAdr) _endAdr = _adr;
            }
            catch(Exception &exception)
            {
                delete de;
                throw Exception("GeneralCase->" + exception.Message);
            }
            Env->RestoreContext(_begAdr);
            delete de;
            return _endAdr;
        }
        //cmp reg, imm
        if (_dd == 'pmc' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM)
        {
            _N = _disInfo.Immediate;
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bj' || _dd == 'gj' || _dd == 'egj')
            {
                _adr = DecompileGeneralCase(_disInfo.Immediate, markAdr, loopInfo, _N1);
                if (_adr > _endAdr) _endAdr = _adr;

                _curAdr += _len; _curPos += _len;

                _len = Disasm.Disassemble(Code + _curPos, (__int64)(_curAdr), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'zj' || _dd == 'ej')
                {
                    Env->AddToBody(String(_N) + ":");
                    _begAdr = _disInfo.Immediate;
                    Env->SaveContext(_begAdr);
                    de = new TDecompiler(Env);
                    de->SetStackPointers(this);
                    de->SetDeFlags(DeFlags);
                    de->MarkGeneralCase(markAdr);
                    de->ClearStop(_begAdr);
                    try
                    {
                        Env->AddToBody("begin");
                        _adr = de->Decompile(_begAdr, 0, loopInfo);
                        if (_adr > _endAdr) _endAdr = _adr;
                        Env->AddToBody("end");
                    }
                    catch(Exception &exception)
                    {
                        delete de;
                        throw Exception("GeneralCase->" + exception.Message);
                    }
                    Env->RestoreContext(_begAdr);
                    delete de;

                    _curAdr += _len; _curPos += _len;
                }
                continue;
            }
        }
        //dec reg; sub reg, imm
        if ((_dd == 'bus' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'ced' && _disInfo.OpType[0] == otREG))
        {
            if (_dd == 'bus')
                _N2 = _disInfo.Immediate;
            else
                _N2 = 1;
            _N1 += _N2;
            if (_disInfo.OpRegIdx[0] < 8)
                _N1 &= 0xFF;
            else if (_disInfo.OpRegIdx[0] < 16)
                _N1 &= 0xFFFF;

            _curAdr += _len; _curPos += _len;

            _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bj')
            {
                Env->AddToBody(String(_N1 - _N2) + ".." + String(_N1 - 1) + ":");
                _begAdr = _disInfo.Immediate;
                Env->SaveContext(_begAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->MarkGeneralCase(markAdr);
                de->ClearStop(_begAdr);
                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(_begAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("GeneralCase->" + exception.Message);
                }
                Env->RestoreContext(_begAdr);
                delete de;

                _curAdr += _len; _curPos += _len;
                _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'zj' || _dd == 'ej')
                {
                    Env->AddToBody(String(_N1) + ":");
                    _begAdr = _disInfo.Immediate;
                    Env->SaveContext(_begAdr);
                    de = new TDecompiler(Env);
                    de->SetStackPointers(this);
                    de->SetDeFlags(DeFlags);
                    de->MarkGeneralCase(markAdr);
                    de->ClearStop(_begAdr);
                    try
                    {
                        Env->AddToBody("begin");
                        _adr = de->Decompile(_begAdr, 0, loopInfo);
                        if (_adr > _endAdr) _endAdr = _adr;
                        Env->AddToBody("end");
                    }
                    catch(Exception &exception)
                    {
                        delete de;
                        throw Exception("GeneralCase->" + exception.Message);
                    }
                    Env->RestoreContext(_begAdr);
                    delete de;

                    _curAdr += _len; _curPos += _len;
                }
                continue;
            }
            if (_dd == 'zj' || _dd == 'ej')
            {
                Env->AddToBody(String(_N1) + ":");
                _begAdr = _disInfo.Immediate;
                Env->SaveContext(_begAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->MarkGeneralCase(markAdr);
                de->ClearStop(_begAdr);
                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(_begAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("GeneralCase->" + exception.Message);
                }
                Env->RestoreContext(_begAdr);
                delete de;

                _curAdr += _len; _curPos += _len;
                continue;
            }
            if (_dd == 'znj' || _dd == 'enj')
            {
                Env->AddToBody(String(_N1) + ":");
                _begAdr = _curAdr + _len;
                Env->SaveContext(_begAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->MarkGeneralCase(markAdr);
                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(_begAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("GeneralCase->" + exception.Message);
                }
                Env->RestoreContext(_begAdr);
                delete de;
                break;
            }
        }
        //inc reg; add reg, imm
        if ((_dd == 'dda' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'cni' && _disInfo.OpType[0] == otREG))
        {
            if (_dd == 'dda')
                _N2 = _disInfo.Immediate;
            else
                _N2 = 1;
            _N1 -= _N2;
            if (_disInfo.OpRegIdx[0] < 8)
                _N1 &= 0xFF;
            else if (_disInfo.OpRegIdx[0] < 16)
                _N1 &= 0xFFFF;

            _curAdr += _len; _curPos += _len;

            _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'zj' || _dd == 'ej')
            {
                Env->AddToBody(String(_N1) + ":");
                _begAdr = _disInfo.Immediate;
                Env->SaveContext(_begAdr);
                de = new TDecompiler(Env);
                de->SetStackPointers(this);
                de->SetDeFlags(DeFlags);
                de->MarkGeneralCase(markAdr);
                de->ClearStop(_begAdr);
                try
                {
                    Env->AddToBody("begin");
                    _adr = de->Decompile(_begAdr, 0, loopInfo);
                    if (_adr > _endAdr) _endAdr = _adr;
                    Env->AddToBody("end");
                }
                catch(Exception &exception)
                {
                    delete de;
                    throw Exception("GeneralCase->" + exception.Message);
                }
                Env->RestoreContext(_begAdr);
                delete de;

                _curAdr += _len; _curPos += _len;
                continue;
            }
        }
        if (_dd == 'pmj') break;
    }
    return _endAdr;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::MarkGeneralCase(DWORD fromAdr)
{
    DWORD       _dd;
    DWORD       _curAdr = fromAdr;
    int         _curPos = Adr2Pos(fromAdr);
    int         _len, _instrLen;
    DISINFO     _disInfo;

    while (1)
    {
        _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
        _dd = *((DWORD*)_disInfo.Mnem);
        //Switch at current address
        if (IsFlagSet(cfSwitch, _curPos))
        {
            //Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);//ja
            MarkCaseEnum(_curAdr);
            return;
        }
        //Switch at next address
        if (IsFlagSet(cfSwitch, _curPos + _len))
        {
            _curPos += _len; _curAdr += _len;
            //_len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);//ja
            //_curPos += _len; _curAdr += _len;
            MarkCaseEnum(_curAdr);
            return;
        }
        //cmp reg, imm
        if (_dd == 'pmc' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM)
        {
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bj' || _dd == 'gj' || _dd == 'egj')
            {
                MarkGeneralCase(_disInfo.Immediate);

                _curAdr += _len; _curPos += _len;

                _len = Disasm.Disassemble(Code + _curPos, (__int64)(_curAdr), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'zj' || _dd == 'ej')
                {
                    SetStop(_disInfo.Immediate);
                    _curAdr += _len; _curPos += _len;
                }
                continue;
            }
        }
        //sub reg, imm; dec reg
        if ((_dd == 'bus' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'ced' && _disInfo.OpType[0] == otREG))
        {
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bus')
            {
                _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
            }
            if (_dd == 'bj')
            {
                SetStop(_disInfo.Immediate);
                _curAdr += _len; _curPos += _len;
                _len = Disasm.Disassemble(Code + _curPos, (__int64)_curAdr, &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'zj' || _dd == 'ej')
                {
                    SetStop(_disInfo.Immediate);
                    _curAdr += _len; _curPos += _len;
                }
                continue;
            }
            if (_dd == 'zj' || _dd == 'ej')
            {
                SetStop(_disInfo.Immediate);
                _curAdr += _len; _curPos += _len;
                continue;
            }
            if (_dd == 'znj' || _dd == 'enj')
                break;
        }
        //add reg, imm; inc reg
        if ((_dd == 'dda' && _disInfo.OpType[0] == otREG && _disInfo.OpType[1] == otIMM) || (_dd == 'cni' && _disInfo.OpType[0] == otREG))
        {
            _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
            _dd = *((DWORD*)_disInfo.Mnem);
            if (_dd == 'bus')
            {
                _len += Disasm.Disassemble(Code + _curPos + _len, (__int64)(_curAdr + _len), &_disInfo, 0);
                _dd = *((DWORD*)_disInfo.Mnem);
                if (_dd == 'bj')
                {
                    SetStop(_disInfo.Immediate);
                    _curAdr += _len; _curPos += _len;
                    continue;
                }
            }
            if (_dd == 'zj' || _dd == 'ej')
            {
                SetStop(_disInfo.Immediate);
                _curAdr += _len; _curPos += _len;
                continue;
            }
        }
        if (_dd == 'pmj')
        {
            SetStop(_disInfo.Immediate);
            break;
        }
    }
}
//---------------------------------------------------------------------------
int __fastcall TDecompiler::GetArrayFieldOffset(String ATypeName, int AFromOfs, int AScale, String& name, String& type)
{
    bool        _vmt;
    int         _size, _lIdx, _hIdx, _ofs, _fofs;
    int         _classSize = GetClassSize(GetClassAdr(ATypeName));
    int         _offset = AFromOfs;

    while (1)
    {
        if (_offset >= _classSize) break;
        _fofs = GetField(ATypeName, _offset, name, type);
        if (_fofs >= 0 && GetTypeKind(type, &_size) == ikArray && GetArrayIndexes(type, 1, &_lIdx, &_hIdx))
        {
            _size = GetArrayElementTypeSize(type);
            _ofs = AFromOfs + AScale * _lIdx;
            if (_ofs >= _fofs && _ofs <= _fofs + (_hIdx - _lIdx) * _size)
                return _fofs;
        }
        //Try next offset
        _offset++;
    }
    return -1;
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::GetInt64ItemFromStack(int Esp, PITEM Dst) 
{
    BYTE        _binData[8];
    __int64     _int64Val;
    ITEM       _item;

    InitItem(Dst);
    memset((void*)_binData, 0, 8);
    _item = Env->Stack[Esp];
    memmove((void*)_binData, (void*)&_item.IntValue, 4);
    _item = Env->Stack[Esp + 4];
    memmove((void*)(_binData + 4), (void*)&_item.IntValue, 4);
    memmove((void*)&_int64Val, _binData, 8);
    Dst->Value = IntToStr(_int64Val);
    Dst->Type = "Single";
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::GetFloatItemFromStack(int Esp, PITEM Dst, int FloatType)
{
    BYTE        _binData[16];
    float       _singleVal;
    double      _doubleVal;
    long double _extendedVal;
    double      _realVal;
    Comp        _compVal;
    Currency    _currVal;
    ITEM       _item;

    InitItem(Dst);
    _item = Env->Stack[Esp];
    if (!(_item.Flags & IF_INTVAL))
    {
        Dst->Value = _item.Value;
        Dst->Type = _item.Type;
        return;
    }
    memset((void*)_binData, 0, 16);
    memmove((void*)_binData, (void*)&_item.IntValue, 4);
    if (FloatType == FT_SINGLE)
    {
        memmove((void*)&_singleVal, _binData, 4);
        Dst->Value = FloatToStr(_singleVal);
        Dst->Type = "Single";
        return;
    }
    if (FloatType == FT_REAL)
    {
        memmove((void*)&_realVal, _binData, 4);
        Dst->Value = FloatToStr(_realVal);
        Dst->Type = "Real";
        return;
    }
    _item = Env->Stack[Esp + 4];
    memmove((void*)(_binData + 4), (void*)&_item.IntValue, 4);
    if (FloatType == FT_DOUBLE)
    {
        memmove((void*)&_doubleVal, _binData, 8);
        Dst->Value = FloatToStr(_doubleVal);
        Dst->Type = "Double";
        return;
    }
    if (FloatType == FT_COMP)
    {
        memmove((void*)&_compVal, _binData, 8);
        Dst->Value = FloatToStr(_compVal);
        Dst->Type = "Comp";
        return;
    }
    if (FloatType == FT_CURRENCY)
    {
        memmove((void*)&_currVal.Val, _binData, 8);
        Dst->Value = _currVal.operator AnsiString();
        Dst->Type = "Currency";
        return;
    }
    _item = Env->Stack[Esp + 8];
    memmove((void*)(_binData + 8), (void*)&_item.IntValue, 4);
    if (FloatType == FT_EXTENDED)
    {
        memmove((void*)&_extendedVal, _binData, 10);
        try
        {
            Dst->Value = FloatToStr(_extendedVal);
            Dst->Type = "Extended";
        }
        catch(Exception &exception)
        {
            throw Exception("Extended type error" + exception.Message);
        }
        return;
    }
}
//---------------------------------------------------------------------------
void __fastcall TDecompiler::GetMemItem(int CurAdr, PITEM Dst, BYTE Op)
{
    bool        _vmt;
    BYTE        _kind;
    int         _offset, _foffset, _pos, _size, _idx, _idx1, _mod, _fofs;
    DWORD       _adr;
    PInfoRec    _recN, _recN1;
    ITEM        _item, _itemBase, _itemIndx;
    String      _fname, _name, _type, _typeName, _iname, _value, _text, _lvarName;

    InitItem(Dst);
    _offset = DisInfo.Offset;
    if (DisInfo.BaseReg == -1)
    {
        if (DisInfo.IndxReg == -1)
        {
            //[Offset]
            if (IsValidImageAdr(_offset))
            {
                Dst->Flags = IF_INTVAL;
                Dst->IntValue = _offset;
                return;
            }
            Env->ErrAdr = CurAdr;
            throw Exception("Address is outside program image");
        }
        //[Offset + IndxReg*Scale]
        if (IsValidImageAdr(_offset))
        {
            _recN = GetInfoRec(_offset);
            if (_recN && _recN->HasName())
                _name = _recN->GetName();
            else
                _name = GetGvarName(_offset);
            Dst->Value = _name + "[" + GetDecompilerRegisterName(DisInfo.IndxReg) + "]";
            return;
        }
        Env->ErrAdr = CurAdr;
        throw Exception("Address is outside program image");
    }
    GetRegItem(DisInfo.BaseReg, &_itemBase);
    //[BaseReg + Offset]
    if (DisInfo.IndxReg == -1)
    {
        //[esp+N]
        if (DisInfo.BaseReg == 20)
        {
            Dst->Flags = IF_STACK_PTR;
            Dst->IntValue = _ESP_ + _offset;
            _item = Env->Stack[_ESP_ + _offset];
            //Field
            if (_item.Flags & IF_FIELD)
            {
                _foffset = _item.Offset;
                _offset -= _foffset;
                _fname = GetRecordFields(_foffset, Env->Stack[_ESP_ + _offset].Type);
                _typeName = ExtractType(_fname);
                _name = Env->GetLvarName(_ESP_ + _offset, _typeName);
                if (_fname.Pos(":"))
                {
                    Dst->Value = _name + "." + ExtractName(_fname);
                    Dst->Type = _typeName;
                }
                else
                {
                    Dst->Value = _name + ".f" + Val2Str0(_foffset);
                }
                Dst->Name = Dst->Value;
                return;
            }
            _value = Env->GetLvarName(_ESP_ + _offset, "");
            if (_item.Value != "") _value += "{" + _item.Value + "}";
            Dst->Value = _value;
            return;
        }
        //lea reg, [ebp + N], bpBased
        if (Op == OP_LEA && DisInfo.BaseReg == 21 && DisInfo.IndxReg == -1 && Env->BpBased)
        {
            Dst->Flags = IF_STACK_PTR;
            Dst->IntValue = _itemBase.IntValue + _offset;
            return;
        }
        //Embedded procedures
        if (Env->Embedded)
        {
            //[ebp+8] - set flag IF_EXTERN_VAR and exit
            if (DisInfo.BaseReg == 21 && _offset == 8)
            {
                Dst->Flags = IF_EXTERN_VAR;
                return;
            }
            if (_itemBase.Flags & IF_EXTERN_VAR)
            {
                Dst->Value = "extlvar_" + Val2Str0(-_offset);
                return;
            }
        }
        //[reg-N]
        if (_itemBase.Flags & IF_STACK_PTR)
        {
            //xchg ecx,[ebp-XXX] - special processing
            if (Op == OP_XCHG)
            {
                Dst->Flags = IF_STACK_PTR;
                Dst->IntValue = _itemBase.IntValue + _offset;
                Dst->Value = _itemBase.Value;
                Dst->Name = _itemBase.Name;
                return;
            }
            if (_itemBase.Flags & IF_ARG)
            {
                Dst->Flags = IF_STACK_PTR;
                Dst->IntValue = _itemBase.IntValue + _offset;
                Dst->Value = _itemBase.Value;
                Dst->Name = _itemBase.Name;
                return;
            }
            if (_itemBase.Flags & IF_ARRAY_PTR)
            {
                Dst->Flags = IF_STACK_PTR;
                Dst->IntValue = _itemBase.IntValue + _offset;
                if (_itemBase.Value != "") Dst->Value = _itemBase.Value + "[]";
                if (_itemBase.Name != "") Dst->Name = _itemBase.Name + "[]";
                return;
            }
            _item = Env->Stack[_itemBase.IntValue + _offset];
            //Arg
            if (_item.Flags & IF_ARG)
            {
                Dst->Flags = IF_STACK_PTR;
                Dst->IntValue = _itemBase.IntValue + _offset;
                //AssignItem(Dst, &_item);
                //Dst->Flags &= ~IF_ARG;
                Dst->Value = _item.Name;
                Dst->Name = _item.Name;
                return;
            }
            //Var
            if (_item.Flags & IF_VAR)
            {
                _item.Flags &= ~IF_VAR;
                _item.Type = "^" + _item.Type;
                AssignItem(Dst, &_item);
                Dst->Name = Env->GetLvarName(_itemBase.IntValue + _offset, "");
                return;
            }
            //Field
            if (_item.Flags & IF_FIELD)
            {
                _foffset = _item.Offset;
                _offset -= _foffset;
                _fname = GetRecordFields(_foffset, Env->Stack[_itemBase.IntValue + _offset].Type);
                _typeName = ExtractType(_fname);
                _name = Env->GetLvarName(_itemBase.IntValue + _offset, _typeName);
                if (_fname.Pos(":"))
                {
                    Dst->Value = _name + "." + ExtractName(_fname);
                    Dst->Type = _typeName;
                }
                else
                {
                    Dst->Value = _name + ".f" + Val2Str0(_foffset);
                }
                Dst->Name = _name;
                return;
            }
            //Not interface
            if (_item.Type == "" || GetTypeKind(_item.Type, &_size) != ikInterface)
            {
                _lvarName = Env->GetLvarName(_itemBase.IntValue + _offset, "");
                Env->Stack[_itemBase.IntValue + _offset].Name = _lvarName;
                Dst->Flags = IF_STACK_PTR;
                Dst->IntValue = _itemBase.IntValue + _offset;
                Dst->Value = _lvarName;
                Dst->Name = Dst->Value;
                return;
            }
        }
        //[BaseReg]
        if (!_offset)
        {
            _typeName = _itemBase.Type;
            if (_itemBase.Flags & IF_VAR)
            {
                AssignItem(Dst, &_itemBase);
                Dst->Flags &= ~IF_VAR;
                Dst->Name = "";
                return;
            }
            if (_itemBase.Flags & IF_RECORD_FOFS)
            {
                _value = _itemBase.Value;
                if (_itemBase.Flags & IF_ARRAY_PTR)
                    _value += "[]";
                _text = GetRecordFields(_itemBase.Offset, _itemBase.Type);
                if (_text.Pos(":"))
                {
                    _value += "." + ExtractName(_text);
                    _typeName = ExtractType(_text);
                }
                else
                {
                    _value += ".f" + Val2Str0(_itemBase.Offset);
                    _typeName = _text;
                }
                Dst->Value = _value;
                Dst->Type = _typeName;
                Dst->Name = "";
                return;
            }
            if (_itemBase.Flags & IF_ARRAY_PTR)
            {
                Dst->Value = _itemBase.Value + "[]";
                Dst->Type = GetArrayElementType(_typeName);
                Dst->Name = "";
                return;
            }
            if (_itemBase.Flags & IF_STACK_PTR)
            {
                _item = Env->Stack[_itemBase.IntValue];
                AssignItem(Dst, &_item);
                return;
            }
            if (_itemBase.Flags & IF_INTVAL)
            {
                _adr = _itemBase.IntValue;
                if (IsValidImageAdr(_adr))
                {
                    _recN = GetInfoRec(_adr);
                    if (_recN)
                    {
                        Dst->Value = _recN->GetName();
                        Dst->Type = _recN->type;
                        return;
                    }
                }
            }
            if (_typeName == "")
            {
                _typeName = "Pointer";
                //_name = GetDecompilerRegisterName(DisInfo.BaseReg);
                //_typeName = ManualInput(CurProcAdr, CurAdr, "Define type of base register (" + _name + ")", "Type:");
                //if (_typeName == "")
                //{
                //    Env->ErrAdr = CurAdr;
                //    throw Exception("Bye!");
                //}
            }
            if (_typeName[1] == '^')//Pointer to var
            {
                _value = _itemBase.Value;
                _typeName = GetTypeDeref(_typeName);
                _kind = GetTypeKind(_typeName, &_size);
                if (_kind == ikRecord)
                {
                    _text = GetRecordFields(0, _typeName);
                    if (_text.Pos(":"))
                    {
                        _value += "." + ExtractName(_text);
                        _typeName = ExtractType(_text);
                    }
                    else
                    {
                        _value += ".f0";
                        _typeName = _text;
                    }
                }
                if (_kind == ikArray)
                {
                    _value += "[ofs=" + String(_offset) + "]";
                }
                Dst->Value = _value;
                Dst->Name = GetDecompilerRegisterName(DisInfo.BaseReg);
                Dst->Type = _typeName;
                return;
            }
            _kind = GetTypeKind(_typeName, &_size);
            if (_kind == ikEnumeration)
            {
                Dst->Value = GetEnumerationString(_typeName, _itemBase.Value);
                Dst->Type = _typeName;
                return;
            }
            if (_kind == ikLString || _kind == ikString)
            {
                Dst->Value = _itemBase.Value + "[1]";
                Dst->Type = "Char";
                return;
            }
            if (_kind == ikRecord)
            {
                _text = GetRecordFields(_offset, _typeName);
                if (_text.Pos(":"))
                {
                    _value = _itemBase.Value + "." + ExtractName(_text);
                    _typeName = ExtractType(_text);
                }
                else
                {
                    _value = _itemBase.Value + ".f" + Val2Str0(_offset);
                    _typeName = _text;
                }
                Dst->Value = _value;
                Dst->Type = _typeName;
                return;
            }

            Dst->Flags = IF_VMT_ADR;
            if (_itemBase.Flags & IF_INTERFACE)
            {
                Dst->Flags |= IF_INTERFACE;
                Dst->Value = _itemBase.Value;
            }
            Dst->IntValue = GetClassAdr(_typeName);
            Dst->Type = _typeName;
            return;
        }
        //[BaseReg+Offset]
        if (IsValidImageAdr(_offset))
        {
            _recN = GetInfoRec(_offset);
            if (_recN && _recN->HasName())
                _name = _recN->GetName();
            else
                _name = GetGvarName(_offset);
            Dst->Value = _name + "[" + GetDecompilerRegisterName(DisInfo.BaseReg) + "]";
            return;
        }
        if (_itemBase.Flags & IF_RECORD_FOFS)
        {
            _value = _itemBase.Value;
            if (_itemBase.Flags & IF_ARRAY_PTR)
                _value += "[]";
            _text = GetRecordFields(_itemBase.Offset + _offset, _itemBase.Type);
            if (_text.Pos(":"))
            {
                _value += "." + ExtractName(_text);
                _typeName = ExtractType(_text);
            }
            else
            {
                _value += ".f" + Val2Str0(_itemBase.Offset + _offset);
                _typeName = _text;
            }
            Dst->Value = _value;
            Dst->Type = _typeName;
            Dst->Name = "";
            return;
        }
        if (_itemBase.Flags & IF_ARRAY_PTR)
        {
            if ((_itemBase.Flags & IF_STACK_PTR) && Env->Stack[_itemBase.IntValue].Value != "")
                Dst->Value = Env->Stack[_itemBase.IntValue].Value + "[";
            else
                Dst->Value = GetDecompilerRegisterName(DisInfo.BaseReg) + "[";
            if (_offset > 0)
                Dst->Value += " + " + String(_offset);
            else if (_offset < 0)
                Dst->Value += " - " + String(-_offset);
            Dst->Value += "]";
            return;
        }
        _typeName = _itemBase.Type;
        if (_typeName == "")
        {
            _typeName = "Pointer";
            //_name = GetDecompilerRegisterName(DisInfo.BaseReg);
            //_typeName = ManualInput(CurProcAdr, CurAdr, "Define type of base register (" + _name + ")", "Type:");
            //if (_typeName == "")
            //{
            //    Env->ErrAdr = CurAdr;
            //    throw Exception("Bye!");
            //}
        }
        if (_typeName[1] == '^') _typeName = GetTypeDeref(_typeName);

        _itemBase.Flags = 0;
        if (_itemBase.Value == "")
            _itemBase.Value = GetDecompilerRegisterName(DisInfo.BaseReg);
        _itemBase.Type = _typeName;
        SetRegItem(DisInfo.BaseReg, &_itemBase);

        _iname = _itemBase.Value;
        _kind = GetTypeKind(_typeName, &_size);
        if (_kind == ikLString || _kind == ikString)
        {
            _value = _iname + "[" + String(_offset + 1) + "]";
            _typeName = "Char";
        }
        else if (_kind == ikRecord)
        {
            if (_itemBase.Value != "")
                _value = _itemBase.Value;
            else
                _value = GetDecompilerRegisterName(DisInfo.BaseReg);
            if (Op == OP_LEA)//address of field with ofs=_offset in structure _typeName
            {
                Dst->Flags = IF_RECORD_FOFS;
                Dst->Value = _value;
                Dst->Type = _typeName;
                Dst->Offset = _offset;
                return;
            }
            _text = GetRecordFields(_offset, _typeName);
            if (_text.Pos(":"))
            {
                _value += "." + ExtractName(_text);
                _typeName = ExtractType(_text);
            }
            else
            {
                _value += ".f" + Val2Str0(_offset);
                _typeName = _text;
            }
        }
        else if (_kind == ikArray)
        {
            _value = _iname + "[ofs=" + String(_offset) + "]";
        }
        else if (_kind == ikDynArray)
        {
            _typeName = GetArrayElementType(_typeName);
            if (_typeName == "")
            {
                Env->ErrAdr = CurAdr;
                throw Exception("Type of array elements not found");
            }
            if (GetTypeKind(_typeName, &_size) == ikRecord)
            {
                int _k = 0;
                _size = GetRecordSize(_typeName);
                if (_offset < 0)
                {
                    while (1)
                    {
                        _offset += _size; _k--;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                if (_offset >= _size)
                {
                    while (1)
                    {
                        _offset -= _size; _k++;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                _text = GetRecordFields(_offset, _typeName);
                if (_text == "")
                {
                    _text = ManualInput(CurProcAdr, CurAdr, "Define [name:]type of field " + _typeName + ".f" + Val2Str0(_offset), "[Name]:Type:");
                    if (_text == "")
                    {
                        Env->ErrAdr = CurAdr;
                        throw Exception("Bye!");
                    }
                }
                _value = _itemBase.Value + "[" + String(_k) + "]";
                if (_text.Pos(":"))
                {
                    _value += "." + ExtractName(_text);
                    _typeName = ExtractType(_text);
                }
                else
                {
                    _value += ".f" + Val2Str0(_offset);
                    _typeName = _text;
                }
            }
            Dst->Value = _value;
            Dst->Type = _typeName;
            return;
        }
        else if (_kind == ikVMT || _kind == ikClass)
        {
            _fofs = GetField(_typeName, _offset, _name, _type);
            if (_fofs < 0)
            {
                _text = ManualInput(CurProcAdr, CurAdr, "Define correct type of field " + _typeName + ".f" + Val2Str0(_offset), "Type:");
                if (_text == "")
                {
                    Env->ErrAdr = CurAdr;
                    throw Exception("Bye!");
                }
                _recN1 = GetInfoRec(GetClassAdr(_typeName));
                _recN1->vmtInfo->AddField(0, 0, FIELD_PUBLIC, _offset, -1, "", _text);

                _fofs = GetField(_typeName, _offset, _name, _type);
                if (_fofs < 0)
                {
                    Env->ErrAdr = CurAdr;
                    throw Exception("Field f" + Val2Str0(_offset) + " not found");
                }
            }
            _value = _name;
            _typeName = _type;
            _foffset = _fofs;

            //if (Op != OP_LEA)
            //{
                _kind = GetTypeKind(_typeName, &_size);
                //Interface
                if (_kind == ikInterface)
                {
                    _typeName[1] = 'T';
                    Dst->Flags = IF_INTERFACE;
                    Dst->Value = _value; 
                    Dst->Type = _typeName;
                    return;
                }
                //Record
                if (_kind == ikRecord)
                {
                    _text = GetRecordFields(_offset - _foffset, _typeName);
                    if (_text == "")
                    {
                        _text = ManualInput(CurProcAdr, CurAdr, "Define [name:]type of field " + _typeName + ".f" + Val2Str0(_offset - _fofs), "[Name]:Type:");
                        if (_text == "")
                        {
                            Env->ErrAdr = CurAdr;
                            throw Exception("Bye!");
                        }
                    }
                    if (_text.Pos(":"))
                    {
                        _value += "." + ExtractName(_text);
                        _typeName = ExtractType(_text);
                    }
                    else
                    {
                        _value += ".f" + Val2Str0(_offset);
                        _typeName = _text;
                    }
                }
                //Array
                if (_kind == ikArray)
                {
                    _value += "[ofs=" + String(_offset - _foffset) + "]";
                }
                if (!SameText(_iname, "Self")) _value = _iname + "." + _value;
            //}
        }
        Dst->Value = _value;
        Dst->Type = _typeName;
        return;
    }
    //[BaseReg + IndxReg*Scale + Offset]
    else
    {
        GetRegItem(DisInfo.IndxReg, &_itemIndx);
        if (Op == OP_LEA)
        {
            if (_itemBase.Flags & IF_STACK_PTR) Dst->Flags |= IF_STACK_PTR;
            Dst->Value = GetDecompilerRegisterName(DisInfo.BaseReg) + " + " + GetDecompilerRegisterName(DisInfo.IndxReg) + " * " + String(DisInfo.Scale);
            if (_offset > 0)
                Dst->Value += String(_offset);
            else if (_offset < 0)
                Dst->Value += String(-_offset);
            return;
        }
        _typeName = _itemBase.Type;
        if (_typeName == "")
        {
            //esp
            if (DisInfo.BaseReg == 20)
            {
                Dst->Value = Env->GetLvarName(_ESP_ + _offset + DisInfo.Scale, "") + "[" + _itemIndx.Value + "]";
                return;
            }
            //ebp
            if (DisInfo.BaseReg == 21 && (_itemBase.Flags & IF_STACK_PTR))
            {
                Dst->Value = Env->GetLvarName(_itemBase.IntValue + _offset + DisInfo.Scale, "") + "[" + _itemIndx.Value + "]";
                return;
            }
            _kind = 0;
            //Lets try analyze _itemBase if it is address
            if (_itemBase.Flags & IF_INTVAL)
            {
                _adr = _itemBase.IntValue;
                if (IsValidImageAdr(_adr))
                {
                    _recN = GetInfoRec(_adr);
                    if (_recN)
                    {
                        _kind = _recN->kind;
                        if (_recN->type != "" && (_kind == ikUnknown || _kind == ikData))
                            _kind = GetTypeKind(_recN->type, &_size);
                    }
                }
            }
            while (_kind == 0 || _kind == ikData)
            {
                _text = "Pointer";
                //_text = ManualInput(CurProcAdr, CurAdr, "Define type of base register", "Type:");
                //if (_text == "")
                //{
                //    Env->ErrAdr = CurAdr;
                //    throw Exception("Bye!");
                //}
                _typeName = _text;
                _kind = GetTypeKind(_typeName, &_size);
            }
        }
        else
        {
            if (_typeName[1] == '^') _typeName = GetTypeDeref(_typeName);

            _kind = GetTypeKind(_typeName, &_size);
            while (_kind != ikClass && _kind != ikVMT && _kind != ikLString &&
                   _kind != ikCString && _kind != ikPointer && _kind != ikRecord &&
                   _kind != ikArray && _kind != ikDynArray)
            {
                _text = "Pointer";
                //_text = ManualInput(CurProcAdr, CurAdr, "Define type of base register", "Type:");
                //if (_text == "")
                //{
                //    Env->ErrAdr = CurAdr;
                //    throw Exception("Bye!");
                //}
                _typeName = _text;
                _kind = GetTypeKind(_typeName, &_size);
            }
        }
        if (_kind == ikClass || _kind == ikVMT)
        {
            _fofs = GetArrayFieldOffset(_typeName, _offset, DisInfo.Scale, _name, _type);
            while (_fofs < 0)
            {
                _text = ManualInput(CurProcAdr, CurAdr, "Define actual offset (in hex) of array field", "Offset:");
                if (_text == "")
                {
                    Env->ErrAdr = CurAdr;
                    throw Exception("Bye!");
                }
                sscanf(_text.c_str(), "%lX", &_offset);
                _fofs = GetArrayFieldOffset(_typeName, _offset, DisInfo.Scale, _name, _type);
            }
            if (!SameText(_itemBase.Value, "Self")) _value = _itemBase.Value + ".";
            if (_name != "")
                _value += _name;
            else
                _value += "f" + Val2Str0(_fofs);
            _value += "[" + GetDecompilerRegisterName(DisInfo.IndxReg) + "]";

            Dst->Value = _value;
            Dst->Type = GetArrayElementType(_type);
            return;
        }
        if (_kind == ikLString || _kind == ikCString || _kind == ikPointer)
        {
            Dst->Value = GetDecompilerRegisterName(DisInfo.BaseReg) + "[" + GetDecompilerRegisterName(DisInfo.IndxReg);
            if (_kind == ikLString) _offset++;
            if (_offset > 0)
                Dst->Value += " + " + String(_offset);
            else if (_offset < 0)
                Dst->Value += " - " + String(-_offset);
            Dst->Value += "]";
            Dst->Type = "Char";
            return;
        }
        if (_kind == ikRecord)
        {
            _value = _itemBase.Value;
            _text = GetRecordFields(_offset + DisInfo.Scale, _typeName);
            if (_text.Pos(":"))
            {
                _value += "." + ExtractName(_text);
                _typeName = ExtractType(_text);
            }
            else
            {
                _value += ".f" + Val2Str0(_offset);
                _typeName = _text;
            }
            if (GetTypeKind(_typeName, &_size) == ikArray)
            {
                _value += "[" + GetDecompilerRegisterName(DisInfo.IndxReg) + "]";
                _typeName = GetArrayElementType(_typeName);
            }
            Dst->Value = _value;
            Dst->Type = _typeName;
            return;
        }
        if (_kind == ikArray)
        {
            if (_itemBase.Flags & IF_INTVAL)
            {
                _adr = _itemBase.IntValue;
                _recN = GetInfoRec(_adr);
                if (_recN && _recN->HasName())
                    _name = _recN->GetName();
                else
                    _name = GetGvarName(_adr);
            }
            _name = _itemBase.Value;
            _typeName = GetArrayElementType(_typeName);
            if (_typeName == "")
            {
                Env->ErrAdr = CurAdr;
                throw Exception("Type of array elements not found");
            }
            if (GetTypeKind(_typeName, &_size) == ikRecord)
            {
                int _k = 0;
                _size = GetRecordSize(_typeName);
                if (_offset < 0)
                {
                    while (1)
                    {
                        _offset += _size; _k--;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                if (_offset > _size)
                {
                    while (1)
                    {
                        _offset -= _size; _k++;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                _text = GetRecordFields(_offset, _typeName);
                if (_text == "")
                {
                    _text = ManualInput(CurProcAdr, CurAdr, "Define [name:]type of field " + _typeName + ".f" + Val2Str0(_offset), "[Name]:Type:");
                    if (_text == "")
                    {
                        Env->ErrAdr = CurAdr;
                        throw Exception("Bye!");
                    }
                }
                _value = _name + "[";
                if (_itemIndx.Value != "")
                    _value += _itemIndx.Value;
                else
                    _value += GetDecompilerRegisterName(DisInfo.IndxReg);
                if (_k < 0)
                    _value += " - " + String(-_k) + "]";
                else if (_k > 0)
                    _value += " + " + String(_k) + "]";
                else
                    _value += "]";

                if (_text.Pos(":"))
                {
                    _value += "." + ExtractName(_text);
                    _typeName = ExtractType(_text);
                }
                else
                {
                    _value += ".f" + Val2Str0(_offset);
                    _typeName = _text;
                }
                Dst->Value = _value;
                Dst->Type = _typeName;
                return;
            }

            if (!GetArrayIndexes(_itemBase.Type, 1, &_idx, &_idx1))
            {
                Env->ErrAdr = CurAdr;
                throw Exception("Incorrect array definition");
            }
            _mod = _offset % DisInfo.Scale;
            if (_mod)
            {
                Env->ErrAdr = CurAdr;
                throw Exception("Array element is record");
            }

            if (_itemIndx.Value != "")
                _value = _itemBase.Value + "[" + _itemIndx.Value + "]";
            else
                _value = _itemBase.Value + "[" + GetDecompilerRegisterName(DisInfo.IndxReg) + "]";
            Dst->Value = _value;
            Dst->Type = GetArrayElementType(_itemBase.Type);
            return;
        }
        if (_kind == ikDynArray)
        {
            _typeName = GetArrayElementType(_typeName);
            if (_typeName == "")
            {
                Env->ErrAdr = CurAdr;
                throw Exception("Type of array elements not found");
            }
            _value = _itemBase.Value + "[";
            if (_itemIndx.Value != "")
                _value += _itemIndx.Value;
            else
                _value += GetDecompilerRegisterName(DisInfo.IndxReg);
            _value += "]";

            if (GetTypeKind(_typeName, &_size) == ikRecord)
            {
                int _k = 0;
                _size = GetRecordSize(_typeName);
                if (_offset < 0)
                {
                    while (1)
                    {
                        _offset += _size; _k--;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                if (_offset > _size)
                {
                    while (1)
                    {
                        _offset -= _size; _k++;
                        if (_offset >= 0 && _offset < _size) break;
                    }
                }
                _text = GetRecordFields(_offset, _typeName);
                if (_text == "")
                {
                    _text = ManualInput(CurProcAdr, CurAdr, "Define [name:]type of field " + _typeName + ".f" + Val2Str0(_offset), "[Name]:Type:");
                    if (_text == "")
                    {
                        Env->ErrAdr = CurAdr;
                        throw Exception("Bye!");
                    }
                }
                _value = _itemBase.Value + "[";
                if (_itemIndx.Value != "")
                    _value += _itemIndx.Value;
                else
                    _value += GetDecompilerRegisterName(DisInfo.IndxReg);
                if (_k < 0)
                    _value += " - " + String(-_k) + "]";
                else if (_k > 0)
                    _value += " + " + String(_k) + "]";
                else
                    _value += "]";

                if (_text.Pos(":"))
                {
                    _value += "." + ExtractName(_text);
                    _typeName = ExtractType(_text);
                }
                else
                {
                    _value += ".f" + Val2Str0(_offset);
                    _typeName = _text;
                }
            }
            Dst->Value = _value;
            Dst->Type = _typeName;
            return;
        }
        Env->ErrAdr = CurAdr;
        throw Exception("Under construction");
    }
}
//---------------------------------------------------------------------------
String __fastcall TDecompiler::GetStringArgument(PITEM item)
{
    int         _idx, _ap, _len, _size;
    DWORD        _adr;
    PInfoRec    _recN;
    String      _key;

    if (item->Name != "") return item->Name;

    if (item->Flags & IF_STACK_PTR)
    {
        Env->Stack[item->IntValue].Type = "String";
        return Env->GetLvarName(item->IntValue, "String");
    }
    else if (item->Flags & IF_INTVAL)
    {
        _adr = item->IntValue;
        if (_adr == 0)
        {
            return "";
        }
        else if (IsValidImageAdr(_adr))
        {
            _ap = Adr2Pos(_adr);
            if (_ap >= 0)
            {
                _recN = GetInfoRec(_adr);
                if (_recN && (_recN->kind == ikLString || _recN->kind == ikWString || _recN->kind == ikUString))
                    return _recN->GetName();

                _len = wcslen((wchar_t*)(Code + _ap));
                WideString wStr = WideString((wchar_t*)(Code + _ap));
                _size = WideCharToMultiByte(CP_ACP, 0, wStr, -1, 0, 0, 0, 0);
                if (_size)
                {
                    char* tmpBuf = new char[_size + 1];
                    WideCharToMultiByte(CP_ACP, 0, wStr, -1, (LPSTR)tmpBuf, _size, 0, 0);
                    String str = TransformString(tmpBuf, _size);
                    delete[] tmpBuf;
                    return str;
                }
            }
            else
            {
                _key = Val2Str8(_adr);
                _idx = BSSInfos->IndexOf(_key);
                if (_idx != -1)
                    _recN = (PInfoRec)BSSInfos->Objects[_idx];
                if (_recN)
                    return _recN->GetName();
                else
                    return item->Value;
            }
        }
        else
        {
            return item->Value;
        }
    }
    else
    {
        return item->Value;
    }
}
//---------------------------------------------------------------------------
int __fastcall TDecompiler::AnalyzeConditions(int brType, DWORD curAdr, DWORD sAdr, DWORD jAdr, PLoopInfo loopInfo, BOOL bFloat)
{
    DWORD       _curAdr = curAdr;
    DWORD       _begAdr, _bodyBegAdr, _bodyEndAdr, _jmpAdr = jAdr;
    TDecompiler *de;
    String      _line;

    //simple if
    if (brType == 0)
    {
        //if (bFloat) SimulateFloatInstruction(sAdr);
        if (CmpInfo.O == 'R')//not in
            _line = "if (not (" + CmpInfo.L + " in " + CmpInfo.R + ")) then";
        else
            _line = "if (" + CmpInfo.L + " " + GetInvertCondition(CmpInfo.O) + " " + CmpInfo.R + ") then";
        if (bFloat) SimulateFloatInstruction(sAdr);
        
        Env->AddToBody(_line);
        _begAdr = _curAdr;
        Env->SaveContext(_begAdr);
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(CmpAdr);
        try
        {
            Env->AddToBody("begin");
            _curAdr = de->Decompile(_begAdr, 0, loopInfo);
            Env->AddToBody("end");
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("If->" + exception.Message);
        }
        Env->RestoreContext(_begAdr);//if (de->WasRet)
        delete de;
    }
    //complex if
    else if (brType == 1)
    {
        _bodyBegAdr = _curAdr;
        if (!Env->GetBJLRange(sAdr, &_bodyBegAdr, &_bodyEndAdr, &_jmpAdr, loopInfo))
        {
            Env->ErrAdr = _curAdr;
            throw Exception("Control flow under construction");
        }
        Env->CmpStack->Clear();
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(_bodyBegAdr);
        try
        {
            de->Decompile(sAdr, CF_BJL, loopInfo);
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("Complex if->" + exception.Message);
        }
        delete de;

        if (_bodyEndAdr > _bodyBegAdr)
        {
            Env->CreateBJLSequence(sAdr, _bodyBegAdr, _bodyEndAdr);
            Env->BJLAnalyze();

            _line = "if " + Env->PrintBJL() + " then";
            Env->AddToBody(_line);
            Env->SaveContext(_bodyBegAdr);
            de = new TDecompiler(Env);
            de->SetStackPointers(this);
            de->SetDeFlags(DeFlags);
            de->SetStop(_bodyEndAdr);
            try
            {
                Env->AddToBody("begin");
                _curAdr = de->Decompile(_bodyBegAdr, 0, loopInfo);
                if (_jmpAdr && IsExit(_jmpAdr))
                {
                    Env->AddToBody("Exit;");
                }
                Env->AddToBody("end");
            }
            catch(Exception &exception)
            {
                delete de;
                throw Exception("Complex if->" + exception.Message);
            }
            Env->RestoreContext(_bodyBegAdr);//if (_jmpAdr || de->WasRet)
            delete de;

            if (_jmpAdr)
            {
                if (!IsExit(_jmpAdr))
                {
                    Env->AddToBody("else");
                    _begAdr = _curAdr;
                    Env->SaveContext(_begAdr);
                    de = new TDecompiler(Env);
                    de->SetStackPointers(this);
                    de->SetDeFlags(DeFlags);
                    de->SetStop(_jmpAdr);
                    try
                    {
                        Env->AddToBody("begin");
                        _curAdr = de->Decompile(_begAdr, CF_ELSE, loopInfo);
                        Env->AddToBody("end");
                    }
                    catch(Exception &exception)
                    {
                        delete de;
                        throw Exception("IfElse->" + exception.Message);
                    }
                    Env->RestoreContext(_begAdr);//if (de->WasRet)
                    delete de;
                }
            }
        }
        else
        {
            Env->CreateBJLSequence(sAdr, _bodyBegAdr, _bodyEndAdr);
            Env->BJLAnalyze();

            _line = "if " + Env->PrintBJL() + " then";
        }
    }
    //cycle
    else if (brType == 2)
    {
        if (bFloat) SimulateFloatInstruction(sAdr);
        if (CmpInfo.O == 'R')//not in
            _line = "if (not (" + CmpInfo.L + " in " + CmpInfo.R + ")) then";
        else
            _line = "if (" + CmpInfo.L + " " + GetInvertCondition(CmpInfo.O) + " " + CmpInfo.R + ") then";
        Env->AddToBody(_line);
        _begAdr = _curAdr;
        Env->SaveContext(_begAdr);
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(CmpAdr);
        try
        {
            Env->AddToBody("begin");
            _curAdr = de->Decompile(_begAdr, 0, loopInfo);
            Env->AddToBody("end");
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("If->" + exception.Message);
        }
        Env->RestoreContext(_begAdr);//if (de->WasRet)
        delete de;
    }
    //simple if else
    else if (brType == 3)
    {
        if (bFloat) SimulateFloatInstruction(sAdr);
        if (CmpInfo.O == 'R')//not in
            _line = "if (not (" + CmpInfo.L + " in " + CmpInfo.R + ")) then";
        else
            _line = "if (" + CmpInfo.L + " " + GetInvertCondition(CmpInfo.O) + " " + CmpInfo.R + ") then";
        Env->AddToBody(_line);
        _begAdr = _curAdr;
        Env->SaveContext(_begAdr);
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(CmpAdr);
        try
        {
            Env->AddToBody("begin");
            _curAdr = de->Decompile(_begAdr, 0, loopInfo);
            Env->AddToBody("end");
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("IfElse->" + exception.Message);
        }
        Env->RestoreContext(_begAdr);
        delete de;

        Env->AddToBody("else");
        _begAdr = _curAdr;
        Env->SaveContext(_begAdr);
        de = new TDecompiler(Env);
        de->SetStackPointers(this);
        de->SetDeFlags(DeFlags);
        de->SetStop(_jmpAdr);
        try
        {
            Env->AddToBody("begin");
            _curAdr = de->Decompile(_begAdr, CF_ELSE, loopInfo);
            Env->AddToBody("end");
        }
        catch(Exception &exception)
        {
            delete de;
            throw Exception("IfElse->" + exception.Message);
        }
        Env->RestoreContext(_begAdr);//if (de->WasRet)
        delete de;
    }
    else
    {
        if (bFloat) SimulateFloatInstruction(sAdr);
        _line = "if (" + CmpInfo.L + " " + GetDirectCondition(CmpInfo.O) + " " + CmpInfo.R + ") then Break;";
        Env->AddToBody(_line);
    }
    return _curAdr;
}
//---------------------------------------------------------------------------
//cfLoc - make XRefs
//Names
//---------------------------------------------------------------------------
//hints
//DynArrayClear == (varName := Nil);
//DynArraySetLength == SetLength(varName, colsnum, rowsnum)
//@Assign  == AssignFile
//@Close == CloseFile
//@Flush == Flush
//@ResetFile;@ResetText == Reset
//@RewritFile4@RewritText == Rewrite
//@Str2Ext == Str(val:width:prec,s)
//@Write0LString == Write
//@WriteLn == WriteLn
//@LStrPos == Pos
//@LStrCopy == Copy
//@LStrLen == Length
//@LStrDelete == Delete
//@LStrInsert == Insert
//@LStrCmp == '='
//@UStrCmp == '='
//@RandInt == Random
//@LStrOfChar == StringOfChar
//0x61 = Ord('a')
//@LStrFromChar=Chr
//@LStrToPChar=PChar
//@LStrFromArray=String(src,len)
//---------------------------------------------------------------------------
//Constructions
//---------------------------------------------------------------------------
//Length
//test reg, reg
//jz @1
//mov reg, [reg-4]
//or
//sub reg, 4
//mov reg, [reg]
//---------------------------------------------------------------------------
//mod 2
//and eax, 80000001
//jns @1
//dec eax
//or eax, 0fffffffe
//inc eax
//@1
//---------------------------------------------------------------------------
//mod 2^k
//and eax, (80000000 | (2^k - 1))
//jns @1
//dec eax
//or eax, -2^k
//inc eax
//@1
//---------------------------------------------------------------------------
//div 2^k
//test reg, reg
//jns @1
//add reg, (2^k - 1)
//sar reg, k
//@1
//---------------------------------------------------------------------------
//mod N
//mov reg, N (reg != eax)
//cdq
//idiv reg
//use edx
//---------------------------------------------------------------------------
//in [0..N]
//sub eax, (N + 1)
//jb @body
//---------------------------------------------------------------------------
//in [N1..N2,M1..M2]
//add eax, -N1
//sub eax, (N2 - N1 + 1)
//jb @body
//add eax, -(M1 - N2 - 1)
//sub eax, (M1 - M2 + 1)
//jnb @end
//@body
//...
//@end
//---------------------------------------------------------------------------
//not in [N1..N2,M1..M2]
//add eax, -N1
//sub eax, (N2 - N1 + 1)
//jb @end
//add eax, -(M1 - N2 - 1)
//sub eax, (M1 - M2 + 1)
//jb @end
//@body
//...
//@end
//---------------------------------------------------------------------------
//Int64 comparison  if (A > B) then >(jbe,jle) >=(jb,jl) <(jae,jge) <=(ja,jg)
//cmp highA,highB
//jnz(jne) @1
//{pop reg}
//{pop reg}
//cmp lowA,lowB
//jbe @2 (setnbe)
//jmp @3
//@1:
//{pop reg}
//{pop reg}
//jle @2 (setnle)
//@3
//body
//@2
//---------------------------------------------------------------------------
//LStrAddRef - if exists, then no prefix const
//---------------------------------------------------------------------------
//Exit in except block
//@DoneExcept 
//jmp ret
//@DoneExcept
//---------------------------------------------------------------------------
//If array of strings before procedure (function), then this array is declared
//in var section
//---------------------------------------------------------------------------
//mov [A],0
//mov [A + 4], 0x3FF00000
//->(Double)A := (00 00 00 00 00 00 F0 3F) Bytes in revers order! 
//---------------------------------------------------------------------------
//exetended -> tools
//push A
//push B
//push C
//C,B,A in reverse order
//---------------------------------------------------------------------------
//@IsClass<-> (eax is edx)
//---------------------------------------------------------------------------
//SE4(5D1817) - while!!!
