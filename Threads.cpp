//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Misc.h"
#include "Threads.h"
#pragma package(smart_init)
//---------------------------------------------------------------------------
extern  bool            ProjectModified;
extern  MKnowledgeBase  KnowledgeBase;
extern  TResourceInfo   *ResInfo;

//as: todo: remove all the global external dependencies
//    all the required vars should be passed into the thread

extern  int         dummy;
extern  int         DelphiVersion;
extern  DWORD       CurProcAdr;
extern  DWORD       CurUnitAdr;
extern  bool        ClassTreeDone;
extern  SysProcInfo SysProcs[];
extern  SysProcInfo SysInitProcs[];
extern  BYTE        *Code;
extern  DWORD       *Flags;
extern  DWORD       CodeSize;
extern  DWORD       CodeBase;
extern  DWORD       TotalSize;
extern  PInfoRec    *Infos;
extern  TList       *OwnTypeList;
extern  int         VmtSelfPtr;
extern  int         VmtInitTable;
extern  int         VmtInstanceSize;
extern  int         VmtParent;
extern  int         VmtClassName;
extern  int         VmtIntfTable;
extern  int         VmtAutoTable;
extern  int         VmtFieldTable;
extern  int         VmtMethodTable;
extern  int         VmtDynamicTable;
extern  int         VmtTypeInfo;
extern  int         VmtDestroy;
extern  int         UnitsNum;
extern  TList       *ExpFuncList;
extern  TList       *VmtList;
extern  TList       *Units;
extern  DWORD       EP;
extern  DWORD       HInstanceVarAdr;
extern  int         LastResStrNo;
extern  MDisasm     Disasm;

//as: print every 10th address in status bar (analysis time booster)
static const int SKIPADDR_COUNT = 10;
//---------------------------------------------------------------------------
__fastcall TAnalyzeThread::TAnalyzeThread(TFMain_11011981* AForm, TFProgressBar* ApbForm, bool AllValues)
    : TThread(true)
{
    Priority = tpLower;
    mainForm = AForm;
    pbForm = ApbForm; 
    all = AllValues;
    adrCnt = 0;
}
//---------------------------------------------------------------------------
__fastcall TAnalyzeThread::~TAnalyzeThread()
{
}
//---------------------------------------------------------------------------
int __fastcall TAnalyzeThread::GetRetVal()
{
    return ReturnValue;
}
//---------------------------------------------------------------------------
//PopupMenu items!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//The Execute method is called when the thread starts
void __fastcall TAnalyzeThread::Execute()
{
    try
    {
        if (all)
        {
            //1
            StrapSysProcs();
            ReturnValue = 1;
            CurProcAdr = EP;
            UpdateCode();
            //2
            if (DelphiVersion != 2)
            {
                FindRTTIs();
                ReturnValue = 2;
                UpdateUnits();
            }
            //3
            if (DelphiVersion == 2)
                FindVMTs2();
            else
                FindVMTs();
            ReturnValue = 3;
            UpdateVmtList();
            UpdateRTTIs();
            UpdateCode();
            //4
            StrapVMTs();
            ReturnValue = 4;
            UpdateCode();
            //5
            ScanCode();
            ReturnValue = 5;
            UpdateCode();
            //6
            ScanVMTs();
            ReturnValue = 6;
            UpdateVmtList();
            UpdateCode();
            UpdateShortClassViewer();
            //7
            ScanConsts();
            ReturnValue = 7;
            UpdateUnits();
            //8
            ScanGetSetStoredProcs();
            ReturnValue = 8;
            //9
            FindStrings();
            ReturnValue = 9;
            //10
            AnalyzeCode1();
            ReturnValue = 10;
            UpdateCode();
            UpdateXrefs();
            //11
            ScanCode1();
            ReturnValue = 11;
            UpdateCode();
            //12
            PropagateClassProps();
            ReturnValue = 12;
            UpdateCode();
            //13
            FindTypeFields();
            ReturnValue = 13;
            UpdateCode();
            //14
            FindPrototypes();
            ReturnValue = 14;
            UpdateCode();
            //15
            AnalyzeCode2(true);
            ReturnValue = 15;
            UpdateCode();
            UpdateStrings();
            //16
            AnalyzeDC();
            ReturnValue = 16;
            UpdateCode();
            //17
            AnalyzeCode2(false);
            ReturnValue = 17;
            UpdateCode();
            UpdateStrings();
            //18
            AnalyzeDC();
            ReturnValue = 18;
            UpdateCode();
            //19
            AnalyzeCode2(false);
            ReturnValue = LAST_ANALYZE_STEP;
            UpdateCode();
            UpdateStrings();

            UpdateBeforeClassViewer();
        }
        //20
        FillClassViewer();
        ReturnValue = LAST_ANALYZE_STEP + 1;
        UpdateClassViewer();
    }
    catch (Exception& e)
    {
        Application->ShowException(&e);
    }

    //as update main wnd about operation over
    //only Post() here!) - async, otehrwise deadlock!
    ::PostMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taFinished, 0);
}
//---------------------------------------------------------------------------
const int PB_MAX_STEPS = 2048;
int __fastcall TAnalyzeThread::StartProgress(int pbMaxCount, const String& sbText)
{
    int stepSize = 1;
    int pbSteps = pbMaxCount / stepSize;
    if (pbSteps * stepSize < pbMaxCount) pbSteps++;

    if (pbMaxCount > PB_MAX_STEPS)
    {
        stepSize = 256;
        while (pbSteps > PB_MAX_STEPS)
        {
            stepSize *= 2;
            pbSteps = pbMaxCount / stepSize;
            if (pbSteps * stepSize < pbMaxCount) pbSteps++;
        }
    }
    ThreadAnalysisData* startOperation = new ThreadAnalysisData(pbSteps, sbText);
    ::SendMessage(pbForm->Handle, WM_UPDANALYSISSTATUS, (int)taStartPrBar, (long)startOperation);//Post

    return stepSize - 1;
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateProgress()
{
    if (!Terminated)
        ::SendMessage(pbForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdatePrBar, 0);//Post
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::StopProgress()
{
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateStatusBar(int adr)
{
    //if (Terminated)
    //    throw Exception("Termination request2");
    if (!Terminated)
    {
        ThreadAnalysisData* updateStatusBar = new ThreadAnalysisData(0, Val2Str8(adr));
        ::SendMessage(pbForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateStBar, (long)updateStatusBar);
    }
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateStatusBar(const String& sbText)
{
    //if (Terminated)
    //    throw Exception("Termination request3");
    if (!Terminated)
    {
        ThreadAnalysisData* updateStatusBar = new ThreadAnalysisData(0, sbText);
        ::SendMessage(pbForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateStBar, (long)updateStatusBar);
    }
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateAddrInStatusBar(DWORD adr)
{
    if (!Terminated)
    {
        adrCnt++;
        if (adrCnt == SKIPADDR_COUNT)
        {
            UpdateStatusBar(adr);
            adrCnt = 0;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateUnits()
{
    if (!Terminated)
    {
        long isLastStep = long(ReturnValue == LAST_ANALYZE_STEP);
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateUnits, isLastStep);
    }
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateRTTIs()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateRTTIs, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateVmtList()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateVmtList, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateStrings()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateStrings, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateCode()
{
    if (!Terminated)
    {
        UpdateUnits();
        //cant use Post here, there are some global shared vars!
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateCode, 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateXrefs()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateXrefs, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateShortClassViewer()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateShortClassViewer, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateClassViewer()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateClassViewer, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::UpdateBeforeClassViewer()
{
    if (!Terminated)
        ::SendMessage(mainForm->Handle, WM_UPDANALYSISSTATUS, (int)taUpdateBeforeClassViewer, 0);
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::StrapSysProcs()
{
    int         n, Idx, pos;
    MProcInfo   aInfo, *pInfo;

    WORD moduleID = KnowledgeBase.GetModuleID("System");
    for (n = 0; SysProcs[n].name && !Terminated; n++)
    {
        Idx = KnowledgeBase.GetProcIdx(moduleID, SysProcs[n].name);
        if (Idx != -1)
        {
            Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
            if (!KnowledgeBase.IsUsedProc(Idx))
            {
                pInfo = KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, &aInfo);
                if (SysProcs[n].impAdr)
                {
                    mainForm->StrapProc(Adr2Pos(SysProcs[n].impAdr), Idx, pInfo, false, 6);
                }
                else
                {
                    pos = KnowledgeBase.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pInfo && pos != -1)
                    {
                        mainForm->StrapProc(pos, Idx, pInfo, true, pInfo->DumpSz);
                    }
                }
            }
        }
    }

    moduleID = KnowledgeBase.GetModuleID("SysInit");
    for (n = 0; SysInitProcs[n].name && !Terminated; n++)
    {
        Idx = KnowledgeBase.GetProcIdx(moduleID, SysInitProcs[n].name);
        if (Idx != -1)
        {
            Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
            if (!KnowledgeBase.IsUsedProc(Idx))
            {
                pInfo = KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, &aInfo);
                pos = KnowledgeBase.ScanCode(Code, Flags, CodeSize, pInfo);
                if (pInfo && pos != -1)
                {
                    mainForm->StrapProc(pos, Idx, pInfo, true, pInfo->DumpSz);
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::FindRTTIs()
{
    BYTE		paramCnt, numOps, methodKind, flags;
    WORD        dw, Count, methCnt;
    DWORD       typInfo, baseTypeAdr, dd, procSig;
    int			j, n, m, minValue, maxValue, elNum, pos, instrLen;
    PInfoRec    recN, recN1;
    String      name, unitName;
    DISINFO     DisInfo;

    int stepMask = StartProgress(CodeSize, "Find Types");

    for (int i = 0; i < CodeSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        DWORD adr = *((DWORD*)(Code + i));
        if (IsValidImageAdr(adr) && adr == Pos2Adr(i) + 4)
        {
            //Euristica - look at byte Code + i - 3 - may be case (jmp [adr + reg*4])
            instrLen = Disasm.Disassemble(Code + i - 3, (__int64)Pos2Adr(i) - 3, &DisInfo, 0);
            if (instrLen > 3 && DisInfo.Branch && DisInfo.Offset == adr) continue;

        	DWORD typeAdr = adr - 4;
            BYTE typeKind = Code[i + 4];
            if (typeKind == ikUnknown || typeKind > ikProcedure) continue;

            BYTE len = Code[i + 5];
            if (!IsValidName(len, i + 6)) continue;

            String TypeName = GetTypeName(adr);
            UpdateStatusBar(TypeName);
            /*
            //Names that begins with '.'
            if (TypeName[1] == '.')
            {
                String prefix;
                switch (typeKind)
                {
                case ikEnumeration:
                    prefix = "_Enumeration_";
                    break;
                case ikArray:
                    prefix = "_Array_";
                    break;
                case ikDynArray:
                    prefix = "_DynArray_";
                    break;
                default:
                    prefix = form->GetUnitName(recU);
                    break;
                }
                TypeName = prefix + Val2Str0(recU->iniOrder) + "_" + TypeName.SubString(2, len);
            }
            */

            n = i + 6 + len;
            SetFlag(cfRTTI, i);
            unitName = "";

            switch (typeKind)
            {
            case ikInteger:         //1
            	n++;	//ordType
                n += 4;	//MinVal
                n += 4;	//MaxVal
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikChar:            //2
            	n++;	//ordType
                n += 4;	//MinVal
                n += 4;	//MaxVal
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikEnumeration:     //3
                n++;    //ordType
                minValue = *((int*)(Code + n)); n += 4;
                maxValue = *((int*)(Code + n)); n += 4;
                //BaseType
                baseTypeAdr = *((DWORD*)(Code + n)); n += 4;

                if (baseTypeAdr == typeAdr)
                {
                    if (SameText(TypeName, "ByteBool") ||
                        SameText(TypeName, "WordBool") ||
                        SameText(TypeName, "LongBool"))
                    {
                        minValue = 0;
                        maxValue = 1;
                    }
                    for (j = minValue; j <= maxValue; j++)
                    {
                        len = Code[n];
                        n += len + 1;
                    }
                }
                /*
                //UnitName
                len = Code[n];
                if (IsValidName(len, n + 1))
                {
                    unitName = String((char*)(Code + n + 1), len).Trim();
                    SetUnitName(recU, unitName);
                }
                 n += len + 1;
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n)); n += dw;
                }
                */
                SetFlags(cfData, i, n - i);
                break;
            case ikFloat:           //4
                n++;    //FloatType
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
			case ikString:          //5
                n++;    //MaxLength
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikSet:             //6
                n += 1 + 4;     //ordType+CompType
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikClass:           //7
                n += 4;     //classVMT
                n += 4;     //ParentInfo
                n += 2;     //PropCount
                //UnitName
                len = Code[n];
                if (IsValidName(len, n + 1))
                {
                    unitName = String((char*)(Code + n + 1), len).Trim();
                    //FMain_11011981->SetUnitName(recU, unitName);
                }
                n += len + 1;
                //PropData
                Count = *((WORD*)(Code + n)); n += 2;
                for (j = 0; j < Count; j++)
                {
                    //TPropInfo
                    n += 26;
                    len = Code[n]; n += len + 1;
                }
                if (DelphiVersion >= 2010)
                {
                    //PropDataEx
                    Count = *((WORD*)(Code + n)); n += 2;
                    for (j = 0; j < Count; j++)
                    {
                        //TPropInfoEx
                        //Flags
                        n++;
                        //Info
                        typInfo = *((DWORD*)(Code + n)); n += 4;
                        pos = Adr2Pos(typInfo);
                        len = Code[pos + 26];
                        SetFlags(cfData, pos, 27 + len);
                        //AttrData
                        dw = *((WORD*)(Code + n));
                        n += dw;//ATR!!
                    }
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                    if (DelphiVersion >= 2012)
                    {
                        //ArrayPropCount
                        Count = *((WORD*)(Code + n)); n += 2;
                        //ArrayPropData
                        for (j = 0; j < Count; j++)
                        {
                            //Flags
                            n++;
                            //ReadIndex
                            n += 2;
                            //WriteIndex
                            n += 2;
                            //Name
                            len = Code[n]; n += len + 1;
                            //AttrData
                            dw = *((WORD*)(Code + n));
                            n += dw;//ATR!!
                        }
                    }
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikMethod:          //8
                //MethodKind
                methodKind = Code[n]; n++;  //0 (mkProcedure) or 1 (mkFunction)
                paramCnt = Code[n]; n++;
                for (j = 0; j < paramCnt; j++)
                {
                    n++;        //Flags
                    len = Code[n]; n += len + 1;    //ParamName
                    len = Code[n]; n += len + 1;    //TypeName
                }
                if (methodKind)
                {
                    //ResultType
                    len = Code[n]; n += len + 1;
                    if (DelphiVersion > 6)
                    {
                        //ResultTypeRef
                        n += 4;
                    }
                }
                if (DelphiVersion > 6)
                {
                    //CC
                    n++;
                    //ParamTypeRefs
                    n += 4*paramCnt;
                    if (DelphiVersion >= 2010)
                    {
                        //MethSig
                        procSig = *((DWORD*)(Code + n)); n += 4;
                        //AttrData
                        dw = *((WORD*)(Code + n));
                        n += dw;//ATR!!
                        //Procedure Signature
                        if (procSig)
                        {
                            if (IsValidImageAdr(procSig))
                                pos = Adr2Pos(procSig);
                            else
                                pos = i + procSig;
                            m = 0;
                            //Flags
                            flags = Code[pos]; m++;
                            if (flags != 0xFF)
                            {
                                //CC
                                m++;
                                //ResultType
                                m += 4;
                                //ParamCount
                                paramCnt = Code[pos + m]; m++;
                                for (j = 0; j < paramCnt; j++)
                                {
                                    //Flags
                                    m++;
                                    //ParamType
                                    m += 4;
                                    //Name
                                    len = Code[pos + m]; m += len + 1;
                                    //AttrData
                                    dw = *((WORD*)(Code + pos + m));
                                    m += dw;//ATR!!
                                }
                            }
                            SetFlags(cfData, pos, m);
                        }
                    }
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikWChar:           //9
                n++;        //ordType
                n += 4;     //MinVal
                n += 4;     //MaxVal
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikLString:         //0xA
            	//CodePage
                if (DelphiVersion >= 2009) n += 2;
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikWString:         //0xB
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikVariant:         //0xC
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikArray:           //0xD
                n += 4;     //Size
                n += 4;     //ElCount
                n += 4;     //ElType
                if (DelphiVersion >= 2010)
                {
                    //DimCount
                    paramCnt = Code[n]; n++;
                    for (j = 0; j < paramCnt; j++)
                    {
                        //Dims
                        n += 4;
                    }
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikRecord:          //0xE
                n += 4; //Size
                elNum = *((int*)(Code + n)); n += 4;    //ManagedFldCount
                for (j = 0; j < elNum; j++)
                {
                    n += 4; //TypeRef
                    n += 4; //FldOffset
                }
                
                if (DelphiVersion >= 2010)
                {
                    numOps = Code[n]; n++;  //NumOps
                    for (j = 0; j < numOps; j++)    //RecOps
                    {
                        n += 4;
                    }
                    elNum = *((int*)(Code + n)); n += 4;    //RecFldCnt
                    for (j = 0; j < elNum; j++)
                    {
                        n += 4; //TypeRef
                        n += 4; //FldOffset
                        n++;    //Flags
                        len = Code[n]; n += len + 1;    //Name
                        dw = *((WORD*)(Code + n));
                        if (dw != 2)
                            dummy = 1;
                        n += dw;//ATR!!
                    }
                    dw = *((WORD*)(Code + n));
                    if (dw != 2)
                        dummy = 1;
                    n += dw;//ATR!!
                    if (DelphiVersion >= 2012)
                    {
                        methCnt = *((WORD*)(Code + n)); n += 2;
                        for (j = 0; j < methCnt; j++)
                        {
                            n++;    //Flags
                            n += 4; //Code
                            len = Code[n]; n += len + 1;    //Name
                            //ProcedureSignature
                            //Flags
                            flags = Code[n]; n++;
                            if (flags != 0xFF)
                            {
                                //CC
                                n++;
                                //ResultType
                                n += 4;
                                //ParamCnt
                                paramCnt = Code[n]; n++;
                                //Params
                                for (m = 0; m < paramCnt; m++)
                                {
                                    n++;    //Flags
                                    n += 4; //ParamType
                                    len = Code[n]; n += len + 1;    //Name
                                    dw = *((WORD*)(Code + n));
                                    if (dw != 2)
                                        dummy = 1;
                                    n += dw;//ATR!!
                                }
                            }
                            dw = *((WORD*)(Code + n));
                            if (dw != 2)
                                dummy = 1;
                            n += dw;//ATR!!
                        }
                    }
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikInterface:       //0xF
                n += 4;     //IntfParent
                n++;        //IntfFlags
                n += 16;    //GUID
                //UnitName
                len = Code[n];
                if (IsValidName(len, n + 1))
                {
                    unitName = String((char*)(Code + n + 1), len).Trim();
                    //FMain_11011981->SetUnitName(recU, unitName);
                }
                n += len + 1;
                //PropCount
                Count = *((WORD*)(Code + n)); n += 2;
                if (DelphiVersion >= 6)
                {
                    //RttiCount
                    dw = *((WORD*)(Code + n)); n += 2;
                    if (dw != 0xFFFF)
                    {
                        if (DelphiVersion >= 2010)
                        {
                            for (j = 0; j < Count; j++)
                            {
                                //Name
                                len = Code[n]; n += len + 1;
                                //Kind
                                methodKind = Code[n]; n++;
                                //CallConv
                                n++;
                                //ParamCount
                                paramCnt = Code[n]; n++;
                                for (m = 0; m < paramCnt; m++)
                                {
                                    //Flags
                                    n++;
                                    //ParamName
                                    len = Code[n]; n += len + 1;
                                    //TypeName
                                    len = Code[n]; n += len + 1;
                                    //ParamType
                                    n += 4;
                                    //AttrData
                                    dw = *((WORD*)(Code + n));
                                    n += dw;//ATR!!
                                }
                                if (methodKind)
                                {
                                    //ResultTypeName
                                    len = Code[n]; n += len + 1;
                                    if (len)
                                    {
                                        //ResultType
                                        n += 4;
                                        //////////////////////////// Insert by Pigrecos
                                        //AttrData
                                        dw = *((WORD*)(Code + n));
                                        n += dw;//ATR!!
                                        ////////////////////////////
                                    }
                                }
                            }
                        }
                        else
                        {
                            for (j = 0; j < Count; j++)
                            {
                                //PropType
                                n += 4;
                                //GetProc
                                n += 4;
                                //SetProc
                                n += 4;
                                //StoredProc
                                n += 4;
                                //Index
                                n += 4;
                                //Default
                                n += 4;
                                //NameIndex
                                n += 2;
                                //Name
                                len = Code[n]; n += len + 1;
                            }
                        }
                    }
                    if (DelphiVersion >= 2010)
                    {
                        //AttrData
                        dw = *((WORD*)(Code + n));
                        n += dw;//ATR!!
                    }
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikInt64:           //0x10
                n += 8;     //MinVal
                n += 8;     //MaxVal
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikDynArray:        //0x11
                n += 4;     //elSize
                n += 4;     //elType
                n += 4;     //varType
                if (DelphiVersion >= 6)
                {
                    n += 4;     //elType2
                    //UnitName
                    len = Code[n];
                    if (IsValidName(len, n + 1))
                    {
                        unitName = String((char*)(Code + n + 1), len).Trim();
                        //FMain_11011981->SetUnitName(recU, unitName);
                    }
                     n += len + 1;
                }
                if (DelphiVersion >= 2010)
                {
                    //DynArrElType
                    n += 4;
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikUString:         //0x12
                if (DelphiVersion >= 2010)
                {
                    //AttrData
                    dw = *((WORD*)(Code + n));
                    n += dw;//ATR!!
                }
                SetFlags(cfData, i, n - i);
                break;
            case ikClassRef:        //0x13
                //InstanceType
                n += 4;
                //AttrData
                dw = *((WORD*)(Code + n));
                n += dw;//ATR!!
                SetFlags(cfData, i, n - i);
            	break;
            case ikPointer:         //0x14
                //RefType
                n += 4;
                //AttrData
                dw = *((WORD*)(Code + n));
                n += dw;//ATR!!
                SetFlags(cfData, i, n - i);
            	break;
            case ikProcedure:       //0x15
                //MethSig
                procSig = *((DWORD*)(Code + n)); n += 4;
                //AttrData
                dw = *((WORD*)(Code + n));
                n += dw;//ATR!!
                SetFlags(cfData, i, n - i);
                //Procedure Signature
                if (procSig)
                {
                    if (IsValidImageAdr(procSig))
                        pos = Adr2Pos(procSig);
                    else
                        pos = i + procSig;
                    m = 0;
                    //Flags
                    flags = Code[pos]; m++;
                    if (flags != 0xFF)
                    {
                        //CC
                        m++;
                        //ResultType
                        m += 4;
                        //ParamCount
                        paramCnt = Code[pos + m]; m++;
                        for (j = 0; j < paramCnt; j++)
                        {
                            //Flags+ParamType
                            m += 5;
                            //Name
                            len = Code[pos + m]; m += len + 1;
                            //AttrData
                            dw = *((WORD*)(Code + pos + m));
                            m += dw;//ATR!!
                        }
                    }
                    SetFlags(cfData, pos, m);
                }
            	break;
            }

            if (!Infos[i])
            {
            	recN = new InfoRec(i, typeKind);
	            recN->SetName(TypeName);
            }
            PTypeRec recT = new TypeRec;
            recT->kind = typeKind;
            recT->adr = typeAdr;
            if (unitName != "") TypeName += " (" + unitName + ")";
            recT->name = TypeName;
            OwnTypeList->Add(recT);
        }
	}
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::FindVMTs2()
{
    BYTE    len;
    WORD    Num16;
    int     i, pos, bytes, _ap, _ap0;
    DWORD   Num32;
    String  TypeName;

    int stepMask = StartProgress(CodeSize, "Find Virtual Method Tables D2");

    for (i = 0; i < CodeSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfCode | cfData, i)) continue;

        DWORD selfPtr = *((DWORD*)(Code + i));
        if (selfPtr && !IsValidImageAdr(selfPtr)) continue;

        DWORD initTableAdr = *((DWORD*)(Code + i + 4));
        if (initTableAdr)
        {
            if (!IsValidImageAdr(initTableAdr)) continue;
            if (*(Code + pos) != 0xE) continue;
            pos = Adr2Pos(initTableAdr);
            Num32 = *((DWORD*)(Code + pos + 5));
            if (Num32 > 10000) continue;
        }

        DWORD typeInfoAdr = *((DWORD*)(Code + i + 8));
        if (typeInfoAdr)
        {
            if (!IsValidImageAdr(typeInfoAdr)) continue;
            //typeInfoAdr contains kind of type
            pos = Adr2Pos(typeInfoAdr);
            BYTE typeKind = *(Code + pos);
            if (typeKind > tkVariant) continue;
            //len = *(Code + pos + 1);
            //if (!IsValidName(len, pos + 2)) continue;
        }

        DWORD fieldTableAdr = *((DWORD*)(Code + i + 0xC));
        if (fieldTableAdr)
        {
            if (!IsValidImageAdr(fieldTableAdr)) continue;
            pos = Adr2Pos(fieldTableAdr);
            Num16 = *((WORD*)(Code + pos));
            if (Num16 > 10000) continue;
        }

        DWORD methodTableAdr = *((DWORD*)(Code + i + 0x10));
        if (methodTableAdr)
        {
            if (!IsValidImageAdr(methodTableAdr)) continue;
            pos = Adr2Pos(methodTableAdr);
            Num16 = *((WORD*)(Code + pos));
            if (Num16 > 10000) continue;
        }

        DWORD dynamicTableAdr = *((DWORD*)(Code + i + 0x14));
        if (dynamicTableAdr)
        {
            if (!IsValidImageAdr(dynamicTableAdr)) continue;
            pos = Adr2Pos(dynamicTableAdr);
            Num16 = *((WORD*)(Code + pos));
            if (Num16 > 10000) continue;
        }

        DWORD classNameAdr = *((DWORD*)(Code + i + 0x18));
        if (!classNameAdr || !IsValidImageAdr(classNameAdr)) continue;

        //_ap = Adr2Pos(classNameAdr);
        //len = *(Code + _ap);
        //if (!IsValidName(len, _ap + 1)) continue;

        DWORD instanceSize = *((DWORD*)(Code + i + 0x1C));
        if (!instanceSize) continue;

        DWORD parentAdr = *((DWORD*)(Code + i + 0x20));
        if (parentAdr && !IsValidImageAdr(parentAdr)) continue;

        DWORD defaultHandlerAdr = *((DWORD*)(Code + i + 0x24));
        if (defaultHandlerAdr && !IsValidImageAdr(defaultHandlerAdr)) continue;

        DWORD newInstanceAdr = *((DWORD*)(Code + i + 0x28));
        if (newInstanceAdr && !IsValidImageAdr(newInstanceAdr)) continue;

        DWORD freeInstanceAdr = *((DWORD*)(Code + i + 0x2C));
        if (freeInstanceAdr && !IsValidImageAdr(freeInstanceAdr)) continue;

        DWORD destroyAdr = *((DWORD*)(Code + i + 0x30));
        if (destroyAdr && !IsValidImageAdr(destroyAdr)) continue;

        DWORD classVMT = Pos2Adr(i) - VmtSelfPtr;
        if (Adr2Pos(classVMT) < 0) continue;

        int StopAt = GetStopAt(classVMT);
        if (i + StopAt - classVMT - VmtSelfPtr >= CodeSize) continue;

        _ap = Adr2Pos(classNameAdr);
        if (_ap <= 0) continue;
        len = *(Code + _ap);
        TypeName = String((char*)(Code + _ap + 1), len);
        UpdateStatusBar(TypeName);

        //Add to TypeList
        PTypeRec recT = new TypeRec;
        recT->kind = ikVMT;
        recT->adr = Pos2Adr(i);
        recT->name = TypeName;
        OwnTypeList->Add(recT);

        //Name already used
        SetFlags(cfData, _ap, len + 1);

        if (!Infos[i])
        {
            PInfoRec recN = new InfoRec(i, ikVMT);
            recN->SetName(TypeName);
        }
        //InitTable
        if (initTableAdr)
        {
            pos = Adr2Pos(initTableAdr); bytes = 0;
            pos++; bytes++;         //skip 0xE
            pos++; bytes++;         //unknown byte
            pos += 4; bytes += 4;   //unknown dd
            Num32 = *((DWORD*)(Code + pos)); bytes += 4;

            for (int m = 0; m < Num32; m++)
            {
                //Type offset (Information about types already extracted)
                bytes += 4;
                //FieldOfs
                bytes += 4;
            }
            //InitTable used
            SetFlags(cfData, Adr2Pos(initTableAdr), bytes);
        }
        //FieldTable
        if (fieldTableAdr)
        {
            pos = Adr2Pos(fieldTableAdr); bytes = 0;
            Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;
            //TypesTab
            DWORD typesTab = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;

            for (int m = 0; m < Num16; m++)
            {
                //FieldOfs
                pos += 4; bytes += 4;
                //idx
                pos += 2; bytes += 2;
                len = Code[pos]; pos++; bytes++;
                //name
                pos += len; bytes += len;
            }
            //FieldTable used
            SetFlags(cfData, Adr2Pos(fieldTableAdr), bytes);
            //Use TypesTab
            Num16 = *((WORD*)(Code + Adr2Pos(typesTab)));
            SetFlags(cfData, Adr2Pos(typesTab), 2 + Num16*4);
        }
        //MethodTable
        if (methodTableAdr)
        {
            pos = Adr2Pos(methodTableAdr); bytes = 0;
            Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;

            for (int m = 0; m < Num16; m++)
            {
                //Length of Method Description
                WORD skipNext = *((WORD*)(Code + pos)); pos += skipNext; bytes += skipNext;
            }
            //MethodTable used
            SetFlags(cfData, Adr2Pos(methodTableAdr), bytes);
        }
        //DynamicTable
        if (dynamicTableAdr)
        {
            pos = Adr2Pos(dynamicTableAdr); bytes = 0;
            Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;

            for (int m = 0; m < Num16; m++)
            {
                //Msg
                bytes += 2;
                //ProcAdr
                bytes += 4;
            }
            //DynamicTable used
            SetFlags(cfData, Adr2Pos(dynamicTableAdr), bytes);
        }

        //DWORD StopAt = GetStopAt(classVMT);
        //Использовали виртуальную таблицу
        SetFlags(cfData, i, StopAt - classVMT - VmtSelfPtr);

        PUnitRec recU = mainForm->GetUnit(classVMT);
        if (recU)
        {
            if (typeInfoAdr)    //extract unit name
            {
                _ap0 = Adr2Pos(typeInfoAdr); _ap = _ap0;
                BYTE b = Code[_ap]; _ap++;
                if (b != 7) continue;
                len = Code[_ap]; _ap++;
                if (!IsValidName(len, _ap)) continue;
                _ap += len + 10;
                len = Code[_ap]; _ap++;
                if (!IsValidName(len, _ap)) continue;
                String unitName = String((char*)(Code + _ap), len).Trim(); _ap += len;
                FMain_11011981->SetUnitName(recU, unitName);
                //Use information about Unit
                SetFlags(cfData, _ap0, _ap - _ap0);
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
//Collect information from VMT structure
void __fastcall TAnalyzeThread::FindVMTs()
{
    WORD    Num16;
    int     bytes, pos, posv, EntryCount;
    DWORD   Num32;

    int stepMask = StartProgress(TotalSize, "Find Virtual Method Tables");

    for (int i = 0; i < TotalSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfCode | cfData, i)) continue;
        DWORD adr = *((DWORD*)(Code + i));  //Points to vmt0 (VmtSelfPtr)
        if (IsValidImageAdr(adr) && Pos2Adr(i) == adr + VmtSelfPtr)
        {
            DWORD classVMT = adr;
            DWORD StopAt = GetStopAt(classVMT);
            //if (i + StopAt - classVMT - VmtSelfPtr >= CodeSize) continue;

            DWORD intfTableAdr = *((DWORD*)(Code + i + 4));
            if (intfTableAdr)
            {
                if (!IsValidImageAdr(intfTableAdr)) continue;
                pos = Adr2Pos(intfTableAdr);
                EntryCount = *((DWORD*)(Code + pos));
                if (EntryCount > 10000) continue;
            }

            DWORD autoTableAdr = *((DWORD*)(Code + i + 8));
            if (autoTableAdr)
            {
                if (!IsValidImageAdr(autoTableAdr)) continue;
                pos = Adr2Pos(autoTableAdr);
                EntryCount = *((DWORD*)(Code + pos));
                if (EntryCount > 10000) continue;
            }

            DWORD initTableAdr = *((DWORD*)(Code + i + 0xC));
            if (initTableAdr)
            {
                if (!IsValidImageAdr(initTableAdr)) continue;
                pos = Adr2Pos(initTableAdr);
                Num32 = *((DWORD*)(Code + pos + 6));
                if (Num32 > 10000) continue;
            }

            DWORD typeInfoAdr = *((DWORD*)(Code + i + 0x10));
            if (typeInfoAdr)
            {
                if (!IsValidImageAdr(typeInfoAdr)) continue;
                //По адресу typeInfoAdr должны быть данные о типе, начинающиеся с определенной информации
                pos = Adr2Pos(typeInfoAdr);
                BYTE typeKind = *(Code + pos);
                if (typeKind > ikProcedure) continue;
                //len = *(Code + pos + 1);
                //if (!IsValidName(len, pos + 2)) continue;
            }

            DWORD fieldTableAdr = *((DWORD*)(Code + i + 0x14));
            if (fieldTableAdr)
            {
                if (!IsValidImageAdr(fieldTableAdr)) continue;
                pos = Adr2Pos(fieldTableAdr);
                Num16 = *((WORD*)(Code + pos));
                if (Num16 > 10000) continue;
            }

            DWORD methodTableAdr = *((DWORD*)(Code + i + 0x18));
            if (methodTableAdr)
            {
                if (!IsValidImageAdr(methodTableAdr)) continue;
                pos = Adr2Pos(methodTableAdr);
                Num16 = *((WORD*)(Code + pos));
                if (Num16 > 10000) continue;
            }

            DWORD dynamicTableAdr = *((DWORD*)(Code + i + 0x1C));
            if (dynamicTableAdr)
            {
                if (!IsValidImageAdr(dynamicTableAdr)) continue;
                pos = Adr2Pos(dynamicTableAdr);
                Num16 = *((WORD*)(Code + pos));
                if (Num16 > 10000) continue;
            }

            DWORD classNameAdr = *((DWORD*)(Code + i + 0x20));
            if (!classNameAdr || !IsValidImageAdr(classNameAdr)) continue;

            //n = Adr2Pos(classNameAdr);
            //len = *(Code + n);
            //if (!IsValidName(len, n + 1)) continue;

            DWORD parentAdr = *((DWORD*)(Code + i + 0x28));
            if (parentAdr && !IsValidImageAdr(parentAdr)) continue;

            int n = Adr2Pos(classNameAdr);
            BYTE len = Code[n];
            //if (!IsValidName(len, n + 1)) continue;
            String TypeName = String((char*)(Code + n + 1), len);
            UpdateStatusBar(TypeName);
            
            //Add to TypeList
            PTypeRec recT = new TypeRec;
            recT->kind = ikVMT;
            recT->adr = Pos2Adr(i);
            recT->name = TypeName;
            OwnTypeList->Add(recT);

            //Name already use
            SetFlags(cfData, n, len + 1);

            if (!GetInfoRec(Pos2Adr(i)))
            {
                PInfoRec recN = new InfoRec(i, ikVMT);
                recN->SetName(TypeName);
            }
            SetFlag(cfData, i);

            //IntfTable
            DWORD vTableAdr;

            if (intfTableAdr)
            {
                pos = Adr2Pos(intfTableAdr); bytes = 0;
                SetFlag(cfData | cfVTable, pos);
                EntryCount = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;
                for (int m = 0; m < EntryCount; m++)
                {
                    //GUID
                    pos += 16; bytes += 16;
                    vTableAdr = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;
                    if (IsValidImageAdr(vTableAdr)) SetFlag(cfData | cfVTable, Adr2Pos(vTableAdr));
                    //IOffset
                    pos += 4; bytes += 4;
                    if (DelphiVersion > 3)
                    {
                        //ImplGetter
                        pos += 4; bytes += 4;
                    }
                }
                //Intfs
                if (DelphiVersion >= 2012) bytes += EntryCount * 4;

                //Use IntfTable
                SetFlags(cfData, Adr2Pos(intfTableAdr), bytes);
                //Second pass (to use already set flags)
                pos = Adr2Pos(intfTableAdr) + 4;
                for (int m = 0; m < EntryCount; m++)
                {
                    //Skip GUID
                    pos += 16;
                    vTableAdr = *((DWORD*)(Code + pos)); pos += 4;
                    //IOffset
                    pos += 4;
                    if (DelphiVersion > 3)
                    {
                        //ImplGetter
                        pos += 4;
                    }
                    //Use VTable
                    if (IsValidImageAdr(vTableAdr))
                    {
                        DWORD vEnd = vTableAdr;
                        DWORD vStart = vTableAdr;
                        posv = Adr2Pos(vTableAdr); bytes = 0;
                        for (int k = 0;; k++)
                        {
                            if (Pos2Adr(posv) == intfTableAdr) break;
                            DWORD vAdr = *((DWORD*)(Code + posv)); posv += 4; bytes += 4;
                            if (vAdr && vAdr < vStart) vStart = vAdr;
                        }
                        //Use VTable
                        SetFlags(cfData, Adr2Pos(vEnd), bytes);
                        //Leading always byte CC
                        vStart--;
                        //Use all refs
                        SetFlags(cfData, Adr2Pos(vStart), vEnd - vStart);
                    }
                }
            }
            //AutoTable
            if (autoTableAdr)
            {
                pos = Adr2Pos(autoTableAdr); bytes = 0;
                EntryCount = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;
                for (int m = 0; m < EntryCount; m++)
                {
                    //DispID
                    pos += 4; bytes += 4;
                    //NameAdr
                    DWORD pos1 = Adr2Pos(*((DWORD*)(Code + pos))); pos += 4; bytes += 4;
                    len = Code[pos1];
                    //Use name
                    SetFlags(cfData, pos1, len + 1);
                    //Flags
                    pos += 4; bytes += 4;
                    //ParamsAdr
                    pos1 = Adr2Pos(*((DWORD*)(Code + pos))); pos += 4; bytes += 4;
                    BYTE ParamCnt = Code[pos1 + 1];
                    //Use Params
                    SetFlags(cfData, pos1, ParamCnt + 2);
                    //AddressAdr
                    pos += 4; bytes += 4;
                }
                //Use AutoTable
                SetFlags(cfData, Adr2Pos(autoTableAdr), bytes);
            }
            //InitTable
            if (initTableAdr)
            {
                pos = Adr2Pos(initTableAdr); bytes = 0;
                //Skip 0xE
                pos++; bytes++;
                //Unknown byte
                pos++; bytes++;
                //Unknown dword
                pos += 4; bytes += 4;
                Num32 = *((DWORD*)(Code + pos)); bytes += 4;

                for (int m = 0; m < Num32; m++)
                {
                    //TypeOfs (information about types is already extracted)
                    bytes += 4;
                    //FieldOfs
                    bytes += 4;
                }
                //Use InitTable
                SetFlags(cfData, Adr2Pos(initTableAdr), bytes);
            }
            //FieldTable
            if (fieldTableAdr)
            {
                pos = Adr2Pos(fieldTableAdr); bytes = 0;
                Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;
                //TypesTab
                DWORD typesTab = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;

                for (int m = 0; m < Num16; m++)
                {
                    //Offset
                    pos += 4; bytes += 4;
                    //Idx
                    pos += 2; bytes += 2;
                    //Name
                    len = Code[pos]; pos++; bytes++;
                    pos += len; bytes += len;
                }
                //Use TypesTab
                if (typesTab)
                {
                    Num16 = *((WORD*)(Code + Adr2Pos(typesTab)));
                    SetFlags(cfData, Adr2Pos(typesTab), 2 + Num16*4);
                }
                //Extended Information
                if (DelphiVersion >= 2010)
                {
                    Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;
                    for (int m = 0; m < Num16; m++)
                    {
                        //Flags
                        pos++; bytes++;
                        //TypeRef
                        pos += 4; bytes += 4;
                        //Offset
                        pos += 4; bytes += 4;
                        //Name
                        len = Code[pos]; pos++; bytes++;
                        pos += len; bytes += len;
                        //AttrData
                        WORD dw = *((WORD*)(Code + pos));
                        pos += dw; bytes += dw;//ATR!!
                    }
                }
                //Use FieldTable
                SetFlags(cfData, Adr2Pos(fieldTableAdr), bytes);
            }
            //MethodTable
            if (methodTableAdr)
            {
                pos = Adr2Pos(methodTableAdr); bytes = 0;
                Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;

                for (int m = 0; m < Num16; m++)
                {
                    //Len
                    WORD skipBytes = *((WORD*)(Code + pos)); pos += skipBytes; bytes += skipBytes;
                }
                if (DelphiVersion >= 2010)
                {
                    Num16 = *((WORD*)(Code + pos)); pos += 2; bytes += 2;
                    for (int m = 0; m < Num16; m++)
                    {
                        //MethodEntry
                        DWORD methodEntry = *((DWORD*)(Code + pos)); pos += 4; bytes += 4;
                        WORD skipBytes = *((WORD*)(Code + Adr2Pos(methodEntry)));
                        SetFlags(cfData, Adr2Pos(methodEntry), skipBytes);
                        //Flags
                        pos += 2; bytes += 2;
                        //VirtualIndex
                        pos += 2; bytes += 2;
                    }
                    if (DelphiVersion >= 2012)
                    {
                        //VirtCount
                        bytes += 2;     
                    }
                }
                //Use MethodTable
                SetFlags(cfData, Adr2Pos(methodTableAdr), bytes);
            }
            //DynamicTable
            if (dynamicTableAdr)
            {
                pos = Adr2Pos(dynamicTableAdr); bytes = 0;
                Num16 = *((WORD*)(Code + pos)); bytes += 2;
                for (int m = 0; m < Num16; m++)
                {
                    //Msg+ProcAdr
                    bytes += 6;
                }
                //Use DynamicTable
                SetFlags(cfData, Adr2Pos(dynamicTableAdr), bytes);
            }

            //DWORD StopAt = GetStopAt(classVMT);
            //Use Virtual Table
            SetFlags(cfData, i, StopAt - classVMT - VmtSelfPtr);

            PUnitRec recU = mainForm->GetUnit(classVMT);
            if (recU)
            {
                adr = *((DWORD*)(Code + i - VmtSelfPtr + VmtTypeInfo));
                if (adr && IsValidImageAdr(adr))
                {
                    //Extract unit name
                    pos = Adr2Pos(adr); bytes = 0;
                    BYTE b = Code[pos]; pos++; bytes++;
                    if (b != 7) continue;
                    len = Code[pos]; pos++; bytes++;
                    if (!IsValidName(len, pos)) continue;
                    pos += len + 10; bytes += len + 10;
                    len = Code[pos]; pos++; bytes++;
                    if (!IsValidName(len, pos)) continue;
                    String unitName = String((char*)(Code + pos), len).Trim(); bytes += len;
                    FMain_11011981->SetUnitName(recU, unitName);
                    //Use information about Unit (including previous dword)
                    SetFlags(cfData, Adr2Pos(adr) - 4, bytes + 4);
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::StrapVMTs()
{
    int stepMask = StartProgress(TotalSize, "Strap Virtual Method Tables");

    MConstInfo cInfo;
    for (int i = 0; i < TotalSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        PInfoRec recN = GetInfoRec(Pos2Adr(i));
    	if (recN && recN->kind == ikVMT)
        {
            String _name = recN->GetName();
            String ConstName = "_DV_" + _name;
            WORD *uses = KnowledgeBase.GetConstUses(ConstName.c_str());
            int ConstIdx = KnowledgeBase.GetConstIdx(uses, ConstName.c_str());
            if (ConstIdx != -1)
            {
                ConstIdx = KnowledgeBase.ConstOffsets[ConstIdx].NamId;
                if (KnowledgeBase.GetConstInfo(ConstIdx, INFO_DUMP, &cInfo))
                {
                    UpdateStatusBar(_name);
                    mainForm->StrapVMT(i + 4, ConstIdx, &cInfo);
                }
            }
            if (uses) delete[] uses;
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::FindTypeFields()
{
    int     typeSize;
    int     stepMask = StartProgress(TotalSize, "Find Type Fields");

    MTypeInfo   atInfo;
    MTypeInfo   *tInfo = &atInfo;

    for (int i = 0; i < TotalSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        PInfoRec recN = GetInfoRec(Pos2Adr(i));
    	if (recN && recN->kind == ikVMT)
        {
            DWORD vmtAdr = Pos2Adr(i) - VmtSelfPtr;
            PUnitRec recU = mainForm->GetUnit(vmtAdr);
            if (recU)
            {
                for (int u = 0; u < recU->names->Count && !Terminated; u++)
                {
                    WORD ModuleID = KnowledgeBase.GetModuleID(recU->names->Strings[u].c_str());
                    WORD *uses = KnowledgeBase.GetModuleUses(ModuleID);
                    //Find Type to extract information about fields
                    int TypeIdx = KnowledgeBase.GetTypeIdxByModuleIds(uses, recN->GetName().c_str());
                    if (TypeIdx != -1)
                    {
                        TypeIdx = KnowledgeBase.TypeOffsets[TypeIdx].NamId;
                        if(KnowledgeBase.GetTypeInfo(TypeIdx, INFO_FIELDS, tInfo))
                        {
                            if (tInfo->Fields)
                            {
                                UpdateStatusBar(tInfo->TypeName);
                                BYTE *p = tInfo->Fields;
                                FIELDINFO fInfo;
                                for (int n = 0; n < tInfo->FieldsNum; n++)
                                {
                                    fInfo.Scope = *p; p++;
                                    fInfo.Offset = *((int*)p); p += 4;
                                    fInfo.Case = *((int*)p); p += 4;
                                    WORD Len = *((WORD*)p); p += 2;
                                    fInfo.Name = String((char*)p, Len); p += Len + 1;
                                    Len = *((WORD*)p); p += 2;
                                    fInfo.Type = TrimTypeName(String((char*)p, Len)); p += Len + 1;
                                    recN->vmtInfo->AddField(0, 0, fInfo.Scope, fInfo.Offset, fInfo.Case, fInfo.Name, fInfo.Type);
                                }
                            }
                        }
                    }
                    if (uses) delete[] uses;
                }
            }
        }
    }
    StopProgress();

    const int cntVmt = VmtList->Count;
    stepMask = StartProgress(cntVmt, "Propagate VMT Names");
    for (int n = 0; n < cntVmt && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        PVmtListRec recV = (PVmtListRec)VmtList->Items[n];
        UpdateStatusBar(GetClsName(recV->vmtAdr));
        mainForm->PropagateVMTNames(recV->vmtAdr);
    }
    StopProgress();
}
//---------------------------------------------------------------------------
String __fastcall TAnalyzeThread::FindEvent(DWORD VmtAdr, String Name)
{
    DWORD adr = VmtAdr;
    while (adr)
    {
        PInfoRec recN = GetInfoRec(adr);
        if (recN && recN->vmtInfo->fields)
        {
            for (int n = 0; n < recN->vmtInfo->fields->Count; n++)
            {
                PFIELDINFO fInfo = (PFIELDINFO)recN->vmtInfo->fields->Items[n];
                if (SameText(fInfo->Name, Name)) return fInfo->Type;
            }
        }
        adr = GetParentAdr(adr);
    }
    return "";
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::FindPrototypes()
{
    int         n, m, k, r, idx, usesNum;
    PInfoRec    recN;
    String      importName, _name;
    MProcInfo   aInfo;
    MProcInfo   *pInfo = &aInfo;
    MTypeInfo   atInfo;
    MTypeInfo   *tInfo = &atInfo;
    WORD        uses[128];

    int stepMask = StartProgress(CodeSize, "Find Import Prototypes");

    for (n = 0; n < CodeSize && !Terminated; n += 4)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfImport, n))
        {
            recN = GetInfoRec(Pos2Adr(n));
            _name = recN->GetName();
            int dot = _name.Pos(".");
            int len = _name.Length();
            bool found = false;
            BYTE *p; WORD wlen;
            //try find arguments
            //FullName
            importName = _name.SubString(dot + 1, len);
            usesNum = KnowledgeBase.GetProcUses(importName.c_str(), uses);
            for (m = 0; m < usesNum && !Terminated; m++)
            {
                idx = KnowledgeBase.GetProcIdx(uses[m], importName.c_str());
                if (idx != -1)
                {
                    idx = KnowledgeBase.ProcOffsets[idx].NamId;
                    if (KnowledgeBase.GetProcInfo(idx, INFO_ARGS, pInfo))
                    {
                        if (pInfo->MethodKind == 'F')
                        {
                            recN->kind = ikFunc;
                            recN->type = pInfo->TypeDef;
                        }
                        else if (pInfo->MethodKind == 'P')
                        {
                            recN->kind = ikProc;
                        }
                        if (!recN->procInfo)
                            recN->procInfo = new InfoProcInfo;

                        if (pInfo->Args)
                        {
                            BYTE callKind = pInfo->CallKind;

                            recN->procInfo->flags |= callKind;

                            ARGINFO argInfo; p = pInfo->Args; int ss = 8;
                            for (k = 0; k < pInfo->ArgsNum; k++)
                            {
                                FillArgInfo(k, callKind, &argInfo, &p, &ss);
                                recN->procInfo->AddArg(&argInfo);
                            }
                            found = true;
                        }
                    }
                    if (found) break;
                }
            }
            if (!found)
            {
                //try short name
                importName = _name.SubString(dot + 1, len - dot - 1);
                usesNum = KnowledgeBase.GetProcUses(importName.c_str(), uses);
                for (m = 0; m < usesNum && !Terminated; m++)
                {
                    idx = KnowledgeBase.GetProcIdx(uses[m], importName.c_str());
                    if (idx != -1)
                    {
                        idx = KnowledgeBase.ProcOffsets[idx].NamId;
                        if (KnowledgeBase.GetProcInfo(idx, INFO_ARGS, pInfo))
                        {
                            if (pInfo->MethodKind == 'F')
                            {
                                recN->kind = ikFunc;
                                recN->type = pInfo->TypeDef;
                            }
                            else if (pInfo->MethodKind == 'P')
                            {
                                recN->kind = ikProc;
                            }
                            if (!recN->procInfo)
                                recN->procInfo = new InfoProcInfo;

                            if (pInfo->Args)
                            {
                                BYTE callKind = pInfo->CallKind;
                                recN->procInfo->flags |= callKind;

                                ARGINFO argInfo; p = pInfo->Args; int ss = 8;
                                for (k = 0; k < pInfo->ArgsNum; k++)
                                {
                                    FillArgInfo(k, callKind, &argInfo, &p, &ss);
                                    recN->procInfo->AddArg(&argInfo);
                                }
                                found = true;
                            }
                        }
                        if (found) break;
                    }
                }
            }
            if (!found)
            {
                //try without arguments
                //FullName
                importName = _name.SubString(dot + 1, len - dot);
                usesNum = KnowledgeBase.GetProcUses(importName.c_str(), uses);
                for (m = 0; m < usesNum && !Terminated; m++)
                {
                    idx = KnowledgeBase.GetProcIdx(uses[m], importName.c_str());
                    if (idx != -1)
                    {
                        idx = KnowledgeBase.ProcOffsets[idx].NamId;
                        if (KnowledgeBase.GetProcInfo(idx, INFO_ARGS, pInfo))
                        {
                            if (pInfo->MethodKind == 'F')
                            {
                                recN->kind = ikFunc;
                                recN->type = pInfo->TypeDef;
                            }
                            else if (pInfo->MethodKind == 'P')
                            {
                                recN->kind = ikProc;
                            }
                            found = true;
                        }
                        if (found) break;
                    }
                }
            }
            if (!found)
            {
                //try without arguments
                //ShortName
                importName = _name.SubString(dot + 1, len - dot - 1);
                usesNum = KnowledgeBase.GetProcUses(importName.c_str(), uses);
                for (m = 0; m < usesNum && !Terminated; m++)
                {
                    idx = KnowledgeBase.GetProcIdx(uses[m], importName.c_str());
                    if (idx != -1)
                    {
                        idx = KnowledgeBase.ProcOffsets[idx].NamId;
                        if (KnowledgeBase.GetProcInfo(idx, INFO_ARGS, pInfo))
                        {
                            if (pInfo->MethodKind == 'F')
                            {
                                recN->kind = ikFunc;
                                recN->type = pInfo->TypeDef;
                            }
                            else if (pInfo->MethodKind == 'P')
                            {
                                recN->kind = ikProc;
                            }
                            found = true;
                        }
                        if (found) break;
                    }
                }
            }
        }
    }
    StopProgress();

    StartProgress(ResInfo->FormList->Count, "Find Event Prototypes");

    for (n = 0; n < ResInfo->FormList->Count && !Terminated; n++)
    {
        UpdateProgress();
        TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
        String className = dfm->ClassName;
        DWORD formAdr = GetClassAdr(className);
        if (!formAdr) continue;
        recN = GetInfoRec(formAdr);
        if (!recN || !recN->vmtInfo->methods) continue;

        //The first: form events
        TList* ev = dfm->Events;
        for (m = 0; m < ev->Count && !Terminated; m++)
        {
            PEventInfo eInfo = (PEventInfo)ev->Items[m];
            DWORD controlAdr = GetClassAdr(dfm->ClassName);
            String typeName = FindEvent(controlAdr, "F" + eInfo->EventName);
            if (typeName == "") typeName = FindEvent(controlAdr, eInfo->EventName);
            if (typeName != "")
            {
                for (k = 0; k < recN->vmtInfo->methods->Count && !Terminated; k++)
                {
                    PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[k];
                    if (SameText(recM->name, className + "." + eInfo->ProcName))
                    {
                        PInfoRec recN1 = GetInfoRec(recM->address);
                        if (recN1)
                        {
                            String clsname = className;
                            while (1)
                            {
                                if (KnowledgeBase.GetKBPropertyInfo(clsname, eInfo->EventName, tInfo))
                                {
                                    recN1->kind = ikProc;
                                    recN1->procInfo->flags |= PF_EVENT;
                                    recN1->procInfo->DeleteArgs();
                                    //eax always Self
                                    recN1->procInfo->AddArg(0x21, 0, 4, "Self", className);
                                    //transform declaration to arguments
                                    recN1->procInfo->AddArgsFromDeclaration(tInfo->Decl.c_str(), 1, 0);
                                    break;
                                }
                                clsname = GetParentName(clsname);
                                if (clsname == "") break;
                            }
                        }
                        else
                        {
                            ShowMessage("recN is Null");
                        }
                        break;
                    }
                }
            }
        }
        //The second: components events
        for (m = 0; m < dfm->Components->Count && !Terminated; m++)
        {
            PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
            TList* ev = cInfo->Events;
            for (k = 0; k < ev->Count && !Terminated; k++)
            {
                PEventInfo eInfo = (PEventInfo)ev->Items[k];
                DWORD controlAdr = GetClassAdr(cInfo->ClassName);
                String typeName = FindEvent(controlAdr, "F" + eInfo->EventName);
                if (typeName == "") typeName = FindEvent(controlAdr, eInfo->EventName);
                if (typeName != "")
                {
                    for (r = 0; r < recN->vmtInfo->methods->Count && !Terminated; r++)
                    {
                        PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[r];
                        if (SameText(recM->name, className + "." + eInfo->ProcName))
                        {
                            PInfoRec recN1 = GetInfoRec(recM->address);
                            if (recN1)
                            {
                                String clsname = className;
                                while (1)
                                {
                                    if (KnowledgeBase.GetKBPropertyInfo(clsname, eInfo->EventName, tInfo))
                                    {
                                        recN1->kind = ikProc;
                                        recN1->procInfo->flags |= PF_EVENT;
                                        recN1->procInfo->DeleteArgs();
                                        //eax always Self
                                        recN1->procInfo->AddArg(0x21, 0, 4, "Self", className);
                                        //transform declaration to arguments
                                        recN1->procInfo->AddArgsFromDeclaration(tInfo->Decl.c_str(), 1, 0);
                                        break;
                                    }
                                    clsname = GetParentName(clsname);
                                    if (clsname == "") break;
                                }
                            }
                            else
                            {
                                ShowMessage("recN is Null");
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
typedef struct
{
    bool    used;
    char    *name;   	//имя юнита
    float   matched;	//максимальное кол-во совпадений
    int     maxno;  	//номер юнита с максимальным кол-вом совпадений
} StdUnitInfo, *PStdUnitInfo;

#define StdUnitsNum 7
StdUnitInfo  StdUnits[] = {
{false, "Types", 0.0, -1},
{false, "Multimon", 0.0, -1},
{false, "VarUtils", 0.0, -1},
{false, "StrUtils", 0.0, -1},
{false, "Registry", 0.0, -1},
{false, "IniFiles", 0.0, -1},
{false, "Clipbrd",  0.0, -1},
{false, 0, 0.0, -1}
};
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ScanCode()
{
    bool        matched;
    WORD        moduleID;
    DWORD       Adr, iniAdr, finAdr, vmtAdr;
    int         FirstProcIdx, LastProcIdx, Num, DumpSize;
    int         i, n, m, k, r, u, Idx, fromPos, toPos, stepMask;
    PUnitRec    recU;
    PInfoRec    recN;
    MProcInfo   aInfo;
    MProcInfo   *pInfo = &aInfo;
    MConstInfo  acInfo;
    MConstInfo  *cInfo = &acInfo;
    String      className, unitName;

    StartProgress(UnitsNum, "Scan Initialization and Finalization procs");

    //Begin with initialization and finalization procs
    for (n = 0; n < UnitsNum && !Terminated; n++)
    {
        UpdateProgress();
        recU = (UnitRec*)Units->Items[n];
        if (recU->trivial) continue;

        iniAdr = recU->iniadr;
        if (iniAdr && !recU->trivialIni)
        {
            mainForm->AnalyzeProc(0, iniAdr);

            for (u = 0; u < recU->names->Count && !Terminated; u++)
            {
                moduleID = KnowledgeBase.GetModuleID(recU->names->Strings[u].c_str());
                if (moduleID == 0xFFFF) continue;

                recU->kb = true;
                //If unit is in knowledge base try to find proc Initialization
                Idx = KnowledgeBase.GetProcIdx(moduleID, recU->names->Strings[u].c_str());
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                    if (!KnowledgeBase.IsUsedProc(Idx))
                    {
                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                        {
                            matched = (MatchCode(Code + Adr2Pos(iniAdr), pInfo) && mainForm->StrapCheck(Adr2Pos(iniAdr), pInfo));
                            if (matched) mainForm->StrapProc(Adr2Pos(iniAdr), Idx, pInfo, true, pInfo->DumpSz);
                        }
                    }
                }
            }
        }
        finAdr = recU->finadr;
        if (finAdr && !recU->trivialFin)
        {
            mainForm->AnalyzeProc(0, finAdr);

            for (u = 0; u < recU->names->Count && !Terminated; u++)
            {
                moduleID = KnowledgeBase.GetModuleID(recU->names->Strings[u].c_str());
                if (moduleID == 0xFFFF) continue;

                recU->kb = true;
                //If unit is in knowledge base try to find proc Finalization
                Idx = KnowledgeBase.GetProcIdx(moduleID, "Finalization");
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                    if (!KnowledgeBase.IsUsedProc(Idx))
                    {
                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                        {
                            matched = (MatchCode(Code + Adr2Pos(finAdr), pInfo) && mainForm->StrapCheck(Adr2Pos(finAdr), pInfo));
                            if (matched) mainForm->StrapProc(Adr2Pos(finAdr), Idx, pInfo, true, pInfo->DumpSz);
                        }
                    }
                }
            }
        }
    }
    StopProgress();
    //EP
    mainForm->AnalyzeProc(0, EP);
    //Classes (methods, dynamics procedures, virtual methods)
    stepMask = StartProgress(TotalSize, "Analyze Class Tables");
    for (n = 0; n < TotalSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->kind == ikVMT)
        {
            vmtAdr = Pos2Adr(n);
            UpdateStatusBar(GetClsName(vmtAdr));

            mainForm->AnalyzeMethodTable(0, vmtAdr, &Terminated);
            if (Terminated) break;
            mainForm->AnalyzeDynamicTable(0, vmtAdr, &Terminated);
            if (Terminated) break;
            mainForm->AnalyzeVirtualTable(0, vmtAdr, &Terminated);
        }
    }
    StopProgress();
    //Scan some standard units
    StartProgress(StdUnitsNum, "Scan Standard Units: step1");
    for (r = 0; !Terminated; r++)
    {
        UpdateProgress();
        if (!StdUnits[r].name) break;
        StdUnits[r].used = false;
        StdUnits[r].matched = 0.0;
        StdUnits[r].maxno = -1;
        moduleID = KnowledgeBase.GetModuleID(StdUnits[r].name);
        if (moduleID == 0xFFFF) continue;

        if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;

        for (n = 0; n < UnitsNum && !Terminated; n++)
        {
            recU = (UnitRec*)Units->Items[n];
            if (recU->trivial) continue;

            //Analyze units without name
            if (!recU->names->Count)
            {
                recU->matchedPercent = 0.0;
                fromPos = Adr2Pos(recU->fromAdr);
                toPos = Adr2Pos(recU->toAdr);
                for (m = fromPos; m < toPos && !Terminated; m++)
                {
                    if (IsFlagSet(cfProcStart, m))
                    {
                        if (r != 5 && Pos2Adr(m) == recU->iniadr) continue;
                        if (r != 5 && Pos2Adr(m) == recU->finadr) continue;

                        recN = GetInfoRec(Pos2Adr(m));
                        if (recN && recN->HasName()) continue;
                        UpdateAddrInStatusBar(Pos2Adr(m));
                        for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                        {
                            Idx = KnowledgeBase.ProcOffsets[k].ModId;
                            if (!KnowledgeBase.IsUsedProc(Idx))
                            {
                                matched = false;
                                if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                                {
                                    matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                    if (matched)
                                    {
                                        //If method of class, check that ClassName is found
                                        //className = ExtractClassName(pInfo->ProcName);
                                        //if (className == "" || GetOwnTypeByName(className))
                                        //{
                                            recU->matchedPercent += 100.0*pInfo->DumpSz/(toPos - fromPos + 1);
                                            break;
                                        //}
                                    }
                                }
                            }
                        }
                    }
                }
                //float matchedPercent = 100.0*recU->matchedSize/(toPos - fromPos + 1);
                if (recU->matchedPercent > StdUnits[r].matched)
                {
                    StdUnits[r].matched = recU->matchedPercent;
                    StdUnits[r].maxno = n;
                }
            }
        }
    }
    StopProgress();
    StartProgress(StdUnitsNum, "Scan Standard Units: step2");
    for (r = 0; !Terminated; r++)
    {
        UpdateProgress();
        if (!StdUnits[r].name) break;
        if (StdUnits[r].used) continue;
        if (StdUnits[r].matched < 50.0) continue;

        moduleID = KnowledgeBase.GetModuleID(StdUnits[r].name);
        if (moduleID == 0xFFFF) continue;

        if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;

        n = StdUnits[r].maxno;
        recU = (UnitRec*)Units->Items[n];
        if (recU->trivial) continue;

        //Analyze units without name
        if (!recU->names->Count)
        {
            fromPos = Adr2Pos(recU->fromAdr);
            toPos = Adr2Pos(recU->toAdr);
            for (m = fromPos; m < toPos && !Terminated; m++)
            {
                if (IsFlagSet(cfProcStart, m))
                {
                    if (r != 5 && Pos2Adr(m) == recU->iniadr) continue;
                    if (r != 5 && Pos2Adr(m) == recU->finadr) continue;

                    recN = GetInfoRec(Pos2Adr(m));
                    if (recN && recN->HasName()) continue;
                    UpdateAddrInStatusBar(Pos2Adr(m));
                    for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                    {
                        Idx = KnowledgeBase.ProcOffsets[k].ModId;
                        if (!KnowledgeBase.IsUsedProc(Idx))
                        {
                            matched = false;
                            if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                            {
                                matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                if (matched)
                                {
                                    //If method of class, check that ClassName is found
                                    //className = ExtractClassName(pInfo->ProcName);
                                    //if (className == "" || GetOwnTypeByName(className))
                                    //{
                                        mainForm->StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                        StdUnits[r].used = true;
                                        break;
                                    //}
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    StopProgress();
    for (n = 0; n < UnitsNum && !Terminated; n++)
    {
        //UpdateProgress();
        recU = (UnitRec*)Units->Items[n];
        if (recU->trivial) continue;

        fromPos = Adr2Pos(recU->fromAdr);
        toPos = Adr2Pos(recU->toAdr);
        //Delphi2 - it is possible that toPos = -1 
        if (toPos == -1) toPos = TotalSize;

        for (u = 0; u < recU->names->Count && !Terminated; u++)
        {
            unitName = recU->names->Strings[u];
            moduleID = KnowledgeBase.GetModuleID(unitName.c_str());
            if (moduleID == 0xFFFF) continue;

            if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx, &DumpSize)) continue;

            stepMask = StartProgress(toPos - fromPos + 1, "Scan Unit " + unitName + ": step1");

            recU->kb = true;
            for (m = fromPos, i = 0; m < toPos && !Terminated; m++, i++)
            {
                if ((i & stepMask) == 0) UpdateProgress();

                if (!*(Code + m)) continue;
                if (IsFlagSet(cfProcStart, m) || !Flags[m])
                {
                    if (Pos2Adr(m) == recU->iniadr && recU->trivialIni) continue;
                    if (Pos2Adr(m) == recU->finadr && recU->trivialFin) continue;
                    recN = GetInfoRec(Pos2Adr(m));
                    if (recN && recN->HasName()) continue;

                    UpdateAddrInStatusBar(Pos2Adr(m));
                    for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                    {
                        Idx = KnowledgeBase.ProcOffsets[k].ModId;
                        if (!KnowledgeBase.IsUsedProc(Idx))
                        {
                            if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                            {
                                //Check code matching
                                matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                if (matched)
                                {
                                    //If method of class, check that ClassName is found
                                    //className = ExtractClassName(pInfo->ProcName);
                                    //if (className == "" || GetOwnTypeByName(className))
                                    //{
                                        mainForm->StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                        m += pInfo->DumpSz - 1;
                                        break;
                                    //}
                                }
                            }
                        }
                    }
                }
            }
            StopProgress();
        }
    }
    //Теперь попробуем опять пройтись по VMT и поискать их в базе знаний
    stepMask = StartProgress(TotalSize, "Scan Units: step 2");
    for (n = 0; n < TotalSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        PInfoRec recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->kind == ikVMT)
        {
            String ConstName = "_DV_" + recN->GetName();
            Num = KnowledgeBase.GetConstIdxs(ConstName.c_str(), &Idx);
            if (Num == 1)
            {
                Adr = Pos2Adr(n);
                recU = mainForm->GetUnit(Adr);
                if (!recU || recU->trivial) continue;

                if (!recU->names->Count)
                {
                    Idx = KnowledgeBase.ConstOffsets[Idx].NamId;
                    if (KnowledgeBase.GetConstInfo(Idx, INFO_DUMP, cInfo))
                    {
                        moduleID = cInfo->ModuleID;
                        if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;
                        
                        fromPos = Adr2Pos(recU->fromAdr);
                        toPos = Adr2Pos(recU->toAdr);
                        for (m = fromPos; m < toPos && !Terminated; m++)
                        {
                            if (IsFlagSet(cfProcStart, m))
                            {
                                if (Pos2Adr(m) == recU->iniadr && recU->trivialIni) continue;
                                if (Pos2Adr(m) == recU->finadr && recU->trivialFin) continue;

                                recN = GetInfoRec(Pos2Adr(m));
                                if (recN && recN->HasName()) continue;
                                UpdateAddrInStatusBar(Pos2Adr(m));
                                for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                                {
                                    Idx = KnowledgeBase.ProcOffsets[k].ModId;
                                    if (!KnowledgeBase.IsUsedProc(Idx))
                                    {
                                        matched = false;
                                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                        {
                                            matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                            if (matched)
                                            {
                                                String unitName = KnowledgeBase.GetModuleName(moduleID);
                                                FMain_11011981->SetUnitName(recU, unitName);
                                                recU->kb = true;
                                                mainForm->StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ScanCode1()
{
    bool        matched;
    WORD        moduleID;
    DWORD       pos, Adr;
    int         FirstProcIdx, LastProcIdx, Num;
    int         n, m, k, r, u, Idx, fromPos, toPos;
    PUnitRec    recU;
    PInfoRec    recN;
    MProcInfo   aInfo;
    MProcInfo   *pInfo = &aInfo;
    String      className;

    StartProgress(UnitsNum, "Scan Code more...");
    for (n = 0; n < UnitsNum && !Terminated; n++)
    {
        UpdateProgress();
        recU = (UnitRec*)Units->Items[n];
        if (recU->trivial) continue;

        fromPos = Adr2Pos(recU->fromAdr);
        toPos = Adr2Pos(recU->toAdr);

        for (u = 0; u < recU->names->Count && !Terminated; u++)
        {
            moduleID = KnowledgeBase.GetModuleID(recU->names->Strings[u].c_str());
            if (moduleID == 0xFFFF) continue;

            if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;
            for (m = fromPos; m < toPos && !Terminated; m++)
            {
                if (IsFlagSet(cfProcStart, m))
                {
                    if (Pos2Adr(m) == recU->iniadr && recU->trivialIni) continue;
                    if (Pos2Adr(m) == recU->finadr && recU->trivialFin) continue;

                    recN = GetInfoRec(Pos2Adr(m));
                    if (recN && recN->HasName()) continue;
                    UpdateAddrInStatusBar(Pos2Adr(m));
                    for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                    {
                        Idx = KnowledgeBase.ProcOffsets[k].ModId;
                        if (!KnowledgeBase.IsUsedProc(Idx))
                        {
                            if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                            {
                                matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                if (matched)
                                {
                                    //If method of class, check that ClassName is found
                                    //className = ExtractClassName(pInfo->ProcName);
                                    //if (className == "" || GetOwnTypeByName(className))
                                    //{
                                        mainForm->StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                        break;
                                    //}
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    StopProgress();
    StartProgress(StdUnitsNum, "Scan Standard Units more: step1");
    //Попробуем некоторые стандартные юниты
    for (r = 0; !Terminated; r++)
    {
        UpdateProgress();
        if (!StdUnits[r].name) break;
        if (StdUnits[r].used) continue;
        StdUnits[r].matched = 0;
        StdUnits[r].maxno = -1;
        moduleID = KnowledgeBase.GetModuleID(StdUnits[r].name);
        if (moduleID == 0xFFFF) continue;

        if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;

        for (n = 0; n < UnitsNum && !Terminated; n++)
        {
            recU = (UnitRec*)Units->Items[n];
            if (recU->trivial) continue;

            //Анализируем непоименованные юниты
            if (!recU->names->Count)
            {
                recU->matchedPercent = 0.0;
                fromPos = Adr2Pos(recU->fromAdr);
                toPos = Adr2Pos(recU->toAdr);
                for (m = fromPos; m < toPos && !Terminated; m++)
                {
                    if (IsFlagSet(cfProcStart, m))
                    {
                        if (Pos2Adr(m) == recU->iniadr) continue;
                        if (Pos2Adr(m) == recU->finadr) continue;

                        recN = GetInfoRec(Pos2Adr(m));
                        if (recN && recN->HasName()) continue;
                        UpdateAddrInStatusBar(Pos2Adr(m));
                        for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                        {
                            Idx = KnowledgeBase.ProcOffsets[k].ModId;
                            if (!KnowledgeBase.IsUsedProc(Idx))
                            {
                                matched = false;
                                if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                                {
                                    matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                    if (matched)
                                    {
                                        //If method of class, check that ClassName is found
                                        //className = ExtractClassName(pInfo->ProcName);
                                        //if (className == "" || GetOwnTypeByName(className))
                                        //{
                                            recU->matchedPercent += 100.0*pInfo->DumpSz/(toPos - fromPos + 1);
                                            break;
                                        //}
                                    }
                                }
                            }
                        }
                    }
                }
                //float matchedPercent = 100.0*recU->matchedSize/(toPos - fromPos + 1);
                if (recU->matchedPercent > StdUnits[r].matched)
                {
                    StdUnits[r].matched = recU->matchedPercent;
                    StdUnits[r].maxno = n;
                }
            }
        }
    }
    StopProgress();
    StartProgress(StdUnitsNum, "Scan Standard Units more: step2");
    for (r = 0; !Terminated; r++)
    {
        UpdateProgress();
        if (!StdUnits[r].name) break;
        if (StdUnits[r].used) continue;
        if (StdUnits[r].matched < 50.0) continue;

        moduleID = KnowledgeBase.GetModuleID(StdUnits[r].name);
        if (moduleID == 0xFFFF) continue;

        if (!KnowledgeBase.GetProcIdxs(moduleID, &FirstProcIdx, &LastProcIdx)) continue;

        n = StdUnits[r].maxno;
        recU = (UnitRec*)Units->Items[n];
        if (recU->trivial) continue;

        //Анализируем непоименованные юниты
        if (!recU->names->Count)
        {
            fromPos = Adr2Pos(recU->fromAdr);
            toPos = Adr2Pos(recU->toAdr);
            for (m = fromPos; m < toPos && !Terminated; m++)
            {
                if (IsFlagSet(cfProcStart, m))
                {
                    if (Pos2Adr(m) == recU->iniadr) continue;
                    if (Pos2Adr(m) == recU->finadr) continue;

                    recN = GetInfoRec(Pos2Adr(m));
                    if (recN && recN->HasName()) continue;
                    UpdateAddrInStatusBar(Pos2Adr(m));
                    for (k = FirstProcIdx; k <= LastProcIdx && !Terminated; k++)
                    {
                        Idx = KnowledgeBase.ProcOffsets[k].ModId;
                        if (!KnowledgeBase.IsUsedProc(Idx))
                        {
                            matched = false;
                            if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo) && pInfo->DumpSz >= 8 && m + pInfo->DumpSz < toPos)
                            {
                                matched = (MatchCode(Code + m, pInfo) && mainForm->StrapCheck(m, pInfo));
                                if (matched)
                                {
                                    //If method of class, check that ClassName is found
                                    //className = ExtractClassName(pInfo->ProcName);
                                    //if (className == "" || GetOwnTypeByName(className))
                                    //{
                                        mainForm->StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                        StdUnits[r].used = true;
                                        break;
                                    //}
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ScanVMTs()
{
    int stepMask = StartProgress(TotalSize, "Scan Virtual Method Tables");
    for (int n = 0; n < TotalSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        PInfoRec recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->kind == ikVMT)
        {
            DWORD vmtAdr = Pos2Adr(n);
            String name = recN->GetName();
            UpdateStatusBar(name);

            mainForm->ScanFieldTable(vmtAdr);
            if (Terminated) break;
            mainForm->ScanMethodTable(vmtAdr, name);
            if (Terminated) break;
            mainForm->ScanVirtualTable(vmtAdr);
            if (Terminated) break;
            mainForm->ScanDynamicTable(vmtAdr);
            if (Terminated) break;

            if (DelphiVersion != 2)
            {
                mainForm->ScanIntfTable(vmtAdr);
                mainForm->ScanAutoTable(vmtAdr);
                if (Terminated) break;
            }

            mainForm->ScanInitTable(vmtAdr);
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ScanConsts()
{
    WORD        ModID;
    int         n, m, u, Bytes, ResStrIdx, pos, ResStrNum, ResStrNo;
	DWORD       adr, resid;
    PUnitRec    recU;
    PInfoRec    recN;
    MResStrInfo arsInfo;
    MResStrInfo *rsInfo = &arsInfo;
    String      uname;
    char        buf[1024];

    if (Terminated) return;
    if (HInstanceVarAdr == 0xFFFFFFFF) return;

    HINSTANCE hInst = LoadLibraryEx(mainForm->SourceFile.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);//DONT_RESOLVE_DLL_REFERENCES);
    if (!hInst) return;

    //Array of counters for units frequences
    int *Counters = new int[KnowledgeBase.ModuleCount];

    StartProgress(UnitsNum, "Scan Resource Strings");
    for (m = 0; m < UnitsNum && !Terminated; m++)
    {
        UpdateProgress();
        recU = (PUnitRec)Units->Items[m];
        if (!recU) continue;

        ModID = 0xFFFF;
        //If module from KB load information about ResStrings
        if (recU->names->Count)
        {
            for (u = 0; u < recU->names->Count && !Terminated; u++)
            {
                UpdateAddrInStatusBar(u);
                ModID = KnowledgeBase.GetModuleID(recU->names->Strings[u].c_str());
                //Если из базы знаний, вытащим из нее информацию о ResStr
                if (ModID != 0xFFFF)
                {
                    for (n = Adr2Pos(recU->fromAdr); (n < Adr2Pos(recU->toAdr)) && !Terminated; n += 4)
                    {
                        adr = *((DWORD*)(Code + n));
                        resid = *((DWORD*)(Code + n + 4));

                        if (IsValidImageAdr(adr) && adr == HInstanceVarAdr && resid < 0x10000)
                        {
                            recN = GetInfoRec(Pos2Adr(n));
                            //If export at this position, delete InfoRec and create new (ikResString)
                            if (IsFlagSet(cfExport, n))
                            {
                                ClearFlag(cfProcStart, n);
                                delete recN;
                                recN = 0;
                            }
                            if (!recN)
                                recN = new InfoRec(n, ikResString);
                            else
                            {
                                if (recN->kind == ikResString) continue;
                                //may be ikData
                                if (!recN->rsInfo) recN->rsInfo = new InfoResStringInfo;
                            }
                            recN->type = "TResStringRec";

                            //Set Flags
                            SetFlags(cfData, n, 8);
                            //Get Context
                            Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                            recN->rsInfo->value = String(buf, Bytes);

                            ResStrIdx = KnowledgeBase.GetResStrIdx(ModID, buf);
                            if (ResStrIdx != -1)
                            {
                                ResStrIdx = KnowledgeBase.ResStrOffsets[ResStrIdx].NamId;
                                if (KnowledgeBase.GetResStrInfo(ResStrIdx, 0, rsInfo))
                                {
                                    if (!recN->HasName())
                                    {
                                        UpdateStatusBar(rsInfo->ResStrName);
                                        recN->SetName(rsInfo->ResStrName);
                                    }
                                }
                            }
                            else
                            {
                                if (!recN->HasName())
                                {
                                    recN->ConcatName("SResString" + String(LastResStrNo));
                                    LastResStrNo++;
                                }
                            }
                        }
                    }
                }
                //Else extract ResStrings from analyzed file
                else
                {
                    for (n = Adr2Pos(recU->fromAdr); (n < Adr2Pos(recU->toAdr)) && !Terminated; n += 4)
                    {
                        adr = *((DWORD*)(Code + n));
                        resid = *((DWORD*)(Code + n + 4));

                        if (IsValidImageAdr(adr) && adr == HInstanceVarAdr && resid < 0x10000)
                        {
                            recN = GetInfoRec(Pos2Adr(n));
                            //If export at this position, delete InfoRec and create new (ikResString)
                            if (IsFlagSet(cfExport, n))
                            {
                                ClearFlag(cfProcStart, n);
                                delete recN;
                                recN = 0;
                            }
                            if (!recN)
                                recN = new InfoRec(n, ikResString);
                            else
                            {
                                if (recN->kind == ikResString) continue;
                                //may be ikData
                                if (!recN->rsInfo) recN->rsInfo = new InfoResStringInfo;
                            }
                            recN->type = "TResStringRec";

                            //Set Flags
                            SetFlags(cfData, n, 8);
                            //Get Context
                            Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                            recN->rsInfo->value = String(buf, Bytes);

                            if (!recN->HasName())
                            {
                                recN->ConcatName("SResString" + String(LastResStrNo));
                                LastResStrNo++;
                            }
                        }
                    }
                }
            }
        }
        //If unit has no name check it is module of ResStrings
        else
        {
            UpdateProgress();
            memset(Counters, 0, KnowledgeBase.ModuleCount*sizeof(int));
            ResStrNum = 0;

            for (n = Adr2Pos(recU->fromAdr); (n < Adr2Pos(recU->toAdr)) && !Terminated; n += 4)
            {
                adr = *((DWORD*)(Code + n));
                resid = *((DWORD*)(Code + n + 4));

                if (IsValidImageAdr(adr) && adr == HInstanceVarAdr && resid < 0x10000)
                {
                    Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                    //Number of ReStrings in this module
                    ResStrNum++;
                    for (ResStrNo = 0; !Terminated; )
                    {
                        ResStrIdx = KnowledgeBase.GetResStrIdx(ResStrNo, buf);
                        if (ResStrIdx == -1) break;
                        ResStrNo = ResStrIdx + 1;
                        ResStrIdx = KnowledgeBase.ResStrOffsets[ResStrIdx].NamId;
                        if (KnowledgeBase.GetResStrInfo(ResStrIdx, 0, rsInfo))
                        {
                            Counters[rsInfo->ModuleID]++;
                        }
                    }
                }
            }
            //What module has frequency >= ResStrNum
            if (ResStrNum)
            {
                for (n = 0; n < KnowledgeBase.ModuleCount && !Terminated; n++)
                {
                    if (Counters[n] >= 0.9*ResStrNum)
                    {
                        ModID = n;
                        break;
                    }
                }
                //Module is found
                if (ModID != 0xFFFF)
                {
                    uname = KnowledgeBase.GetModuleName(ModID);
                    FMain_11011981->SetUnitName(recU, uname);
                    recU->kb = true;

                    for (n = Adr2Pos(recU->fromAdr); (n < Adr2Pos(recU->toAdr)) && !Terminated; n += 4)
                    {
                        adr = *((DWORD*)(Code + n));
                        resid = *((DWORD*)(Code + n + 4));

                        if (IsValidImageAdr(adr) && adr == HInstanceVarAdr && resid < 0x10000)
                        {
                            recN = GetInfoRec(Pos2Adr(n));
                            //If export at this position, delete InfoRec and create new (ikResString)
                            if (IsFlagSet(cfExport, n))
                            {
                                ClearFlag(cfProcStart, n);
                                delete recN;
                                recN = 0;
                            }
                            if (!recN)
                                recN = new InfoRec(n, ikResString);
                            else
                            {
                                if (recN->kind == ikResString) continue;
                                //may be ikData
                            	if (!recN->rsInfo) recN->rsInfo = new InfoResStringInfo;
                            }
                            recN->type = "TResStringRec";

                            //Set Flags
                            SetFlags(cfData, n, 8);
                            //Get Context
                            Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                            recN->rsInfo->value = String(buf, Bytes);

                            ResStrIdx = KnowledgeBase.GetResStrIdx(ModID, buf);
                            if (ResStrIdx != -1)
                            {
                                ResStrIdx = KnowledgeBase.ResStrOffsets[ResStrIdx].NamId;
                                if (KnowledgeBase.GetResStrInfo(ResStrIdx, 0, rsInfo))
                                {
                                    if (!recN->HasName())
                                    {
                                        UpdateStatusBar(rsInfo->ResStrName);
                                        recN->SetName(rsInfo->ResStrName);
                                    }
                                }
                            }
                            else
                            {
                                if (!recN->HasName())
                                {
                                    recN->ConcatName("SResString" + String(LastResStrNo));
                                    LastResStrNo++;
                                }
                            }
                        }
                    }
                }
                //Module not found, get ResStrings from analyzed file
                else
                {
                    for (n = Adr2Pos(recU->fromAdr); (n < Adr2Pos(recU->toAdr)) && !Terminated; n += 4)
                    {
                        adr = *((DWORD*)(Code + n));
                        resid = *((DWORD*)(Code + n + 4));

                        if (IsValidImageAdr(adr) && adr == HInstanceVarAdr && resid < 0x10000)
                        {
                            recN = GetInfoRec(Pos2Adr(n));
                            //If export at this position, delete InfoRec and create new (ikResString)
                            if (IsFlagSet(cfExport, n))
                            {
                                ClearFlag(cfProcStart, n);
                                delete recN;
                                recN = 0;
                            }
                            if (!recN)
                                recN = new InfoRec(n, ikResString);
                            else
                            {
                                if (recN->kind == ikResString) continue;
                                //may be ikData
                                if (!recN->rsInfo) recN->rsInfo = new InfoResStringInfo;
                            }
                            recN->type = "TResStringRec";

                            //Set Flags
                            SetFlags(cfData, n, 8);
                            //Get Context
                            Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                            recN->rsInfo->value = String(buf, Bytes);

                            if (!recN->HasName())
                            {
                                recN->ConcatName("SResString" + String(LastResStrNo));
                                LastResStrNo++;
                            }
                        }
                    }
                }
            }
        }
    }
    FreeLibrary(hInst);    
    StopProgress();
    delete[] Counters;
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ScanGetSetStoredProcs()
{
    for (int m = 0; m < OwnTypeList->Count && !Terminated; m++)
    {
        PTypeRec recT = (PTypeRec)OwnTypeList->Items[m];
        if (recT->kind == ikClass)
        {
            int n = Adr2Pos(recT->adr);
            //SelfPtr
            n += 4;
            //typeKind
            n++;
            BYTE len = Code[n]; n++;
            //clsName
            n += len;

            DWORD classVMT = *((DWORD*)(Code + n)); n += 4;
            PInfoRec recN1 = GetInfoRec(classVMT + VmtSelfPtr);
            /*DWORD parentAdr = *((DWORD*)(Code + n));*/ n += 4;
            WORD propCount = *((WORD*)(Code + n)); n += 2;
            //Skip unit name
            len = Code[n]; n++;
            n += len;
            //Real properties count
            propCount = *((WORD*)(Code + n)); n += 2;

            for (int i = 0; i < propCount && !Terminated; i++)
            {
                DWORD propType = *((DWORD*)(Code + n)); n += 4;
                int posn = Adr2Pos(propType);
                posn += 4;
                posn++; //property type
                len = Code[posn]; posn++;
                String fieldType = String((char*)(Code + posn), len);

                DWORD getProc = *((DWORD*)(Code + n)); n += 4;
                DWORD setProc = *((DWORD*)(Code + n)); n += 4;
                DWORD storedProc = *((DWORD*)(Code + n)); n += 4;
                //idx
                n += 4;
                //defval
                n += 4;
                //nameIdx
                n += 2;
                len = Code[n]; n++;
                String fieldName = String((char*)(Code + n), len); n += len;

                int fieldOfs = -1;
                if ((getProc & 0xFFFFFF00))
                {
                    if ((getProc & 0xFF000000) == 0xFF000000)
                        fieldOfs = getProc & 0x00FFFFFF;
                }
                if ((setProc & 0xFFFFFF00))
                {
                    if ((setProc & 0xFF000000) == 0xFF000000)
                        fieldOfs = setProc & 0x00FFFFFF;
                }
                if ((storedProc & 0xFFFFFF00))
                {
                    if ((storedProc & 0xFF000000) == 0xFF000000)
                        fieldOfs = storedProc & 0x00FFFFFF;
                }
                if (recN1 && fieldOfs != -1) recN1->vmtInfo->AddField(0, 0, FIELD_PUBLIC, fieldOfs, -1, fieldName, fieldType);
            }
        }
    }
}
//---------------------------------------------------------------------------
//LString
//RefCnt     Length     Data
//0          4          8
//                      recN (kind = ikLString, name = context)
//UString
//CodePage  Word    RefCnt  Length  Data
//0         2       4       8       12
//                                  recN (kind = ikUString, name = context)
void __fastcall TAnalyzeThread::FindStrings()
{
    int			i, len;
    WORD        codePage, elemSize;
    DWORD		refCnt;
    PInfoRec	recN;

    if (DelphiVersion >= 2009)
    {
        int stepMask = StartProgress(CodeSize, "Scan UStrings");
        //Scan UStrings
        for (i = 0; i < CodeSize && !Terminated; i += 4)
        {
            if ((i & stepMask) == 0) UpdateProgress();
            if (IsFlagSet(cfData, i)) continue;

            codePage = *((WORD*)(Code + i));
            elemSize = *((WORD*)(Code + i + 2));
            if (!elemSize || elemSize > 4) continue;
            refCnt = *((DWORD*)(Code + i + 4));
            if (refCnt != 0xFFFFFFFF) continue;
            //len = wcslen((wchar_t*)(Code + i + 12));
            len = *((int*)(Code + i + 8));
            if (len <= 0 || len > 10000) continue;
            if (i + 12 + (len + 1)*elemSize >= CodeSize) continue;
            if (!Infos[i + 12])
            {
                UpdateAddrInStatusBar(Pos2Adr(i));
                recN = new InfoRec(i + 12, ikUString);
                if (elemSize == 1)
                    recN->SetName(TransformString(Code + i + 12, len));
                else
                    recN->SetName(TransformUString(codePage, (wchar_t*)(Code + i + 12), len));
            }
            //Align to 4 bytes
            len = (12 + (len + 1)*elemSize + 3) & (-4);
            SetFlags(cfData, i, len);
        }
        StopProgress();
    }

    int stepMask = StartProgress(CodeSize, "Scan LStrings");
    //Scan LStrings
    for (i = 0; i < CodeSize && !Terminated; i += 4)
    {
        if ((i & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfData, i)) continue;

        refCnt = *((DWORD*)(Code + i));
        if (refCnt != 0xFFFFFFFF) continue;
        len = *((int*)(Code + i + 4));
        if (len <= 0 || len > 10000) continue;
        if (i + 8 + len + 1 >= CodeSize) continue;
        //Check last 0
        if (*(Code + i + 8 + len)) continue;
        //Check flags
        //!!!
        if (!Infos[i + 8])
        {
            UpdateAddrInStatusBar(Pos2Adr(i));
        	recN = new InfoRec(i + 8, ikLString);
            recN->SetName(TransformString(Code + i + 8, len));
        }
        //Align to 4 bytes
        len = (8 + len + 1 + 3) & (-4);
        SetFlags(cfData, i, len);
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::AnalyzeCode1()
{
    String      msg = "Analyzing step 1...";
    PUnitRec    recU;

    StartProgress(UnitsNum, msg);

    //Initialization and Finalization procedures
    for (int n = 0; n < UnitsNum && !Terminated; n++)
    {
        UpdateProgress();
        recU = (UnitRec*)Units->Items[n];
        if (recU)
        {
            DWORD iniAdr = recU->iniadr;
            if (iniAdr)
            {
                UpdateStatusBar(iniAdr);
                mainForm->AnalyzeProc(1, iniAdr);
            }
            DWORD finAdr = recU->finadr;
            if (finAdr)
            {
                UpdateStatusBar(finAdr);
                mainForm->AnalyzeProc(1, finAdr);
            }
        }
    }
    StopProgress();
    int stepMask = StartProgress(TotalSize, msg);
    //EP
    mainForm->AnalyzeProc(1, EP);
    DWORD vmtAdr;
    //Classes (methods, dynamics procedures, virtual methods)
    for (int n = 0; n < TotalSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        PInfoRec recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->kind == ikVMT)
        {
            vmtAdr = Pos2Adr(n);
            UpdateAddrInStatusBar(vmtAdr);

            mainForm->AnalyzeMethodTable(1, vmtAdr, &Terminated);
            if (Terminated) break;
            mainForm->AnalyzeDynamicTable(1, vmtAdr, &Terminated);
            if (Terminated) break;
            mainForm->AnalyzeVirtualTable(1, vmtAdr, &Terminated);
        }
    }
    StopProgress();
    //All procs
    stepMask = StartProgress(CodeSize, msg);
    for (int n = 0; n < CodeSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfProcStart, n))
        {
            DWORD adr = Pos2Adr(n);
            UpdateAddrInStatusBar(adr);
            mainForm->AnalyzeProc(1, adr);
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::AnalyzeCode2(bool args)
{
    PUnitRec    recU;

    int stepMask = StartProgress(CodeSize, "Analyzing step 2...");

    //EP
    mainForm->AnalyzeProc(2, EP);

    for (int n = 0; n < CodeSize && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        if (IsFlagSet(cfProcStart, n))
        {
            DWORD adr = Pos2Adr(n);
            UpdateAddrInStatusBar(adr);
            if (args) mainForm->AnalyzeArguments(adr);
            mainForm->AnalyzeProc(2, adr);
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::PropagateClassProps()
{
    PInfoRec    recN, recN1;

    int stepMask = StartProgress(CodeSize, "Propagating Class Properties...");
    for (int n = 0; n < CodeSize && !Terminated; n += 4)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->HasName())
        {
            BYTE typeKind = recN->kind;
            if (typeKind > ikProcedure) continue;
            //Пройдемся по свойствам класса и поименуем процедуры
            if (typeKind == ikClass)
            {
                int pos = n;
                //SelfPointer
                pos += 4;
                //TypeKind
                pos++;
                BYTE len = Code[pos]; pos++;
                String clsName = String((char*)(Code + pos), len); pos += len;
                DWORD classVMT = *((DWORD*)(Code + pos)); pos += 4;
                pos += 4;   //ParentAdr
                pos += 2;   //PropCount
                //UnitName
                len = Code[pos]; pos++;
                pos += len;
                WORD propCount = *((WORD*)(Code + pos)); pos += 2;

                for (int i = 0; i < propCount && !Terminated; i++)
                {
                    DWORD propType = *((DWORD*)(Code + pos)); pos += 4;
                    int posn = Adr2Pos(propType); posn += 4;
                    posn++; //property type
                    len = Code[posn]; posn++;
                    String typeName = String((char*)(Code + posn), len);

                    DWORD getProc = *((DWORD*)(Code + pos)); pos += 4;
                    DWORD setProc = *((DWORD*)(Code + pos)); pos += 4;
                    DWORD storedProc = *((DWORD*)(Code + pos)); pos += 4;
                    pos += 4;   //Idx
                    pos += 4;   //DefVal
                    pos += 2;   //NameIdx
                    len = Code[pos]; pos++;
                    String name = String((char*)(Code + pos), len); pos += len;

                    int vmtofs, fieldOfs;
                    PFIELDINFO fInfo;

                    if ((getProc & 0xFFFFFF00))
                    {
                        if ((getProc & 0xFF000000) == 0xFF000000)
                        {
                            fieldOfs = getProc & 0x00FFFFFF;
                            recN1 = GetInfoRec(classVMT + VmtSelfPtr);
                            if (recN1 && recN1->vmtInfo)
                                recN1->vmtInfo->AddField(0, 0, FIELD_PUBLIC, fieldOfs, -1, name, typeName);
                        }
                        else if ((getProc & 0xFF000000) == 0xFE000000)
                        {
                            if ((getProc & 0x00008000))
                                vmtofs = -((int)getProc & 0x0000FFFF);
                            else
                                vmtofs = getProc & 0x0000FFFF;
                            posn = Adr2Pos(classVMT) + vmtofs;
                            getProc = *((DWORD*)(Code + posn));
                            recN1 = GetInfoRec(getProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(getProc), ikFunc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;

                            recN1->kind = ikFunc;
                            recN1->type = typeName;
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".Get" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            mainForm->AnalyzeProc1(getProc, 0, 0, 0, false);
                        }
                        else
                        {
                            recN1 = GetInfoRec(getProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(getProc), ikFunc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;
                            recN1->kind = ikFunc;
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".Get" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            mainForm->AnalyzeProc1(getProc, 0, 0, 0, false);
                        }
                    }
                    if ((setProc & 0xFFFFFF00))
                    {
                        if ((setProc & 0xFF000000) == 0xFF000000)
                        {
                            fieldOfs = setProc & 0x00FFFFFF;
                            recN1 = GetInfoRec(classVMT + VmtSelfPtr);
                            if (recN1 && recN1->vmtInfo)
                                recN1->vmtInfo->AddField(0, 0, FIELD_PUBLIC, fieldOfs, -1, name, typeName);
                        }
                        else if ((setProc & 0xFF000000) == 0xFE000000)
                        {
                            if ((setProc & 0x00008000))
                                vmtofs = -((int)setProc & 0x0000FFFF);
                            else
                                vmtofs = setProc & 0x0000FFFF;
                            posn = Adr2Pos(classVMT) + vmtofs;
                            setProc = *((DWORD*)(Code + posn));
                            recN1 = GetInfoRec(setProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(setProc), ikProc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;
                            recN1->kind = ikProc;
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".Set" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            recN1->procInfo->AddArg(0x21, 1, 4, "Value", typeName);
                            mainForm->AnalyzeProc1(setProc, 0, 0, 0, false);
                        }
                        else
                        {
                            recN1 = GetInfoRec(setProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(setProc), ikProc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;
                            recN1->kind = ikProc;
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".Set" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            recN1->procInfo->AddArg(0x21, 1, 4, "Value", typeName);
                            mainForm->AnalyzeProc1(setProc, 0, 0, 0, false);
                        }
                    }
                    if ((storedProc & 0xFFFFFF00))
                    {
                        if ((storedProc & 0xFF000000) == 0xFF000000)
                        {
                            fieldOfs = storedProc & 0x00FFFFFF;
                            recN1 = GetInfoRec(classVMT + VmtSelfPtr);
                            if (recN1 && recN1->vmtInfo)
                                recN1->vmtInfo->AddField(0, 0, FIELD_PUBLIC, fieldOfs, -1, name, typeName);
                        }
                        else if ((storedProc & 0xFF000000) == 0xFE000000)
                        {
                            if ((storedProc & 0x00008000))
                                vmtofs = -((int)storedProc & 0x0000FFFF);
                            else
                                vmtofs = storedProc & 0x0000FFFF;
                            posn = Adr2Pos(classVMT) + vmtofs;
                            storedProc = *((DWORD*)(Code + posn));
                            recN1 = GetInfoRec(storedProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(storedProc), ikFunc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;
                            recN1->kind = ikFunc;
                            recN1->type = "Boolean";
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".IsStored" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            recN1->procInfo->AddArg(0x21, 1, 4, "Value", typeName);
                            mainForm->AnalyzeProc1(storedProc, 0, 0, 0, false);
                        }
                        else
                        {
                            recN1 = GetInfoRec(storedProc);
                            if (!recN1)
                                recN1 = new InfoRec(Adr2Pos(storedProc), ikFunc);
                            else if (!recN1->procInfo)
                                recN1->procInfo = new InfoProcInfo;
                            recN1->kind = ikFunc;
                            recN1->type = "Boolean";
                            if (!recN1->HasName())
                                recN1->SetName(clsName + ".IsStored" + name);
                            recN1->procInfo->flags |= PF_METHOD;
                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
                            recN1->procInfo->AddArg(0x21, 1, 4, "Value", typeName);
                            mainForm->AnalyzeProc1(storedProc, 0, 0, 0, false);
                        }
                    }
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::FillClassViewer()
{
    PVmtListRec recV;

    mainForm->tvClassesFull->Items->Clear();
    const int cntVmt = VmtList->Count;
    if (!cntVmt) return;

    int stepMask = StartProgress(cntVmt, "Building ClassViewer Tree...");
    TStringList *tmpList = new TStringList;
    tmpList->Sorted = false;

    mainForm->tvClassesFull->Items->BeginUpdate();

    for (int n = 0; n < cntVmt && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();

        recV = (PVmtListRec)VmtList->Items[n];
        UpdateStatusBar(GetClsName(recV->vmtAdr));
        mainForm->FillClassViewerOne(n, tmpList, &Terminated);
    }

    mainForm->tvClassesFull->Items->EndUpdate();
    
    ProjectModified = true;
    ClassTreeDone = true;
    delete tmpList;
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::AnalyzeDC()
{
    int         n, m, k, dotpos, pos, stepMask, instrLen;
    DWORD       vmtAdr, adr, procAdr, stopAt, classAdr;
    String      className, name;
    PVmtListRec recV;
    PInfoRec    recN, recN1, recN2;
    PARGINFO    argInfo;
    PMethodRec  recM, recM1;
    DISINFO     disInfo;

    //Создаем временный список пар (высота, адрес VMT)
    const int cntVmt = VmtList->Count;
    if (!cntVmt) return;

    stepMask = StartProgress(cntVmt, "Analyzing Constructors and Destructors, step 1...");
    for (n = 0; n < cntVmt && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        recV = (PVmtListRec)VmtList->Items[n];
        vmtAdr = recV->vmtAdr;
        className = GetClsName(vmtAdr);
        UpdateStatusBar(className);

        //Destructor
        pos = Adr2Pos(vmtAdr) - VmtSelfPtr + VmtDestroy;
        adr = *((DWORD*)(Code + pos));
        if (IsValidImageAdr(adr))
        {
            recN = GetInfoRec(adr);
            if (recN && !recN->HasName())
            {
                recN->kind = ikDestructor;
                recN->SetName(className + ".Destroy");
            }
        }
        //Constructor
        recN = GetInfoRec(vmtAdr);

        if (recN && recN->xrefs)
        {
            for (m = 0; m < recN->xrefs->Count; m++)
            {
                PXrefRec recX = (PXrefRec)recN->xrefs->Items[m];
                adr = recX->adr + recX->offset;
                recN1 = GetInfoRec(adr);
                if (recN1 && !recN1->HasName())
                {
                    if (IsFlagSet(cfProcStart, Adr2Pos(adr)))
                    {
                        recN1->kind = ikConstructor;
                        recN1->SetName(className + ".Create");
                    }
                }
            }
        }
    }
    StopProgress();
    stepMask = StartProgress(cntVmt, "Analyzing Constructors, step 2...");
    for (n = 0; n < cntVmt && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        recV = (PVmtListRec)VmtList->Items[n];
        vmtAdr = recV->vmtAdr;
        stopAt = GetStopAt(vmtAdr - VmtSelfPtr);
        if (vmtAdr == stopAt) continue;

        className = GetClsName(vmtAdr);
        UpdateStatusBar(className);
        pos = Adr2Pos(vmtAdr) - VmtSelfPtr + VmtParent + 4;

        for (m = VmtParent + 4;; m += 4, pos += 4)
        {
            if (Pos2Adr(pos) == stopAt) break;
            procAdr = *((DWORD*)(Code + pos));
            if (m >= 0)
            {
                recN = GetInfoRec(procAdr);
                if (recN && recN->kind == ikConstructor && !recN->HasName())
                {
                    classAdr = vmtAdr;
                    while (classAdr)
                    {
                        recM = mainForm->GetMethodInfo(classAdr, 'V', m);
                        if (recM)
                        {
                            name = recM->name;
                            if (name != "")
                            {
                                dotpos = name.Pos(".");
                                if (dotpos)
                                    recN->SetName(className + name.SubString(dotpos, name.Length()));
                                else
                                    recN->SetName(name);
                                break;
                            }
                        }
                        classAdr = GetParentAdr(classAdr);
                    }
                }
            }
        }
    }
    StopProgress();
    stepMask = StartProgress(cntVmt, "Analyzing Dynamic Methods...");
    for (n = 0; n < cntVmt && !Terminated; n++)
    {
        if ((n & stepMask) == 0) UpdateProgress();
        recV = (PVmtListRec)VmtList->Items[n];
        vmtAdr = recV->vmtAdr;
        className = GetClsName(vmtAdr);
        UpdateStatusBar(className);

        recN = GetInfoRec(vmtAdr);

        if (recN && recN->vmtInfo->methods)
        {
            for (m = 0; m < recN->vmtInfo->methods->Count; m++)
            {
                recM = (PMethodRec)recN->vmtInfo->methods->Items[m];
                if (recM->kind == 'D')
                {
                    recN1 = GetInfoRec(recM->address);
                    if (recN1)
                    {
                        classAdr = GetParentAdr(vmtAdr);
                        while (classAdr)
                        {
                            recM1 = mainForm->GetMethodInfo(classAdr, 'D', recM->id);
                            if (recM1)
                            {
                                recN2 = GetInfoRec(recM1->address);
                                if (recN2 && recN2->procInfo->args)
                                {
                                    for (k = 0; k < recN2->procInfo->args->Count; k++)
                                    {
                                        if (!k)
                                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", className);
                                        else
                                        {
                                            argInfo = (PARGINFO)recN2->procInfo->args->Items[k];
                                            recN1->procInfo->AddArg(argInfo);
                                        }
                                    }
                                }
                            }
                            classAdr = GetParentAdr(classAdr);
                        }
                    }
                }
            }
        }
    }
    StopProgress();
}
//---------------------------------------------------------------------------
void __fastcall TAnalyzeThread::ClearPassFlags()
{
    if (!Terminated) mainForm->ClearPassFlags();
}
//---------------------------------------------------------------------------
