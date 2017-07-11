//---------------------------------------------------------------------------
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#include "SyncObjs.hpp"
#include <dir.h>
#include <io.h>
#include <stdio.h>
#include <winnt.h>
#include "Main.h"
#include "Misc.h"
#include "Threads.h"
#include "ProgressBar.h"
#include "TypeInfo.h"
#include "StringInfo.h"
#include "StrUtils.hpp"
#include "FindDlg.h"
#include "InputDlg.h"
#include "Disasm.h"
#include "Explorer.h"
#include "KBViewer.h"
#include "EditFunctionDlg.h"
#include "EditFieldsDlg.h"
#include "AboutDlg.h"
#include "Legend.h"
#include "IDCGen.h"
#include "IdcSplitSize.h"
#include "Decompiler.h"
#include "Hex2Double.h"
#include "Plugins.h"
#include "ActiveProcesses.h"
#include <ComCtrls.hpp>
#include <Chart.hpp>
#include <Outline.hpp>
#include <Tabnotbk.hpp>
#include <IniFiles.hpp>
#include <assert.h>
/*
//----Highlighting-----------------------------------------------------------
#include "Highlight.h"
DWORD       DelphiLbId;
int         DelphiThemesCount;
//----Highlighting-----------------------------------------------------------
*/
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma resource "idr_manifest.res"
//---------------------------------------------------------------------------
//as statistics of analysis flow
//unsigned long stat_GetClassAdr_calls = 0;
//unsigned long stat_GetClassAdr_adds = 0;
//---------------------------------------------------------------------------
String  IDRVersion = "01.04.2017"; 
//---------------------------------------------------------------------------
SysProcInfo    SysProcs[] = {
    {"@HandleFinally", 0},
    {"@HandleAnyException", 0},
    {"@HandleOnException", 0},
    {"@HandleAutoException", 0},
    {"@RunError", 0},
    //{"@Halt", 0},
    {"@Halt0", 0},
    {"@AbstractError", 0},
    {0, 0}
};

SysProcInfo    SysInitProcs[] = {
    {"@InitExe", 0},
    {"@InitLib", 0},
    {0, 0}
};
//Image       Code
//|===========|======================================|
//ImageBase   CodeBase
//---------------------------------------------------------------------------
char    StringBuf[MAXSTRBUFFER];    //Buffer to make string

extern  char*   Reg32Tab[8];
extern  char*   Reg16Tab[8];
extern  char*   Reg8Tab[8];
extern  char*   SegRegTab[8];
extern  char*   RepPrefixTab[4];

extern  RegClassInfo RegClasses[];

int             dummy = 0;          //for debugging purposes!!!

MDisasm         Disasm;				//Дизассемблер для анализатора кода
MKnowledgeBase  KnowledgeBase;
TResourceInfo   *ResInfo = 0;       //Information about forms
int             CodeHistorySize;    //Current size of Code navigation History Array
int             CodeHistoryPtr;     //Curent pointer of Code navigation History Array
int				CodeHistoryMax;		//Max pointer position of Code navigation History Array (for ->)
DynamicArray<PROCHISTORYREC> CodeHistory;   //Code navigation History Array

TAnalyzeThread  *AnalyzeThread = 0; //Поток для фонового анализа кода
int             AnalyzeThreadRetVal = 0;
bool            SourceIsLibrary = false;
bool            ClassTreeDone;
bool            ProjectModified = false;
bool            UserKnowledgeBase = false;
bool            SplitIDC = false;
int             SplitSize = 0;
//Common variables
String          IDPFile;
int             MaxBufLen;      //Максимальная длина буфера (для загрузки)
int             DelphiVersion;
DWORD           EP;
DWORD           ImageBase;
DWORD           ImageSize;
DWORD           TotalSize;      //Size of sections CODE + DATA
DWORD           CodeBase;
DWORD           CodeSize;
DWORD           CodeStart;
DWORD           DataBase = 0;
DWORD           DataSize = 0;
DWORD           DataStart = 0;
BYTE            *Image = 0;
DWORD           *Flags = 0;     //flags for used data
PInfoRec        *Infos = 0;	    //Array of pointers to store items data
TStringList     *BSSInfos = 0;  //Data from BSS
BYTE            *Code = 0;
BYTE            *Data = 0;

TList           *ExpFuncList;   //Exported functions list (temporary)
TList           *ImpFuncList;   //Imported functions list (temporary)
TStringList     *ImpModuleList; //Imported modules   list (temporary)
TList           *SegmentList;   //Information about Image Segments
TList           *VmtList;       //VMT list

//Units
int             UnitsNum = 0;
TList           *Units = 0;
int             UnitSortField = 0; //0 - by address, 1 - by initialization order, 2 - by name
//Types
TList           *OwnTypeList = 0;
int             RTTISortField = 0; //0 - by address, 1 - by initialization order, 2 - by name

DWORD           CurProcAdr;
int				CurProcSize;
String          SelectedAsmItem = "";    //Selected item in Asm Listing
String          SelectedSourceItem = ""; //Selected item in Source Code
DWORD           CurUnitAdr;
DWORD           HInstanceVarAdr;
DWORD           LastTls;            //Last bust index Tls shows how many ThreadVars in program
int             Reserved;
int             LastResStrNo = 0;   //Last ResourceStringNo
DWORD			CtdRegAdr;			//Procedure CtdRegAdr address

int             VmtSelfPtr			 = 0;
int             VmtIntfTable		 = 0;
int             VmtAutoTable		 = 0;
int             VmtInitTable		 = 0;
int             VmtTypeInfo			 = 0;
int             VmtFieldTable		 = 0;
int             VmtMethodTable		 = 0;
int             VmtDynamicTable		 = 0;
int             VmtClassName		 = 0;
int             VmtInstanceSize		 = 0;
int             VmtParent			 = 0;
int             VmtEquals			 = 0;
int             VmtGetHashCode		 = 0;
int             VmtToString			 = 0;
int             VmtSafeCallException = 0;
int             VmtAfterConstruction = 0;
int             VmtBeforeDestruction = 0;
int             VmtDispatch			 = 0;
int             VmtDefaultHandler	 = 0;
int             VmtNewInstance		 = 0;
int             VmtFreeInstance		 = 0;
int             VmtDestroy			 = 0;

//as
//class addresses cache
typedef std::map<const String, DWORD> TClassAdrMap;
TClassAdrMap classAdrMap;

void __fastcall ClearClassAdrMap();
//String __fastcall UnmangleName(char* Name);

TCriticalSection    *CrtSection = 0;
TFMain_11011981 *FMain_11011981;
//---------------------------------------------------------------------------
__fastcall TFMain_11011981::TFMain_11011981(TComponent* Owner)
    : dragdropHelper(Handle), TForm(Owner)
{
    CrtSection = new TCriticalSection;
}
//---------------------------------------------------------------------------
__fastcall TFMain_11011981::~TFMain_11011981()
{
    if (CrtSection) delete CrtSection;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormClose(TObject *Sender,
      TCloseAction &Action)
{
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    switch (Key)
    {
    case 'G':
        GoToAddress();
        break;
    case 'N':
        NamePosition();
        break;
    case 'F':  //CTRL + F (1st search on different areas)
    {
        if (Shift.Contains(ssCtrl))
        {
            switch (WhereSearch)
            {
            case SEARCH_UNITS:
                miSearchUnitClick(Sender);
                break;
            case SEARCH_UNITITEMS:
                miSearchItemClick(Sender);
                break;
            case SEARCH_RTTIS:
                miSearchRTTIClick(Sender);
                break;
            case SEARCH_FORMS:
                miSearchFormClick(Sender);
                break;
            case SEARCH_CLASSVIEWER:
                miSearchVMTClick(Sender);
                break;
            case SEARCH_STRINGS:
                miSearchStringClick(Sender);
                break;
            case SEARCH_NAMES:
                miSearchNameClick(Sender);
                break;            
            //todo rest of locations
            }
        }
        break;
    }
    case VK_F3:  // F3 - (2nd search, continue search with same text)
        switch (WhereSearch)
        {
        case SEARCH_UNITS:
            FindText(UnitsSearchText);
            break;
        case SEARCH_UNITITEMS:
            FindText(UnitItemsSearchText);
            break;
        case SEARCH_RTTIS:
            FindText(RTTIsSearchText);
            break;
        case SEARCH_FORMS:
            FindText(FormsSearchText);
            break;            
        case SEARCH_CLASSVIEWER:
            FindText(VMTsSearchText);
            break;
        case SEARCH_STRINGS:
        	FindText(StringsSearchText);
            break;
        case SEARCH_NAMES:
        	FindText(NamesSearchText);        
            break;            
        }
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::Units1Click(TObject *Sender)
{
    //if (tsUnits->Enabled)
    //{
        pcInfo->ActivePage = tsUnits;
        if (lbUnits->CanFocus()) ActiveControl = lbUnits;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::RTTI1Click(TObject *Sender)
{
    //if (tsRTTIs->Enabled)
    //{
        pcInfo->ActivePage = tsRTTIs;
        if (lbRTTIs->CanFocus()) ActiveControl = lbRTTIs;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::Forms1Click(TObject *Sender)
{
    //if (tsForms->Enabled)
    //{
        pcInfo->ActivePage = tsForms;
        if (lbForms->CanFocus()) ActiveControl = lbForms;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::CodeViewer1Click(TObject *Sender)
{
    //if (tsCodeView->Enabled)
    //{
        pcWorkArea->ActivePage = tsCodeView;
        if (lbCode->CanFocus()) ActiveControl = lbCode;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ClassViewer1Click(TObject *Sender)
{
    //if (tsClassView->Enabled)
    //{
        pcWorkArea->ActivePage = tsClassView;
        if (!rgViewerMode->ItemIndex)
            if (tvClassesFull->CanFocus()) ActiveControl = tvClassesFull;
        else
            if (tvClassesShort->CanFocus()) ActiveControl = tvClassesShort;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::Strings1Click(TObject *Sender)
{
    //if (tsStrings->Enabled)
    //{
        pcWorkArea->ActivePage = tsStrings;
        if (lbStrings->CanFocus()) ActiveControl = lbStrings;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::Names1Click(TObject *Sender)
{
    //if (tsNames->Enabled)
    //{
        pcWorkArea->ActivePage = tsNames;
        if (lbNames->CanFocus()) ActiveControl = lbNames;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::SourceCode1Click(TObject *Sender)
{
    //if (tsSourceCode->Enabled)
    //{
        pcWorkArea->ActivePage = tsSourceCode;
        if (lbSourceCode->CanFocus()) ActiveControl = lbSourceCode;
    //}
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExitClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::Init()
{
    IDPFile = "";
    UnitsNum = 0;
    CurProcAdr = 0;
    CurProcSize = 0;
    SelectedAsmItem = "";
    CurUnitAdr = 0;
    CodeHistoryPtr = -1;
    CodeHistorySize = 0;//HISTORY_CHUNK_LENGTH;
    CodeHistory.Length = CodeHistorySize;
    CodeHistoryMax = CodeHistoryPtr;

	DelphiVersion = -1;
    Caption = "Interactive Delphi Reconstructor by crypto";

    HInstanceVarAdr = 0xFFFFFFFF;
    LastTls = 0;
    CtdRegAdr = 0;

    WhereSearch = SEARCH_UNITS;
    UnitsSearchFrom = -1;
    UnitsSearchText = "";

    RTTIsSearchFrom = -1;
    RTTIsSearchText = "";

    FormsSearchFrom = -1;
    FormsSearchText = "";

    UnitItemsSearchFrom = -1;
    UnitItemsSearchText = "";

    TreeSearchFrom = 0;
    BranchSearchFrom = 0;
    VMTsSearchText = "";

    StringsSearchFrom = 0;
    StringsSearchText = "";

    NamesSearchFrom = 0;
    NamesSearchText = "";

    //Init Menu
    miLoadFile->Enabled = true;
    miOpenProject->Enabled = true;
    miMRF->Enabled = true;
    miSaveProject->Enabled = false;
    miSaveDelphiProject->Enabled = false;
    miExit->Enabled = true;
    miMapGenerator->Enabled = false;
    miCommentsGenerator->Enabled = false;
    miIDCGenerator->Enabled = false;
    miLister->Enabled = false;
    miClassTreeBuilder->Enabled = false;
    miKBTypeInfo->Enabled = false;
    miCtdPassword->Enabled = false;
    miHex2Double->Enabled = false;

    //Init Units
    lbUnits->Clear();
    miRenameUnit->Enabled = false;
    miSearchUnit->Enabled = false;
    miSortUnits->Enabled = false;
    miCopyList->Enabled = false;
    UnitSortField = 0;
    miSortUnitsByAdr->Checked = true;
    miSortUnitsByOrd->Checked = false;
    miSortUnitsByNam->Checked = false;
    tsUnits->Enabled = false;

    //Init RTTIs
    lbRTTIs->Clear();
    miSearchRTTI->Enabled = false;
    miSortRTTI->Enabled = false;
    RTTISortField = 0;
    miSortRTTIsByAdr->Checked = true;
    miSortRTTIsByKnd->Checked = false;
    miSortRTTIsByNam->Checked = false;
    tsRTTIs->Enabled = false;

    //Init Forms
    lbForms->Clear();
    lbAliases->Clear();
    lClassName->Caption = "";
    cbAliases->Clear();
    rgViewFormAs->ItemIndex = 0;
    tsForms->Enabled = false;

    //Init Code
    lProcName->Caption = "";
    lbCode->Clear();
    lbCode->ScrollWidth = 0;
    miGoTo->Enabled = false;
    miExploreAdr->Enabled = false;
    miName->Enabled = false;
    miViewProto->Enabled = false;
    miEditFunctionC->Enabled = false;
    miXRefs->Enabled = false;
    miSwitchFlag->Enabled = false;
    bEP->Enabled = false;
    bDecompile->Enabled = false;
    bCodePrev->Enabled = false;
    bCodeNext->Enabled = false;
    tsCodeView->Enabled = false;
    lbCXrefs->Clear();
    lbCXrefs->Visible = true;

    //Init Strings
    lbStrings->Clear();
    miSearchString->Enabled = false;
    tsStrings->Enabled = false;
    //Init Names
    lbNames->Clear();
    tsNames->Enabled = false;

    //Xrefs
    lbSXrefs->Clear();
    lbSXrefs->Visible = true;

    //Init Unit Items
    lbUnitItems->Clear();
    lbUnitItems->ScrollWidth = 0;
    miEditFunctionI->Enabled = false;
    miFuzzyScanKB->Enabled = false;
    miSearchItem->Enabled = false;

    //Init ClassViewer
    ClassTreeDone = false;
    tvClassesFull->Items->Clear();
    tvClassesShort->Items->Clear();
    tvClassesShort->BringToFront();
    rgViewerMode->ItemIndex = 1;    //Short View
    tsClassView->Enabled = false;

    ClearTreeNodeMap();
    ClearClassAdrMap();

    Update();
    Sleep(0);

    ProjectLoaded = false;
    ProjectModified = false;
    UserKnowledgeBase = false;
    SourceIsLibrary = false;
}
//---------------------------------------------------------------------------
PImportNameRec __fastcall TFMain_11011981::GetImportRec(DWORD adr)
{
    for (int n = 0; n < ImpFuncList->Count; n++)
    {
        PImportNameRec recI = (PImportNameRec)ImpFuncList->Items[n];
        if (adr == recI->address)
            return recI;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FindExports()
{
    int     pos;

    for (int i = 0; i < ExpFuncList->Count; i++)
    {
        PExportNameRec recE = (PExportNameRec)ExpFuncList->Items[i];
        DWORD Adr = recE->address;
        if (IsValidImageAdr(Adr) && (pos = Adr2Pos(Adr)) != -1)
        {
            PInfoRec recN = new InfoRec(pos, ikRefine);
            recN->SetName(recE->name);
            recN->procInfo->flags = 3;     //stdcall
            Infos[pos] = recN;
            SetFlag(cfProcStart | cfExport, pos);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FindImports()
{
    char        *b, *e;
    int         pos;
    String      name;

    for (int i = 0; i < TotalSize - 6; i++)
    {
        if (Code[i] == 0xFF && Code[i + 1] == 0x25)
        {
            DWORD adr = *((DWORD*)(Code + i + 2));
            PImportNameRec recI = GetImportRec(adr);
            if (recI)
            {
                //name = UnmangleName(recI->name);
                if (!Infos[i])
                {
                    PInfoRec recN = new InfoRec(i, ikRefine);
                    recN->procInfo->procSize = 6;
                    SetFlag(cfProcStart, i);
                    SetFlag(cfProcEnd, i + 6);

                    if (recI->name.Pos("@Initialization$") || recI->name.Pos("@Finalization$"))
                    {
                        recN->SetName(recI->module + "." + recI->name);
                    }
                    else
                    {
                        b = strchr(recI->name.c_str() + 1, '@');
                        if (b)
                        {
                            e = strchr(b + 1, '$');
                            if (e)
                            {
                                if (*(e - 1) != '@')
                                    recN->SetName(String(b + 1, e - b - 1));
                                else
                                    recN->SetName(String(b + 1, e - b - 2));
                                if (recI->name.Pos("$bctr$"))
                                {
                                    recN->ConcatName("@Create");
                                    recN->kind = ikConstructor;
                                }
                                else if (recI->name.Pos("$bdtr$"))
                                {
                                    recN->ConcatName("@Destroy");
                                    recN->kind = ikDestructor;
                                }
                                pos = recN->GetName().Pos("@");
                                if (pos > 1) recN->GetName()[pos] = '.';
                            }
                        }
                        else
                            recN->SetName(recI->module + "." + recI->name);
                    }
                    for (int n = 0; SysProcs[n].name; n++)
                    {
                        if (recI->name.Pos(SysProcs[n].name))
                        {
                            SysProcs[n].impAdr = Pos2Adr(i);
                            break;
                        }
                    }
                    SetFlag(cfImport, i);
                }
                SetFlags(cfCode, i, 6);
            }
        }
    }
}
//---------------------------------------------------------------------------
//"Вшивает" VMT в код с позиции pos
void __fastcall TFMain_11011981::StrapVMT(int pos, int ConstId, MConstInfo* ConstInfo)
{
    if (!ConstInfo) return;

    //Check dump VMT
    BYTE *dump = ConstInfo->Dump;
    BYTE *relocs = ConstInfo->Dump + ConstInfo->DumpSz;
    bool match = true;
    for (int n = 0; n < ConstInfo->DumpSz; n++)
    {
        if (relocs[n] != 0xFF && Code[pos + n] != dump[n])
        {
            match = false;
            break;
        }
    }
    if (!match) return;

    SetFlags(cfData , pos - 4, ConstInfo->DumpSz + 4);

    int Idx, Pos, VMTOffset = VmtSelfPtr + 4;
    //"Strap" fixups
    //Get used modules array
    WORD *uses = KnowledgeBase.GetModuleUses(ConstInfo->ModuleID);
    //Begin fixups data
    BYTE *p = ConstInfo->Dump + 2*(ConstInfo->DumpSz);
    FIXUPINFO fixupInfo;
    MProcInfo aInfo; MProcInfo* pInfo = &aInfo;
    for (int n = 0; n < ConstInfo->FixupNum; n++)
    {
        fixupInfo.Type = *p; p++;
        fixupInfo.Ofs = *((DWORD*)p); p += 4;
        WORD Len = *((WORD*)p); p += 2;
        fixupInfo.Name = p; p += Len + 1;
        //Name begins with _D - skip it
        if (fixupInfo.Name[0] == '_' && fixupInfo.Name[1] == 'D') continue;
        //In VMT all fixups has type 'A'
        DWORD Adr = *((DWORD*)(Code + pos + fixupInfo.Ofs));

        VMTOffset = VmtSelfPtr + 4 + fixupInfo.Ofs;

        if (VMTOffset == VmtIntfTable)
        {
            if (IsValidCodeAdr(Adr) && !Infos[Adr2Pos(Adr)])
            {
                //Strap IntfTable
                Idx = KnowledgeBase.GetProcIdx(ConstInfo->ModuleID, fixupInfo.Name, Code + Adr2Pos(Adr));
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                    if (!KnowledgeBase.IsUsedProc(Idx))
                    {
                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                        {
                            StrapProc(Adr2Pos(Adr), Idx, pInfo, true, pInfo->DumpSz);
                        }
                    }
                }
            }
            continue;
        }
        if (VMTOffset == VmtAutoTable)
        {
            //Strap AutoTable
            //Unknown - no examples
            continue;
        }
        if (VMTOffset == VmtInitTable)
        {
            //InitTable представляет собой ссылки на типы, которые встретятся позже
            continue;
        }
        if (VMTOffset == VmtTypeInfo)
        {
            //Информация о типе уже обработана, пропускаем
            continue;
        }
        if (VMTOffset == VmtFieldTable)
        {
            //Пропускаем, поскольку будем обрабатывать информацию о полях позднее
            continue;
        }
        if (VMTOffset == VmtMethodTable)
        {
            //Пропускаем, поскольку методы будут обработаны среди прочих фиксапов
            continue;
        }
        if (VMTOffset == VmtDynamicTable)
        {
            //Пропускаем, поскольку динамические вызовы будут обработаны среди прочих фиксапов
            continue;
        }
        if (VMTOffset == VmtClassName)
        {
            //ClassName не обрабатываем
            continue;
        }
        if (VMTOffset == VmtParent)
        {
            //Указывает на родительский класс, не обрабатываем, поскольку он все-равно встретится отдельно
            continue;
        }
        if (VMTOffset >= VmtParent + 4 && VMTOffset <= VmtDestroy)
        {
            if (IsValidCodeAdr(Adr) && !Infos[Adr2Pos(Adr)])
            {
                Idx = KnowledgeBase.GetProcIdx(uses, fixupInfo.Name, Code + Adr2Pos(Adr));
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                    if (!KnowledgeBase.IsUsedProc(Idx))
                    {
                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                        {
                            StrapProc(Adr2Pos(Adr), Idx, pInfo, true, pInfo->DumpSz);
                        }
                    }
                }
                //Code not matched, but prototype may be used
                else
                {
                    PInfoRec recN = new InfoRec(Adr2Pos(Adr), ikRefine);
                    recN->SetName(fixupInfo.Name);
                    //Prototype???
                    if (uses)
                    {
                        for (int m = 0; uses[m] != 0xFFFF; m++)
                        {
                            Idx = KnowledgeBase.GetProcIdx(uses[m], fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                if (KnowledgeBase.GetProcInfo(Idx, INFO_ARGS, pInfo))
                                {
                                    switch (pInfo->MethodKind)
                                    {
                                    case 'C':
                                        recN->kind = ikConstructor;
                                        break;
                                    case 'D':
                                        recN->kind = ikDestructor;
                                        break;
                                    case 'P':
                                        recN->kind = ikProc;
                                        break;
                                    case 'F':
                                        recN->kind = ikFunc;
                                        recN->type = pInfo->TypeDef;
                                        break;
                                    }

                                    if (pInfo->Args)
                                    {
                                    	BYTE callKind = pInfo->CallKind;
                                        recN->procInfo->flags |= callKind;

                                        ARGINFO argInfo; BYTE *pp = pInfo->Args; int ss = 8;
                                        for (int k = 0; k < pInfo->ArgsNum; k++)
                                        {
                                            FillArgInfo(k, callKind, &argInfo, &pp, &ss);
                                            recN->procInfo->AddArg(&argInfo);
                                        }
                                    }
                                    //Set kbIdx for fast search
                                    recN->kbIdx = Idx;
                                    recN->procInfo->flags |= PF_KBPROTO;
                                }
                            }
                        }
                    }
                }
            }
            continue;
        }
        //Если адрес в кодовом сегменте и с ним не связано никакой информации
        if (IsValidCodeAdr(Adr) && !Infos[Adr2Pos(Adr)])
        {
            //Название типа?
            if (!IsFlagSet(cfRTTI, Adr2Pos(Adr)))
            {
                //Процедура?
                Idx = KnowledgeBase.GetProcIdx(uses, fixupInfo.Name, Code + Adr2Pos(Adr));
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                    if (!KnowledgeBase.IsUsedProc(Idx))
                    {
                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                        {
                            StrapProc(Adr2Pos(Adr), Idx, pInfo, true, pInfo->DumpSz);
                        }
                    }
                }
                //Code not matched, but prototype may be used
                else
                {
                    PInfoRec recN = new InfoRec(Adr2Pos(Adr), ikRefine);
                    recN->SetName(fixupInfo.Name);
                    //Prototype???
                    if (uses)
                    {
                        for (int m = 0; uses[m] != 0xFFFF; m++)
                        {
                            Idx = KnowledgeBase.GetProcIdx(uses[m], fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                if (KnowledgeBase.GetProcInfo(Idx, INFO_ARGS, pInfo))
                                {
                                    switch (pInfo->MethodKind)
                                    {
                                    case 'C':
                                        recN->kind = ikConstructor;
                                        break;
                                    case 'D':
                                        recN->kind = ikDestructor;
                                        break;
                                    case 'P':
                                        recN->kind = ikProc;
                                        break;
                                    case 'F':
                                        recN->kind = ikFunc;
                                        recN->type = pInfo->TypeDef;
                                        break;
                                    }

                                    if (pInfo->Args)
                                    {
                                    	BYTE callKind = pInfo->CallKind;
                                        recN->procInfo->flags |= callKind;

                                        ARGINFO argInfo; BYTE *pp = pInfo->Args; int ss = 8;
                                        for (int k = 0; k < pInfo->ArgsNum; k++)
                                        {
                                            FillArgInfo(k, callKind, &argInfo, &pp, &ss);
                                            recN->procInfo->AddArg(&argInfo);
                                        }
                                    }
                                    //Set kbIdx for fast search
                                    recN->kbIdx = Idx;
                                    recN->procInfo->flags |= PF_KBPROTO;
                                }
                            }
                        }
                    }
                }
            }
            continue;
        }
    }
    if (uses) delete[] uses;
}
//---------------------------------------------------------------------------
//Check possibility of "straping" procedure (only at the first level)
bool __fastcall TFMain_11011981::StrapCheck(int pos, MProcInfo* ProcInfo)
{
    int         _ap;
    String      name, fName, _key;
    PInfoRec    recN;

    if (!ProcInfo) return false;

    BYTE *dump = ProcInfo->Dump;
    //Fixup data begin
    BYTE *p = dump + 2*(ProcInfo->DumpSz);
    //If procedure is jmp off_XXXXXXXX, return false
    if (*dump == 0xFF && *(dump + 1) == 0x25) return false;

    FIXUPINFO fixupInfo;
    for (int n = 0; n < ProcInfo->FixupNum; n++)
    {
        fixupInfo.Type = *p; p++;
        fixupInfo.Ofs = *((DWORD*)p); p += 4;
        WORD Len = *((WORD*)p); p += 2;
        fixupInfo.Name = p; p += Len + 1;

        String fName = String(fixupInfo.Name);
        //Fixupname begins with "_Dn_" - skip it
        if (fName.Pos("_Dn_") == 1) continue;
        //Fixupname begins with "_NF_" - skip it
        if (fName.Pos("_NF_") == 1) continue;
        //Fixupname is "_DV_" - skip it
        if (SameText(fName, "_DV_")) continue;
        //Fixupname begins with "_DV_"
        if (fName.Pos("_DV_") == 1)
        {
            char c = fName[5];
            //Fixupname is _DV_number - skip it
            if (c >= '0' && c <= '9') continue;
            //Else transfrom fixupname to normal
            if (fName[Len] == '_')
                fName = fName.SubString(5, Len - 5);
            else
                fName = fName.SubString(5, Len - 4);
        }
        //Empty fixupname - skip it
        if (fName == "") continue;

        DWORD Adr, Ofs, Val = *((DWORD*)(Code + pos + fixupInfo.Ofs));
        if (fixupInfo.Type == 'A' || fixupInfo.Type == 'S')
        {
            Ofs = *((DWORD*)(dump + fixupInfo.Ofs));
            Adr = Val - Ofs;
            if (IsValidImageAdr(Adr))
            {
                _ap = Adr2Pos(Adr); recN = GetInfoRec(Adr);
                if (recN && recN->HasName())
                {
                    //If not import call just compare names
                    if (_ap >= 0 && !IsFlagSet(cfImport, _ap))
                    {
                        if (!recN->SameName(fName)) return false;
                    }
                    //Else may be partial unmatching
                    else
                    {
                        name = ExtractProcName(recN->GetName());
                        if (!SameText(name, fName) && !SameText(name.SubString(1, name.Length() - 1), fName))
                            return false;
                    }
                }
            }
        }
        else if (fixupInfo.Type == 'J')
        {
            Adr = CodeBase + pos + fixupInfo.Ofs + 4 + Val;
            if (IsValidCodeAdr(Adr))
            {
                _ap = Adr2Pos(Adr); recN = GetInfoRec(Adr);
                if (recN && recN->HasName())
                {
                    //If not import call just compare names
                    if (_ap >= 0 && !IsFlagSet(cfImport, _ap))
                    {
                        if (!recN->SameName(fName)) return false;
                    }
                    //Else may be partial unmatching
                    else
                    {
                        name = ExtractProcName(recN->GetName());
                        if (!SameText(name, fName))
                        {
                            String name1 = name.SubString(1, name.Length() - 1);//Trim last symbol ('A','W') - GetWindowLongW(A)
                            if (!SameText(fName.SubString(1, name1.Length()), name1)) return false;
                        }
                    }
                }
            }
        }
        else if (fixupInfo.Type == 'D')
        {
            Adr = Val;
            if (IsValidImageAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                if (recN && recN->HasName())
                {
                    if (!recN->SameName(fName)) return false;
                }
            }
        }
    }
    return true;
}
//---------------------------------------------------------------------------
//"Strap" procedure ProcIdx int code from position pos
void __fastcall TFMain_11011981::StrapProc(int pos, int ProcIdx, MProcInfo* ProcInfo, bool useFixups, int procSize)
{
    if (!ProcInfo) return;
    //Citadel!!!
	if (SameText(ProcInfo->ProcName, "CtdReg"))
    {
    	if (procSize == 1) return;
        CtdRegAdr = Pos2Adr(pos);
    }
    DWORD ProcStart = Pos2Adr(pos);
    DWORD ProcEnd = ProcStart + procSize;
//!!!
if(ProcStart == 0x04061A8)
pos = pos;

    String ModuleName = KnowledgeBase.GetModuleName(ProcInfo->ModuleID);
    if (!IsUnitExist(ModuleName))
    {
        //Get unit by pos
        PUnitRec recU = GetUnit(Pos2Adr(pos));
        if (recU)
        {
            SetUnitName(recU, ModuleName);
            recU->kb = true;
        }
    }
    BYTE* p; PInfoRec recN;
    if (ProcInfo->DumpType == 'D')
    {
    	SetFlags(cfData, pos, procSize);
    }
    else
    {
        SetFlags(cfCode, pos, procSize);
        //Mark proc begin
        SetFlag(cfProcStart, pos);
        //SetFlag(cfProcEnd, pos + procSize - 1);

        recN = GetInfoRec(Pos2Adr(pos));
        if (!recN) recN = new InfoRec(pos, ikRefine);
        //Mark proc end
        recN->procInfo->procSize = procSize;

        switch (ProcInfo->MethodKind)
        {
        case 'C':
            recN->kind = ikConstructor;
            break;
        case 'D':
            recN->kind = ikDestructor;
            break;
        case 'F':
            recN->kind = ikFunc;
            recN->type = ProcInfo->TypeDef;
            break;
        case 'P':
            recN->kind = ikProc;
            break;
        }
        
        recN->kbIdx = ProcIdx;
        recN->SetName(ProcInfo->ProcName);
        //Get Args
        if (!recN->MakeArgsManually())
        {
            BYTE callKind = ProcInfo->CallKind;
            recN->procInfo->flags |= callKind;

            int aa = 0, ss = 8;
            ARGINFO argInfo;
            p = ProcInfo->Args;
            if (p)
            {
                for (int k = 0; k < ProcInfo->ArgsNum; k++)
                {
                    argInfo.Tag = *p; p++;
                    int locflags = *((int*)p); p += 4;

                    if ((locflags & 7) == 1) argInfo.Tag = 0x23;  //Add by ZGL
                    
                    argInfo.Register = (locflags & 8);
                    //Ndx
                    int ndx = *((int*)p); p += 4;

                    argInfo.Size = 4;
                    WORD wlen = *((WORD*)p); p += 2;
                    argInfo.Name = String((char*)p, wlen); p += wlen + 1;
                    wlen = *((WORD*)p); p += 2;
                    argInfo.TypeDef = TrimTypeName(String((char*)p, wlen)); p += wlen + 1;
                    //Some correction of knowledge base
                    if (SameText(argInfo.Name, "Message") && SameText(argInfo.TypeDef, "void"))
                    {
                        argInfo.Name = "Msg";
                        argInfo.TypeDef = "TMessage";
                    }

                    if (SameText(argInfo.TypeDef, "String")) argInfo.TypeDef = "AnsiString";
                    if (SameText(argInfo.TypeDef, "Int64")    ||
                        SameText(argInfo.TypeDef, "Real")     ||
                        SameText(argInfo.TypeDef, "Real48")   ||
                        SameText(argInfo.TypeDef, "Comp")     ||
                        SameText(argInfo.TypeDef, "Double")   ||
                        SameText(argInfo.TypeDef, "Currency") ||
                        SameText(argInfo.TypeDef, "TDateTime"))
                        argInfo.Size = 8;
                    if (SameText(argInfo.TypeDef, "Extended")) argInfo.Size = 12;

                    if (!callKind)
                    {
                        if (aa < 3 && argInfo.Size == 4)
                        {
                            argInfo.Ndx = aa;
                            aa++;
                        }
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
                    recN->procInfo->AddArg(&argInfo);
                }
            }
        }
        recN->procInfo->flags |= PF_KBPROTO;
    }
    //Fix used procedure
    KnowledgeBase.SetUsedProc(ProcIdx);

    if (useFixups && ProcInfo->FixupNum)
    {
        //Get array of used modules
        int Idx, size;
        WORD *uses = KnowledgeBase.GetModuleUses(ProcInfo->ModuleID);
        //Начало данных по фиксапам
        p = ProcInfo->Dump + 2*ProcInfo->DumpSz;
        FIXUPINFO fixupInfo;

        MConstInfo acInfo;
        MConstInfo *cInfo = &acInfo;
        MTypeInfo  atInfo;
        MTypeInfo *tInfo = &atInfo;
        MVarInfo  avInfo;
        MVarInfo  *vInfo = &avInfo;
        MResStrInfo arsInfo;
        MResStrInfo *rsInfo = &arsInfo;
        MProcInfo aInfo; MProcInfo* pInfo = &aInfo;
        DWORD Adr, Adr1, Ofs, Val;
        WORD Len;
        String fName;

        for (int n = 0; n < ProcInfo->FixupNum; n++)
        {
            fixupInfo.Type = *p; p++;
            fixupInfo.Ofs = *((DWORD*)p); p += 4;
            Len = *((WORD*)p); p += 2;
            fixupInfo.Name = p; p += Len + 1;
            fName = String(fixupInfo.Name, Len);
            //Fixupname begins with _Dn_ - skip it
            if (fName.Pos("_Dn_") == 1) continue;
            //Fixupname begins with _NF_ - skip it
            if (fName.Pos("_NF_") == 1) continue;
            //Fixupname is "_DV_" - skip it
            if (SameText(fName, "_DV_")) continue;
            //Fixupname begins with _DV_
            if (fName.Pos("_DV_") == 1)
            {
                char c = fName[5];
                //Fixupname is _DV_number - skip it
                if (c >= '0' && c <= '9') continue;
                //Else transfrom fixupname to normal
                if (fName[Len] == '_')
                    fName = fName.SubString(5, Len - 5);
                else
                    fName = fName.SubString(5, Len - 4);
            }
            if (fName == "" || fName == ".") continue;

            Val = *((DWORD*)(Code + pos + fixupInfo.Ofs));
            //FixupName is the same as ProcName
            if (SameText(fName, ProcInfo->ProcName))
            {
                //!!!
                //Need to use this information:
                //CaseStudio, 405ae4 - call [offs+4*eax] - how to use offs? And offs has cfLoc
                //Val inside procedure - possible jump address for switch (or call)
                if (fixupInfo.Type == 'J')
                {
                    Adr = CodeBase + pos + fixupInfo.Ofs + Val + 4;
                    if (Adr >= ProcStart && Adr < ProcEnd)
                        SetFlag(cfLoc | cfEmbedded, Adr2Pos(Adr));
                }
                continue;
            }
            //Сначала подсчитаем адрес, а потом будем пытаться определять секцию
            if (fixupInfo.Type == 'A' || fixupInfo.Type == 'S' || fixupInfo.Type == '4' || fixupInfo.Type == '8')
            {
                //Смотрим, какая величина стоит в дампе в позиции фиксапа
                Ofs = *((DWORD*)(ProcInfo->Dump + fixupInfo.Ofs));
                Adr   = Val - Ofs;
            }
            else if (fixupInfo.Type == 'J')
            {
                Adr = CodeBase + pos + fixupInfo.Ofs + Val + 4;
            }
            else if (fixupInfo.Type == 'D' || fixupInfo.Type == '6' || fixupInfo.Type == '5')
            {
                Adr = Val;
            }
            else
            {
                ShowMessage("Unknown fixup type:" + String(fixupInfo.Type));
            }

            bool isHInstance = (stricmp(fixupInfo.Name, "HInstance") == 0);
            if (!IsValidImageAdr(Adr))
            {
                //Пока здесь наблюдались лишь одни ThreadVars и TlsLast
                if (!stricmp(fixupInfo.Name, "TlsLast"))
                {
                    LastTls = Val;
                }
                else
                {
                    recN = GetInfoRec(Pos2Adr(pos + fixupInfo.Ofs));
                    if (!recN)
                    {
                        recN = new InfoRec(pos + fixupInfo.Ofs, ikData);
                        recN->SetName(fixupInfo.Name);
                        //Определим тип Var
                        Idx = KnowledgeBase.GetVarIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.VarOffsets[Idx].NamId;
                            if (KnowledgeBase.GetVarInfo(Idx, 0, vInfo))
                            {
                                if (vInfo->Type == 'T')
                                    recN->kind = ikThreadVar;
                                recN->kbIdx = Idx;
                                recN->type = TrimTypeName(vInfo->TypeDef);
                            }
                        }
                    }
                }
                continue;
            }
        
            if (Adr >= ProcStart && Adr < ProcEnd) continue;

            if (isHInstance)
            {
                Adr1 = *((DWORD*)(Code + Adr2Pos(Adr)));
                if (IsValidImageAdr(Adr1))
                    HInstanceVarAdr = Adr1;
                else
                    HInstanceVarAdr = Adr;
            }

            int Sections = KnowledgeBase.GetItemSection(uses, fixupInfo.Name);
            //Адрес в кодовом сегменте вне тела самой функции
            if (IsValidCodeAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                if (!recN)
                {
                    switch (Sections)
                    {
                    case KB_CONST_SECTION:
                        Idx = KnowledgeBase.GetConstIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.ConstOffsets[Idx].NamId;
                            //Если имя начинается на _DV_, значит это VMT
                            if (!memcmp(fixupInfo.Name, "_DV_", 4))
                            {
                                if (KnowledgeBase.GetConstInfo(Idx, INFO_DUMP, cInfo))
                                    StrapVMT(Adr2Pos(Adr) + 4, Idx, cInfo);
                            }
                            else
                            {
                            }
                        }
                        break;
                    case KB_TYPE_SECTION:
                        Idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.TypeOffsets[Idx].NamId;
                            if (KnowledgeBase.GetTypeInfo(Idx, 0, tInfo))
                            {
                                recN = new InfoRec(Adr2Pos(Adr), ikData);
                                recN->kbIdx = Idx;
                                recN->SetName(tInfo->TypeName);
                            }
                        }
                        break;
                    case KB_VAR_SECTION:
                        Idx = KnowledgeBase.GetVarIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.VarOffsets[Idx].NamId;
                            if (KnowledgeBase.GetVarInfo(Idx, 0, vInfo))
                            {
                                recN = new InfoRec(Adr2Pos(Adr), ikData);
                                recN->kbIdx = Idx;
                                recN->SetName(vInfo->VarName);
                                recN->type = TrimTypeName(vInfo->TypeDef);
                            }
                        }
                        break;
                    case KB_RESSTR_SECTION:
                        Idx = KnowledgeBase.GetResStrIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.ResStrOffsets[Idx].NamId;
                            if (KnowledgeBase.GetResStrInfo(Idx, 0, rsInfo))
                            {
                                recN = new InfoRec(Adr2Pos(Adr), ikData);
                                recN->kbIdx = Idx;
                                recN->SetName(rsInfo->ResStrName);
                                recN->type = rsInfo->TypeDef;
                            }
                        }
                        break;
                    case KB_PROC_SECTION:
                        Idx = KnowledgeBase.GetProcIdx(uses, fixupInfo.Name, Code + Adr2Pos(Adr));
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                            if (!KnowledgeBase.IsUsedProc(Idx))
                            {
                                if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                    StrapProc(Adr2Pos(Adr), Idx, pInfo, true, pInfo->DumpSz);
                            }
                        }
                        else
                        {
                            Idx = KnowledgeBase.GetProcIdx(uses, fixupInfo.Name, 0);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                if (!KnowledgeBase.IsUsedProc(Idx))
                                {
                                    if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                    {
                                        if (!SameText(fName, "@Halt"))
                                        {
                                            StrapProc(Adr2Pos(Adr), Idx, pInfo, false, EstimateProcSize(Adr));
                                        }
                                        else
                                        {
                                            DISINFO _disInfo;
                                            int _bytes = EstimateProcSize(Adr);
                                            while (_bytes > 0)
                                            {
                                                int _instrlen = Disasm.Disassemble(Code + Adr2Pos(Adr), (__int64)Adr, &_disInfo, 0);
                                                if (_disInfo.Branch && !_disInfo.Conditional)
                                                {
                                                    Adr = _disInfo.Immediate;
                                                    Idx = KnowledgeBase.GetProcIdx(uses, "@Halt0", 0);
                                                    if (Idx != -1)
                                                    {
                                                        Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                                        if (!KnowledgeBase.IsUsedProc(Idx))
                                                        {
                                                            if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                                                StrapProc(Adr2Pos(Adr), Idx, pInfo, false, EstimateProcSize(Adr));
                                                        }
                                                    }
                                                    break;
                                                }
                                                _bytes -= _instrlen;
                                            }
                                        }
                                    }
                                }
                            }

                        }
                        break;
                    }
                    continue;
                }
            }
            //Адрес в секции DATA
            if (IsValidImageAdr(Adr))
            {
                int _pos = Adr2Pos(Adr);
                if (_pos >= 0)
                {
                    recN = GetInfoRec(Adr);
                    if (!recN)
                    {
                        switch (Sections)
                        {
                        case KB_CONST_SECTION:
                            Idx = KnowledgeBase.GetConstIdx(uses, fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ConstOffsets[Idx].NamId;
                                if (KnowledgeBase.GetConstInfo(Idx, INFO_DUMP, cInfo))
                                {
                                    String cname = "";
                                    if (cInfo->ConstName.Pos("_DV_") == 1)
                                    {
                                        char c = cInfo->ConstName[5];
                                        if (c > '9')
                                        {
                                            if (cInfo->ConstName[Len] == '_')
                                                cname = cInfo->ConstName.SubString(5, Len - 5);
                                            else
                                                cname = cInfo->ConstName.SubString(5, Len - 4);
                                        }
                                    }
                                    else
                                        cname = cInfo->ConstName;

                                    recN = new InfoRec(_pos, ikData);
                                    recN->kbIdx = Idx;
                                    recN->SetName(cname);
                                    recN->type = cInfo->TypeDef;
                                }
                            }
                            break;
                        case KB_TYPE_SECTION:
                            Idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.TypeOffsets[Idx].NamId;
                                if (KnowledgeBase.GetTypeInfo(Idx, 0, tInfo))
                                {
                                    recN = new InfoRec(_pos, ikData);
                                    recN->kbIdx = Idx;
                                    recN->SetName(tInfo->TypeName);
                                }
                            }
                            break;
                        case KB_VAR_SECTION:
                            Idx = KnowledgeBase.GetVarIdx(uses, fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.VarOffsets[Idx].NamId;
                                if (KnowledgeBase.GetVarInfo(Idx, 0, vInfo))
                                {
                                    recN = new InfoRec(_pos, ikData);
                                    recN->kbIdx = Idx;
                                    recN->SetName(vInfo->VarName);
                                    recN->type = TrimTypeName(vInfo->TypeDef);
                                }
                            }
                            break;
                        case KB_RESSTR_SECTION:
                            Idx = KnowledgeBase.GetResStrIdx(uses, fixupInfo.Name);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ResStrOffsets[Idx].NamId;
                                if (KnowledgeBase.GetResStrInfo(Idx, 0, rsInfo))
                                {
                                    recN = new InfoRec(_pos, ikData);
                                    recN->kbIdx = Idx;
                                    recN->SetName(rsInfo->ResStrName);
                                    recN->type = rsInfo->TypeDef;
                                }
                            }
                            break;
                        }
                    }
                }
                else
                {
                    switch (Sections)
                    {
                    case KB_CONST_SECTION:
                        Idx = KnowledgeBase.GetConstIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.ConstOffsets[Idx].NamId;
                            if (KnowledgeBase.GetConstInfo(Idx, INFO_DUMP, cInfo))
                            {
                                String cname = "";
                                if (cInfo->ConstName.Pos("_DV_") == 1)
                                {
                                    char c = cInfo->ConstName[5];
                                    if (c > '9')
                                    {
                                        if (cInfo->ConstName[Len] == '_')
                                            cname = cInfo->ConstName.SubString(5, Len - 5);
                                        else
                                            cname = cInfo->ConstName.SubString(5, Len - 4);
                                    }
                                }
                                else
                                    cname = cInfo->ConstName;

                                AddToBSSInfos(Adr, cname, cInfo->TypeDef);
                            }
                        }
                        break;
                    case KB_TYPE_SECTION:
                        Idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.TypeOffsets[Idx].NamId;
                            if (KnowledgeBase.GetTypeInfo(Idx, 0, tInfo))
                            {
                                AddToBSSInfos(Adr, tInfo->TypeName, "");
                            }
                        }
                        break;
                    case KB_VAR_SECTION:
                        Idx = KnowledgeBase.GetVarIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.VarOffsets[Idx].NamId;
                            if (KnowledgeBase.GetVarInfo(Idx, 0, vInfo))
                            {
                                AddToBSSInfos(Adr, vInfo->VarName, TrimTypeName(vInfo->TypeDef));
                            }
                        }
                        break;
                    case KB_RESSTR_SECTION:
                        Idx = KnowledgeBase.GetResStrIdx(uses, fixupInfo.Name);
                        if (Idx != -1)
                        {
                            Idx = KnowledgeBase.ResStrOffsets[Idx].NamId;
                            if (KnowledgeBase.GetResStrInfo(Idx, 0, rsInfo))
                            {
                                AddToBSSInfos(Adr, rsInfo->ResStrName, rsInfo->TypeDef);
                            }
                        }
                        break;
                    }
                }
            }
        }
        if (uses) delete[] uses;
    }
}
//---------------------------------------------------------------------------
int DelphiVersions[] = {2, 3, 4, 5, 6, 7, 2005, 2006, 2007, 2009, 2010, 2011, 2012, 2013, 2014, 0};

int __fastcall TFMain_11011981::GetDelphiVersion()
{
    WORD            moduleID;
    int             idx, num, pos, version;
    DWORD           unitsTab, iniAdr, finAdr, vmtAdr, adr;
    DWORD           TControlInstSize;
    PInfoRec        recN;
    String          name, KBFileName;
    MKnowledgeBase	SysKB;
    MProcInfo       aInfo;
    MProcInfo*      pInfo = &aInfo;

    if (IsExtendedInitTab(&unitsTab))//>=2010
    {
        KBFileName = AppDir + "syskb2014.bin";
        if (SysKB.Open(KBFileName.c_str()))
        {
            moduleID = SysKB.GetModuleID("System");
            if (moduleID != 0xFFFF)
            {
                //Find index of function "StringCopy" in this module
                idx = SysKB.GetProcIdx(moduleID, "StringCopy");
                if (idx != -1)
                {
                    pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                    pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        SysKB.Close();
                        return 2014;
                    }
                }
            }
            SysKB.Close();
        }
        KBFileName = AppDir + "syskb2013.bin";
        if (SysKB.Open(KBFileName.c_str()))
        {
            moduleID = SysKB.GetModuleID("System");
            if (moduleID != 0xFFFF)
            {
                //Find index of function "@FinalizeResStrings" in this module
                idx = SysKB.GetProcIdx(moduleID, "@FinalizeResStrings");
                if (idx != -1)
                {
                    pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                    pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        SysKB.Close();
                        return 2013;
                    }
                }
            }
            SysKB.Close();
        }

        KBFileName = AppDir + "syskb2012.bin";
        if (SysKB.Open(KBFileName.c_str()))
        {
            moduleID = SysKB.GetModuleID("System");
            if (moduleID != 0xFFFF)
            {
                //Find index of function "@InitializeControlWord" in this module
                idx = SysKB.GetProcIdx(moduleID, "@InitializeControlWord");
                if (idx != -1)
                {
                    pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                    pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        SysKB.Close();
                        return 2012;
                    }
                }
            }
            SysKB.Close();
        }

        KBFileName = AppDir + "syskb2011.bin";
        if (SysKB.Open(KBFileName.c_str()))
        {
            moduleID = SysKB.GetModuleID("System");
            if (moduleID != 0xFFFF)
            {
                //Find index of function "ErrorAt" in this module
                idx = SysKB.GetProcIdx(moduleID, "ErrorAt");
                if (idx != -1)
                {
                    pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                    pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        SysKB.Close();
                        return 2011;
                    }
                }
            }
            SysKB.Close();
        }
        return 2010;
    }

	TControlInstSize = 0;
	//Пробуем для начала как в DeDe (Ищем тип TControl)
    for (int n = 0; n < TotalSize - 14; n += 4)
    {
    	if (Image[n] == 7 && Image[n + 1] == 8 &&
        	Image[n + 2] == 'T' && Image[n + 3] == 'C' &&
            Image[n + 4] == 'o' && Image[n + 5] == 'n' &&
            Image[n + 6] == 't' && Image[n + 7] == 'r' &&
            Image[n + 8] == 'o' && Image[n + 9] == 'l')
        {
        	//После типа должен следовать указатель на таблицу VMT (0)
            vmtAdr = *((DWORD*)(Image + n + 10));
            if (IsValidImageAdr(vmtAdr))
            {
            	//Проверяем смещение -0x18
                TControlInstSize = *((DWORD*)(Image + Adr2Pos(vmtAdr) - 0x18));
                if (TControlInstSize == 0xA8) return 2;
            	//Проверяем смещение -0x1C
                TControlInstSize = *((DWORD*)(Image + Adr2Pos(vmtAdr) - 0x1C));
                if (TControlInstSize == 0xB0 || TControlInstSize == 0xB4) return 3;
            	//Проверяем смещение -0x28
                TControlInstSize = *((DWORD*)(Image + Adr2Pos(vmtAdr) - 0x28));
                if (TControlInstSize == 0x114) return 4;
                if (TControlInstSize == 0x120) return 5;
                if (TControlInstSize == 0x15C)	//6 или 7
                {
                    DWORD TFormInstSize = 0;
                    //Ищем тип TForm (выбор между версией 6 и 7)
                    for (int m = 0; m < TotalSize - 11; m += 4)
                    {
                        if (Image[m] == 7 && Image[m + 1] == 5 && Image[m + 2] == 'T' &&
                            Image[m + 3] == 'F' && Image[m + 4] == 'o' &&
                            Image[m + 5] == 'r' && Image[m + 6] == 'm')
                        {
                            //После типа должен следовать указатель на таблицу VMT (0)
                            vmtAdr = *((DWORD*)(Image + m + 7));
                            if (IsValidImageAdr(vmtAdr))
                            {
                                //Проверяем смещение -0x28
                                TFormInstSize = *((DWORD*)(Image + Adr2Pos(vmtAdr) - 0x28));
                                if (TFormInstSize == 0x2F0) return 6;
                                if (TFormInstSize == 0x2F8) return 7;
                            }
                        }
                    }
                    break;
                }
                if (TControlInstSize == 0x164) return 2005;
                if (TControlInstSize == 0x190) break;	//2006 или 2007
                //Проверяем смещение -0x34
                TControlInstSize = *((DWORD*)(Image + Adr2Pos(vmtAdr) - 0x34));
                if (TControlInstSize == 0x1A4) return 2009;
                //if (TControlInstSize == 0x1AC) return 2010;
            }
        }
    }
    //Оставшиеся варианты проверяем через базу знаний
    for (int v = 0; DelphiVersions[v]; v++)
    {
        version = DelphiVersions[v];
        if (TControlInstSize == 0x190 && version != 2006 && version != 2007) continue;
        KBFileName = AppDir + "syskb" + version + ".bin";
        if (SysKB.Open(KBFileName.c_str()))
        {
            moduleID = SysKB.GetModuleID("System");
            if (moduleID != 0xFFFF)
            {
                //Ищем индекс функции "System" в данном модуле
                idx = SysKB.GetProcIdx(moduleID, "System");
                if (idx != -1)
                {
                    pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                    pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        if (version == 4 || version == 5)
                        {
                            idx = SysKB.GetProcIdx(moduleID, "@HandleAnyException");
                            if (idx != -1)
                            {
                                pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                                pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                                if (pos != -1)
                                {
                                    SysKB.Close();
                                    return version;
                                }
                            }
                        }
                        else if (version == 2006 || version == 2007)
                        {
                            idx = SysKB.GetProcIdx(moduleID, "GetObjectClass");
                            if (idx != -1)
                            {
                                pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                                pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                                if (pos != -1)
                                {
                                    SysKB.Close();
                                    return version;
                                }
                            }
                        }
                        else if (version == 2009 || version == 2010)
                        {
                            //Ищем индекс функции "@Halt0" в данном модуле
                            idx = SysKB.GetProcIdx(moduleID, "@Halt0");
                            if (idx != -1)
                            {
                                pInfo = SysKB.GetProcInfo(SysKB.ProcOffsets[idx].NamId, INFO_DUMP, &aInfo);
                                pos = SysKB.ScanCode(Code, Flags, CodeSize, pInfo);
                                if (pos != -1)
                                {
                                    SysKB.Close();
                                    return version;
                                }
                            }
                        }
                        else
                        {
                            SysKB.Close();
                            return version;
                        }
                    }
                }
            }
            SysKB.Close();
        }
    }
    //Analyze VMTs (if exists)
    version = -1;
    for (int n = 0; n < CodeSize; n += 4)
    {
        vmtAdr = *((DWORD*)(Code + n));  //Points to vmt0 (VmtSelfPtr)
        //VmtSelfPtr
        if (IsValidCodeAdr(vmtAdr))
        {
            if (Pos2Adr(n) == vmtAdr - 0x34)
            {
                //VmtInitTable
                adr = *((DWORD*)(Code + n + 4));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtTypeInfo
                adr = *((DWORD*)(Code + n + 8));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtFieldTable
                adr = *((DWORD*)(Code + n + 12));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtMethodTable
                adr = *((DWORD*)(Code + n + 16));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtDynamicTable
                adr = *((DWORD*)(Code + n + 20));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtClassName
                adr = *((DWORD*)(Code + n + 24));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtInstanceSize
                adr = *((DWORD*)(Code + n + 28));
                if (!adr || IsValidCodeAdr(adr)) continue;
                version = Pos2Adr(n) - vmtAdr;
                break;
            }
            else if (Pos2Adr(n) == vmtAdr - 0x40 || Pos2Adr(n) == vmtAdr - 0x4C || Pos2Adr(n) == vmtAdr - 0x58)
            {
                //VmtIntfTable
                adr = *((DWORD*)(Code + n + 4));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtAutoTable
                adr = *((DWORD*)(Code + n + 8));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtInitTable
                adr = *((DWORD*)(Code + n + 12));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtTypeInfo
                adr = *((DWORD*)(Code + n + 16));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtFieldTable
                adr = *((DWORD*)(Code + n + 20));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtMethodTable
                adr = *((DWORD*)(Code + n + 24));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtDynamicTable
                adr = *((DWORD*)(Code + n + 28));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtClassName
                adr = *((DWORD*)(Code + n + 32));
                if (adr && !IsValidCodeAdr(adr)) continue;
                //VmtInstanceSize
                adr = *((DWORD*)(Code + n + 36));
                if (!adr || IsValidCodeAdr(adr)) continue;
                version = Pos2Adr(n) - vmtAdr;
                break;
            }
        }
    }
    if (version == -0x34) return 2;
    if (version == -0x40) return 3;
    if (version == -0x58) return 2009;
    /*
    //Check that system is in external rtl
    num = *((DWORD*)(Code + unitsTab - 8));
    for (int n = 0; n < num; n++, unitsTab += 8)
    {
        iniAdr = *((DWORD*)(Code + unitsTab));
        if (iniAdr && IsValidImageAdr(iniAdr) && IsFlagSet(cfImport, Adr2Pos(iniAdr)))
        {
            recN = GetInfoRec(iniAdr);
            pos = recN->name.Pos(".");
            if (pos && SameText(recN->name.SubString(pos + 1, 22), "@System@Initialization"))
            {
                name = recN->name.SubString(1, pos - 1);
                if (SameText(name, "rtl120")) return 2009;
            }
        }
        finAdr = *((DWORD*)(Code + unitsTab + 4));
        if (finAdr && IsValidImageAdr(finAdr) && IsFlagSet(cfImport, Adr2Pos(finAdr))))
        {
            recN = GetInfoRec(finAdr);
            pos = recN->name.Pos(".");
            if (pos)
            {
                name = recN->name.SubString(1, pos - 1);
            }
        }
    }
    */
    //as
    //try to find the Delphi version based on imported delphi bpls.... (if any)
    typedef struct
    {
        char* versionVcl;     //digital part of version present in vclXXX file name
        char* versionRtl;     //digital part of version present in rtlXXX file name
        char* versionString;  //full string of vcl/rtl bpl from file properties
        int   versionForIdr;  //value that returns to caller
    } DelphiBplVersionRec;

    const DelphiBplVersionRec delphiBplVer[] = {//Borland
        {"vcl30.dpl",  "",          "3.0.5.53"      ,  3   }, //no rtl30.dpl!
        {"vcl40.bpl",  "",          "4.0.5.38"      ,  4   }, //no rtl40.bpl!
        {"vcl60.bpl",  "rtl60.bpl", "6.0.6.240"     ,  6   },
        {"vcl60.bpl",  "rtl60.bpl", "6.0.6.243"     ,  6   },
        {"vcl70.bpl",  "rtl70.bpl", "7.0.4.453"     ,  7   },
        {"vcl90.bpl",  "rtl90.bpl", "9.0.1761.24408",  2005},
        {"vcl100.bpl", "rtl100.bpl","10.0.2151.25345", 2006}, //SP2
        {"vcl100.bpl", "rtl100.bpl","11.0.2627.5503",  2007}, //CodeGear
        {"vcl100.bpl", "rtl100.bpl","11.0.2902.10471", 2007},
        {"vcl120.bpl", "rtl120.bpl","12.0.3210.17555", 2009}, //Embarcadero
        {"", "", "", 0}
    };
    
    String rtlBplName, vclBplName, rtlBpl, vclBpl;

    for (int n = 0; n < ImpModuleList->Count; n++)
    {
        if (ImpModuleList->Strings[n].SubString(1,3).LowerCase() == "rtl")
            rtlBpl = ImpModuleList->Strings[n].LowerCase();
        else if (ImpModuleList->Strings[n].SubString(1,3).LowerCase() == "vcl")
            vclBpl = ImpModuleList->Strings[n].LowerCase();

        if (rtlBpl != "" && vclBpl != "") break;
    }

    // or (||) is here because Delphi3 and 4 do not have rtl...
    if (rtlBpl != "" || vclBpl != "")
    {
        for (int i = 0; delphiBplVer[i].versionForIdr; ++i)
        {
            rtlBplName = String(delphiBplVer[i].versionRtl);
            vclBplName = String(delphiBplVer[i].versionVcl);

            if (vclBplName != vclBpl) continue;

            //if we found something - lets continue the analysis
            //here we have two cases
            // (i) bpl are near the input Delphi target or
            // (ii) it was installed into system dir...(or any dir in %PATH% !)

            String srcPath = ExtractFilePath(SourceFile);
            String rtlFile = !rtlBplName.IsEmpty() ? srcPath+rtlBplName : rtlBplName;
            String vclFile = srcPath+vclBplName;

            if (!FileExists(vclFile))
            {
                //we'll not look into windows\\system32 only...
                //instead we leave path as no path -> system will scan for us
                //in all the %PATH% directories (when doing LoadLibrary)!
                rtlFile = rtlBplName;
                vclFile = vclBplName;
            }

            //check the export of bpl - it must have 2 predefined functions
            if (!((rtlFile == "" || IsBplByExport(rtlFile.c_str())) && IsBplByExport(vclFile.c_str())))
            {
                ShowMessage("Input file imports Delphi system packages ("
                    + rtlFile + ", " + vclFile + ")."
                    "\r\nIn order to figure out the version, please put it nearby");
                break;
            }

            String rtlBplVersion = GetModuleVersion(rtlFile);
            String vclBplVersion = GetModuleVersion(vclFile);
            //hack for D3 and 4
            if (rtlBplVersion == "") rtlBplVersion = vclBplVersion;
            if (rtlBplVersion == vclBplVersion && rtlBplVersion == String(delphiBplVer[i].versionString))
                return delphiBplVer[i].versionForIdr;
        }
    }
    if (version == -0x4C) return 1;
    //here we failed to find the version.....sorry
    return -1;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::InitSysProcs()
{
    int         Idx, pos;
    MProcInfo   aInfo, *pInfo;

    SysProcsNum = 0;
    WORD moduleID = KnowledgeBase.GetModuleID("System");
    for (int n = 0; SysProcs[n].name; n++)
    {
        Idx = KnowledgeBase.GetProcIdx(moduleID, SysProcs[n].name);
        if (Idx != -1)
        {
            Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
            if (!KnowledgeBase.IsUsedProc(Idx))
            {
                pInfo = KnowledgeBase.GetProcInfo(Idx, INFO_DUMP, &aInfo);
                if (SysProcs[n].impAdr)
                {
                    StrapProc(Adr2Pos(SysProcs[n].impAdr), Idx, pInfo, false, 6);
                }
                else
                {
                    pos = KnowledgeBase.ScanCode(Code, Flags, CodeSize, pInfo);
                    if (pos != -1)
                    {
                        PInfoRec recN = new InfoRec(pos, ikRefine);
                        recN->SetName(SysProcs[n].name);
                        StrapProc(pos, Idx, pInfo, true, pInfo->DumpSz);
                    }
                }
            }
        }
        SysProcsNum++;
    }
    moduleID = KnowledgeBase.GetModuleID("SysInit");
    for (int n = 0; SysInitProcs[n].name; n++)
    {
        Idx = KnowledgeBase.GetProcIdx(moduleID, SysInitProcs[n].name);
        if (Idx != -1)
        {
            Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
            if (!KnowledgeBase.IsUsedProc(Idx))
            {
                pInfo = KnowledgeBase.GetProcInfo(Idx, INFO_DUMP, &aInfo);
                if (SysInitProcs[n].impAdr)
                {
                    StrapProc(Adr2Pos(SysInitProcs[n].impAdr), Idx, pInfo, false, 6);
                }
                pos = KnowledgeBase.ScanCode(Code, Flags, CodeSize, pInfo);
                if (pos != -1)
                {
                    PInfoRec recN = new InfoRec(pos, ikRefine);
                    recN->SetName(SysInitProcs[n].name);
                    StrapProc(pos, Idx, pInfo, true, pInfo->DumpSz);
                    if (n == 1) SourceIsLibrary = true;
                }
            }
        }
        SysProcsNum++;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::SetVmtConsts(int version)
{
    switch (version)
    {
    case 2:
        VmtSelfPtr			 = -0x34;     //??? (=0???)
        VmtInitTable		 = -0x30;
        VmtTypeInfo			 = -0x2C;
        VmtFieldTable		 = -0x28;
        VmtMethodTable		 = -0x24;
        VmtDynamicTable		 = -0x20;
        VmtClassName		 = -0x1C;
        VmtInstanceSize		 = -0x18;
        VmtParent			 = -0x14;
        VmtDefaultHandler	 = -0x10;
        VmtNewInstance		 = -0xC;
        VmtFreeInstance		 = -8;
        VmtDestroy			 = -4;
        break;
    case 3:
        VmtSelfPtr			 = -0x40;
        VmtIntfTable		 = -0x3C;
        VmtAutoTable		 = -0x38;
        VmtInitTable		 = -0x34;
        VmtTypeInfo			 = -0x30;
        VmtFieldTable		 = -0x2C;
        VmtMethodTable		 = -0x28;
        VmtDynamicTable		 = -0x24;
        VmtClassName		 = -0x20;
        VmtInstanceSize		 = -0x1C;
        VmtParent			 = -0x18;
        VmtSafeCallException = -0x14;
        VmtDefaultHandler	 = -0x10;
        VmtNewInstance		 = -0xC;
        VmtFreeInstance		 = -8;
        VmtDestroy			 = -4;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 2005:
    case 2006:
    case 2007:
        VmtSelfPtr			 = -0x4C;
        VmtIntfTable 		 = -0x48;
        VmtAutoTable 		 = -0x44;
        VmtInitTable 		 = -0x40;
        VmtTypeInfo 		 = -0x3C;
        VmtFieldTable 		 = -0x38;
        VmtMethodTable 		 = -0x34;
        VmtDynamicTable 	 = -0x30;
        VmtClassName 		 = -0x2C;
        VmtInstanceSize 	 = -0x28;
        VmtParent 			 = -0x24;
        VmtSafeCallException = -0x20;
        VmtAfterConstruction = -0x1C;
        VmtBeforeDestruction = -0x18;
        VmtDispatch 		 = -0x14;
        VmtDefaultHandler 	 = -0x10;
        VmtNewInstance 		 = -0xC;
        VmtFreeInstance 	 = -8;
        VmtDestroy 			 = -4;
        break;
	case 2009:
    case 2010:
        VmtSelfPtr           = -0x58;
        VmtIntfTable         = -0x54;
        VmtAutoTable         = -0x50;
        VmtInitTable         = -0x4C;
        VmtTypeInfo          = -0x48;
        VmtFieldTable        = -0x44;
        VmtMethodTable       = -0x40;
        VmtDynamicTable      = -0x3C;
        VmtClassName         = -0x38;
        VmtInstanceSize      = -0x34;
        VmtParent            = -0x30;
        VmtEquals            = -0x2C;
        VmtGetHashCode       = -0x28;
        VmtToString          = -0x24;
        VmtSafeCallException = -0x20;
        VmtAfterConstruction = -0x1C;
        VmtBeforeDestruction = -0x18;
        VmtDispatch          = -0x14;
        VmtDefaultHandler    = -0x10;
        VmtNewInstance       = -0xC;
        VmtFreeInstance      = -8;
        VmtDestroy           = -4;
        break;
    case 2011:
    case 2012:
    case 2013:
    case 2014:
        VmtSelfPtr           = -0x58;
        VmtIntfTable         = -0x54;
        VmtAutoTable         = -0x50;
        VmtInitTable         = -0x4C;
        VmtTypeInfo          = -0x48;
        VmtFieldTable        = -0x44;
        VmtMethodTable       = -0x40;
        VmtDynamicTable      = -0x3C;
        VmtClassName         = -0x38;
        VmtInstanceSize      = -0x34;
        VmtParent            = -0x30;
        VmtEquals            = -0x2C;
        VmtGetHashCode       = -0x28;
        VmtToString          = -0x24;
        VmtSafeCallException = -0x20;
        VmtAfterConstruction = -0x1C;
        VmtBeforeDestruction = -0x18;
        VmtDispatch          = -0x14;
        VmtDefaultHandler    = -0x10;
        VmtNewInstance       = -0xC;
        VmtFreeInstance      = -8;
        VmtDestroy           = -4;
        //VmtQueryInterface    = 0;
        //VmtAddRef            = 4;
        //VmtRelease           = 8;
        //VmtCreateObject      = 0xC;
        break;
    }
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::IsUnitExist(String Name)
{
    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        if (recU->names->IndexOf(Name) != -1) return true;
    }
    return false;
}
//---------------------------------------------------------------------------
PUnitRec __fastcall TFMain_11011981::GetUnit(DWORD Adr)
{
    PUnitRec    _res;

    CrtSection->Enter();
    _res = 0;
    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        if (Adr >= recU->fromAdr && Adr < recU->toAdr) _res = recU;
    }
    CrtSection->Leave();
    return _res;
}
//---------------------------------------------------------------------------
String __fastcall TFMain_11011981::GetUnitName(PUnitRec recU)
{
    if (recU)
    {
        if (recU->names->Count == 1)
            return recU->names->Strings[0];
        else
            return "_Unit" + String(recU->iniOrder);
    }
    return "";
}
//---------------------------------------------------------------------------
String __fastcall TFMain_11011981::GetUnitName(DWORD Adr)
{
    int     n;
    String  Result = "";

    PUnitRec recU = GetUnit(Adr);
    if (recU)
    {
        for (n = 0; n < recU->names->Count; n++)
        {
            if (n) Result += ", ";
            Result += recU->names->Strings[n];
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::SetUnitName(PUnitRec recU, String name)
{
    if (recU && recU->names->IndexOf(name) == -1)
        recU->names->Add(name);
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::InOneUnit(DWORD Adr1, DWORD Adr2)
{
    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        if (Adr1 >= recU->fromAdr && Adr1 < recU->toAdr &&
            Adr2 >= recU->fromAdr && Adr2 < recU->toAdr) return true;
    }
    return false;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::GetSegmentNo(DWORD Adr)
{
    for (int n = 0; n < SegmentList->Count; n++)
    {
        PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
        if (segInfo->Start <= Adr && Adr < segInfo->Start + segInfo->Size) return n;
    }
    return -1;
}
//---------------------------------------------------------------------------
DWORD __fastcall FollowInstructions(DWORD fromAdr, DWORD toAdr)
{
    int         instrLen;
    int         fromPos = Adr2Pos(fromAdr);
    int         curPos = fromPos;
    DWORD       curAdr = fromAdr;
    DISINFO     DisInfo;
    
    while (1)
    {
        if (curAdr >= toAdr) break;
        if (IsFlagSet(cfInstruction, curPos)) break;
        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        if (!instrLen) break;
        SetFlag(cfInstruction, curPos);
    }
    return curAdr;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::EstimateProcSize(DWORD fromAdr)
{
    BYTE        op;
    int         row, num, instrLen, instrLen1, instrLen2, Pos;
    int         fromPos = Adr2Pos(fromAdr);
    int         curPos = fromPos;
    DWORD       curAdr = fromAdr;
    DWORD       lastAdr = 0;
    DWORD       Adr, Adr1, lastMovAdr = 0;
    PInfoRec    recN;
    DISINFO     DisInfo;

    int outRows = MAX_DISASSEMBLE;
    if (IsFlagSet(cfImport, fromPos)) outRows = 1;

    for (row = 0; row < outRows; row++)
    {
        //Exception table
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            curPos += 4 + 8*num; curAdr += 4 + 8*num;
            continue;
        }

        BYTE b1 = Code[curPos];
        BYTE b2 = Code[curPos + 1];
        if (!b1 && !b2 && !lastAdr) break;

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) break;
        if (!instrLen)
        {
            curPos++; curAdr++;
            continue;
        }
        //Code
        SetFlags(cfCode, curPos, instrLen);
        //Instruction begin
        SetFlag(cfInstruction, curPos);

        op = Disasm.GetOp(DisInfo.Mnem);

        if (curAdr >= lastAdr) lastAdr = 0;

        if (op == OP_JMP)
        {
            if (curAdr == fromAdr)
            {
                curAdr += instrLen;
                break;
            }
            if (DisInfo.OpType[0] == otMEM)
            {
                if (Adr2Pos(DisInfo.Offset) < 0 && (!lastAdr || curAdr == lastAdr))
                {
                    curAdr += instrLen;
                    break;
                }
            }
            if (DisInfo.OpType[0] == otIMM)
            {
                Adr = DisInfo.Immediate;
                if (Adr2Pos(Adr) < 0 && (!lastAdr || curAdr == lastAdr))
                {
                    curAdr += instrLen;
                    break;
                }
                if (GetSegmentNo(Adr) != 0 && GetSegmentNo(fromAdr) != GetSegmentNo(Adr) && (!lastAdr || curAdr == lastAdr))
                {
                    curAdr += instrLen;
                    break;
                }
                if (Adr < fromAdr && (!lastAdr || curAdr == lastAdr))
                {
                    curAdr += instrLen;
                    break;
                }
            }
        }
        //End of procedure
        if (DisInfo.Ret)// && (!lastAdr || curAdr == lastAdr))
        {
            if (!IsFlagSet(cfLoc, Pos + instrLen))
            {
                curAdr += instrLen;
                break;
            }
        }

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset))
            {
            	curAdr += instrLen;
                break;
            }
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Адрес таблицы - последние 4 байта инструкции
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Анализируем промежуток на предмет таблицы cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //Если есть cTblAdr, пропускаем эту таблицу
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                Pos += CNum; Adr += CNum;
            }
            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidImageAdr(Adr1) || Adr1 < fromAdr) break;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }

        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    if (Pos >= 0)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                            //if (!instrLen1) return Size;

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    /*
                                    //@2
                                    Adr1 = DisInfo.Immediate - 4;
                                    Adr = *((DWORD*)(Code + Adr2Pos(Adr1)));
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    */
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Set cfETable to output data correctly
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                }
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }

        if (b1 == 0xEB ||				 //short relative abs jmp or cond jmp
        	(b1 >= 0x70 && b1 <= 0x7F) ||
            (b1 == 0xF && b2 >= 0x80 && b2 <= 0x8F))
        {
            Adr = DisInfo.Immediate;
            if (IsValidImageAdr(Adr))
            {
                SetFlag(cfLoc, Adr2Pos(Adr));
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (IsValidImageAdr(Adr))
            {
                SetFlag(cfLoc, Adr2Pos(Adr));
                recN = GetInfoRec(Adr);
                if (!recN && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (DisInfo.Call)
        {
            Adr = DisInfo.Immediate; Pos = Adr2Pos(Adr);
            if (IsValidImageAdr(Adr))
            {
                if (Pos >= 0)
                {
                    SetFlag(cfLoc, Pos);
                    recN = GetInfoRec(Adr);
                    if (recN && recN->SameName("@Halt0"))
                    {
                        if (fromAdr == EP && !lastAdr) break;
                    }
                }
                /*
                //Call is inside procedure (may be embedded or filled by data)
                if (lastAdr && Adr > fromAdr && Adr <= lastAdr)
                {
                    if (!IsFlagSet(cfInstruction, Pos))
                    {
                        curAdr = Adr;
                        curPos = Pos;
                    }
                    else
                    {
                        curPos += instrLen;
                        curAdr += instrLen;
                    }
                }
                */
            }
        }
        curPos += instrLen; curAdr += instrLen;
    }
    return curAdr - fromAdr;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::GetUnits2(String dprName)
{
    int         instrLen, iniProcSize, pos, num, start, n;
    DWORD       iniAdr;
    DWORD       curAdr = EP;
    DWORD       curPos = Adr2Pos(curAdr);
    PUnitRec    recU;
    DISINFO     DisInfo;

    //Ищем первую инструкцию call = @InitExe
    n = 0;
    while (1)
    {
        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) return num;
        if (!instrLen)
        {
            curPos++; curAdr++;
            continue;
        }
        if (DisInfo.Call) n++;
        if (n == 2) break;
        curPos += instrLen;
        curAdr += instrLen;
    }
    //Читаем список вызовов (процедуры инициализации)
    int no = 1;
    for (num = 0;; num++)
    {
        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) return num;
        if (!instrLen)
        {
            curPos++; curAdr++;
            continue;
        }
        if (!DisInfo.Call) break;
        iniAdr = DisInfo.Immediate;
        iniProcSize = EstimateProcSize(iniAdr);

        recU = new UnitRec;
        recU->trivial = false;
        recU->trivialIni = false;
        recU->trivialFin = true;
        recU->kb = false;
        recU->names = new TStringList;

        recU->fromAdr = 0;
        recU->toAdr = 0;
        recU->matchedPercent = 0.0;
        recU->iniOrder = no; no++;

        recU->toAdr = (iniAdr + iniProcSize + 3) & 0xFFFFFFFC;

        recU->finadr = 0;
        recU->finSize = 0;
        recU->iniadr = iniAdr;
        recU->iniSize = iniProcSize;

        pos = Adr2Pos(iniAdr);
        SetFlag(cfProcStart, pos);
        PInfoRec recN = GetInfoRec(iniAdr);
        if (!recN) recN = new InfoRec(pos, ikProc);
        recN->procInfo->procSize = iniProcSize;

        Units->Add((void*)recU);

        curPos += instrLen;
        curAdr += instrLen;
    }
    
    start = CodeBase;
    num = Units->Count;
	for (int i = 0; i < num; i++)
    {
    	recU = (PUnitRec)Units->Items[i];
        recU->fromAdr = start;
        start = recU->toAdr;
        //Last Unit has program name
        if (i == num - 1)
        {
            recU->toAdr = CodeBase + CodeSize;
            SetUnitName(recU, dprName);
        }
    }

    return num;
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::IsExtendedInitTab(DWORD* unitsTab)
{
    int     i, num, pos, n;
    DWORD   adr, initTable, iniAdr, finAdr;

    *unitsTab = 0;
    for (i = 0; i < ((TotalSize - 4) & (-4)); i += 4)
    {
        initTable = *((DWORD*)(Image + i));
        if (initTable == CodeBase + i + 4)
        {
            num = *((DWORD*)(Image + i - 4));
            if (num <= 0 || num > 10000) continue;

        	pos = *unitsTab = i + 4;
            for (n = 0; n < num; n++, pos += 8)
            {
                iniAdr = *((DWORD*)(Image + pos));
                if (iniAdr)
                {
                    if (!IsValidImageAdr(iniAdr))
                    {
                        *unitsTab = 0;
                        break;
                    }
                }
                finAdr = *((DWORD*)(Image + pos + 4));
                if (finAdr)
                {
                    if (!IsValidImageAdr(finAdr))
                    {
                        *unitsTab = 0;
                        break;
                    }
                }
            }
            if (*unitsTab) break;
        }
    }
    if (*unitsTab) return false;

    //May be D2010
    *unitsTab = 0;
    for (i = 0; i < ((TotalSize - 20) & (-4)); i += 4)
    {
        initTable = *((DWORD*)(Image + i));
        if (initTable == CodeBase + i + 20)
        {
            num = *((DWORD*)(Image + i - 4));
            if (num <= 0 || num > 10000) continue;

        	pos = *unitsTab = i + 20;
            for (n = 0; n < num; n++, pos += 8)
            {
                iniAdr = *((DWORD*)(Image + pos));
                if (iniAdr)
                {
                    if (!IsValidImageAdr(iniAdr))
                    {
                        *unitsTab = 0;
                        break;
                    }
                }
                finAdr = *((DWORD*)(Image + pos + 4));
                if (finAdr)
                {
                    if (!IsValidImageAdr(finAdr))
                    {
                        *unitsTab = 0;
                        break;
                    }
                }
            }
            if (*unitsTab) break;
        }
    }
    if (*unitsTab) return true;
    
    return false;
}
//---------------------------------------------------------------------------
DWORD __fastcall TFMain_11011981::EvaluateInitTable(BYTE* Data, DWORD Size, DWORD Base)
{
    int     i, num, pos, unitsPos = 0, n;
    DWORD   initTable, result, iniAdr, finAdr, maxAdr = 0;

    for (i = 0; i < ((Size - 4) & (-4)); i += 4)
    {
        initTable = result = *((DWORD*)(Data + i));
        if (initTable == Base + i + 4)
        {
            num = *((DWORD*)(Data + i - 4));
            if (num <= 0 || num > 10000) continue;
        	pos = unitsPos = i + 4;
            for (n = 0; n < num; n++, pos += 8)
            {
                iniAdr = *((DWORD*)(Data + pos));
                if (iniAdr)
                {
                    if (iniAdr < Base || iniAdr >= Base + Size)//!IsValidImageAdr(iniAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    else if (iniAdr > maxAdr)
                    {
                        maxAdr = iniAdr;
                    }
                }
                finAdr = *((DWORD*)(Data + pos + 4));
                if (finAdr)
                {
                    if (finAdr < Base || finAdr >= Base + Size)//!IsValidImageAdr(finAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    else if (finAdr > maxAdr)
                    {
                        maxAdr = finAdr;
                    }
                }
                result += 8;
            }
            if (unitsPos) break;
        }
    }
    if (maxAdr > result) result = (maxAdr + 3) & (-4);

    if (unitsPos) return initTable - 8;

    //May be D2010
    maxAdr = 0;
    for (i = 0; i < ((Size - 20) & (-4)); i += 4)
    {
        initTable = result = *((DWORD*)(Data + i));
        if (initTable == Base + i + 20)
        {
            num = *((DWORD*)(Data + i - 4));
            if (num <= 0 || num > 10000) continue;

        	pos = unitsPos = i + 20;
            for (n = 0; n < num; n++, pos += 8)
            {
                iniAdr = *((DWORD*)(Data + pos));
                if (iniAdr)
                {
                    if (iniAdr < Base || iniAdr >= Base + Size)//!IsValidImageAdr(iniAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    else if (iniAdr > maxAdr)
                    {
                        if (*((DWORD*)(Data + Adr2Pos(iniAdr)))) maxAdr = iniAdr;
                    }
                }
                finAdr = *((DWORD*)(Data + pos + 4));
                if (finAdr)
                {
                    if (finAdr < Base || finAdr >= Base + Size)//!IsValidImageAdr(finAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    else if (finAdr > maxAdr)
                    {
                        if (*((DWORD*)(Data + Adr2Pos(finAdr)))) maxAdr = finAdr;
                    }
                }
                result += 8;
            }
            if (unitsPos) break;
        }
    }
    if (maxAdr > result) result = (maxAdr + 3) & (-4);

    if (unitsPos) return initTable - 24;

    return 0;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::GetUnits(String dprName)
{
    BYTE        len;
    char        *b, *e;
	int         n, i, no, unitsPos = 0, start, spos, pos, iniProcSize, finProcSize, unitsNum = 0;
    int         typesNum, units1Num, typesTable, units1Table;   //For D2010
	DWORD       initTable, iniAdr, finAdr, unitsTabEnd, toAdr;
	PUnitRec    recU;
    PInfoRec    recN;

    if (DelphiVersion >= 2010)
    {
        for (i = 0; i < ((TotalSize - 20) & (-4)); i += 4)
        {
            initTable = *((DWORD*)(Image + i));
            if (initTable == CodeBase + i + 20)
            {
                unitsNum = *((DWORD*)(Image + i - 4));
                if (unitsNum <= 0 || unitsNum > 10000) continue;

                pos = unitsPos = i + 20;
                for (n = 0; n < unitsNum; n++, pos += 8)
                {
                    iniAdr = *((DWORD*)(Image + pos));
                    if (iniAdr && !IsValidImageAdr(iniAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    finAdr = *((DWORD*)(Image + pos + 4));
                    if (finAdr && !IsValidImageAdr(finAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                }
                if (unitsPos) break;
            }
        }
    }
    else
    {
        for (i = 0; i < ((CodeSize - 4) & (-4)); i += 4)
        {
            initTable = *((DWORD*)(Image + i));
            if (initTable == CodeBase + i + 4)
            {
                unitsNum = *((DWORD*)(Image + i - 4));
                if (unitsNum <= 0 || unitsNum > 10000) continue;

                pos = unitsPos = i + 4;
                for (n = 0; n < unitsNum; n++, pos += 8)
                {
                    iniAdr = *((DWORD*)(Image + pos));
                    if (iniAdr && !IsValidImageAdr(iniAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                    finAdr = *((DWORD*)(Image + pos + 4));
                    if (finAdr && !IsValidImageAdr(finAdr))
                    {
                        unitsPos = 0;
                        break;
                    }
                }
                if (unitsPos) break;
            }
        }
    }
    //if (!unitsPos) return 0;

    if (!unitsPos)
    {
        unitsNum = 1;
        recU = new UnitRec;
        recU->trivial = false;
        recU->trivialIni = true;
        recU->trivialFin = true;
        recU->kb = false;
        recU->names = new TStringList;

        recU->fromAdr = CodeBase;
        recU->toAdr = CodeBase + TotalSize;
        recU->matchedPercent = 0.0;
        recU->iniOrder = 1;

        recU->finadr = 0;
        recU->finSize = 0;
        recU->iniadr = 0;
        recU->iniSize = 0;
        Units->Add((void*)recU);
        return unitsNum;
    }

    unitsTabEnd = initTable + 8 * unitsNum;
    if (DelphiVersion >= 2010)
    {
        initTable -= 24;
        SetFlags(cfData, Adr2Pos(initTable), 8*unitsNum + 24);
        typesNum = *((int*)(Image + Adr2Pos(initTable + 8)));
        typesTable = *((int*)(Image + Adr2Pos(initTable + 12)));
        SetFlags(cfData, Adr2Pos(typesTable), 4*typesNum);
        units1Num = *((int*)(Image + Adr2Pos(initTable + 16)));
        units1Table = *((int*)(Image + Adr2Pos(initTable + 20)));
        spos = pos = Adr2Pos(units1Table);
        for (i = 0; i < units1Num; i++)
        {
            len = *(Image + pos);
            pos += len + 1;
        }
        SetFlags(cfData, Adr2Pos(units1Table), pos - spos);
    }
    else
    {
        initTable -= 8;
        SetFlags(cfData, Adr2Pos(initTable), 8*unitsNum + 8);
    }

    for (i = 0, no = 1; i < unitsNum; i++, unitsPos += 8)
    {
        iniAdr = *((DWORD*)(Image + unitsPos));
        finAdr = *((DWORD*)(Image + unitsPos + 4));

        if (!iniAdr && !finAdr) continue;

        if (iniAdr && *((DWORD*)(Image + Adr2Pos(iniAdr))) == 0) continue;
        if (finAdr && *((DWORD*)(Image + Adr2Pos(finAdr))) == 0) continue;

        //MAY BE REPEATED ADRESSES!!!
        bool found = false;
        for (n = 0; n < Units->Count; n++)
        {
            recU = (PUnitRec)Units->Items[n];
            if (recU->iniadr == iniAdr && recU->finadr == finAdr)
            {
                found = true;
                break;
            }
        }
        if (found) continue;

        if (iniAdr)
        	iniProcSize = EstimateProcSize(iniAdr);
        else
        	iniProcSize = 0;

        if (finAdr)
        	finProcSize = EstimateProcSize(finAdr);
        else
        	finProcSize = 0;

        toAdr = 0;
        if (iniAdr && iniAdr < unitsTabEnd)
        {
            if (iniAdr >= finAdr + finProcSize)
                toAdr = (iniAdr + iniProcSize + 3) & 0xFFFFFFFC;
            if (finAdr >= iniAdr + iniProcSize)
                toAdr = (finAdr + finProcSize + 3) & 0xFFFFFFFC;
        }
        else if (finAdr)
        {
            toAdr = (finAdr + finProcSize + 3) & 0xFFFFFFFC;
        }

        if (!toAdr)
        {
            if (Units->Count > 0) continue;
            toAdr = CodeBase + CodeSize;
        }

        recU = new UnitRec;
        recU->trivial = false;
        recU->trivialIni = true;
        recU->trivialFin = true;
        recU->kb = false;
        recU->names = new TStringList;

        recU->fromAdr = 0;
        recU->toAdr = toAdr;
        recU->matchedPercent = 0.0;
        recU->iniOrder = no; no++;

        recU->finadr = finAdr;
        recU->finSize = finProcSize;
        recU->iniadr = iniAdr;
        recU->iniSize = iniProcSize;

        if (iniAdr)
        {
            pos = Adr2Pos(iniAdr);
	        //Check trivial initialization
        	if (iniProcSize > 8) recU->trivialIni = false;
            SetFlag(cfProcStart, pos);
            //SetFlag(cfProcEnd, pos + iniProcSize - 1);
            recN = GetInfoRec(iniAdr);
            if (!recN) recN = new InfoRec(pos, ikProc);
            recN->procInfo->procSize = iniProcSize;
        }
        if (finAdr)
        {
            pos = Adr2Pos(finAdr);
	        //Check trivial finalization
        	if (finProcSize > 46) recU->trivialFin = false;
            SetFlag(cfProcStart, pos);
            //SetFlag(cfProcEnd, pos + finProcSize - 1);
            recN = GetInfoRec(finAdr);
            if (!recN) recN = new InfoRec(pos, ikProc);
            recN->procInfo->procSize = finProcSize;
            //import?
            if (IsFlagSet(cfImport, pos))
            {
                b = strchr(recN->GetName().c_str(), '@');
                if (b)
                {
                    e = strchr(b + 1, '@');
                    if (e) SetUnitName(recU, String(b + 1, e - b - 1));
                }
            }
        }
        Units->Add((void*)recU);
    }

    Units->Sort(&SortUnitsByAdr);

    start = CodeBase;
    unitsNum = Units->Count;
	for (i = 0; i < unitsNum; i++)
    {
    	recU = (PUnitRec)Units->Items[i];
        recU->fromAdr = start;
        if (recU->toAdr) start = recU->toAdr;
        //Is unit trivial?
        if (recU->trivialIni && recU->trivialFin)
        {
            int isize = (recU->iniSize + 3) & 0xFFFFFFFC;
            int fsize = (recU->finSize + 3) & 0xFFFFFFFC;
            if (isize + fsize == recU->toAdr - recU->fromAdr) recU->trivial = true;
        }
        //Last Unit has program name and toAdr = initTable
        if (i == unitsNum - 1)
        {
            recU->toAdr = initTable;
            SetUnitName(recU, dprName);
        }
    }

    return unitsNum;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::GetBCBUnits(String dprName)
{
    int         n, pos, curPos, instrLen, iniNum, finNum, unitsNum, no;
    DWORD       dd, adr, curAdr, modTable, iniTable, iniTableEnd, finTable, finTableEnd, fromAdr, toAdr;
    PUnitRec    recU;
    PInfoRec    recN;
    DISINFO     disInfo;

    //EP: jmp @1
    curAdr = EP; curPos = Adr2Pos(curAdr);
    instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &disInfo, 0);
    dd = *((DWORD*)disInfo.Mnem);
    if (dd == 'pmj')
    {
        curAdr = disInfo.Immediate; curPos = Adr2Pos(curAdr);
        while (1)
        {
            instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &disInfo, 0);
            dd = *((DWORD*)disInfo.Mnem);
            if (dd == 'pmj') break;
            if (dd == 'hsup' && disInfo.OpType[0] == otIMM && disInfo.Immediate)
            {
                modTable = disInfo.Immediate;
                if (IsValidImageAdr(modTable))
                {
                    pos = Adr2Pos(modTable);
                    iniTable = *((DWORD*)(Image + pos));
                    iniTableEnd = *((DWORD*)(Image + pos + 4));
                    finTable = *((DWORD*)(Image + pos + 8));
                    finTableEnd = *((DWORD*)(Image + pos + 12));
                    for (n = 16; n < 32; n += 4)
                    {
                        adr = *((DWORD*)(Image + pos + n));
                        if (IsValidImageAdr(adr))
                        {
                            pos = Adr2Pos(adr);
                            SetFlag(cfProcStart, pos);
                            recN = GetInfoRec(adr);
                            if (!recN) recN = new InfoRec(pos, ikProc);
                            recN->SetName("WinMain");
                            break;
                        }
                    }

                    iniNum = (iniTableEnd - iniTable) / 6;
                    SetFlags(cfData, Adr2Pos(iniTable), iniTableEnd - iniTable);
                    finNum = (finTableEnd - finTable) / 6;
                    SetFlags(cfData, Adr2Pos(finTable), finTableEnd - finTable);

                    TStringList* list = new TStringList;
                    list->Sorted = false;
                    list->Duplicates = dupIgnore;
                    if (iniNum > finNum)
                    {
                        pos = Adr2Pos(iniTable);
                        for (n = 0; n < iniNum; n++)
                        {
                            adr = *((DWORD*)(Image + pos + 2));
                            pos += 6;
                            list->Add(Val2Str8(adr));
                        }
                    }
                    else
                    {
                        pos = Adr2Pos(finTable);
                        for (n = 0; n < finNum; n++)
                        {
                            adr = *((DWORD*)(Image + pos + 2));
                            pos += 6;
                            list->Add(Val2Str8(adr));
                        }
                    }
                    list->Sort();
                    fromAdr = CodeBase; no = 1;
                    for (n = 0; n < list->Count; n++)
                    {
                        recU = new UnitRec;
                        recU->trivial = false;
                        recU->trivialIni = true;
                        recU->trivialFin = true;
                        recU->kb = false;
                        recU->names = new TStringList;

                        recU->matchedPercent = 0.0;
                        recU->iniOrder = no; no++;

                        toAdr = StrToInt(String("$") + list->Strings[n]);
                        recU->finadr = 0;
                        recU->finSize = 0;
                        recU->iniadr = toAdr;
                        recU->iniSize = 0;

                        recU->fromAdr = fromAdr;
                        recU->toAdr = toAdr;
                        Units->Add((void*)recU);

                        fromAdr = toAdr;
                        if (n == list->Count - 1) SetUnitName(recU, dprName);
                    }
                    delete list;
                    Units->Sort(&SortUnitsByAdr);
                    return Units->Count;
                }
            }
            curAdr += instrLen; curPos += instrLen;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//-1 - not Code
//0 - possible Code
//1 - Code
int __fastcall TFMain_11011981::IsValidCode(DWORD fromAdr)
{
    BYTE        op;
    int         firstPushReg, lastPopReg;
    int         firstPushPos, lastPopPos;
    int         row, num, instrLen, instrLen1, instrLen2;
    int         fromPos;
    int         curPos;
    DWORD       curAdr;
    DWORD       lastAdr = 0;
    DWORD       Adr, Adr1, Pos, lastMovAdr = 0;
    PInfoRec    recN;
    DISINFO     DisInfo;

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return -1;

    if (fromAdr > EP) return -1;
    //DISPLAY
    if (!stricmp(Code + fromPos, "DISPLAY")) return -1;

    //recN = GetInfoRec(fromAdr);
    int outRows = MAX_DISASSEMBLE;
    if (IsFlagSet(cfImport, fromPos)) outRows = 1;

    firstPushReg = lastPopReg = -1;
    firstPushPos = lastPopPos = -1;
    curPos = fromPos; curAdr = fromAdr;

    for (row = 0; row < outRows; row++)
    {
        //Таблица exception
        if (!IsValidImageAdr(curAdr)) return -1;
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            curPos += 4 + 8*num; curAdr += 4 + 8*num;
            continue;
        }

        BYTE b1 = Code[curPos];
        BYTE b2 = Code[curPos + 1];
        //00,00 - Data!
        if (!b1 && !b2 && !lastAdr) return -1;

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) return -1;
        if (!instrLen)//????????
        {
            curPos++; curAdr++;
            continue;
        }
        if (!memcmp(DisInfo.Mnem, "arpl", 4) ||
            !memcmp(DisInfo.Mnem, "out", 3)  ||
            (DisInfo.Mnem[0] == 'i' && DisInfo.Mnem[1] == 'n' && DisInfo.Mnem[2] != 'c'))
        {
            return -1;
        }
        op = Disasm.GetOp(DisInfo.Mnem);

        if (op == OP_JMP)
        {
            if (curAdr == fromAdr) break;
            if (DisInfo.OpType[0] == otMEM)
            {
                if (Adr2Pos(DisInfo.Offset) < 0 && (!lastAdr || curAdr == lastAdr)) break;
            }
            if (DisInfo.OpType[0] == otIMM)
            {
                Adr = DisInfo.Immediate; Pos = Adr2Pos(Adr);
                if (Pos < 0 && (!lastAdr || curAdr == lastAdr)) break;
                if (GetSegmentNo(Adr) != 0 && GetSegmentNo(fromAdr) != GetSegmentNo(Adr) && (!lastAdr || curAdr == lastAdr)) break;
                if (Adr < fromAdr && (!lastAdr || curAdr == lastAdr)) break;
                curAdr = Adr; curPos = Pos;
                continue;
            }
        }

        //Mark push or pop
        if (op == OP_PUSH)
        {
            //Если первая инструкция не push reg
            if (!row && DisInfo.OpType[0] != otREG) return -1;
            if (DisInfo.OpType[0] == otREG && firstPushReg == -1)
            {
                firstPushReg = DisInfo.OpRegIdx[0];
                firstPushPos = curPos;
            }
        }
        else if (op == OP_POP)
        {
            if (DisInfo.OpType[0] == otREG)
            {
                lastPopReg = DisInfo.OpRegIdx[0];
                lastPopPos = curPos;
            }
        }

        //Отбраковка по первой инструкции
        if (!row)
        {
            //Инструкция перехода или ret c аргументами
            if (DisInfo.Ret && DisInfo.OpNum >= 1) return -1;
            if (DisInfo.Branch) return -1;
            if (!memcmp(DisInfo.Mnem, "bound", 5) ||
                !memcmp(DisInfo.Mnem, "retf", 4)  ||
                !memcmp(DisInfo.Mnem, "pop", 3)   ||
                !memcmp(DisInfo.Mnem, "aaa", 3)   ||
                !memcmp(DisInfo.Mnem, "adc", 3)   ||
                !memcmp(DisInfo.Mnem, "sbb", 3)   ||
                !memcmp(DisInfo.Mnem, "rcl", 3)   ||
                !memcmp(DisInfo.Mnem, "rcr", 3)   ||
                !memcmp(DisInfo.Mnem, "clc", 3)   ||
                !memcmp(DisInfo.Mnem, "stc", 3))
                return -1;
        }
        //Если в позиции встретился уже определенный ранее код, выходим
        for (int k = 0; k < instrLen; k++)
        {
            if (IsFlagSet(cfProcStart, curPos + k) || IsFlagSet(cfCode, curPos + k))
                return -1;
        }

        if (curAdr >= lastAdr) lastAdr = 0;
        //Конец процедуры
        if (DisInfo.Ret && (!lastAdr || curAdr == lastAdr))
        {
            //Standard frame
            if (Code[fromPos] == 0x55 && Code[fromPos + 1] == 0x8B && Code[fromPos + 2] == 0xEC)
                break;
            if (firstPushReg == lastPopReg && firstPushPos == fromPos && lastPopPos == curPos - 1)
                break;
            return 0;
        }

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            /*
            //Первая инструкция
            if (curAdr == fromAdr) break;
            */
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Адрес таблицы - последние 4 байта инструкции
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Анализируем промежуток на предмет таблицы cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //Если есть cTblAdr, пропускаем эту таблицу
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                Pos += CNum; Adr += CNum;
            }
            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidCodeAdr(Adr1) || Adr1 < fromAdr) break;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }

        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    if (Pos >= 0)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Дизассемблируем jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                            //if (!instrLen1) return -1;

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    /*
                                    //@2
                                    Adr1 = DisInfo.Immediate - 4;
                                    Adr = *((DWORD*)(Code + Adr2Pos(Adr1)));
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    */
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Флажок cfETable, чтобы правильно вывести данные
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                }
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }

        if (b1 == 0xEB ||				 //short relative abs jmp or cond jmp
        	(b1 >= 0x70 && b1 <= 0x7F) ||
            (b1 == 0xF && b2 >= 0x80 && b2 <= 0x8F))
        {
            Adr = DisInfo.Immediate;
            if (!IsValidImageAdr(Adr)) return -1;
            if (IsValidCodeAdr(Adr))
            {
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (!IsValidImageAdr(Adr)) return -1;
            if (IsValidCodeAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                if (!recN && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        curPos += instrLen; curAdr += instrLen;
    }
    return 1;
}
//---------------------------------------------------------------------------
//Sort VmtList by height and vmtAdr
int __fastcall SortPairsCmpFunction(void *item1, void *item2)
{
    PVmtListRec rec1 = (PVmtListRec)item1;
    PVmtListRec rec2 = (PVmtListRec)item2;

    if (rec1->height > rec2->height) return 1;
    if (rec1->height < rec2->height) return -1;
    if (rec1->vmtAdr > rec2->vmtAdr) return 1;
    if (rec1->vmtAdr < rec2->vmtAdr) return -1;
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FillVmtList()
{
    VmtList->Clear();

    for (int n = 0; n < TotalSize; n++)
    {
        PInfoRec recN = GetInfoRec(Pos2Adr(n));
    	if (recN && recN->kind == ikVMT && recN->HasName())
        {
            PVmtListRec recV = new VmtListRec;
            recV->height = GetClassHeight(Pos2Adr(n));
            recV->vmtAdr = Pos2Adr(n);
            recV->vmtName = recN->GetName();
            VmtList->Add((void*)recV);
        }
    }
    VmtList->Sort(SortPairsCmpFunction);
}
//---------------------------------------------------------------------------
//Return virtual method offset of procedure with address procAdr
int __fastcall TFMain_11011981::GetMethodOfs(PInfoRec rec, DWORD procAdr)
{
    if (rec && rec->vmtInfo->methods)
    {
        for (int m = 0; m < rec->vmtInfo->methods->Count; m++)
        {
            PMethodRec recM = (PMethodRec)rec->vmtInfo->methods->Items[m];
            if (recM->kind == 'V' && recM->address == procAdr) return recM->id;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
//Return PMethodRec of method with given name
PMethodRec __fastcall TFMain_11011981::GetMethodInfo(PInfoRec rec, String name)
{
    if (rec && rec->vmtInfo->methods)
    {
        for (int m = 0; m < rec->vmtInfo->methods->Count; m++)
        {
            PMethodRec recM = (PMethodRec)rec->vmtInfo->methods->Items[m];
            if (SameText(recM->name, name)) return recM;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
//IntfTable присутствует, если класс порожден от интерфейсов
/*
Interfaces
    Any class can implement any number of interfaces. The compiler stores a
    table of interfaces as part of the class's RTTI. The VMT points to the table
    of interfaces, which starts with a 4-byte count, followed by a list of
    interface records. Each interface record contains the GUID, a pointer to the
    interface's VMT, the offset to the interface's hidden field, and a pointer
    to a property that implements the interface with the implements directive.
    If the offset is zero, the interface property (called ImplGetter) must be
    non-nil, and if the offset is not zero, ImplGetter must be nil. The interface
    property can be a reference to a field, a virtual method, or a static method,
    following the conventions of a property reader (which is described earlier
    in this chapter, under Section 3.2.3"). When an object is constructed, Delphi
    automatically checks all the interfaces, and for each interface with a
    non-zero IOffset, the field at that offset is set to the interface's VTable
    (a pointer to its VMT). Delphi defines the the types for the interface table,
    unlike the other RTTI tables, in the System unit. These types are shown in
    Example 3.9.
    Example 3.9. Type Declarations for the Interface Table

    type
      PInterfaceEntry = ^TInterfaceEntry;
      TInterfaceEntry = record
        IID: TGUID;
        VTable: Pointer;
        IOffset: Integer;
        ImplGetter: Integer;
      end;

      PInterfaceTable = ^TInterfaceTable;
      TInterfaceTable = record
        EntryCount: Integer;
        // Declare the type with the largest possible size,
        // but the true size of the array is EntryCount elements.
        Entries: array[0..9999] of TInterfaceEntry;
        //>=DXE2
        Intfs:array[0..EntryCount - 1] of PPTypeInfo;
      end;

    TObject implements several methods for accessing the interface table. See for the details of the GetInterface, GetInterfaceEntry, and GetInterfaceTable methods.
*/
void __fastcall TFMain_11011981::ScanIntfTable(DWORD adr)
{
    bool        vmtProc;
    WORD        _dx, _bx, _si;
    int         n, pos, entryCount, cnt, vmtOfs, vpos, _pos, iOffset;
    DWORD       vmtAdr, intfAdr, vAdr, iAdr, _adr;
    String      className, name;
    PInfoRec    recN, recN1;
    PMethodRec  recM;
    BYTE        GUID[16];

    if (!IsValidImageAdr(adr)) return;

    className = GetClsName(adr);
    recN = GetInfoRec(adr);
    vmtAdr = adr - VmtSelfPtr;
    pos = Adr2Pos(vmtAdr) + VmtIntfTable;
    intfAdr = *((DWORD*)(Code + pos));
    if (!intfAdr) return;

    pos = Adr2Pos(intfAdr);
    entryCount = *((int*)(Code + pos)); pos += 4;

    for (n = 0; n < entryCount; n++)
    {
        memmove(GUID, Code + pos, 16); pos += 16;
        //VTable
        vAdr = *((DWORD*)(Code + pos)); pos += 4;
        if (IsValidCodeAdr(vAdr))
        {
            cnt = 0;
            vpos = Adr2Pos(vAdr);
            for (int v = 0;;v++)
            {
                if (IsFlagSet(cfVTable, vpos)) cnt++;
                if (cnt == 2) break;
                iAdr = *((DWORD*)(Code + vpos)); _adr = iAdr;
                _pos = Adr2Pos(_adr); DISINFO disInfo;
                vmtProc = false; vmtOfs = 0;
                _dx = 0; _bx = 0; _si = 0;
                while (1)
                {
                    int instrlen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &disInfo, 0);
                    if ((disInfo.OpType[0] == otMEM || disInfo.OpType[1] == otMEM) &&
                        disInfo.BaseReg != 20)//to exclude instruction "xchg reg, [esp]"
                    {
                        vmtOfs = disInfo.Offset;
                    }
                    if (disInfo.OpType[0] == otREG && disInfo.OpType[1] == otIMM)
                    {
                        if (disInfo.OpRegIdx[0] == 10)//dx
                            _dx = disInfo.Immediate;
                        else if (disInfo.OpRegIdx[0] == 11)//bx
                            _bx = disInfo.Immediate;
                        else if (disInfo.OpRegIdx[0] == 14)//si
                            _si = disInfo.Immediate;
                    }
                    if (disInfo.Call)
                    {
                        recN1 = GetInfoRec(disInfo.Immediate);
                        if (recN1)
                        {
                            if (recN1->SameName("@CallDynaInst") ||
                                recN1->SameName("@CallDynaClass"))
                            {
                                if (DelphiVersion <= 5)
                                    GetDynaInfo(adr, _bx, &iAdr);
                                else
                                    GetDynaInfo(adr, _si, &iAdr);
                                break;
                            }
                            else if (recN1->SameName("@FindDynaInst") ||
                                     recN1->SameName("@FindDynaClass"))
                            {
                                GetDynaInfo(adr, _dx, &iAdr);
                                break;
                            }
                        }
                    }
                    if (disInfo.Branch && !disInfo.Conditional)
                    {
                        if (IsValidImageAdr(disInfo.Immediate))
                            iAdr = disInfo.Immediate;
                        else
                            vmtProc = true;
                        break;
                    }
                    else if (disInfo.Ret)
                    {
                        vmtProc = true;
                        break;
                    }
                    _pos += instrlen; _adr += instrlen;
                }
                if (!vmtProc && IsValidImageAdr(iAdr))
                {
                    AnalyzeProcInitial(iAdr);
                    recN1 = GetInfoRec(iAdr);
                    if (recN1)
                    {
                        if (recN1->HasName())
                        {
                            className = ExtractClassName(recN1->GetName());
                            name = ExtractProcName(recN1->GetName());
                        }
                        else
                            name = GetDefaultProcName(iAdr);
                        if (v >= 3)
                        {
                            if (!recN1->HasName()) recN1->SetName(className + "." + name);
                        }
                    }
                }
                vpos += 4;
            }
        }
        //iOffset
        iOffset = *((int*)(Code + pos)); pos += 4;
        //ImplGetter
        if (DelphiVersion > 3) pos += 4;
        recN->vmtInfo->AddInterface(Val2Str8(vAdr) + " " + Val2Str4(iOffset) + " " + Guid2String(GUID));
    }
}
//---------------------------------------------------------------------------
/*
Automated Methods
    The automated section of a class declaration is now obsolete because it is
    easier to create a COM automation server with Delphi's type library editor,
    using interfaces. Nonetheless, the compiler currently supports automated
    declarations for backward compatibility. A future version of the compiler
    might drop support for automated declarations.
    The OleAuto unit tells you the details of the automated method table: The
    table starts with a 2-byte count, followed by a list of automation records.
    Each record has a 4-byte dispid (dispatch identifier), a pointer to a short
    string method name, 4-bytes of flags, a pointer to a list of parameters,
    and a code pointer. The parameter list starts with a 1-byte return type,
    followed by a 1-byte count of parameters, and ends with a list of 1-byte
    parameter types. The parameter names are not stored. Example 3.8 shows the
    declarations for the automated method table.
    Example 3.8. The Layout of the Automated Method Table

    const
      { Parameter type masks }
      atTypeMask = $7F;
      varStrArg  = $48;
      atByRef    = $80;
      MaxAutoEntries = 4095;
      MaxAutoParams = 255;

    type
      TVmtAutoType = Byte;
      { Automation entry parameter list }
      PAutoParamList = ^TAutoParamList;
      TAutoParamList = packed record
        ReturnType: TVmtAutoType;
        Count: Byte;
        Types: array[1..Count] of TVmtAutoType;
      end;
      { Automation table entry }
      PAutoEntry = ^TAutoEntry;
      TAutoEntry = packed record
        DispID: LongInt;
        Name: PShortString;
        Flags: LongInt; { Lower byte contains flags }
        Params: PAutoParamList;
        Address: Pointer;
      end;

      { Automation table layout }
      PAutoTable = ^TAutoTable;
      TAutoTable = packed record
        Count: LongInt;
        Entries: array[1..Count] of TAutoEntry; 
      end;
*/
//Auto function prototype can be recovered from AutoTable!!!
void __fastcall TFMain_11011981::ScanAutoTable(DWORD Adr)
{
    if (!IsValidImageAdr(Adr)) return;

    DWORD vmtAdr = Adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtAutoTable;
    DWORD autoAdr = *((DWORD*)(Code + pos));
    if (!autoAdr) return;

    String className = GetClsName(Adr);
    PInfoRec recN = GetInfoRec(Adr);
    
    pos = Adr2Pos(autoAdr);
    int entryCount = *((int*)(Code + pos)); pos += 4;

    for (int i = 0; i < entryCount; i++)
    {
        int dispID = *((int*)(Code + pos)); pos += 4;

        DWORD nameAdr = *((DWORD*)(Code + pos)); pos += 4;
        DWORD posn = Adr2Pos(nameAdr);
        BYTE len = *(Code + posn); posn++;
        String name = String((char*)(Code + posn), len);
        String procname = className + ".";

        int flags = *((int*)(Code + pos)); pos += 4;
        DWORD params = *((int*)(Code + pos)); pos += 4;
        DWORD address = *((DWORD*)(Code + pos)); pos += 4;
        
        //afVirtual
        if ((flags & 8) == 0)
        {
            //afPropGet
            if (flags & 2) procname += "Get";
            //afPropSet
            if (flags & 4) procname += "Set";
        }
        else
        {
            //virtual table function
            address = *((DWORD*)(Code + Adr2Pos(vmtAdr + address)));
        }

        procname += name;
        AnalyzeProcInitial(address);
        PInfoRec recN1 = GetInfoRec(address);
        if (!recN1) recN1 = new InfoRec(Adr2Pos(address), ikRefine);
        if (!recN1->HasName()) recN1->SetName(procname);
        //Method
        if ((flags & 1) != 0) recN1->procInfo->flags |= PF_METHOD;
        //params
        int ppos = Adr2Pos(params);
        BYTE typeCode = *(Code + ppos); ppos++;
        BYTE paramsNum = *(Code + ppos); ppos++;
        for (int m = 0; m < paramsNum; m++)
        {
            BYTE argType = *(Code + ppos); ppos++;

        }
        recN->vmtInfo->AddMethod(false, 'A', dispID, address, procname);
    }
}
//---------------------------------------------------------------------------
/*
Initialization and Finalization
    When Delphi constructs an object, it automatically initializes strings,
    dynamic arrays, interfaces, and Variants. When the object is destroyed,
    Delphi must decrement the reference counts for strings, interfaces, dynamic
    arrays, and free Variants and wide strings. To keep track of this information,
    Delphi uses initialization records as part of a class's RTTI. In fact, every
    record and array that requires finalization has an associated initialization
    record, but the compiler hides these records. The only ones you have access
    to are those associated with an object's fields.
    A VMT points to an initialization table. The table contains a list of
    initialization records. Because arrays and records can be nested, each
    initialization record contains a pointer to another initialization table,
    which can contain initialization records, and so on. An initialization table
    uses a TTypeKind field to keep track of whether it is initializing a string,
    a record, an array, etc.
    An initialization table begins with the type kind (1 byte), followed by the
    type name as a short string, a 4-byte size of the data being initialized, a
    4-byte count for initialization records, and then an array of zero or more
    initialization records. An initialization record is just a pointer to a
    nested initialization table, followed by a 4-byte offset for the field that
    must be initialized. Example 3.7 shows the logical layout of the initialization
    table and record, but the declarations depict the logical layout without
    being true Pascal code.
    Example 3.7. The Layout of the Initialization Table and Record

    type
      { Initialization/finalization record }
      PInitTable = ^TInitTable;
      TInitRecord = packed record
        InitTable: ^PInitTable;
        Offset: LongWord;        // Offset of field in object
      end;
      { Initialization/finalization table }
      TInitTable = packed record
      {$MinEnumSize 1} // Ensure that TypeKind takes up 1 byte.
        TypeKind: TTypeKind;
        TypeName: packed ShortString;
        DataSize: LongWord;
        Count: LongWord;
        // If TypeKind=ikArray, Count is the array size, but InitRecords
        // has only one element; if the type kind is tkRecord, Count is the
        // number of record members, and InitRecords[] has a
        // record for each member. For all other types, Count=0.
        InitRecords: array[1..Count] of TInitRecord;
      end;
*/
void __fastcall TFMain_11011981::ScanInitTable(DWORD Adr)
{
    if (!IsValidImageAdr(Adr)) return;

    PInfoRec recN = GetInfoRec(Adr);
    DWORD vmtAdr = Adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtInitTable;
    DWORD initAdr = *((DWORD*)(Code + pos));
    if (!initAdr) return;

    pos = Adr2Pos(initAdr);
    pos++;  	//skip 0xE
    pos++;    	//unknown
    pos += 4;	//unknown
    DWORD num = *((DWORD*)(Code + pos)); pos += 4;

    for (int i = 0; i < num; i++)
    {
        DWORD typeAdr = *((DWORD*)(Code + pos)); pos += 4;
        DWORD post = Adr2Pos(typeAdr);
        if (DelphiVersion != 2) post += 4;  //skip SelfPtr
        post++;     //skip tkKind
        BYTE len = *(Code + post); post++;
        String typeName = String((char*)&Code[post], len);
        int fieldOfs = *((int*)(Code + pos)); pos += 4;
        recN->vmtInfo->AddField(0, 0, FIELD_PUBLIC, fieldOfs, -1, "", typeName);
    }
}
//---------------------------------------------------------------------------
//For Version>=2010
//Count: Word; // Published fields
//ClassTab: PVmtFieldClassTab
//Entry: array[1..Count] of TVmtFieldEntry
//ExCount: Word;
//ExEntry: array[1..ExCount] of TVmtFieldExEntry;
//================================================
//TVmtFieldEntry
//FieldOffset: Longword;
//TypeIndex: Word; // index into ClassTab
//Name: ShortString;
//================================================
//TFieldExEntry = packed record
//Flags: Byte;
//TypeRef: PPTypeInfo;
//Offset: Longword;
//Name: ShortString;
//AttrData: TAttrData
void __fastcall TFMain_11011981::ScanFieldTable(DWORD Adr)
{
    if (!IsValidImageAdr(Adr)) return;

    PInfoRec recN = GetInfoRec(Adr);
    DWORD vmtAdr = Adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtFieldTable;
    DWORD fieldAdr = *((DWORD*)(Code + pos));
    if (!fieldAdr) return;

    pos = Adr2Pos(fieldAdr);
    WORD count = *((WORD*)(Code + pos)); pos += 2;
    DWORD typesTab = *((DWORD*)(Code + pos)); pos += 4;

    for (int i = 0; i < count; i++)
    {
        int fieldOfs = *((int*)(Code + pos)); pos += 4;
        WORD idx = *((WORD*)(Code + pos)); pos += 2;
        BYTE len = Code[pos]; pos++;
        String name = String((char*)(Code + pos), len); pos += len;

        DWORD post = Adr2Pos(typesTab) + 2 + 4 * idx;
        DWORD classAdr = *((DWORD*)(Code + post));
        if (IsFlagSet(cfImport, Adr2Pos(classAdr)))
        {
            PInfoRec recN1 = GetInfoRec(classAdr);
            recN->vmtInfo->AddField(0, 0, FIELD_PUBLISHED, fieldOfs, -1, name, recN1->GetName());
        }
        else
        {
            if (DelphiVersion == 2) classAdr += VmtSelfPtr;
            recN->vmtInfo->AddField(0, 0, FIELD_PUBLISHED, fieldOfs, -1, name, GetClsName(classAdr));
        }
    }
    if (DelphiVersion >= 2010)
    {
        WORD exCount = *((WORD*)(Code + pos)); pos += 2;
        for (int i = 0; i < exCount; i++)
        {
            BYTE flags = Code[pos]; pos++;
            DWORD typeRef = *((DWORD*)(Code + pos)); pos += 4;
            int offset = *((int*)(Code + pos)); pos += 4;
            BYTE len = Code[pos]; pos++;
            String name = String((char*)(Code + pos), len); pos += len;
            WORD dw = *((WORD*)(Code + pos)); pos += dw;
            recN->vmtInfo->AddField(0, 0, FIELD_PUBLISHED, offset, -1, name, GetTypeName(typeRef));
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ScanMethodTable(DWORD adr, String className)
{
    BYTE        len;
    WORD        skipNext;
    DWORD       codeAdr;
    int         spos, pos;
    String      name, methodName;

    if (!IsValidImageAdr(adr)) return;

    DWORD vmtAdr = adr - VmtSelfPtr;
    DWORD methodAdr = *((DWORD*)(Code + Adr2Pos(vmtAdr) + VmtMethodTable));
    if (!methodAdr) return;

    pos = Adr2Pos(methodAdr);
    WORD count = *((WORD*)(Code + pos)); pos += 2;

    for (int n = 0; n < count; n++)
    {
        spos = pos;
    	skipNext = *((WORD*)(Code + pos)); pos += 2;
        codeAdr = *((DWORD*)(Code + pos)); pos += 4;
        len = Code[pos]; pos++;
        name = String((char*)&Code[pos], len); pos += len;

        //as added   why this code was removed? 
        methodName = className + "." + name;
        DWORD pos1 = Adr2Pos(codeAdr);
        PInfoRec recN1 = GetInfoRec(codeAdr);
        if (!recN1)
        {
            recN1 = new InfoRec(pos1, ikRefine);
            recN1->SetName(methodName);
        }        
        //~

        Infos[Adr2Pos(adr)]->vmtInfo->AddMethod(false, 'M', -1, codeAdr, methodName);
        pos = spos + skipNext;
    }
    if (DelphiVersion >= 2010)
    {
        WORD exCount = *((WORD*)(Code + pos)); pos += 2;
        for (int n = 0; n < exCount; n++)
        {
            //Entry
            DWORD entry = *((DWORD*)(Code + pos)); pos += 4;
            //Flags
            pos += 2;
            //VirtualIndex
            pos += 2;
            spos = pos;
            //Entry
            pos = Adr2Pos(entry);
            skipNext = *((WORD*)(Code + pos)); pos += 2;
            codeAdr = *((DWORD*)(Code + pos)); pos += 4;
            len = Code[pos]; pos++;
            name = String((char*)&Code[pos], len); pos += len;
            Infos[Adr2Pos(adr)]->vmtInfo->AddMethod(false, 'M', -1, codeAdr, className + "." + name);
            pos = spos;
        }
    }
}
//---------------------------------------------------------------------------
typedef struct
{
    int     id;
    char    *typname;
    char    *msgname;
} MsgInfo, *PMsgMInfo;

MsgInfo  WindowsMsgTab[] =
{
{1, "WMCreate", "WM_CREATE"},
{2, "WMDestroy", "WM_DESTROY"},
{3, "WMMove", "WM_MOVE"},
{5, "WMSize", "WM_SIZE"},
{6, "WMActivate", "WM_ACTIVATE"},
{7, "WMSetFocus", "WM_SETFOCUS"},
{8, "WMKillFocus", "WM_KILLFOCUS"},
{0xA, "WMEnable", "WM_ENABLE"},
{0xB, "WMSetRedraw", "WM_SETREDRAW"},
{0xC, "WMSetText", "WM_SETTEXT"},
{0xD, "WMGetText", "WM_GETTEXT"},
{0xE, "WMGetTextLength", "WM_GETTEXTLENGTH"},
{0xF, "WMPaint", "WM_PAINT"},
{0x10, "WMClose", "WM_CLOSE"},
{0x11, "WMQueryEndSession", "WM_QUERYENDSESSION"},
{0x12, "WMQuit", "WM_QUIT"},
{0x13, "WMQueryOpen", "WM_QUERYOPEN"},
{0x14, "WMEraseBkgnd", "WM_ERASEBKGND"},
{0x15, "WMSysColorChange", "WM_SYSCOLORCHANGE"},
{0x16, "WMEndSession", "WM_ENDSESSION"},
{0x17, "WMSystemError", "WM_SYSTEMERROR"},
{0x18, "WMShowWindow", "WM_SHOWWINDOW"},
{0x19, "WMCtlColor", "WM_CTLCOLOR"},
{0x1A, "WMSettingChange", "WM_SETTINGCHANGE"},
{0x1B, "WMDevModeChange", "WM_DEVMODECHANGE"},
{0x1C, "WMActivateApp", "WM_ACTIVATEAPP"},
{0x1D, "WMFontChange", "WM_FONTCHANGE"},
{0x1E, "WMTimeChange", "WM_TIMECHANGE"},
{0x1F, "WMCancelMode", "WM_CANCELMODE"},
{0x20, "WMSetCursor", "WM_SETCURSOR"},
{0x21, "WMMouseActivate", "WM_MOUSEACTIVATE"},
{0x22, "WMChildActivate", "WM_CHILDACTIVATE"},
{0x23, "WMQueueSync", "WM_QUEUESYNC"},
{0x24, "WMGetMinMaxInfo", "WM_GETMINMAXINFO"},
{0x26, "WMPaintIcon", "WM_PAINTICON"},
{0x27, "WMEraseBkgnd", "WM_ICONERASEBKGND"},
{0x28, "WMNextDlgCtl", "WM_NEXTDLGCTL"},
{0x2A, "WMSpoolerStatus", "WM_SPOOLERSTATUS"},
{0x2B, "WMDrawItem", "WM_DRAWITEM"},
{0x2C, "WMMeasureItem", "WM_MEASUREITEM"},
{0x2D, "WMDeleteItem", "WM_DELETEITEM"},
{0x2E, "WMVKeyToItem", "WM_VKEYTOITEM"},
{0x2F, "WMCharToItem", "WM_CHARTOITEM"},
{0x30, "WMSetFont", "WM_SETFONT"},
{0x31, "WMGetFont", "WM_GETFONT"},
{0x32, "WMSetHotKey", "WM_SETHOTKEY"},
{0x33, "WMGetHotKey", "WM_GETHOTKEY"},
{0x37, "WMQueryDragIcon", "WM_QUERYDRAGICON"},
{0x39, "WMCompareItem", "WM_COMPAREITEM"},
//{0x3D, "?", "WM_GETOBJECT"},
{0x41, "WMCompacting", "WM_COMPACTING"},
//{0x44, "?", "WM_COMMNOTIFY"},
{0x46, "WMWindowPosChangingMsg", "WM_WINDOWPOSCHANGING"},
{0x47, "WMWindowPosChangedMsg", "WM_WINDOWPOSCHANGED"},
{0x48, "WMPower", "WM_POWER"},
{0x4A, "WMCopyData", "WM_COPYDATA"},
//{0x4B, "?", "WM_CANCELJOURNAL"},
{0x4E, "WMNotify", "WM_NOTIFY"},
//{0x50, "?", "WM_INPUTLANGCHANGEREQUEST"},
//{0x51, "?", "WM_INPUTLANGCHANGE"},
//{0x52, "?", "WM_TCARD"},
{0x53, "WMHelp", "WM_HELP"},
//{0x54, "?", "WM_USERCHANGED"},
{0x55, "WMNotifyFormat", "WM_NOTIFYFORMAT"},
{0x7B, "WMContextMenu", "WM_CONTEXTMENU"},
{0x7C, "WMStyleChanging", "WM_STYLECHANGING"},
{0x7D, "WMStyleChanged", "WM_STYLECHANGED"},
{0x7E, "WMDisplayChange", "WM_DISPLAYCHANGE"},
{0x7F, "WMGetIcon", "WM_GETICON"},
{0x80, "WMSetIcon", "WM_SETICON"},
{0x81, "WMNCCreate", "WM_NCCREATE"},
{0x82, "WMNCDestroy", "WM_NCDESTROY"},
{0x83, "WMNCCalcSize", "WM_NCCALCSIZE"},
{0x84, "WMNCHitTest", "WM_NCHITTEST"},
{0x85, "WMNCPaint", "WM_NCPAINT"},
{0x86, "WMNCActivate", "WM_NCACTIVATE"},
{0x87, "WMGetDlgCode", "WM_GETDLGCODE"},
{0xA0, "WMNCMouseMove", "WM_NCMOUSEMOVE"},
{0xA1, "WMNCLButtonDown", "WM_NCLBUTTONDOWN"},
{0xA2, "WMNCLButtonUp", "WM_NCLBUTTONUP"},
{0xA3, "WMNCLButtonDblClk", "WM_NCLBUTTONDBLCLK"},
{0xA4, "WMNCRButtonDown", "WM_NCRBUTTONDOWN"},
{0xA5, "WMNCRButtonUp", "WM_NCRBUTTONUP"},
{0xA6, "WMNCRButtonDblClk", "WM_NCRBUTTONDBLCLK"},
{0xA7, "WMNCMButtonDown", "WM_NCMBUTTONDOWN"},
{0xA8, "WMNCMButtonUp", "WM_NCMBUTTONUP"},
{0xA9, "WMNCMButtonDblClk", "WM_NCMBUTTONDBLCLK"},
{0x100, "WMKeyDown", "WM_KEYDOWN"},
{0x101, "WMKeyUp", "WM_KEYUP"},
{0x102, "WMChar", "WM_CHAR"},
{0x103, "WMDeadChar", "WM_DEADCHAR"},
{0x104, "WMSysKeyDown", "WM_SYSKEYDOWN"},
{0x105, "WMSysKeyUp", "WM_SYSKEYUP"},
{0x106, "WMSysChar", "WM_SYSCHAR"},
{0x107, "WMSysDeadChar", "WM_SYSDEADCHAR"},
//{0x108, "?", "WM_KEYLAST"},
//{0x10D, "?", "WM_IME_STARTCOMPOSITION"},
//{0x10E, "?", "WM_IME_ENDCOMPOSITION"},
//{0x10F, "?", "WM_IME_COMPOSITION"},
{0x110, "WMInitDialog", "WM_INITDIALOG"},
{0x111, "WMCommand", "WM_COMMAND"},
{0x112, "WMSysCommand", "WM_SYSCOMMAND"},
{0x113, "WMTimer", "WM_TIMER"},
{0x114, "WMHScroll", "WM_HSCROLL"},
{0x115, "WMVScroll", "WM_VSCROLL"},
{0x116, "WMInitMenu", "WM_INITMENU"},
{0x117, "WMInitMenuPopup", "WM_INITMENUPOPUP"},
{0x11F, "WMMenuSelect", "WM_MENUSELECT"},
{0x120, "WMMenuChar", "WM_MENUCHAR"},
{0x121, "WMEnterIdle", "WM_ENTERIDLE"},
//{0x122, "?", "WM_MENURBUTTONUP"},
//{0x123, "?", "WM_MENUDRAG"},
//{0x124, "?", "WM_MENUGETOBJECT"},
//{0x125, "?", "WM_UNINITMENUPOPUP"},
//{0x126, "?", "WM_MENUCOMMAND"},
{0x127, "WMChangeUIState", "WM_CHANGEUISTATE"},
{0x128, "WMUpdateUIState", "WM_UPDATEUISTATE"},
{0x129, "WMQueryUIState", "WM_QUERYUISTATE"},
{0x132, "WMCtlColorMsgBox", "WM_CTLCOLORMSGBOX"},
{0x133, "WMCtlColorEdit", "WM_CTLCOLOREDIT"},
{0x134, "WMCtlColorListBox", "WM_CTLCOLORLISTBOX"},
{0x135, "WMCtlColorBtn", "WM_CTLCOLORBTN"},
{0x136, "WMCtlColorDlg", "WM_CTLCOLORDLG"},
{0x137, "WMCtlColorScrollBar", "WM_CTLCOLORSCROLLBAR"},
{0x138, "WMCtlColorStatic", "WM_CTLCOLORSTATIC"},
{0x200, "WMMouseMove", "WM_MOUSEMOVE"},
{0x201, "WMLButtonDown", "WM_LBUTTONDOWN"},
{0x202, "WMLButtonUp", "WM_LBUTTONUP"},
{0x203, "WMLButtonDblClk", "WM_LBUTTONDBLCLK"},
{0x204, "WMRButtonDown", "WM_RBUTTONDOWN"},
{0x205, "WMRButtonUp", "WM_RBUTTONUP"},
{0x206, "WMRButtonDblClk", "WM_RBUTTONDBLCLK"},
{0x207, "WMMButtonDown", "WM_MBUTTONDOWN"},
{0x208, "WMMButtonUp", "WM_MBUTTONUP"},
{0x209, "WMMButtonDblClk", "WM_MBUTTONDBLCLK"},
{0x20A, "WMMouseWheel", "WM_MOUSEWHEEL"},
{0x210, "WMParentNotify", "WM_PARENTNOTIFY"},
{0x211, "WMEnterMenuLoop", "WM_ENTERMENULOOP"},
{0x212, "WMExitMenuLoop", "WM_EXITMENULOOP"},
//{0x213, "?", "WM_NEXTMENU"},
//{0x214, "?", "WM_SIZING"},
//{0x215, "?", "WM_CAPTURECHANGED"},
//{0x216, "?", "WM_MOVING"},
//{0x218, "?", "WM_POWERBROADCAST"},
//{0x219, "?", "WM_DEVICECHANGE"},
{0x220, "WMMDICreate", "WM_MDICREATE"},
{0x221, "WMMDIDestroy", "WM_MDIDESTROY"},
{0x222, "WMMDIActivate", "WM_MDIACTIVATE"},
{0x223, "WMMDIRestore", "WM_MDIRESTORE"},
{0x224, "WMMDINext", "WM_MDINEXT"},
{0x225, "WMMDIMaximize", "WM_MDIMAXIMIZE"},
{0x226, "WMMDITile", "WM_MDITILE"},
{0x227, "WMMDICascade", "WM_MDICASCADE"},
{0x228, "WMMDIIconArrange", "WM_MDIICONARRANGE"},
{0x229, "WMMDIGetActive", "WM_MDIGETACTIVE"},
{0x230, "WMMDISetMenu", "WM_MDISETMENU"},
//{0x231, "?", "WM_ENTERSIZEMOVE"},
//{0x232, "?", "WM_EXITSIZEMOVE"},
{0x233, "WMDropFiles", "WM_DROPFILES"},
{0x234, "WMMDIRefreshMenu", "WM_MDIREFRESHMENU"},
//{0x281, "?", "WM_IME_SETCONTEXT"},
//{0x282, "?", "WM_IME_NOTIFY"},
//{0x283, "?", "WM_IME_CONTROL"},
//{0x284, "?", "WM_IME_COMPOSITIONFULL"},
//{0x285, "?", "WM_IME_SELECT"},
//{0x286, "?", "WM_IME_CHAR"},
//{0x288, "?", "WM_IME_REQUEST"},
//{0x290, "?", "WM_IME_KEYDOWN"},
//{0x291, "?", "WM_IME_KEYUP"},
//{0x2A1, "?", "WM_MOUSEHOVER"},
//{0x2A3, "?", "WM_MOUSELEAVE"},
{0x300, "WMCut", "WM_CUT"},
{0x301, "WMCopy", "WM_COPY"},
{0x302, "WMPaste", "WM_PASTE"},
{0x303, "WMClear", "WM_CLEAR"},
{0x304, "WMUndo", "WM_UNDO"},
{0x305, "WMRenderFormat", "WM_RENDERFORMAT"},
{0x306, "WMRenderAllFormats", "WM_RENDERALLFORMATS"},
{0x307, "WMDestroyClipboard", "WM_DESTROYCLIPBOARD"},
{0x308, "WMDrawClipboard", "WM_DRAWCLIPBOARD"},
{0x309, "WMPaintClipboard", "WM_PAINTCLIPBOARD"},
{0x30A, "WMVScrollClipboard", "WM_VSCROLLCLIPBOARD"},
{0x30B, "WMSizeClipboard", "WM_SIZECLIPBOARD"},
{0x30C, "WMAskCBFormatName", "WM_ASKCBFORMATNAME"},
{0x30D, "WMChangeCBChain", "WM_CHANGECBCHAIN"},
{0x30E, "WMHScrollClipboard", "WM_HSCROLLCLIPBOARD"},
{0x30F, "WMQueryNewPalette", "WM_QUERYNEWPALETTE"},
{0x310, "WMPaletteIsChanging", "WM_PALETTEISCHANGING"},
{0x311, "WMPaletteChanged", "WM_PALETTECHANGED"},
{0x312, "WMHotKey", "WM_HOTKEY"},
//{0x317, "?", "WM_PRINT"},
//{0x318, "?", "WM_PRINTCLIENT"},
//{0x358, "?", "WM_HANDHELDFIRST"},
//{0x35F, "?", "WM_HANDHELDLAST"},
//{0x380, "?", "WM_PENWINFIRST"},
//{0x38F, "?", "WM_PENWINLAST"},
//{0x390, "?", "WM_COALESCE_FIRST"},
//{0x39F, "?", "WM_COALESCE_LAST"},
{0x3E0, "WMDDE_Initiate", "WM_DDE_INITIATE"},
{0x3E1, "WMDDE_Terminate", "WM_DDE_TERMINATE"},
{0x3E2, "WMDDE_Advise", "WM_DDE_ADVISE"},
{0x3E3, "WMDDE_UnAdvise", "WM_DDE_UNADVISE"},
{0x3E4, "WMDDE_Ack", "WM_DDE_ACK"},
{0x3E5, "WMDDE_Data", "WM_DDE_DATA"},
{0x3E6, "WMDDE_Request", "WM_DDE_REQUEST"},
{0x3E7, "WMDDE_Poke", "WM_DDE_POKE"},
{0x3E8, "WMDDE_Execute", "WM_DDE_EXECUTE"},
{0, ""}
};

MsgInfo VCLControlsMsgTab[] =
{
{0xB000, "CMActivate", "CM_ACTIVATE"},
{0xB001, "CMDeactivate", "CM_DEACTIVATE"},
{0xB002, "CMGotFocus", "CM_GOTFOCUS"},
{0xB003, "CMLostFocus", "CM_LOSTFOCUS"},
{0xB004, "CMCancelMode", "CM_CANCELMODE"},
{0xB005, "CMDialogKey", "CM_DIALOGKEY"},
{0xB006, "CMDialogChar", "CM_DIALOGCHAR"},
{0xB007, "CMFocusChenged", "CM_FOCUSCHANGED"},
{0xB008, "CMParentFontChanged", "CM_PARENTFONTCHANGED"},
{0xB009, "CMParentColorChanged", "CM_PARENTCOLORCHANGED"},
{0xB00A, "CMHitTest", "CM_HITTEST"},
{0xB00B, "CMVisibleChanged", "CM_VISIBLECHANGED"},
{0xB00C, "CMEnabledChanged", "CM_ENABLEDCHANGED"},
{0xB00D, "CMColorChanged", "CM_COLORCHANGED"},
{0xB00E, "CMFontChanged", "CM_FONTCHANGED"},
{0xB00F, "CMCursorChanged", "CM_CURSORCHANGED"},
{0xB010, "CMCtl3DChanged", "CM_CTL3DCHANGED"},
{0xB011, "CMParentCtl3DChanged", "CM_PARENTCTL3DCHANGED"},
{0xB012, "CMTextChanged", "CM_TEXTCHANGED"},
{0xB013, "CMMouseEnter", "CM_MOUSEENTER"},
{0xB014, "CMMouseLeave", "CM_MOUSELEAVE"},
{0xB015, "CMMenuChanged", "CM_MENUCHANGED"},
{0xB016, "CMAppKeyDown", "CM_APPKEYDOWN"},
{0xB017, "CMAppSysCommand", "CM_APPSYSCOMMAND"},
{0xB018, "CMButtonPressed", "CM_BUTTONPRESSED"},
{0xB019, "CMShowingChanged", "CM_SHOWINGCHANGED"},
{0xB01A, "CMEnter", "CM_ENTER"},
{0xB01B, "CMExit", "CM_EXIT"},
{0xB01C, "CMDesignHitTest", "CM_DESIGNHITTEST"},
{0xB01D, "CMIconChanged", "CM_ICONCHANGED"},
{0xB01E, "CMWantSpecialKey", "CM_WANTSPECIALKEY"},
{0xB01F, "CMInvokeHelp", "CM_INVOKEHELP"},
{0xB020, "CMWondowHook", "CM_WINDOWHOOK"},
{0xB021, "CMRelease", "CM_RELEASE"},
{0xB022, "CMShowHintChanged", "CM_SHOWHINTCHANGED"},
{0xB023, "CMParentShowHintChanged", "CM_PARENTSHOWHINTCHANGED"},
{0xB024, "CMSysColorChange", "CM_SYSCOLORCHANGE"},
{0xB025, "CMWinIniChange", "CM_WININICHANGE"},
{0xB026, "CMFontChange", "CM_FONTCHANGE"},
{0xB027, "CMTimeChange", "CM_TIMECHANGE"},
{0xB028, "CMTabStopChanged", "CM_TABSTOPCHANGED"},
{0xB029, "CMUIActivate", "CM_UIACTIVATE"},
{0xB02A, "CMUIDeactivate", "CM_UIDEACTIVATE"},
{0xB02B, "CMDocWindowActivate", "CM_DOCWINDOWACTIVATE"},
{0xB02C, "CMControlLIstChange", "CM_CONTROLLISTCHANGE"},
{0xB02D, "CMGetDataLink", "CM_GETDATALINK"},
{0xB02E, "CMChildKey", "CM_CHILDKEY"},
{0xB02F, "CMDrag", "CM_DRAG"},
{0xB030, "CMHintShow", "CM_HINTSHOW"},
{0xB031, "CMDialogHanlde", "CM_DIALOGHANDLE"},
{0xB032, "CMIsToolControl", "CM_ISTOOLCONTROL"},
{0xB033, "CMRecreateWnd", "CM_RECREATEWND"},
{0xB034, "CMInvalidate", "CM_INVALIDATE"},
{0xB035, "CMSysFontChanged", "CM_SYSFONTCHANGED"},
{0xB036, "CMControlChange", "CM_CONTROLCHANGE"},
{0xB037, "CMChanged", "CM_CHANGED"},
{0xB038, "CMDockClient", "CM_DOCKCLIENT"},
{0xB039, "CMUndockClient", "CM_UNDOCKCLIENT"},
{0xB03A, "CMFloat", "CM_FLOAT"},
{0xB03B, "CMBorderChanged", "CM_BORDERCHANGED"},
{0xB03C, "CMBiDiModeChanged", "CM_BIDIMODECHANGED"},
{0xB03D, "CMParentBiDiModeChanged", "CM_PARENTBIDIMODECHANGED"},
{0xB03E, "CMAllChildrenFlipped", "CM_ALLCHILDRENFLIPPED"},
{0xB03F, "CMActionUpdate", "CM_ACTIONUPDATE"},
{0xB040, "CMActionExecute", "CM_ACTIONEXECUTE"},
{0xB041, "CMHintShowPause", "CM_HINTSHOWPAUSE"},
{0xB044, "CMDockNotification", "CM_DOCKNOTIFICATION"},
{0xB043, "CMMouseWheel", "CM_MOUSEWHEEL"},
{0xB044, "CMIsShortcut", "CM_ISSHORTCUT"},
{0xB045, "CMRawX11Event", "CM_RAWX11EVENT"},
{0, "", ""}
};

PMsgMInfo __fastcall GetMsgInfo(WORD msg)
{
    //WindowsMsgTab
    if (msg < 0x400)
    {
        for (int m = 0;; m++)
        {
            if (!WindowsMsgTab[m].id) break;
            if (WindowsMsgTab[m].id == msg) return &WindowsMsgTab[m];
        }
    }
    //VCLControlsMsgTab
    if (msg >= 0xB000 && msg < 0xC000)
    {
        for (int m = 0;; m++)
        {
            if (!VCLControlsMsgTab[m].id) break;
            if (VCLControlsMsgTab[m].id == msg) return &VCLControlsMsgTab[m];
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ScanDynamicTable(DWORD adr)
{
    PInfoRec    recN, recN1, recN2;

    if (!IsValidImageAdr(adr)) return;

    recN = GetInfoRec(adr);

    if (!recN) return;

    DWORD vmtAdr = adr - VmtSelfPtr;
    DWORD pos = Adr2Pos(vmtAdr) + VmtDynamicTable;
    DWORD dynamicAdr = *((DWORD*)(Code + pos));
    if (!dynamicAdr) return;

    String className = GetClsName(adr);

    pos = Adr2Pos(dynamicAdr);
    WORD num = *((WORD*)(Code + pos)); pos += 2;
    DWORD post = pos + 2 * num;
    //First fill wellknown names
    for (int i = 0; i < num; i++)
    {
        WORD msg = *((WORD*)(Code + pos)); pos += 2;
        DWORD procAdr = *((DWORD*)(Code + post)); post += 4;
    	MethodRec recM;
        recM.abstract = false;
        recM.kind = 'D';
        recM.id = (int)msg;
        recM.address = procAdr;
        recM.name = "";

        recN1 = GetInfoRec(procAdr);
        if (recN1 && recN1->HasName())
            recM.name = recN1->GetName();
        else
        {
            PMsgMInfo _info = GetMsgInfo(msg);
            if (_info)
            {
                String typname = _info->typname;
                if (typname != "")
                {
                    if (!recN1) recN1 = new InfoRec(Adr2Pos(procAdr), ikRefine);
                    recM.name = className + "." + typname;
                    recN1->SetName(recM.name);
                }
            }
            if (recM.name == "")
            {
                DWORD parentAdr = GetParentAdr(adr);
                while (parentAdr)
                {
                    recN2 = GetInfoRec(parentAdr);
                    if (recN2)
                    {
                        DWORD vmtAdr1 = parentAdr - VmtSelfPtr;
                        DWORD pos1 = Adr2Pos(vmtAdr1) + VmtDynamicTable;
                        dynamicAdr = *((DWORD*)(Code + pos1));
                        if (dynamicAdr)
                        {
                            pos1 = Adr2Pos(dynamicAdr);
                            WORD num1 = *((WORD*)(Code + pos1)); pos1 += 2;
                            DWORD post1 = pos1 + 2 * num1;

                            for (int j = 0; j < num1; j++)
                            {
                                WORD msg1 = *((WORD*)(Code + pos1)); pos1 += 2;
                                DWORD procAdr1 = *((DWORD*)(Code + post1)); post1 += 4;
                                if (msg1 == msg)
                                {
                                    recN2 = GetInfoRec(procAdr1);
                                    if (recN2 && recN2->HasName())
                                    {
                                        int dpos = recN2->GetName().Pos(".");
                                        if (dpos)
                                            recM.name = className + recN2->GetName().SubString(dpos, recN2->GetNameLength() - dpos + 1);
                                        else
                                            recM.name = recN2->GetName();
                                    }
                                    break;
                                }
                            }
                            if (recM.name != "") break;
                        }
                    }
                    parentAdr = GetParentAdr(parentAdr);
                }
            }

            if (recM.name == "" || SameText(recM.name, "@AbstractError"))
            	recM.name = className + ".sub_" + Val2Str8(recM.address);

            recN1 = new InfoRec(Adr2Pos(procAdr), ikRefine);
            recN1->SetName(recM.name);
        }
        recN->vmtInfo->AddMethod(recM.abstract, recM.kind, recM.id, recM.address, recM.name);
    }
}
//---------------------------------------------------------------------------
bool __fastcall IsOwnVirtualMethod(DWORD vmtAdr, DWORD procAdr)
{
    DWORD parentAdr = GetParentAdr(vmtAdr);
    if (!parentAdr) return true;
    DWORD stopAt = GetStopAt(parentAdr - VmtSelfPtr);
    if (vmtAdr == stopAt) return false;

    int pos = Adr2Pos(parentAdr) + VmtParent + 4;

    for (int m = VmtParent + 4;; m += 4, pos += 4)
    {
        if (Pos2Adr(pos) == stopAt) break;

        if (*((DWORD*)(Code + pos)) == procAdr) return false;
    }
    return true;
}
//---------------------------------------------------------------------------
//Create recN->methods list
void __fastcall TFMain_11011981::ScanVirtualTable(DWORD adr)
{
    int         m, pos, idx;
    DWORD       vmtAdr, stopAt;
    String      clsName;
    PInfoRec    recN, recN1;
    MethodRec   recM;
    MProcInfo   aInfo;
    MProcInfo*  pInfo = &aInfo;

    if (!IsValidImageAdr(adr)) return;
    clsName = GetClsName(adr);
    vmtAdr = adr - VmtSelfPtr;
    stopAt = GetStopAt(vmtAdr);
    if (vmtAdr == stopAt) return;

    pos = Adr2Pos(vmtAdr) + VmtParent + 4;
    recN = GetInfoRec(vmtAdr + VmtSelfPtr);

    for (m = VmtParent + 4;; m += 4, pos += 4)
    {
        if (Pos2Adr(pos) == stopAt) break;

        recM.abstract = false;
        recM.address = *((DWORD*)(Code + pos));

        recN1 = GetInfoRec(recM.address);
        if (recN1 && recN1->HasName())
        {
            if (recN1->HasName())
            {
                if (!recN1->SameName("@AbstractError"))
                {
                    recM.name = recN1->GetName();
                }
                else
                {
                    recM.abstract = true;
                    recM.name = "";
                }
            }
        }
        else
        {
            recM.name = "";
            if (m == VmtFreeInstance)
                recM.name = clsName + "." + "FreeInstance";
            else if (m == VmtNewInstance)
                recM.name = clsName + "." + "NewInstance";
            else if (m == VmtDefaultHandler)
                recM.name = clsName + "." + "DefaultHandler";
            if (DelphiVersion == 3 && m == VmtSafeCallException)
                    recM.name = clsName + "." + "SafeCallException";
            if (DelphiVersion >= 4)
            {
                if (m == VmtSafeCallException)
                    recM.name = clsName + "." + "SafeCallException";
                else if (m == VmtAfterConstruction)
                    recM.name = clsName + "." + "AfterConstruction";
                else if (m == VmtBeforeDestruction)
                    recM.name = clsName + "." + "BeforeDestruction";
                else if (m == VmtDispatch)
                    recM.name = clsName + "." + "Dispatch";
            }
            if (DelphiVersion >= 2009)
            {
                if (m == VmtEquals)
                    recM.name = clsName + "." + "Equals";
                else if (m == VmtGetHashCode)
                    recM.name = clsName + "." + "GetHashCode";
                else if (m == VmtToString)
                    recM.name = clsName + "." + "ToString";
            }
            if (recM.name != "" && KnowledgeBase.GetKBProcInfo(recM.name, pInfo, &idx))
                StrapProc(Adr2Pos(recM.address), idx, pInfo, true, pInfo->DumpSz);
        }
        recN->vmtInfo->AddMethod(recM.abstract, 'V', m, recM.address, recM.name);
    }
}
//---------------------------------------------------------------------------
//Возвращает "высоту" класса (число родительских классов до 0)
int __fastcall TFMain_11011981::GetClassHeight(DWORD adr)
{
    int level = 0;
    while (1)
    {
        adr = GetParentAdr(adr);
        if (!adr) break;
        level++;
    }

    return level;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::PropagateVMTNames(DWORD adr)
{
    String  className = GetClsName(adr);
    PInfoRec recN = GetInfoRec(adr);

    DWORD vmtAdr = adr - VmtSelfPtr;
    DWORD stopAt = GetStopAt(vmtAdr);
    if (vmtAdr == stopAt) return;

    int pos = Adr2Pos(vmtAdr) + VmtParent + 4;
    for (int m = VmtParent + 4;; m += 4, pos += 4)
    {
        if (Pos2Adr(pos) == stopAt) break;

        DWORD procAdr = *((DWORD*)(Code + pos));
        PInfoRec recN1 = GetInfoRec(procAdr);
        if (!recN1) recN1 = new InfoRec(Adr2Pos(procAdr), ikRefine);

        if (!recN1->HasName())
        {
        	DWORD classAdr = adr;
            while (classAdr)
            {
            	PMethodRec recM = GetMethodInfo(classAdr, 'V', m);
                if (recM)
                {
                    String name = recM->name;
                    if (name != "")
                    {
                        int dotpos = name.Pos(".");
                        if (dotpos)
                            recN1->SetName(className + name.SubString(dotpos, name.Length()));
                        else
                            recN1->SetName(name);

                        PInfoRec recN2 = GetInfoRec(recM->address);
                        recN1->kind = recN2->kind;
                        if (!recN1->procInfo->args && recN2->procInfo->args)
                        {
                        	recN1->procInfo->flags |= recN2->procInfo->flags & 7;
                            //Get Arguments
                            for (int n = 0; n < recN2->procInfo->args->Count; n++)
                            {
                                PARGINFO argInfo2 = (PARGINFO)recN2->procInfo->args->Items[n];
                                ARGINFO argInfo;
                                argInfo.Tag = argInfo2->Tag;
                                argInfo.Register = argInfo2->Register;
                                argInfo.Ndx = argInfo2->Ndx;
                                argInfo.Size = 4;
                                argInfo.Name = argInfo2->Name;
                                argInfo.TypeDef = TrimTypeName(argInfo2->TypeDef);
                                recN1->procInfo->AddArg(&argInfo);
                            }
                        }
                        recN->vmtInfo->AddMethod(false, 'V', m, procAdr, recN1->GetName());
                        break;
                    }
                }
                classAdr = GetParentAdr(classAdr);
            }
        }
    }
}
//---------------------------------------------------------------------------
PMethodRec __fastcall TFMain_11011981::GetMethodInfo(DWORD adr, char kind, int methodOfs)
{
    if (!IsValidCodeAdr(adr)) return 0;

    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->id == methodOfs && recM->kind == kind) return recM;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall ClearClassAdrMap()
{
    classAdrMap.clear();
}
//---------------------------------------------------------------------------
DWORD __fastcall FindClassAdrByName(const String& AName)
{
    TClassAdrMap::const_iterator it = classAdrMap.find(AName);
    if (it != classAdrMap.end()) return it->second;

    return 0;
}
//---------------------------------------------------------------------------
void __fastcall AddClassAdr(DWORD Adr, const String& AName)
{
    classAdrMap[AName] = Adr;
}
//---------------------------------------------------------------------------
//Возвращает общий родительский тип для типов Name1, Name2
String __fastcall TFMain_11011981::GetCommonType(String Name1, String Name2)
{
	if (SameText(Name1, Name2)) return Name1;
    
	DWORD adr1 = GetClassAdr(Name1);
    DWORD adr2 = GetClassAdr(Name2);
    //Synonims
    if (!adr1 || !adr2)
    {
        //dword and ClassName -> ClassName
        if (SameText(Name1, "Dword") && IsValidImageAdr(GetClassAdr(Name2))) return Name2;
        if (SameText(Name2, "Dword") && IsValidImageAdr(GetClassAdr(Name1))) return Name1;
        if (DelphiVersion >= 2009)
        {
            //UString - UnicodeString
        	if ((SameText(Name1, "UString") && SameText(Name2, "UnicodeString")) ||
            	(SameText(Name1, "UnicodeString") && SameText(Name2, "UString"))) return "UnicodeString";
        }
    	//String - AnsiString
    	if ((SameText(Name1, "String") && SameText(Name2, "AnsiString")) ||
        	(SameText(Name1, "AnsiString") && SameText(Name2, "String"))) return "AnsiString";
        //Text - TTextRec
        if ((SameText(Name1, "Text") && SameText(Name2, "TTextRec")) ||
        	(SameText(Name1, "TTextRec") && SameText(Name2, "Text"))) return "TTextRec";
        return "";
    }
    
	int h1 = GetClassHeight(adr1);
    int h2 = GetClassHeight(adr2);

    while (h1 != h2)
    {
    	if (h1 > h2)
        {
        	adr1 = GetParentAdr(adr1);
            h1--;
        }
        else
        {
        	adr2 = GetParentAdr(adr2);
            h2--;
        }
    }

    while (adr1 != adr2)
    {
    	adr1 = GetParentAdr(adr1);
        adr2 = GetParentAdr(adr2);
    }

    return GetClsName(adr1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormCreate(TObject *Sender)
{
    int         n;
    TMenuItem*  mi;
    LPCSTR      buf[256];

    if (!Disasm.Init())
    {
        ShowMessage("Cannot initialize Disasm");
        Application->Terminate();
    }

    AppDir = ExtractFilePath(Application->ExeName);
    if (AppDir[AppDir.Length()] != '\\') AppDir += "\\";
    Application->HelpFile = AppDir + "idr.chm";
    IniFileRead();

    miDelphi2->Enabled = FileExists(AppDir + "kb2.bin");
    miDelphi3->Enabled = FileExists(AppDir + "kb3.bin");
    miDelphi4->Enabled = FileExists(AppDir + "kb4.bin");
    miDelphi5->Enabled = FileExists(AppDir + "kb5.bin");
    miDelphi6->Enabled = FileExists(AppDir + "kb6.bin");
    miDelphi7->Enabled = FileExists(AppDir + "kb7.bin");
    miDelphi2005->Enabled = FileExists(AppDir + "kb2005.bin");
    miDelphi2006->Enabled = FileExists(AppDir + "kb2006.bin");
    miDelphi2007->Enabled = FileExists(AppDir + "kb2007.bin");
    miDelphi2009->Enabled = FileExists(AppDir + "kb2009.bin");
    miDelphi2010->Enabled = FileExists(AppDir + "kb2010.bin");
    miDelphiXE1->Enabled = FileExists(AppDir + "kb2011.bin");
    miDelphiXE2->Enabled = FileExists(AppDir + "kb2012.bin");
    miDelphiXE3->Enabled = FileExists(AppDir + "kb2013.bin");
    miDelphiXE4->Enabled = FileExists(AppDir + "kb2014.bin");

    SegmentList = new TList;
    ExpFuncList = new TList;
    ImpFuncList = new TList;
    ImpModuleList = new TStringList;
    VmtList = new TList;
    ResInfo = new TResourceInfo;
    Units = new TList;
    OwnTypeList = new TList;
    UnitsSearchList = new TStringList;
    RTTIsSearchList = new TStringList;
    UnitItemsSearchList = new TStringList;
    VMTsSearchList = new TStringList;
    FormsSearchList = new TStringList;
    StringsSearchList = new TStringList;
    NamesSearchList = new TStringList;

    Init();

    lbUnits->Canvas->Font->Assign(lbUnits->Font);
    lbRTTIs->Canvas->Font->Assign(lbRTTIs->Font);
    lbForms->Canvas->Font->Assign(lbForms->Font);
    lbCode->Canvas->Font->Assign(lbCode->Font);
    lbUnitItems->Canvas->Font->Assign(lbUnitItems->Font);
    lbSourceCode->Canvas->Font->Assign(lbSourceCode->Font);

    lbCXrefs->Canvas->Font->Assign(lbCXrefs->Font);
    lbCXrefs->Width = lbCXrefs->Canvas->TextWidth("T")*14;
    ShowCXrefs->Width = lbCXrefs->Width;

    lbSXrefs->Canvas->Font->Assign(lbSXrefs->Font);
    lbSXrefs->Width = lbSXrefs->Canvas->TextWidth("T")*14;
    ShowSXrefs->Width = lbSXrefs->Width;

    lbNXrefs->Canvas->Font->Assign(lbNXrefs->Font);
    lbNXrefs->Width = lbNXrefs->Canvas->TextWidth("T")*14;
    ShowNXrefs->Width = lbNXrefs->Width;
/*
//----Highlighting-----------------------------------------------------------
	if (InitHighlight())
    {
        lbSourceCode->Style = lbOwnerDrawFixed;
        DelphiLbId          = CreateHighlight(lbSourceCode->Handle, ID_DELPHI);
        DelphiThemesCount   = GetThemesCount(ID_DELPHI);
        for (n = 0; n < DelphiThemesCount; n++)
        {
            mi = new TMenuItem(pmCode->Items);
            GetThemeName(ID_DELPHI, (DWORD)n,(LPCSTR)buf, 256);
            mi->Caption = String((char *)buf);
            mi->Tag = n;
            mi->OnClick = ChangeDelphiHighlightTheme;
            miDelphiAppearance->Add(mi);
        }
	};
//----Highlighting-----------------------------------------------------------
*/
    ScaleForm(this);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormDestroy(TObject *Sender)
{
    CloseProject();

    delete SegmentList;
    delete ExpFuncList;
    delete ImpFuncList;
    delete ImpModuleList;
    delete VmtList;
    delete ResInfo;
    delete OwnTypeList;
    delete UnitsSearchList;
    delete RTTIsSearchList;
    delete UnitItemsSearchList;
    delete VMTsSearchList;
    delete FormsSearchList;
    delete StringsSearchList;
    delete NamesSearchList;

    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        delete recU->names;
    }
    delete Units;
/*
//----Highlighting-----------------------------------------------------------
    if (DeleteHighlight)
    {
        DeleteHighlight(DelphiLbId);
    }
    FreeHighlight();
//----Highlighting-----------------------------------------------------------
*/
}
//---------------------------------------------------------------------------
String __fastcall GetFilenameFromLink(String LinkName)
{
    String          result = "";
    IPersistFile*   ppf;
    IShellLink*     pshl;
    WIN32_FIND_DATA wfd;

    //Initialize COM-library
    CoInitialize(NULL);
    //Create COM-object and get pointer to interface IPersistFile
    CoCreateInstance(CLSID_ShellLink, 0, CLSCTX_INPROC_SERVER, IID_IPersistFile, (void**)(&ppf));
    //Load Shortcut
    wchar_t* temp = new WCHAR[MAX_PATH];
    StringToWideChar(LinkName, temp, MAX_PATH);
    ppf->Load(temp, STGM_READ);
    delete[] temp;
    
    //Get pointer to IShellLink
    ppf->QueryInterface(IID_IShellLink, (void**)(&pshl));
    //Find Object shortcut points to
    pshl->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI);
    //Get Object name
    char* targetName = new char[MAX_PATH];
    pshl->GetPath(targetName, MAX_PATH, &wfd, 0);
    result = String(targetName);
    delete[] targetName;

    pshl->Release();
    ppf->Release();

    CoFreeUnusedLibraries();
    CoUninitialize();
    
    return result;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormShow(TObject *Sender)
{
    if (ParamCount() > 0)
    {
        String FileName = ParamStr(1);
        String fileExtension = ExtractFileExt(FileName);
        //Shortcut
        if (SameText(fileExtension, ".lnk"))
        {
            FileName = GetFilenameFromLink(FileName);
        }

        if (IsExe(FileName))
        {
            LoadDelphiFile1(FileName, DELHPI_VERSION_AUTO, true, true);
        }
        else if (IsIdp(FileName))
        {
            OpenProject(FileName);
        }
        else
        {
            ShowMessage("File " + FileName + " is not executable or IDR project file");
        }
    }
}
//---------------------------------------------------------------------------
/*
void __fastcall TFMain_11011981::ScanImports()
{
    String  name;
    int *cnt = new int[KnowledgeBase.ModuleCount];
    //Попробуем сканировать интервал адресов безымянных модулей по именам импортируемых функций
    for (int m = 0; m < UnitsNum; m++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[m];
        if (!recU->names->Count)
        {
            memset(cnt, 0, KnowledgeBase.ModuleCount*sizeof(int));

            int fromPos = Adr2Pos(recU->fromAdr);
            int toPos = Adr2Pos(recU->toAdr);
            int totcnt = 0;

            for (int n = fromPos; n < toPos; n++)
            {
                PInfoRec recN = GetInfoRec(Pos2Adr(n));
                if (recN && IsFlagSet(cfImport, n))
                {
                    name = ExtractProcName(recN->name);
                    KnowledgeBase.GetModuleIdsByProcName(name.c_str());
                    for (int k = 0;;k++)
                    {
                        if (KnowledgeBase.Mods[k] == 0xFFFF) break;
                        cnt[KnowledgeBase.Mods[k]]++;
                        totcnt++;
                    }
                }
            }

            if (totcnt)
            {
                int num = 0; WORD id;
                for (int k = 0; k < KnowledgeBase.ModuleCount; k++)
                {
                    if (cnt[k] == totcnt)
                    {
                        id = k;
                        num++;
                    }
                }
                DWORD iniadr; PInfoRec recN;
                //Если все импорты нашлись только в одном юните, значит это он и есть
                if (num == 1)
                {
                    name = KnowledgeBase.GetModuleName(id);
                    if (recU->names->IndexOf(name) == -1)
                    {
                        recU->kb = true;
                        SetUnitName(recU, name);
                    }
                }
                //Если в нескольких, попробуем поискать процедуры по cfProcStart (если таковые имеются)
                else
                {
                    for (int k = 0; k < KnowledgeBase.ModuleCount; k++)
                    {
                        if (cnt[k] == totcnt)
                        {
                            id = k;
                            int FirstProcIdx, LastProcIdx;
                            if (!KnowledgeBase.GetProcIdxs(id, &FirstProcIdx, &LastProcIdx)) continue;

                            for (int m = fromPos; m < toPos; m++)
                            {
                                if (IsFlagSet(cfProcStart, m) || !Flags[m])
                                {
                                    for (int Idx = FirstProcIdx; Idx <= LastProcIdx; Idx++)
                                    {
                                        Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                        if (!KnowledgeBase.IsUsedProc(Idx))
                                        {
                                            MProcInfo *pInfo = KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS);
                                            //Находим совпадение кода
                                            if (KnowledgeBase.MatchCode(Code + m, pInfo) && StrapCheck(m, pInfo))
                                            {
                                                name = KnowledgeBase.GetModuleName(id);
                                                if (recU->names->IndexOf(name) == -1)
                                                {
                                                    recU->kb = true;
                                                    SetUnitName(recU, name);
                                                }
                                                StrapProc(m, Idx, pInfo, true, pInfo->DumpSz);
                                            }
                                            //as if (pInfo) delete pInfo;
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
    delete[] cnt;
}
*/
//---------------------------------------------------------------------------
String __fastcall TFMain_11011981::MakeComment(PPICODE Picode)
{
    bool	vmt;
    DWORD	vmtAdr;
	String	comment = "";

    if (Picode->Op == OP_CALL || Picode->Op == OP_COMMENT)
    {
    	comment = Picode->Name;
    }
    else
    {
        PFIELDINFO fInfo = GetField(Picode->Name, Picode->Ofs.Offset, &vmt, &vmtAdr, "");
        if (fInfo)
        {
            comment = Picode->Name + ".";
            if (fInfo->Name == "")
                comment += "?f" + Val2Str0(Picode->Ofs.Offset);
            else
                comment += fInfo->Name;
            comment += ":";
            if (fInfo->Type == "")
                comment += "?";
            else
                comment += TrimTypeName(fInfo->Type);

            if (!vmt) delete fInfo;
        }
        else if (Picode->Name != "")
        {
            comment = Picode->Name + ".?f" + Val2Str0(Picode->Ofs.Offset) + ":?";
        }
    }
    return comment;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::RedrawCode()
{
    DWORD adr = CurProcAdr;
    CurProcAdr = 0;
    ShowCode(adr, lbCode->ItemIndex, lbCXrefs->ItemIndex, lbCode->TopIndex);
}
//---------------------------------------------------------------------------
//MXXXXXXXX    textF
//M:<,>,=
//XXXXXXXX - adr
//F - flags
int __fastcall TFMain_11011981::AddAsmLine(DWORD Adr, String text, BYTE Flags)
{
    String _line = " " + Val2Str8(Adr) + "        " + text + " ";
    if (Flags & 1) _line[1] = '>';
    if (Flags & 8) _line[10] = '>';
        
    int _len = _line.Length();
    _line[_len] = Flags;

    lbCode->Items->Add(_line);
    return lbCode->Canvas->TextWidth(_line);
}
//Argument SelectedIdx can be address (for selection) and index of list
void __fastcall TFMain_11011981::ShowCode(DWORD fromAdr, int SelectedIdx, int XrefIdx, int topIdx)
{
    BYTE        op, flags;
    int         row = 0, wid, maxwid = 0, _pos, _idx, _ap;
    TCanvas*    canvas = lbCode->Canvas;
    int			num, instrLen, instrLen1, instrLen2, _procSize;
    DWORD       Adr, Adr1, Pos, lastMovAdr = 0;
    int         fromPos;
    int         curPos;
    DWORD       curAdr;
    DWORD       lastAdr = 0;
    String      line, line1;
    PInfoRec    recN;
    DISINFO     DisInfo, DisInfo1;
    char        disLine[100];

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return;

    bool selectByAdr = (IsValidImageAdr(SelectedIdx) == true);
    //If procedure is the same then move selection and not update Xrefs
    if (fromAdr == CurProcAdr)
    {
        if (selectByAdr)
        {
            for (int i = 1; i < lbCode->Items->Count; i++)
            {
                line = lbCode->Items->Strings[i];
                sscanf(line.c_str() + 1, "%lX", &Adr);
                if (Adr >= SelectedIdx)
                {
                    if (Adr == SelectedIdx)
                    {
                        lbCode->ItemIndex = i;
                        break;
                    }
                    else
                    {
                        lbCode->ItemIndex = i - 1;
                        break;
                    }
                }
            }
        }
        else
            lbCode->ItemIndex = SelectedIdx;

        pcWorkArea->ActivePage = tsCodeView;
        return;
    }
    if (!AnalyzeThread)//Clear all Items (used in highlighting)
    {
        //AnalyzeProc1(fromAdr, 0, 0, 0, false);//!!!
        AnalyzeProc2(fromAdr, false, false);
    }

    CurProcAdr = fromAdr;
    CurProcSize = 0;
	lbCode->Clear();
    lbCode->Items->BeginUpdate();

    recN = GetInfoRec(fromAdr);

    int outRows = MAX_DISASSEMBLE;
    if (IsFlagSet(cfImport, fromPos)) outRows = 2;

    line = " ";
    if (fromAdr == EP)
    {
        line += "EntryPoint";
    }
    else
    {
        String moduleName = "";
        String procName = "";

        PUnitRec recU = GetUnit(fromAdr);
        if (recU)
        {
            moduleName = GetUnitName(recU);
            if (fromAdr == recU->iniadr)
                procName = "Initialization";
            else if (fromAdr == recU->finadr)
                procName = "Finalization";
        }
        if (recN && procName == "") procName = recN->MakeMapName(fromAdr);

        if (moduleName != "")
            line += moduleName + "." + procName;
        else
            line += procName;
    }
    lProcName->Caption = line;
    lbCode->Items->Add(line); row++;

    _procSize = GetProcSize(fromAdr);
    curPos = fromPos; curAdr = fromAdr;

    while (row < outRows)
    {
        //End of procedure
        if (curAdr != fromAdr && _procSize && curAdr - fromAdr >= _procSize) break;
        //Loc?
        flags = ' ';
        if (curAdr != CurProcAdr && IsFlagSet(cfLoc, curPos))
            flags |= 1;
        if (IsFlagSet(cfFrame | cfSkip, curPos))
            flags |= 2;
        if (IsFlagSet(cfLoop, curPos))
            flags |= 4;

        //If exception table, output it
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            wid = AddAsmLine(curAdr, "dd          " + String(num), flags); row++;
            if (wid > maxwid) maxwid = wid;

            curPos += 4; curAdr += 4;

            for (int k = 0; k < num; k++)
            {
                //dd offset ExceptionInfo
                Adr = *((DWORD*)(Code + curPos));
                line1 = "dd          " + Val2Str8(Adr);
                //Name of Exception
                if (IsValidCodeAdr(Adr))
                {
                    recN = GetInfoRec(Adr);
                    if (recN && recN->HasName()) line1 += ";" + recN->GetName();
                }
                wid = AddAsmLine(curAdr, line1, flags); row++;
                if (wid > maxwid) maxwid = wid;

                //dd offset ExceptionProc
                curPos += 4; curAdr += 4;
                Adr = *((DWORD*)(Code + curPos));
                wid = AddAsmLine(curAdr, "dd          " + Val2Str8(Adr), flags); row++;
                if (wid > maxwid) maxwid = wid;

                curPos += 4; curAdr += 4;
            }
            continue;
        }

        BYTE b1 = Code[curPos];
        BYTE b2 = Code[curPos + 1];
        if (!b1 && !b2 && !lastAdr) break;

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, disLine);
        if (!instrLen)
        {
            wid = AddAsmLine(curAdr, "???", 0x22); row++;
            if (wid > maxwid) maxwid = wid;
            curPos++; curAdr++;
            continue;
        }
        op = Disasm.GetOp(DisInfo.Mnem);

        //Check inside instruction Fixup or ThreadVar
        bool NameInside = false; DWORD NameInsideAdr;
        for (int k = 1; k < instrLen; k++)
        {
            if (Infos[curPos + k])
            {
                NameInside = true;
                NameInsideAdr= curAdr + k;
                break;
            }
        }

        line = String(disLine);

        if (curAdr >= lastAdr) lastAdr = 0;

        //Proc end
        if (DisInfo.Ret && (!lastAdr || curAdr == lastAdr))
        {
            wid = AddAsmLine(curAdr, line, flags); row++;
            if (wid > maxwid) maxwid = wid;
            break;
        }

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        if (b1 == 0xEB ||				 //short relative abs jmp or cond jmp
        	(b1 >= 0x70 && b1 <= 0x7F) ||
            (b1 == 0xF && b2 >= 0x80 && b2 <= 0x8F))
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                if (op == OP_JMP)
                {
                    _ap = Adr2Pos(Adr);
                    recN = GetInfoRec(Adr);
                    if (recN && IsFlagSet(cfProcStart, _ap) && recN->HasName())
                    {
                        line = "jmp         " + recN->GetName();
                    }
                }
                flags |= 8;
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            wid = AddAsmLine(curAdr, line, flags); row++;
            if (wid > maxwid) maxwid = wid;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                _ap = Adr2Pos(Adr);
                recN = GetInfoRec(Adr);
                if (recN && IsFlagSet(cfProcStart, _ap) && recN->HasName())
                {
                    line = "jmp         " + recN->GetName();
                }
                flags |= 8;
                if (!recN && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            wid = AddAsmLine(curAdr, line, flags); row++;
            if (wid > maxwid) maxwid = wid;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        
        if (DisInfo.Call)  //call sub_XXXXXXXX
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                if (recN && recN->HasName())
                {
                    line = "call        " + recN->GetName();
                    //Found @Halt0 - exit
                    if (recN->SameName("@Halt0") && fromAdr == EP && !lastAdr)
                    {
                        wid = AddAsmLine(curAdr, line, flags); row++;
                        if (wid > maxwid) maxwid = wid;
                        break;
                    }
                }
            }
            recN = GetInfoRec(curAdr);
            if (recN && recN->picode) line += ";" + MakeComment(recN->picode);
            wid = AddAsmLine(curAdr, line, flags); row++;
            if (wid > maxwid) maxwid = wid;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            wid = AddAsmLine(curAdr, line, flags); row++;
            if (wid > maxwid) maxwid = wid;
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            /*
            //First instruction
            if (curAdr == fromAdr) break;
            */
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Table address - last 4 bytes of instruction
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Analyze address range to find table cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //If exist cTblAdr, skip this table
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                for (int k = 0; k < CNum; k++)
                {
                    BYTE db = Code[Pos];
                    CTab[k] = db;
                    wid = AddAsmLine(Adr, "db          " + String(db), 0x22); row++;
                    if (wid > maxwid) maxwid = wid;
                    Pos++; Adr++;
                }
            }
            //Check transitions by negative register values (in Delphi 2009)
            //bool neg = false;
            //Adr1 = *((DWORD*)(Code + Pos - 4));
            //if (IsValidCodeAdr(Adr1) && IsFlagSet(cfLoc, Adr2Pos(Adr1))) neg = true;

            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidCodeAdr(Adr1) || Adr1 < fromAdr) break;

                //Add row to assembler listing
                wid = AddAsmLine(Adr, "dd          " + Val2Str8(Adr1), 0x22); row++;
                if (wid > maxwid) maxwid = wid;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }
//----------------------------------
//PosTry: xor reg, reg
//        push ebp
//        push offset @1
//        push fs:[reg]
//        mov fs:[reg], esp
//        ...
//@2:     ...
//At @1 various variants:
//----------------------------------
//@1:     jmp @HandleFinally
//        jmp @2
//----------------------------------
//@1:     jmp @HandleAnyException
//        call DoneExcept
//----------------------------------
//@1:     jmp HandleOnException
//        dd num
//Далее таблица из num записей вида
//        dd offset ExceptionInfo
//        dd offset ExceptionProc
//----------------------------------
        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    if (Pos >= 0)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    /*
                                    //@2
                                    Adr1 = DisInfo.Immediate - 4;
                                    Adr = *((DWORD*)(Code + Adr2Pos(Adr1)));
                                    if (IsValidCodeAdr(Adr) && Adr > lastAdr) lastAdr = Adr;
                                    */
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Set flag cfETable to correct output data
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                }
                wid = AddAsmLine(curAdr, line, flags); row++;
                if (wid > maxwid) maxwid = wid;
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }

        //Name inside instruction (Fixip, ThreadVar)
        String namei = "", comment = "", name, pname, type, ptype;
        if (NameInside)
        {
            recN = GetInfoRec(NameInsideAdr);
            if (recN && recN->HasName())
            {
                namei += recN->GetName();
                if (recN->type != "") namei +=  ":" + recN->type;
            }
        }
        //comment
        recN = GetInfoRec(curAdr);
        if (recN && recN->picode) comment = MakeComment(recN->picode);

        DWORD targetAdr = 0;
        if (IsValidImageAdr(DisInfo.Immediate))
        {
        	if (!IsValidImageAdr(DisInfo.Offset)) targetAdr = DisInfo.Immediate;
        }
        else if (IsValidImageAdr(DisInfo.Offset))
        	targetAdr = DisInfo.Offset;

        if (targetAdr)
        {
            name = pname = type = ptype = "";
            _pos = Adr2Pos(targetAdr);
            if (_pos >= 0)
            {
                recN = GetInfoRec(targetAdr);
                if (recN)
                {
                    if (recN->kind == ikResString)
                    {
                        name = recN->GetName() + ":PResStringRec";
                    }
                    else
                    {
                        if (recN->HasName())
                        {
                            name = recN->GetName();
                            if (recN->type != "") type = recN->type;
                        }
                        else if (IsFlagSet(cfProcStart, _pos))
                            name = GetDefaultProcName(targetAdr);
                    }
                }
                //For Delphi2 pointers to VMT are distinct
                else if (DelphiVersion == 2)
                {
                    recN = GetInfoRec(targetAdr + VmtSelfPtr);
                    if (recN && recN->kind == ikVMT && recN->HasName())
                    {
                        name = recN->GetName();
                    }
                }
                Adr = *((DWORD*)(Code + _pos));
                if (IsValidImageAdr(Adr))
                {
                    recN = GetInfoRec(Adr);
                    if (recN)
                    {
                        if (recN->HasName())
                        {
                            pname = recN->GetName();
                            ptype = recN->type;
                        }
                        else if (IsFlagSet(cfProcStart, _pos))
                            pname = GetDefaultProcName(Adr);
                    }
                }
            }
            else
            {
                _idx = BSSInfos->IndexOf(Val2Str8(targetAdr));
                if (_idx != -1)
                {
                    recN = (PInfoRec)BSSInfos->Objects[_idx];
                    name = recN->GetName();
                    type = recN->type;
                }
            }
        }
        if (SameText(comment, name)) name = "";
        if (pname != "")
        {
            if (comment != "") comment += " ";
            comment += "^" + pname;
            if (ptype != "") comment += ":" + ptype;
        }
        else if (name != "")
        {
            if (comment != "") comment += " ";
           	comment += name;
            if (type != "") comment += ":" + type;
        }

        if (comment != "" || namei != "")
        {
            line += ";";
            if (comment != "") line += comment;
            if (namei != "") line += "{" + namei + "}";
        }
        if (line.Length() > MAXLEN) line = line.SubString(1, MAXLEN) + "...";
        wid = AddAsmLine(curAdr, line, flags); row++;
        if (wid > maxwid) maxwid = wid;
        curPos += instrLen; curAdr += instrLen;
    }

    CurProcSize = (curAdr + instrLen) - CurProcAdr;

    if (selectByAdr)
    {
        for (int i = 1; i < lbCode->Items->Count; i++)
        {
            line = lbCode->Items->Strings[i];
            sscanf(line.c_str() + 1, "%lX", &Adr);
            if (Adr >= SelectedIdx)
            {
                if (Adr == SelectedIdx)
                {
                    lbCode->ItemIndex = i;
                    break;
                }
                else
                {
                    lbCode->ItemIndex = i - 1;
                    break;
                }
            }
        }
    }
    else
        lbCode->ItemIndex = SelectedIdx;

    if (topIdx != -1) lbCode->TopIndex = topIdx;
    lbCode->ItemHeight = lbCode->Canvas->TextHeight("T");
    lbCode->ScrollWidth = maxwid + 2;
    lbCode->Items->EndUpdate();

    ShowCodeXrefs(CurProcAdr, XrefIdx);
    pcWorkArea->ActivePage = tsCodeView;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AnalyzeMethodTable(int Pass, DWORD Adr, const bool* Terminated)
{
    BYTE        sLen, paramFlags, paramCount, cc;
    WORD        skipNext, dw, parOfs;
    int         procPos;
    DWORD       procAdr, paramType, resultType;
    PInfoRec    recN;
    String      paramName, methodName;
    DWORD vmtAdr = Adr - VmtSelfPtr;
    DWORD methodAdr = *((DWORD*)(Code + Adr2Pos(vmtAdr) + VmtMethodTable));

    if (!methodAdr) return;

	String className = GetClsName(Adr);
    int pos = Adr2Pos(methodAdr);
    WORD count = *((WORD*)(Code + pos)); pos += 2;

    for (int n = 0; n < count && !*Terminated; n++)
    {
        skipNext = *((WORD*)(Code + pos));
        procAdr = *((DWORD*)(Code + pos + 2));
        procPos = Adr2Pos(procAdr);
        sLen = Code[pos + 6];
        methodName = String((char*)(Code + pos + 7), sLen);

        AnalyzeProc(Pass, procAdr);

        if (Pass == 1)
        {
            recN = GetInfoRec(procAdr);
            if (recN && recN->kind != ikConstructor && recN->kind != ikDestructor && recN->kind != ikClassRef)
            {
                recN->SetName(className + "." + methodName);
                recN->kind = ikProc;
                recN->AddXref('D', Adr, 0);
                recN->procInfo->AddArg(0x21, 0, 4, "Self", className);
            }
        }
        pos += skipNext;
    }
    if (DelphiVersion >= 2010)
    {
        WORD exCount = *((WORD*)(Code + pos)); pos += 2;
        for (int n = 0; n < exCount && !*Terminated; n++)
        {
            DWORD methodEntry = *((DWORD*)(Code + pos)); pos += 4;
            WORD flags = *((WORD*)(Code + pos)); pos += 2;
            WORD vIndex = *((WORD*)(Code + pos)); pos += 2;
            int spos = pos;
            pos = Adr2Pos(methodEntry);
            //Length
            skipNext = *((WORD*)(Code + pos)); pos += 2;
            procAdr = *((DWORD*)(Code + pos)); pos += 4;
            procPos = Adr2Pos(procAdr);
            sLen = Code[pos];
            methodName = String((char*)(Code + pos + 1), sLen); pos += sLen + 1;

            if (procAdr == Adr) continue;

            recN = GetInfoRec(procAdr);
            //IMHO it means that methods are pure virtual calls and must be readed in child classes
            if (recN && recN->kind == ikVMT)
            {
                pos = spos;
                continue;
            }
            AnalyzeProc(Pass, procAdr);
            recN = GetInfoRec(procAdr);

            if (Pass == 1)
            {
                if (recN && recN->procInfo && recN->kind != ikConstructor && recN->kind != ikDestructor)//recN->kind != ikClassRef
                {
                    recN->SetName(className + "." + methodName);
                    recN->kind = ikProc;
                    recN->AddXref('D', Adr, 0);
                    recN->procInfo->AddArg(0x21, 0, 4, "Self", className);
                }
            }
            if (pos - Adr2Pos(methodEntry) < skipNext)
            {
                //Version
                pos++;
                cc = Code[pos]; pos++;
                resultType = *((DWORD*)(Code + pos)); pos += 4;
                //ParOff
                pos += 2;
                if (Pass == 1)
                {
                    if (recN && recN->procInfo && recN->kind != ikConstructor && recN->kind != ikDestructor)//recN->kind != ikClassRef)
                    {
                        if (resultType)
                        {
                            recN->kind = ikFunc;
                            recN->type = GetTypeName(resultType);
                        }
                        if (cc != 0xFF) recN->procInfo->flags |= cc;
                    }
                }
                paramCount = Code[pos]; pos++;
                if (Pass == 1)
                {
                    if (recN && recN->procInfo)
                    {
                        recN->procInfo->DeleteArgs();
                        if (!paramCount) recN->procInfo->AddArg(0x21, 0, 4, "Self", className);
                    }
                }
                for (int m = 0; m < paramCount && !*Terminated; m++)
                {
                    paramFlags = Code[pos]; pos++;
                    paramType = *((DWORD*)(Code + pos)); pos += 4;
                    //ParOff
                    parOfs = *((WORD*)(Code + pos)); pos += 2;
                    sLen = Code[pos];
                    paramName = String((char*)(Code + pos + 1), sLen); pos += sLen + 1;
                    //AttrData
                    dw = *((WORD*)(Code + pos));
                    pos += dw;//ATR!!
                    if (paramFlags & 0x40) continue;//Result
                    if (Pass == 1)
                    {
                        if (recN && recN->procInfo)//recN->kind != ikClassRef)
                        {
                            Byte tag = 0x21;
                            if (paramFlags & 1) tag = 0x22;
                            recN->procInfo->AddArg(tag, parOfs, 4, paramName, GetTypeName(paramType));
                        }
                    }
                }
            }
            else
            {
                cc = 0xFF;
                paramCount = 0;
            }
            pos = spos;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AnalyzeDynamicTable(int Pass, DWORD Adr, const bool* Terminated)
{
    DWORD   vmtAdr = Adr - VmtSelfPtr;
    DWORD   DynamicAdr = *((DWORD*)(Code + Adr2Pos(vmtAdr) + VmtDynamicTable));
    if (!DynamicAdr) return;

	String clsName = GetClsName(Adr);
    DWORD pos = Adr2Pos(DynamicAdr);
    WORD Num = *((WORD*)(Code + pos)); pos += 2;
    DWORD post = pos + 2 * Num;

    for (int i = 0; i < Num && !*Terminated; i++, post+=4)
    {
        //WORD Msg
        pos += 2;
        DWORD procAdr = *((DWORD*)(Code + post));
        int procPos = Adr2Pos(procAdr);
        if (!procPos) continue;//Something wrong!
        bool skip = (*(Code + procPos) == 0 && *(Code + procPos + 1) == 0);
        if (!skip) AnalyzeProc(Pass, procAdr);

        if (Pass == 1 && !skip)
        {
            PInfoRec recN = GetInfoRec(procAdr);
            if (recN)
            {
                recN->kind = ikProc;
                recN->procInfo->flags |= PF_DYNAMIC;
                recN->AddXref('D', Adr, 0);
                recN->procInfo->AddArg(0x21, 0, 4, "Self", clsName);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AnalyzeVirtualTable(int Pass, DWORD Adr, const bool* Terminated)
{
    DWORD   parentAdr = GetParentAdr(Adr);
    DWORD   vmtAdr = Adr - VmtSelfPtr;
    DWORD   stopAt = GetStopAt(vmtAdr);
    if (vmtAdr == stopAt) return;
    
    int pos = Adr2Pos(vmtAdr) + VmtParent + 4;
    for (int n = VmtParent + 4; !*Terminated; n += 4, pos += 4)
    {
        if (Pos2Adr(pos) == stopAt) break;
        DWORD procAdr = *((DWORD*)(Code + pos));
        int procPos = Adr2Pos(procAdr);
        bool skip = (*(Code + procPos) == 0 && *(Code + procPos + 1) == 0);
        if (!skip) AnalyzeProc(Pass, procAdr);
        PInfoRec recN = GetInfoRec(procAdr);

        if (recN)
        {
            if (Pass == 1 && !skip)
            {
                recN->procInfo->flags |= PF_VIRTUAL;
                recN->AddXref('D', Adr, 0);
            }

            DWORD pAdr = parentAdr;
            while (pAdr&& !*Terminated)
            {
                PInfoRec recN1 = GetInfoRec(pAdr);
                //Look at parent class methods
                if (recN1 && recN1->vmtInfo && recN1->vmtInfo->methods)
                {
                    for (int m = 0; m < recN1->vmtInfo->methods->Count; m++)
                    {
                        PMethodRec recM = (PMethodRec)recN1->vmtInfo->methods->Items[m];
                        if (recM->abstract && recM->kind == 'V' && recM->id == n && recM->name == "")
                        {
                            String procName = recN->GetName();
                            if (procName != "" && !SameText(procName, "@AbstractError"))
                            {
                                recM->name = GetClsName(pAdr) + "." + ExtractProcName(procName);
                            }
                            break;
                        }
                    }
                }
                pAdr = GetParentAdr(pAdr);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AnalyzeProc(int pass, DWORD procAdr)
{
    switch (pass)
    {
    case 0:
        AnalyzeProcInitial(procAdr);
        break;
    case 1:
        AnalyzeProc1(procAdr, 0, 0, 0, false);
        break;
    case 2:
        AnalyzeProc2(procAdr, true, true);
        break;
    }
}
//---------------------------------------------------------------------------
//Scan proc calls
DWORD __fastcall TFMain_11011981::AnalyzeProcInitial(DWORD fromAdr)
{
    BYTE        op, b1, b2;
    int         num, instrLen, instrLen1, instrLen2, _procSize;
    int         fromPos;
    int         curPos;
    DWORD       curAdr;
    DWORD       lastAdr = 0;
    DWORD       Adr, Adr1, Pos, lastMovAdr = 0;
    PInfoRec    recN;
    DISINFO     DisInfo, DisInfo1;

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return 0;
    if (IsFlagSet(cfPass0, fromPos)) return 0;
    if (IsFlagSet(cfEmbedded, fromPos)) return 0;
    if (IsFlagSet(cfExport, fromPos)) return 0;

    //b1 = Code[fromPos];
    //b2 = Code[fromPos + 1];
    //if (!b1 && !b2) return 0;

    SetFlag(cfProcStart | cfPass0, fromPos);
    
    //Don't analyze imports
    if (IsFlagSet(cfImport, fromPos)) return 0;

    _procSize = GetProcSize(fromAdr);
    curPos = fromPos; curAdr = fromAdr;

    while (1)
    {
        if (curAdr >= CodeBase + TotalSize) break;
        //For example, cfProcEnd can be set for interface procs
        if (_procSize && curAdr - fromAdr >= _procSize) break;
        //Skip exception table
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            curPos += 4 + 8*num; curAdr += 4 + 8*num;
            continue;
        }

        b1 = Code[curPos];
        b2 = Code[curPos + 1];
        if (!b1 && !b2 && !lastAdr) break;

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) break;
        if (!instrLen)
        {
            curPos++; curAdr++;
            continue;
        }

        op = Disasm.GetOp(DisInfo.Mnem);
        //Code
        SetFlags(cfCode, curPos, instrLen);
        //Instruction begin
        SetFlag(cfInstruction, curPos);

        if (curAdr >= lastAdr) lastAdr = 0;

        //End of procedure
        if (DisInfo.Ret && (!lastAdr || curAdr == lastAdr)) break;

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            /*
            //First instruction
            if (curAdr == fromAdr) break;
            */
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Table address - last 4 bytes of instruction
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Scan gap to find table cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //If exists cTblAdr skip it
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                Pos += CNum; Adr += CNum;
            }
            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidCodeAdr(Adr1) || Adr1 < fromAdr) break;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }
        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //Check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    int delta = Pos - NPos;
                    if (delta >= 0 && delta < MAX_DISASSEMBLE)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    /*
                                    //@2
                                    Adr1 = DisInfo.Immediate - 4;
                                    Adr = *((DWORD*)(Code + Adr2Pos(Adr1)));
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    */
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Set flag cfETable to correctly output data
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                    curPos += instrLen; curAdr += instrLen;
                    continue;
                }
            }
        }

        if (DisInfo.Call)  //call sub_XXXXXXXX
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                //If @Halt0 - end of procedure
                if (recN && recN->SameName("@Halt0"))
                {
                    if (fromAdr == EP && !lastAdr) break;
                }
                AnalyzeProcInitial(Adr);
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (op == OP_JMP)
        {
            if (curAdr == fromAdr) return 0;
            if (DisInfo.OpType[0] == otMEM)
            {
                if (Adr2Pos(DisInfo.Offset) < 0 && (!lastAdr || curAdr == lastAdr)) return 0;
            }
            if (DisInfo.OpType[0] == otIMM)
            {
                Adr = DisInfo.Immediate;
                if (Adr2Pos(Adr) < 0 && (!lastAdr || curAdr == lastAdr)) return 0;
                if (GetSegmentNo(Adr) != 0 && GetSegmentNo(fromAdr) != GetSegmentNo(Adr) && (!lastAdr || curAdr == lastAdr)) return 0;
                if (Adr < fromAdr && (!lastAdr || curAdr == lastAdr)) return Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (DisInfo.Conditional)
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        curPos += instrLen; curAdr += instrLen;
    }
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::CodeGetTargetAdr(String Line, DWORD* trgAdr)
{
	char	    *s, *p, c;
    int			n, wid, instrlen;
    DWORD		adr, targetAdr;
    TPoint      cursorPos;
    TCanvas*    canvas = lbCode->Canvas;
    DISINFO     DisInfo;

    *trgAdr = 0;
    s = Line.c_str() + 1;

    //If db - no address
    if (strstr(s, " db ")) return 0;
    //If dd - address
    p = strstr(s, " dd ");
    if (p) sscanf(p + 4, "%lX", &targetAdr);

    if (!IsValidImageAdr(targetAdr))
    {
        sscanf(s, "%lX", &adr);
        instrlen = Disasm.Disassemble(Code + Adr2Pos(adr), (__int64)adr, &DisInfo, 0);
        if (!instrlen) return 0;

        if (IsValidImageAdr(DisInfo.Immediate))
        {
            if (!IsValidImageAdr(DisInfo.Offset))
                targetAdr = DisInfo.Immediate;
        }
        else if (IsValidImageAdr(DisInfo.Offset))
            targetAdr = DisInfo.Offset;
    }
    if (!IsValidImageAdr(targetAdr))
    {
        GetCursorPos(&cursorPos);
        cursorPos = lbCode->ScreenToClient(cursorPos);
        for (n = 0, wid = 0; n < strlen(s); n++)
        {
            int cwid = canvas->TextWidth(s[n]);
            if (wid >= cursorPos.x)
            {
                while (n >= 0)
                {
                    c = s[n];
                    if (c == ' ' || c == ',' || c == '[' || c == '+')
                    {
                        sscanf(s + n + 1, "%lX", &targetAdr);
                        break;
                    }
                    n--;
                }
                break;
            }
            wid += cwid;
        }
    }
    if (IsValidImageAdr(targetAdr)) *trgAdr = targetAdr;
    return DisInfo.OpSize;
}
//---------------------------------------------------------------------------
//May be Plugin!!!
String __fastcall sub_004AFB28(BYTE* AStr)
{
    Integer   _n, _num;
    Byte      _b, _b1, _b2, _m;
    String    _result;

    if (AStr[0] == 0x7B)
    {
        _m = 1;
        _n = 2;
        _b1 = AStr[1];
        _num = _b1 ^ 0xA1;
        _result = "";
        if (_num > 0)
        {
          do
          {
              _b2 = AStr[_n];
              _b1 = (3 * _m + _b1) ^ _b2;
              _b = _b1;
              _b1 = _b2;
              _m = _m + 1;
              _n = _n + 1;
              _num = _num - 1;
              _b2 = AStr[_n];
              _b1 = (3 * _m + _b1) ^ _b2;
              _b = _b | _b1;
              if (_b)
              {
                _result += Char(_b);
              }
              _b1 = _b2;
              _m = _m + 1;
              _n = _n + 1;
              _num = _num - 1;
          }
          while (_num > 0);
        }
    }
    else
    {
        _result = "!";
    }
    return _result;
}
//---------------------------------------------------------------------------
void __fastcall sub_004AF80C(BYTE* AStr1, BYTE* AStr2)
{
    BYTE*   _p;
    BYTE    _b, _n;
    int     _num;

    _n = *(AStr1 + 7);
    _p = AStr1 + 2 + 8;
    _num = AStr2 - _p;
    if (_num > 0)
    {
        do
        {
            _b = *_p;
            _b = ((0xFF - _b + 12) ^ 0xC2) - 3 * _n - 0x62;
            *_p = _b;
            _p++;
            _n++;
            _num--;
        }
        while (_num > 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbCodeDblClick(TObject *Sender)
{
    int             pos, bytes, size;
    DWORD		    adr, adr1, targetAdr;
    PInfoRec 		recN;
    PROCHISTORYREC  rec;
    String			text;

    if (lbCode->ItemIndex <= 0) return;

    text = lbCode->Items->Strings[lbCode->ItemIndex];
    size = CodeGetTargetAdr(text, &targetAdr);

    if (IsValidImageAdr(targetAdr))
    {
        pos = Adr2Pos(targetAdr);
        if (pos == -2) return;
        if (pos == -1)
        {
            ShowMessage("BSS");
            return;
        }
        if (IsFlagSet(cfImport, pos))
        {
            ShowMessage("Import");
            return;
        }
        //RTTI
        if (IsFlagSet(cfRTTI, pos))
        {
            FTypeInfo_11011981->ShowRTTI(targetAdr);
            return;
        }
        //if start of procedure, show it
        if (IsFlagSet(cfProcStart, pos))
        {
            rec.adr = CurProcAdr;
            rec.itemIdx = lbCode->ItemIndex;
            rec.xrefIdx = lbCXrefs->ItemIndex;
            rec.topIdx = lbCode->TopIndex;
            ShowCode(Pos2Adr(pos), targetAdr, -1, -1);
            CodeHistoryPush(&rec);
            return;
        }

        recN = GetInfoRec(targetAdr);
        if (recN)
        {
            if (recN->kind == ikVMT && tsClassView->TabVisible)
            {
                ShowClassViewer(targetAdr);
                return;
            }
            if (recN->kind == ikResString)
            {
                FStringInfo_11011981->memStringInfo->Clear();
                FStringInfo_11011981->Caption = "ResString context";
                FStringInfo_11011981->memStringInfo->Lines->Add(recN->rsInfo->value);
                FStringInfo_11011981->ShowModal();
                return;
            }
            if (recN->HasName())
            {
                WORD *uses = KnowledgeBase.GetTypeUses(recN->GetName().c_str());
                int idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, recN->GetName().c_str());
                if (uses) delete[] uses;

                if (idx != -1)
                {
                    idx = KnowledgeBase.TypeOffsets[idx].NamId;
                    MTypeInfo tInfo;
                    if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS | INFO_PROPS | INFO_METHODS | INFO_DUMP, &tInfo))
                    {
                        FTypeInfo_11011981->ShowKbInfo(&tInfo);
                        //as delete tInfo;
                        return;
                    }
                }
            }
        }
        //may be ->
        adr = *((DWORD*)(Code + pos));
        if (IsValidImageAdr(adr))
        {
            recN = GetInfoRec(adr);
            if (recN)
            {
                if (recN->kind == ikResString)
                {
                    FStringInfo_11011981->memStringInfo->Clear();
                    FStringInfo_11011981->Caption = "ResString context";
                    FStringInfo_11011981->memStringInfo->Lines->Add(recN->rsInfo->value);
                    FStringInfo_11011981->ShowModal();
                    return;
                }
            }
        }

        //if in current proc
        if (CurProcAdr <= targetAdr && targetAdr < CurProcAdr + CurProcSize)
        {
            rec.adr = CurProcAdr;
            rec.itemIdx = lbCode->ItemIndex;
            rec.xrefIdx = lbCXrefs->ItemIndex;
            rec.topIdx = lbCode->TopIndex;
            ShowCode(CurProcAdr, targetAdr, lbCXrefs->ItemIndex, -1);
            CodeHistoryPush(&rec);
            return;
        }
        //Else show explorer
        FExplorer_11011981->tsCode->TabVisible = true;
        FExplorer_11011981->ShowCode(targetAdr, 1024);
        FExplorer_11011981->tsData->TabVisible = true;
        FExplorer_11011981->ShowData(targetAdr, 1024);
        FExplorer_11011981->tsString->TabVisible = true;
        FExplorer_11011981->ShowString(targetAdr, 1024);
        FExplorer_11011981->tsText->TabVisible = false;
        FExplorer_11011981->pc1->ActivePage = FExplorer_11011981->tsData;
        FExplorer_11011981->WAlign = -4;

        FExplorer_11011981->btnDefCode->Enabled = true;
        if (IsFlagSet(cfCode, pos)) FExplorer_11011981->btnDefCode->Enabled = false;
        FExplorer_11011981->btnUndefCode->Enabled = false;
        if (IsFlagSet(cfCode | cfData, pos)) FExplorer_11011981->btnUndefCode->Enabled = true;

        if (FExplorer_11011981->ShowModal() == mrOk)
        {
            if (FExplorer_11011981->DefineAs == DEFINE_AS_CODE)
            {
                //Delete any information at this address
                recN = GetInfoRec(Pos2Adr(pos));
                if (recN) delete recN;
                //Create new info about proc
                recN = new InfoRec(pos, ikRefine);

                //AnalyzeProcInitial(targetAdr);
                AnalyzeProc1(targetAdr, 0, 0, 0, false);
                AnalyzeProc2(targetAdr, true, true);
                AnalyzeArguments(targetAdr);
                AnalyzeProc2(targetAdr, true, true);

                if (!ContainsUnexplored(GetUnit(targetAdr))) ShowUnits(true);
                ShowUnitItems(GetUnit(targetAdr), lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
                rec.adr = CurProcAdr;
                rec.itemIdx = lbCode->ItemIndex;
                rec.xrefIdx = lbCXrefs->ItemIndex;
                rec.topIdx = lbCode->TopIndex;
                ShowCode(targetAdr, 0, -1, -1);
                CodeHistoryPush(&rec);
                ProjectModified = true;
            }
        }
    }
    //Try picode
    else
    {
        sscanf(text.c_str() + 2, "%lX", &adr);
        recN = GetInfoRec(adr);
        if (recN && recN->picode && IsValidCodeAdr(recN->picode->Ofs.Address))
        {
            pos = Adr2Pos(recN->picode->Ofs.Address);
            if (IsFlagSet(cfProcStart, pos))
            {
                rec.adr = CurProcAdr;
                rec.itemIdx = lbCode->ItemIndex;
                rec.xrefIdx = lbCXrefs->ItemIndex;
                rec.topIdx = lbCode->TopIndex;
                ShowCode(Pos2Adr(pos), targetAdr, -1, -1);
                CodeHistoryPush(&rec);
                return;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bEPClick(TObject *Sender)
{
    PROCHISTORYREC  rec;

    rec.adr = CurProcAdr;
    rec.itemIdx = lbCode->ItemIndex;
    rec.xrefIdx = lbCXrefs->ItemIndex;
    rec.topIdx = lbCode->TopIndex;
    ShowCode(EP, 0, -1, -1);
    CodeHistoryPush(&rec);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::GoToAddress()
{
    int             pos;
    DWORD           gotoAdr;
    String          sAdr;
    PROCHISTORYREC  rec;

    //if (lbCode->ItemIndex <= 0) return;

    sAdr = InputDialogExec("Enter Address", "Address:", "");
    if (sAdr != "")
    {
        sscanf(sAdr.c_str(), "%lX", &gotoAdr);
        if (IsValidCodeAdr(gotoAdr))
        {
            pos = Adr2Pos(gotoAdr);
            //Если импорт - ничего не отображаем
            if (IsFlagSet(cfImport, pos)) return;
            //Ищем, куда попадает адрес
            while (pos >= 0)
            {
                //Нашли начало процедуры
                if (IsFlagSet(cfProcStart, pos))
                {
                    rec.adr = CurProcAdr;
                    rec.itemIdx = lbCode->ItemIndex;
                    rec.xrefIdx = lbCXrefs->ItemIndex;
                    rec.topIdx = lbCode->TopIndex;
                    ShowCode(Pos2Adr(pos), gotoAdr, -1, -1);
                    CodeHistoryPush(&rec);
                    break;
                }
                //Нашли начало типа
                if (IsFlagSet(cfRTTI, pos))
                {
                    FTypeInfo_11011981->ShowRTTI(Pos2Adr(pos));
                    break;
                }
                pos--;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miGoToClick(TObject *Sender)
{
    GoToAddress();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExploreAdrClick(TObject *Sender)
{
    int         size;
	DWORD       viewAdr;
    String      text = "", sAdr;
    PInfoRec    recN;

    if (lbCode->ItemIndex <= 0) return;

    size = CodeGetTargetAdr(lbCode->Items->Strings[lbCode->ItemIndex], &viewAdr);
    if (viewAdr) text = Val2Str8(viewAdr);
    sAdr = InputDialogExec("Enter Address", "Address:", text);
    if (sAdr != "")
    {
        sscanf(sAdr.c_str(), "%lX", &viewAdr);
        if (IsValidImageAdr(viewAdr))
        {
            int pos = Adr2Pos(viewAdr);
            if (pos == -2) return;
            if (pos == -1)
            {
                ShowMessage("BSS");
                return;
            }
            FExplorer_11011981->tsCode->TabVisible = true;
            FExplorer_11011981->ShowCode(viewAdr, 1024);
            FExplorer_11011981->tsData->TabVisible = true;
            FExplorer_11011981->ShowData(viewAdr, 1024);
            FExplorer_11011981->tsString->TabVisible = true;
            FExplorer_11011981->ShowString(viewAdr, 1024);
            FExplorer_11011981->tsText->TabVisible = false;
            FExplorer_11011981->pc1->ActivePage = FExplorer_11011981->tsCode;
            FExplorer_11011981->WAlign = -4;
            
            FExplorer_11011981->btnDefCode->Enabled = true;
            if (IsFlagSet(cfCode, pos)) FExplorer_11011981->btnDefCode->Enabled = false;
            FExplorer_11011981->btnUndefCode->Enabled = false;
            if (IsFlagSet(cfCode | cfData, pos)) FExplorer_11011981->btnUndefCode->Enabled = true;

            if (FExplorer_11011981->ShowModal() == mrOk)
            {
                switch (FExplorer_11011981->DefineAs)
                {
                case DEFINE_AS_CODE:
                    //Delete any information at this address
                    recN = GetInfoRec(viewAdr);
                    if (recN) delete recN;
                    //Create new info about proc
                    recN = new InfoRec(pos, ikRefine);

                    //AnalyzeProcInitial(viewAdr);
                    AnalyzeProc1(viewAdr, 0, 0, 0, false);
                    AnalyzeProc2(viewAdr, true, true);
                    AnalyzeArguments(viewAdr);
                    AnalyzeProc2(viewAdr, true, true);

                    if (!ContainsUnexplored(GetUnit(viewAdr))) ShowUnits(true);
                    ShowUnitItems(GetUnit(viewAdr), lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
                    ShowCode(viewAdr, 0, -1, -1);
                    break;
                case DEFINE_AS_STRING:
                    break;
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::NamePosition()
{
	int			pos, _idx, size;
    DWORD 		adr, nameAdr;
    PInfoRec 	recN;
    String 		line, text = "", sNameType, newName, newType;

    if (lbCode->ItemIndex >= 1)
    {
        line = lbCode->Items->Strings[lbCode->ItemIndex];
   	    size = CodeGetTargetAdr(line, &nameAdr);
    }

    if (IsValidImageAdr(nameAdr))
    {
        pos = Adr2Pos(nameAdr);
        recN = GetInfoRec(nameAdr);
        //VMT
        if (recN && recN->kind == ikVMT) return;

        //if (size == 4)
        //{
            adr = *((DWORD*)(Code + pos));
            if (IsValidImageAdr(adr)) nameAdr = adr;
        //}
    }
    else
    {
        nameAdr = CurProcAdr;
    }

    pos = Adr2Pos(nameAdr);
    recN = GetInfoRec(nameAdr);
    if (recN && recN->HasName())
    {
        text = recN->GetName();
        if (recN->type != "") text = recN->GetName() + ":" + recN->type;
    }

    sNameType = InputDialogExec("Enter Name:Type (at " + Val2Str8(nameAdr) + ")", "Name:Type", text);
    if (sNameType != "")
    {
        if (sNameType.Pos(":"))
        {
            newName = ExtractName(sNameType).Trim();
            newType = ExtractType(sNameType).Trim();
        }
        else
        {
            newName = sNameType;
            newType = "";
        }

        if (newName == "") return;

        //If call
        if (pos >= 0 && IsFlagSet(cfProcStart, pos))
        {
            if (!recN) recN = new InfoRec(pos, ikRefine);
            recN->kind = ikProc;
            recN->SetName(newName);
            if (newType != "")
            {
                recN->kind = ikFunc;
                recN->type = newType;
            }
        }
        else
        {
            if (pos >= 0)
            {
                //Address points to Data
                if (!recN) recN = new InfoRec(pos, ikUnknown);
                recN->SetName(newName);
                if (newType != "") recN->type = newType;
            }
            else
            {
                _idx = BSSInfos->IndexOf(Val2Str8(nameAdr));
                if (_idx != -1)
                {
                    recN = (PInfoRec)BSSInfos->Objects[_idx];
                    recN->SetName(newName);
                    recN->type = newType;
                }
                else
                    recN = AddToBSSInfos(nameAdr, newName, newType);
            }
        }

        RedrawCode();
        ShowUnitItems(GetUnit(CurUnitAdr), lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
        ProjectModified = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miNameClick(TObject *Sender)
{
    NamePosition();
}
//---------------------------------------------------------------------------
TTreeNode* __fastcall TFMain_11011981::GetNodeByName(String AName)
{
    for (int n = 0; n < tvClassesFull->Items->Count; n++)
    {
        TTreeNode *node = tvClassesFull->Items->Item[n];
        String text = node->Text;
        if (AName[1] != ' ')
        {
            if (text[1] != '<' && text[1] == AName[1] && text[2] == AName[2] && text.Pos(AName) == 1) return node;
        }
        else
        {
            if (text[1] != '<' && text.Pos(AName)) return node;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ClearTreeNodeMap()
{
    tvClassMap.clear();
}
//---------------------------------------------------------------------------
TTreeNode* __fastcall TFMain_11011981::FindTreeNodeByName(const String& name)
{
    TTreeNodeNameMap::const_iterator it = tvClassMap.find(name);
    if (it != tvClassMap.end()) return it->second;

    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AddTreeNodeWithName(TTreeNode* node, const String& name)
{
    tvClassMap[name] = node;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowClassViewer(DWORD VmtAdr)
{
    bool        vmtProc;
    WORD        _dx, _bx, _si;
    int         cnt, vmtOfs, _pos;
    DWORD       parentAdr, adr = VmtAdr, vAdr, iAdr;
    DWORD       vmtAdresses[1024];
    String      SelName = GetClsName(VmtAdr), line, name;
    TTreeNode*  selNode = 0;
    TTreeNode*  node = 0;
    PInfoRec    recN;
    PMethodRec  recM;
    DISINFO     disInfo;

    if (SelName == "" || !IsValidImageAdr(VmtAdr)) return;
    
    if (!tsClassView->Enabled) return;
    
    if (ClassTreeDone)
    {
        node = GetNodeByName(SelName + " #" + Val2Str8(adr) + " Sz=");
        if (node)
        {
            node->Selected = true;
            node->Expanded = true;
            tvClassesFull->TopItem = node;
        }
    }

    TList *fieldsList = new TList;
    TStringList *tmpList = new TStringList;
    tmpList->Sorted = false;

    tvClassesShort->Items->Clear(); node = 0;

    for (int n = 0;; n++)
    {
        parentAdr = GetParentAdr(adr);
        vmtAdresses[n] = adr;
        if (!parentAdr)
        {
            while (n >= 0)
            {
                adr = vmtAdresses[n]; n--;
                String className = GetClsName(adr);
                int m, size = GetClassSize(adr); if (DelphiVersion >= 2009) size += 4;

                String text = className + " #" + Val2Str8(adr) + " Sz=" + Val2Str0(size);

                if (!node)  //Root
                    node = tvClassesShort->Items->Add(0, text);
                else
                    node = tvClassesShort->Items->AddChild(node, text);

                if (adr == VmtAdr && SameText(className, SelName)) selNode = node;

                //Interfaces
                int intfsNum = LoadIntfTable(adr, tmpList);
                if (intfsNum)
                {
                    for (m = 0; m < intfsNum; m++)
                    {
                        String item = tmpList->Strings[m];
                        sscanf(item.c_str(), "%lX", &vAdr);
                        if (IsValidCodeAdr(vAdr))
                        {
                            int pos = item.Pos(' ');
                            TTreeNode* intfsNode = tvClassesShort->Items->AddChild(node, "<I> " + item.SubString(pos + 1, item.Length()));
                            cnt = 0;
                            pos = Adr2Pos(vAdr);
                            for (int v = 0;;v += 4)
                            {
                                if (IsFlagSet(cfVTable, pos)) cnt++;
                                if (cnt == 2) break;
                                iAdr = *((DWORD*)(Code + pos));
                                DWORD _adr = iAdr;
                                _pos = Adr2Pos(_adr);
                                vmtProc = false; vmtOfs = 0;
                                _dx = 0; _bx = 0; _si = 0;
                                while (1)
                                {
                                    int instrlen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &disInfo, 0);
                                    if ((disInfo.OpType[0] == otMEM || disInfo.OpType[1] == otMEM) &&
                                        disInfo.BaseReg != 20)//to exclude instruction "xchg reg, [esp]"
                                    {
                                        vmtOfs = disInfo.Offset;
                                    }
                                    if (disInfo.OpType[0] == otREG && disInfo.OpType[1] == otIMM)
                                    {
                                        if (disInfo.OpRegIdx[0] == 10)//dx
                                            _dx = disInfo.Immediate;
                                        else if (disInfo.OpRegIdx[0] == 11)//bx
                                            _bx = disInfo.Immediate;
                                        else if (disInfo.OpRegIdx[0] == 14)//si
                                            _si = disInfo.Immediate;
                                    }
                                    if (disInfo.Call)
                                    {
                                        recN = GetInfoRec(disInfo.Immediate);
                                        if (recN)
                                        {
                                            if (recN->SameName("@CallDynaInst") ||
                                                recN->SameName("@CallDynaClass"))
                                            {
                                                if (DelphiVersion <= 5)
                                                    GetDynaInfo(adr, _bx, &iAdr);
                                                else
                                                    GetDynaInfo(adr, _si, &iAdr);
                                                break;
                                            }
                                            else if (recN->SameName("@FindDynaInst") ||
                                                     recN->SameName("@FindDynaClass"))
                                            {
                                                GetDynaInfo(adr, _dx, &iAdr);
                                                break;
                                            }
                                        }
                                    }
                                    if (disInfo.Branch && !disInfo.Conditional)
                                    {
                                        if (IsValidImageAdr(disInfo.Immediate))
                                        {
                                            iAdr = disInfo.Immediate;
                                        }
                                        else
                                        {
                                            vmtProc = true;
                                            iAdr = *((DWORD*)(Code + Adr2Pos(VmtAdr - VmtSelfPtr + vmtOfs)));
                                            recM = GetMethodInfo(VmtAdr, 'V', vmtOfs);
                                            if (recM) name = recM->name;
                                        }
                                        break;
                                    }
                                    else if (disInfo.Ret)
                                    {
                                        vmtProc = true;
                                        iAdr = *((DWORD*)(Code + Adr2Pos(VmtAdr - VmtSelfPtr + vmtOfs)));
                                        recM = GetMethodInfo(VmtAdr, 'V', vmtOfs);
                                        if (recM) name = recM->name;
                                        break;
                                    }
                                    _pos += instrlen; _adr += instrlen;
                                }
                                if (!vmtProc && IsValidImageAdr(iAdr))
                                {
                                    recN = GetInfoRec(iAdr);
                                    if (recN && recN->HasName())
                                        name = recN->GetName();
                                    else
                                        name = "";
                                }
                                line = "I" + Val2Str4(v) + " #" + Val2Str8(iAdr);
                                if (name != "") line += " " + name;
                                tvClassesShort->Items->AddChild(intfsNode, line);
                                pos += 4;
                            }
                        }
                        else
                        {
                            TTreeNode* intfsNode = tvClassesShort->Items->AddChild(node, "<I> " + item);
                        }
                    }
                }
                //Automated
                int autoNum = LoadAutoTable(adr, tmpList);
                if (autoNum)
                {
                    TTreeNode* autoNode = tvClassesShort->Items->AddChild(node, "<A>");
                    for (m = 0; m < autoNum; m++)
                    {
                        tvClassesShort->Items->AddChild(autoNode, tmpList->Strings[m]);
                    }
                }
                //Fields
                int fieldsNum = LoadFieldTable(adr, fieldsList);
                if (fieldsNum)
                {
                    TTreeNode* fieldsNode = tvClassesShort->Items->AddChild(node, "<F>");
                    for (m = 0; m < fieldsNum; m++)
                    {
                        PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[m];
                        text = Val2Str5(fInfo->Offset) + " ";
                        if (fInfo->Name != "")
                            text += fInfo->Name;
                        else
                            text += "?";
                        text += ":";
                        if (fInfo->Type != "")
                            text += TrimTypeName(fInfo->Type);
                        else
                            text += "?";

                        tvClassesShort->Items->AddChild(fieldsNode, text);
                    }
                }
                //Events
                int methodsNum = LoadMethodTable(adr, tmpList);
                if (methodsNum)
                {
                    tmpList->Sort();
                    TTreeNode* methodsNode = tvClassesShort->Items->AddChild(node, "<E>");
                    for (m = 0; m < methodsNum; m++)
                    {
                        tvClassesShort->Items->AddChild(methodsNode, tmpList->Strings[m]);
                    }
                }
                int dynamicsNum = LoadDynamicTable(adr, tmpList);
                if (dynamicsNum)
                {
                    tmpList->Sort();
                    TTreeNode* dynamicsNode = FMain_11011981->tvClassesShort->Items->AddChild(node, "<D>");
                    for (m = 0; m < dynamicsNum; m++)
                    {
                        tvClassesShort->Items->AddChild(dynamicsNode, tmpList->Strings[m]);
                    }
                }
                //Virtual
                int virtualsNum = LoadVirtualTable(adr, tmpList);
                if (virtualsNum)
                {
                    TTreeNode* virtualsNode = tvClassesShort->Items->AddChild(node, "<V>");
                    for (m = 0; m < virtualsNum; m++)
                    {
                        tvClassesShort->Items->AddChild(virtualsNode, tmpList->Strings[m]);
                    }
                }
            }
            if (selNode)
            {
                selNode->Selected = true;
                selNode->Expand(true);
                tvClassesShort->TopItem = selNode;
            }
            break;
        }
        adr = parentAdr;
    }

    delete fieldsList;
    delete tmpList;

    pcWorkArea->ActivePage = tsClassView;
    if (!rgViewerMode->ItemIndex)
    {
        tvClassesFull->BringToFront();
        //if (tvClassesFull->CanFocus()) ActiveControl = tvClassesFull;
    }
    else
    {
        tvClassesShort->BringToFront();
        //if (tvClassesShort->CanFocus()) ActiveControl = tvClassesShort;
    }
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadIntfTable(DWORD adr, TStringList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->interfaces)
    {
        for (int n = 0; n < recN->vmtInfo->interfaces->Count; n++)
        {
            dstList->Add(recN->vmtInfo->interfaces->Strings[n]);
        }
    }
    dstList->Sort();
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadAutoTable(DWORD adr, TStringList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'A')
            {
                String line = "A" + Val2Str4(recM->id) + " #" + Val2Str8(recM->address) + " " + recM->name;
                dstList->Add(line);
            }
        }
    }
    dstList->Sort();
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadFieldTable(DWORD adr, TList* dstList)
{
    dstList->Clear();
    DWORD parentSize = GetParentSize(adr);
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->fields)
    {
        for (int n = 0; n < recN->vmtInfo->fields->Count; n++)
        {
            PFIELDINFO fInfo = (PFIELDINFO)recN->vmtInfo->fields->Items[n];
            if (fInfo->Offset >= parentSize)
            {
                bool exist = false;
                for (int m = 0; m < dstList->Count; m++)
                {
                    PFIELDINFO fInfo1 = (PFIELDINFO)dstList->Items[m];
                    if (fInfo1->Offset == fInfo->Offset)
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist) dstList->Add((void*)fInfo);
            }
        }
    }
    /*
    while (1)
    {
        PInfoRec recN = GetInfoRec(adr);
        if (recN && recN->info && recN->info.vmtInfo->fields)
        {
            for (int n = recN->info.vmtInfo->fields->Count - 1; n >= 0; n--)
            {
                PFIELDINFO fInfo = (PFIELDINFO)recN->info.vmtInfo->fields->Items[n];
                if (!GetVMTField(dstList, fInfo->offset)) dstList->Add((void*)fInfo);
            }
        }
        //ParentAdr
        adr = GetParentAdr(adr);
        if (!adr) break;
    }
    */
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadMethodTable(DWORD adr, TList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        String className = GetClsName(adr) + ".";
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'M')
            {
                if (recM->name.Pos(".") == 0 || recM->name.Pos(className) == 1) dstList->Add((void*)recM);
            }
        }
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadMethodTable(DWORD adr, TStringList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'M')
            {
                String line = "#" + Val2Str8(recM->address) + " " + recM->name;
                dstList->Add(line);
            }
        }
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadDynamicTable(DWORD adr, TList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        String className = GetClsName(adr) + ".";
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'D')
            {
                if (recM->name.Pos(".") == 0 || recM->name.Pos(className) == 1) dstList->Add((void*)recM);
            }
        }
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadDynamicTable(DWORD adr, TStringList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'D')
            {
                String line = "D" + Val2Str4(recM->id) + " #" + Val2Str8(recM->address) + " " + recM->name;
                dstList->Add(line);
            }
        }
        dstList->Sort();
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadVirtualTable(DWORD adr, TList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        String className = GetClsName(adr) + ".";
        recN->vmtInfo->methods->Sort(MethodsCmpFunction);
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'V')
            {
                if (recM->name.Pos(".") == 0 || recM->name.Pos(className) == 1) dstList->Add((void*)recM);
            }
        }
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadVirtualTable(DWORD adr, TStringList* dstList)
{
    dstList->Clear();
    PInfoRec recN = GetInfoRec(adr);
    if (recN && recN->vmtInfo && recN->vmtInfo->methods)
    {
        recN->vmtInfo->methods->Sort(MethodsCmpFunction);
        for (int n = 0; n < recN->vmtInfo->methods->Count; n++)
        {
            PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[n];
            if (recM->kind == 'V')// && recM->id >= -4)
            {
                String line = "";
                PInfoRec recN1 = GetInfoRec(recM->address);

                if (recM->id < 0)
                    line += "-" + Val2Str4(-(recM->id));
                else
                    line += "V" + Val2Str4(recM->id);

                line += " #" + Val2Str8(recM->address);
                if (recM->name != "")
                {
                    line +=  + " " + recM->name;
                    if (recN1 && recN1->HasName() && !recN1->SameName(recM->name))
                    {
                        //Change "@AbstractError" to "abstract"
                        if (SameText(recN1->GetName(), "@AbstractError"))
                            line += " (abstract)";
                        else
                            line += " (" + recN1->GetName() + ")";
                    }
                }
                else
                {
                    if (recN1 && recN1->HasName()) line += " " + recN1->GetName();
                }

                dstList->Add(line);
            }
        }
    }
    return dstList->Count;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miViewProtoClick(TObject *Sender)
{
    int         Idx;
    DWORD		Adr;
    PInfoRec 	recN;
    MProcInfo   pInfo;
    String      item;
    DISINFO     DisInfo;

    if (lbCode->ItemIndex <= 0) return;

    item = lbCode->Items->Strings[lbCode->ItemIndex];
    sscanf(item.c_str() + 1, "%lX", &Adr);
    int instrlen = Disasm.Disassemble(Code + Adr2Pos(Adr), (__int64)Adr, &DisInfo, 0);
    if (!instrlen) return;
    
    String proto = "";
    if (DisInfo.Call)
    {
    	//Адрес задан явно
        if (IsValidCodeAdr(DisInfo.Immediate))
        {
            recN = GetInfoRec(DisInfo.Immediate);
            if (recN) proto = recN->MakePrototype(DisInfo.Immediate, true, false, false, true, true);
        }
        //Адрес не задан, пробуем пи-код
        else
        {
        	recN = GetInfoRec(Adr);
            if (recN && recN->picode && IsValidCodeAdr(recN->picode->Ofs.Address))
            {
                if (KnowledgeBase.GetProcInfo(recN->picode->Name.c_str(), INFO_ARGS, &pInfo, &Idx))
                    proto = KnowledgeBase.GetProcPrototype(&pInfo);
            }
        }
    }
    if (proto != "")
    {
        FStringInfo_11011981->memStringInfo->Clear();
        FStringInfo_11011981->Caption = "Prototype";
        FStringInfo_11011981->memStringInfo->Lines->Add(proto);
        FStringInfo_11011981->ShowModal();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowCXrefsClick(TObject *Sender)
{
    if (lbCXrefs->Visible)
    {
        ShowCXrefs->BevelOuter = bvRaised;
        lbCXrefs->Visible = false;
    }
    else
    {
        ShowCXrefs->BevelOuter = bvLowered;
        lbCXrefs->Visible = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bCodePrevClick(TObject *Sender)
{
	//first add to array current subroutine info (for ->)
    if (CodeHistoryPtr == CodeHistorySize - 1)
    {
        CodeHistorySize += HISTORY_CHUNK_LENGTH;
        CodeHistory.Length = CodeHistorySize;
    }

    PROCHISTORYREC rec;
    rec.adr = CurProcAdr;
    rec.itemIdx = lbCode->ItemIndex;
    rec.xrefIdx = lbCXrefs->ItemIndex;
    rec.topIdx = lbCode->TopIndex;
    memmove(&CodeHistory[CodeHistoryPtr + 1], &rec, sizeof(PROCHISTORYREC));
    //next pop from array
    PPROCHISTORYREC prec = CodeHistoryPop();
    if (prec) ShowCode(prec->adr, prec->itemIdx, prec->xrefIdx, prec->topIdx);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bCodeNextClick(TObject *Sender)
{
    PROCHISTORYREC rec;
    rec.adr = CurProcAdr;
    rec.itemIdx = lbCode->ItemIndex;
    rec.xrefIdx = lbCXrefs->ItemIndex;
    rec.topIdx = lbCode->TopIndex;

	CodeHistoryPtr++;
    memmove(&CodeHistory[CodeHistoryPtr], &rec, sizeof(PROCHISTORYREC));

	PPROCHISTORYREC prec = &CodeHistory[CodeHistoryPtr + 1];
    ShowCode(prec->adr, prec->itemIdx, prec->xrefIdx, prec->topIdx);
    
    bCodePrev->Enabled = (CodeHistoryPtr >= 0);
    bCodeNext->Enabled = (CodeHistoryPtr < CodeHistoryMax);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::CodeHistoryPush(PPROCHISTORYREC rec)
{
    if (CodeHistoryPtr == CodeHistorySize - 1)
    {
        CodeHistorySize += HISTORY_CHUNK_LENGTH;
        CodeHistory.Length = CodeHistorySize;
    }

    CodeHistoryPtr++;
    memmove(&CodeHistory[CodeHistoryPtr], rec, sizeof(PROCHISTORYREC));

    CodeHistoryMax = CodeHistoryPtr;
    bCodePrev->Enabled = (CodeHistoryPtr >= 0);
    bCodeNext->Enabled = (CodeHistoryPtr < CodeHistoryMax);
}
//---------------------------------------------------------------------------
PPROCHISTORYREC __fastcall TFMain_11011981::CodeHistoryPop()
{
    PPROCHISTORYREC prec = 0;
    if (CodeHistoryPtr >= 0)
    {
        prec = &CodeHistory[CodeHistoryPtr];
        CodeHistoryPtr--;
    }
    bCodePrev->Enabled = (CodeHistoryPtr >= 0);
    bCodeNext->Enabled = (CodeHistoryPtr < CodeHistoryMax);
    return prec;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesDblClick(TObject *Sender)
{
    int             k, m, n;
    TTreeView*      tv;
    PROCHISTORYREC  rec;

    if (ActiveControl == tvClassesFull || ActiveControl == tvClassesShort)
        tv = (TTreeView*)ActiveControl;

    TTreeNode   *node = tv->Selected;
    if (node)
    {
        DWORD adr;
        String line = node->Text;
        int pos = line.Pos("#");
        //Указан адрес
        if (pos && !line.Pos("Sz="))
        {
            sscanf(line.c_str() + pos, "%lX", &adr);
            if (IsValidCodeAdr(adr))
            {
                rec.adr = CurProcAdr;
                rec.itemIdx = lbCode->ItemIndex;
                rec.xrefIdx = lbCXrefs->ItemIndex;
                rec.topIdx = lbCode->TopIndex;
                ShowCode(adr, 0, -1, -1);
                CodeHistoryPush(&rec);
            }
            return;
        }
        //Указан тип поля
        if (line.Pos(":"))
        {
            String typeName = ExtractType(line);
            //Если тип задан в виде Unit.TypeName
            if (typeName.Pos(".")) typeName = ExtractProcName(typeName);

            adr = GetClassAdr(typeName);
            if (IsValidImageAdr(adr))
            {
                ShowClassViewer(adr);
            }
            else
            {
                WORD* uses = KnowledgeBase.GetTypeUses(typeName.c_str());
                int Idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, typeName.c_str());
                if (Idx != -1)
                {
                    Idx = KnowledgeBase.TypeOffsets[Idx].NamId;
                    MTypeInfo tInfo;
                    if (KnowledgeBase.GetTypeInfo(Idx, INFO_FIELDS | INFO_PROPS | INFO_METHODS, &tInfo))
                    {
                        FTypeInfo_11011981->ShowKbInfo(&tInfo);
                        //as delete tInfo;
                    }
                }
                if (uses) delete[] uses;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesShortKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key == VK_RETURN) tvClassesDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmVMTsPopup(TObject *Sender)
{
	bool	b;
	if (ActiveControl == tvClassesFull)
    {
    	b = (tvClassesFull->Items->Count != 0);
    	miSearchVMT->Visible = b;
        miCollapseAll->Visible = b;
        miEditClass->Visible = false;
        return;
    }
    if (ActiveControl == tvClassesShort)
    {
    	b = (tvClassesShort->Items->Count != 0);
    	miSearchVMT->Visible = b;
        miCollapseAll->Visible = b;
        miEditClass->Visible = !AnalyzeThread && b && tvClassesShort->Selected;
        return;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miViewClassClick(TObject *Sender)
{
    String sName = InputDialogExec("Enter Name of Type", "Name:", "");
    if (sName != "") ShowClassViewer(GetClassAdr(sName));
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchVMTClick(TObject *Sender)
{
    WhereSearch = SEARCH_CLASSVIEWER;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < VMTsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(VMTsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (ActiveControl == tvClassesFull)
        {
            if (tvClassesFull->Selected)
                TreeSearchFrom = tvClassesFull->Selected;
            else
                TreeSearchFrom = tvClassesFull->Items->Item[0];
        }
        else if (ActiveControl == tvClassesShort)
        {
            if (tvClassesShort->Selected)
                BranchSearchFrom = tvClassesShort->Selected;
            else
                BranchSearchFrom = tvClassesShort->Items->Item[0];
        }

        VMTsSearchText = FindDlg_11011981->cbText->Text;
        if (VMTsSearchList->IndexOf(VMTsSearchText) == -1) VMTsSearchList->Add(VMTsSearchText);
        FindText(VMTsSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCollapseAllClick(TObject *Sender)
{
    TTreeView* tv;
    if (ActiveControl == tvClassesFull || ActiveControl == tvClassesShort)
    {
        tv = (TTreeView*)ActiveControl;
        tv->Items->BeginUpdate();
        TTreeNode* rootNode = tv->Items->Item[0];
        const int cnt = rootNode->Count;
        for (int n = 0; n < cnt; n++)
        {
            TTreeNode* node = rootNode->Item[n];
            if (node->HasChildren && node->Expanded) node->Collapse(true);
        }
        tv->Items->EndUpdate();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miEditClassClick(TObject *Sender)
{
	if (ActiveControl == tvClassesShort)
    {
    	TTreeNode* node = tvClassesShort->Selected;
        if (node)
        {
            int FieldOfs = -1;
            if (!node->Text.Pos("#"))
                sscanf(node->Text.c_str(), "%lX", &FieldOfs);
        	while (node)
            {
                int pos = node->Text.Pos("#");
                //Указан адрес
                if (pos && node->Text.Pos("Sz="))
                {
                	DWORD vmtAdr;
                    sscanf(node->Text.c_str() + pos, "%lX", &vmtAdr);
                    if (IsValidImageAdr(vmtAdr))
                    {
                    	FEditFieldsDlg_11011981->VmtAdr = vmtAdr;
                        FEditFieldsDlg_11011981->FieldOffset = FieldOfs;
                        if (FEditFieldsDlg_11011981->Visible) FEditFieldsDlg_11011981->Close();
                        FEditFieldsDlg_11011981->FormStyle = fsStayOnTop;
                    	FEditFieldsDlg_11011981->Show();
                        return;
                    }
                }
                node = node->GetPrev();
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopyCodeClick(TObject *Sender)
{
    Copy2Clipboard(lbCode->Items, 1, true);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbFormsDblClick(TObject *Sender)
{
	int		n, m;

    TDfm *dfm = (TDfm*)ResInfo->FormList->Items[lbForms->ItemIndex];
    switch (rgViewFormAs->ItemIndex)
    {
    //As Text
    case 0:
        FExplorer_11011981->tsCode->TabVisible = false;
        FExplorer_11011981->tsData->TabVisible = false;
        FExplorer_11011981->tsString->TabVisible = false;
        FExplorer_11011981->tsText->TabVisible = true;
        FExplorer_11011981->pc1->ActivePage = FExplorer_11011981->tsText;
        ResInfo->GetFormAsText(dfm, FExplorer_11011981->lbText->Items);
        FExplorer_11011981->ShowModal();
        break;
    //As Form
    case 1:
        if (dfm->Open != 2)
        {
            //Если есть открытые формы, закрываем их
            ResInfo->CloseAllForms();

            ShowDfm(dfm);
        }
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowDfm(TDfm* dfm)
{
    if (!dfm)
        return;
        
    //if inherited find parent form
    if ((dfm->Flags & FF_INHERITED) && !dfm->ParentDfm)
        dfm->ParentDfm = ResInfo->GetParentDfm(dfm);

    dfm->Loader = new IdrDfmLoader(0);
    dfm->Form = dfm->Loader->LoadForm(dfm->MemStream, dfm);
    delete dfm->Loader;
    dfm->Loader = 0;

    if (dfm->Form)
    {
        PUnitRec recU = GetUnit(GetClassAdr(dfm->ClassName));
        if (recU)
        {
            int stringLen = sprintf(StringBuf, "[#%03d] %s", recU->iniOrder, dfm->Form->Caption.c_str());
            dfm->Form->Caption = String(StringBuf, stringLen);
        }
        dfm->Open = 2;
        dfm->Form->Show();

        //if (!AnalyzeThread)
        //    sb->Panels->Items[0]->Text = "Press F11 to open form controls tree";
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbFormsKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (lbForms->ItemIndex >= 0 && Key == VK_RETURN) lbFormsDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbCodeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    switch (Key)
    {
    case VK_RETURN:
        lbCodeDblClick(Sender);
        break;
    case VK_ESCAPE:
    	bCodePrevClick(Sender);
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::CleanProject()
{
    int         n;
    PInfoRec    recN;

    if (Image)
    {
        delete[] Image;
        Image = 0;
    }
    if (Flags)
    {
        delete[] Flags;
        Flags = 0;
    }
    if (Infos)
    {
        for (n = 0; n < TotalSize; n++)
        {
            recN = GetInfoRec(Pos2Adr(n));
            if (recN) delete recN;
        }
        delete[] Infos;
        Infos = 0;
    }
    if (BSSInfos)
    {
        for (n = 0; n < BSSInfos->Count; n++)
        {
            recN = (PInfoRec)BSSInfos->Objects[n];
            if (recN) delete recN;
        }

        delete BSSInfos;
        BSSInfos = 0;
    }

    for (n = 0; n < SegmentList->Count; n++)
    {
        PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
        delete segInfo;
    }
    SegmentList->Clear();

    for (n = 0; n < ExpFuncList->Count; n++)
    {
        PExportNameRec recE = (PExportNameRec)ExpFuncList->Items[n];
        delete recE;
    }
    ExpFuncList->Clear();

    for (n = 0; n < ImpFuncList->Count; n++)
    {
        PImportNameRec recI = (PImportNameRec)ImpFuncList->Items[n];
        delete recI;
    }
    ImpFuncList->Clear();

    VmtList->Clear();

    for (n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        delete recU;
    }
    Units->Clear();
    UnitsNum = 0;

    for (n = 0; n < OwnTypeList->Count; n++)
    {
        PTypeRec recT = (PTypeRec)OwnTypeList->Items[n];
        delete recT;
    }
    OwnTypeList->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::CloseProject()
{
    CleanProject();

    ResInfo->CloseAllForms();

    for (int n = 0; n < ResInfo->FormList->Count; n++)
    {
        TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
        delete dfm;
    }

    ResInfo->FormList->Clear();
    ResInfo->Aliases->Clear();
    if (ResInfo->hFormPlugin)
    {
        FreeLibrary(ResInfo->hFormPlugin);
        ResInfo->hFormPlugin = 0;
    }
    ResInfo->Counter = 0;

    OwnTypeList->Clear();

    UnitsSearchList->Clear();
    RTTIsSearchList->Clear();
    UnitItemsSearchList->Clear();
    VMTsSearchList->Clear();
    FormsSearchList->Clear();
    StringsSearchList->Clear();
    NamesSearchList->Clear();

    CodeHistoryPtr = -1;
    CodeHistoryMax = CodeHistoryPtr;
    CodeHistory.Length = 0;

    KnowledgeBase.Close();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesFullClick(TObject *Sender)
{
    TreeSearchFrom = tvClassesFull->Selected;
    WhereSearch = SEARCH_CLASSVIEWER;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesShortClick(TObject *Sender)
{
    BranchSearchFrom = tvClassesShort->Selected;
    WhereSearch = SEARCH_CLASSVIEWER;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FindText(String Text)
{
    int         n, pos, idx = -1;
    String      line, findText, msg;
    TTreeNode*  node;
    TTreeView*  tv;

    if (Text == "") return;
    
    msg = "Search string \"" + Text + "\" not found";

    switch (WhereSearch)
    {
    case SEARCH_UNITS:
        for (n = UnitsSearchFrom; n < lbUnits->Items->Count; n++)
        {
            if (AnsiContainsText(lbUnits->Items->Strings[n], Text))
            {
                idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < UnitsSearchFrom; n++)
            {
                if (AnsiContainsText(lbUnits->Items->Strings[n], Text))
                {
                    idx = n;
                    break;
                }
            }
        }
        if (idx != -1)
        {
        	UnitsSearchFrom = (idx < lbUnits->Items->Count - 1) ? idx + 1 : 0;
            lbUnits->ItemIndex = idx;
            lbUnits->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
        break;
    case SEARCH_UNITITEMS:
        for (n = UnitItemsSearchFrom; n < lbUnitItems->Items->Count; n++)
        {
            if (AnsiContainsText(lbUnitItems->Items->Strings[n], Text))
            {
                idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < UnitItemsSearchFrom; n++)
            {
                if (AnsiContainsText(lbUnitItems->Items->Strings[n], Text))
                {
                    idx = n;
                    break;
                }
            }
		}
        if (idx != -1)
        {
        	UnitItemsSearchFrom = (idx < lbUnitItems->Items->Count) ? idx + 1 : 0;
            lbUnitItems->ItemIndex = idx;
            lbUnitItems->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
        break;
    case SEARCH_RTTIS:
        for (n = RTTIsSearchFrom; n < lbRTTIs->Items->Count; n++)
        {
            if (AnsiContainsText(lbRTTIs->Items->Strings[n], Text))
            {
            	idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < RTTIsSearchFrom; n++)
            {
                if (AnsiContainsText(lbRTTIs->Items->Strings[n], Text))
                {
                	idx = n;
                    break;
                }
            }
        }
        if (idx != -1)
        {
        	RTTIsSearchFrom = (idx < lbRTTIs->Items->Count - 1) ? idx + 1 : 0;
            lbRTTIs->ItemIndex = idx;
        	lbRTTIs->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
        break;
    case SEARCH_FORMS:
        for (n = FormsSearchFrom; n < lbForms->Items->Count; n++)
        {
            if (AnsiContainsText(lbForms->Items->Strings[n], Text))
            {
            	idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < FormsSearchFrom; n++)
            {
                if (AnsiContainsText(lbForms->Items->Strings[n], Text))
                {
                	idx = n;
                    break;
                }
            }
        }
        if (idx != -1)
        {
        	FormsSearchFrom = (idx < lbForms->Items->Count - 1) ? idx + 1 : 0;
            lbForms->ItemIndex = idx;
        	lbForms->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
        break;
    case SEARCH_CLASSVIEWER:
        if (!rgViewerMode->ItemIndex)
        {
        	node = TreeSearchFrom;
            while (node)
            {
            	line = node->Text;
                //Skip <>
                if (line[1] != '<' && AnsiContainsText(line, Text))
                {
                    idx = 0;
                    break;
                }
                node = node->GetNext();
            }
            if (idx == -1 && tvClassesFull->Items->Count)
            {
            	node = tvClassesFull->Items->Item[0];
            	while (node != TreeSearchFrom)
                {
                    line = node->Text;
                    //Skip <>
                    if (line[1] != '<' && AnsiContainsText(line, Text))
                    {
                        idx = 0;
                        break;
                    }
                    node = node->GetNext();
                }
            }
            if (idx != -1)
            {
                TreeSearchFrom = (node->GetNext()) ? node->GetNext() : tvClassesFull->Items->Item[0];
                node->Selected = true;
                node->Expanded = true;
                tvClassesFull->Show();
            }
            else
            {
                ShowMessage(msg);
            }
        }
        else
        {
        	node = BranchSearchFrom;
            while (node)
            {
            	line = node->Text;
                //Skip <>
                if (line[1] != '<' && AnsiContainsText(line, Text))
                {
                    idx = 0;
                    break;
                }
                node = node->GetNext();
            }
            if (idx == -1 && tvClassesShort->Items->Count)
            {
            	node = tvClassesShort->Items->Item[0];
            	while (node != BranchSearchFrom)
                {
                    line = node->Text;
                    //Skip <>
                    if (line[1] != '<' && AnsiContainsText(line, Text))
                    {
                        idx = 0;
                        break;
                    }
                    node = node->GetNext();
                }
            }
            if (idx != -1)
            {
                BranchSearchFrom = (node->GetNext()) ? node->GetNext() : tvClassesShort->Items->Item[0];
                node->Selected = true;
                node->Expanded = true;
                tvClassesShort->Show();
            }
            else
            {
                ShowMessage(msg);
            }
        }
        break;
    case SEARCH_STRINGS:
        for (n = StringsSearchFrom; n < lbStrings->Items->Count; n++)
        {
        	line = lbStrings->Items->Strings[n];
        	pos = line.Pos("'");
        	line = line.SubString(pos + 1, line.Length() - pos);
            if (AnsiContainsText(line, Text))
            {
                idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < StringsSearchFrom; n++)
            {
                line = lbStrings->Items->Strings[n];
                pos = line.Pos("'");
                line = line.SubString(pos + 1, line.Length() - pos);
                if (AnsiContainsText(line, Text))
                {
                    idx = n;
                    break;
                }
            }
        }
        if (idx != -1)
        {
        	StringsSearchFrom = (idx < lbStrings->Items->Count - 1) ? idx + 1 : 0;
            lbStrings->ItemIndex = idx;
            lbStrings->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
    	break;
    case SEARCH_NAMES:
        for (n = NamesSearchFrom; n < lbNames->Items->Count; n++)
        {
        	line = lbNames->Items->Strings[n];
            if (AnsiContainsText(line, Text))
            {
                idx = n;
                break;
            }
        }
        if (idx == -1)
        {
            for (n = 0; n < NamesSearchFrom; n++)
            {
                line = lbNames->Items->Strings[n];
                if (AnsiContainsText(line, Text))
                {
                    idx = n;
                    break;
                }
            }
        }
        if (idx != -1)
        {
        	NamesSearchFrom = (idx < lbNames->Items->Count - 1) ? idx + 1 : 0;
            lbNames->ItemIndex = idx;
            lbNames->SetFocus();
        }
        else
        {
            ShowMessage(msg);
        }
    	break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbFormsMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (lbForms->CanFocus()) ActiveControl = lbForms;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbCodeMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (lbCode->CanFocus()) ActiveControl = lbCode;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesFullMouseMove(
      TObject *Sender, TShiftState Shift, int X, int Y)
{
    if (tvClassesFull->CanFocus()) ActiveControl = tvClassesFull;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::tvClassesShortMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (tvClassesShort->CanFocus()) ActiveControl = tvClassesShort;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::rgViewerModeClick(TObject *Sender)
{
    if (!rgViewerMode->ItemIndex)
        tvClassesFull->BringToFront();
    else
        tvClassesShort->BringToFront();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miClassTreeBuilderClick(TObject *Sender)
{
    miLoadFile->Enabled = false;
    miOpenProject->Enabled = false;
    miMRF->Enabled = false;
    miSaveProject->Enabled = false;
    miSaveDelphiProject->Enabled = false;
    miMapGenerator->Enabled = false;
    miCommentsGenerator->Enabled = false;
    miIDCGenerator->Enabled = false;
    miLister->Enabled = false;
    miClassTreeBuilder->Enabled = false;
    miKBTypeInfo->Enabled = false;
    miCtdPassword->Enabled = false;
    miHex2Double->Enabled = false;

    FProgressBar->Show();

    AnalyzeThread = new TAnalyzeThread(FMain_11011981, FProgressBar, false);
    AnalyzeThread->Resume();
}
//---------------------------------------------------------------------------
//INI FILE
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::IniFileRead()
{
    int         n, m, pos, version; String str, filename, ident;
    TMenuItem   *item;
    TIniFile    *iniFile;
    TFont       *_font;
    TMonitor    *_monitor;

    iniFile = new TIniFile(ChangeFileExt(Application->ExeName, ".ini"));

    _font = new TFont;
    _font->Name = iniFile->ReadString("Settings", "FontName", "Fixedsys");
    _font->Charset = iniFile->ReadInteger("Settings", "FontCharset", 1);
    _font->Size = iniFile->ReadInteger("Settings", "FontSize", 9);
    _font->Color = iniFile->ReadInteger("Settings", "FontColor", 0);
    if (iniFile->ReadBool("Settings", "FontBold", False))
        _font->Style = _font->Style << fsBold;
    if (iniFile->ReadBool("Settings", "FontItalic", False))
        _font->Style = _font->Style << fsItalic;
    SetupAllFonts(_font);
    delete _font;

    WrkDir = iniFile->ReadString("MainForm", "WorkingDir", AppDir);

    for (n = 0; n < Screen->MonitorCount; n++)
    {
        _monitor = Screen->Monitors[n];
        if (_monitor->Primary)
        {
            Left = iniFile->ReadInteger("MainForm", "Left", _monitor->WorkareaRect.Left);
            Top = iniFile->ReadInteger("MainForm", "Top", _monitor->WorkareaRect.Top);
            Width = iniFile->ReadInteger("MainForm", "Width", _monitor->WorkareaRect.Width());
            Height = iniFile->ReadInteger("MainForm", "Height", _monitor->WorkareaRect.Height());
            break;
        }
    }
    pcInfo->Width = iniFile->ReadInteger("MainForm", "LeftWidth", Width / 5);
    pcInfo->ActivePage = tsUnits;
    lbUnitItems->Height = iniFile->ReadInteger("MainForm", "BottomHeight", Height / 8);
    //Most Recent Files

    for (n = 0, m = 0; n < 8; n++)
    {
        ident = "File" + String(n + 1);
        str = iniFile->ReadString("Recent Executable Files", ident, "");
        pos = str.LastDelimiter(",");
        if (pos)
        {
            filename = str.SubString(2, pos - 3);   //Modified by ZGL
            version = str.SubString(pos + 1, str.Length() - pos).ToInt();
        }
        else
        {
            filename = str;
            version = -1;
        }
        if (FileExists(filename))
        {
            item = miMRF->Items[m]; m++;
            item->Caption = filename;
            item->Tag = version;
            item->Visible = (filename != "");
            item->Enabled = true;
        }
        else
        {
            iniFile->DeleteKey("Recent Executable Files", ident);
        }
    }
    for (n = 9, m = 9; n < 17; n++)
    {
        ident = "File" + String(n - 8);
        filename = iniFile->ReadString("Recent Project Files", ident, "");
        if (FileExists(filename))
        {
            item = miMRF->Items[m]; m++;
            item->Caption = filename;
            item->Tag = 0;
            item->Visible = (item->Caption != "");
            item->Enabled = true;
        }
        else
        {
            iniFile->DeleteKey("Recent Project Files", ident);
        }
    }
    delete iniFile;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::IniFileWrite()
{
    TIniFile *iniFile = new TIniFile(ChangeFileExt(Application->ExeName, ".ini"));
    iniFile->WriteString("Settings", "FontName", lbCode->Font->Name);
    iniFile->WriteInteger("Settings", "FontCharset", lbCode->Font->Charset);
    iniFile->WriteInteger("Settings", "FontSize", lbCode->Font->Size);
    iniFile->WriteInteger("Settings", "FontColor", lbCode->Font->Color);
    iniFile->WriteBool("Settings", "FontBold", lbCode->Font->Style.Contains(fsBold));
    iniFile->WriteBool("Settings", "FontItalic", lbCode->Font->Style.Contains(fsItalic));

    iniFile->WriteString("MainForm", "WorkingDir", WrkDir);
    iniFile->WriteInteger("MainForm", "Left", Left);
    iniFile->WriteInteger("MainForm", "Top", Top);
    iniFile->WriteInteger("MainForm", "Width", Width);
    iniFile->WriteInteger("MainForm", "Height", Height);
    iniFile->WriteInteger("MainForm", "LeftWidth", pcInfo->Width);
    iniFile->WriteInteger("MainForm", "BottomHeight", lbUnitItems->Height);

    //Delete all
    int n; String ident;
    for (n = 0; n < 8; n++) iniFile->DeleteKey("Recent Executable Files", "File" + String(n + 1));
    for (n = 9; n < 17; n++) iniFile->DeleteKey("Recent Executable Files", "File" + String(n - 8));

    //Fill
    for (n = 0; n < 8; n++)
    {
        TMenuItem *item = miMRF->Items[n];
        if (item->Visible && item->Enabled) iniFile->WriteString("Recent Executable Files", "File" + String(n + 1), "\"" + item->Caption + "\"," + String(item->Tag));
    }
    for (n = 9; n < 17; n++)
    {
        TMenuItem *item = miMRF->Items[n];
        if (item->Visible && item->Enabled) iniFile->WriteString("Recent Project Files", "File" + String(n - 8), "\"" + item->Caption + "\"");
    }

    delete iniFile;
}
//---------------------------------------------------------------------------
//LOAD EXE AND IDP
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::IsExe(String FileName)
{
    IMAGE_DOS_HEADER    DosHeader;
    IMAGE_NT_HEADERS    NTHeaders;

    FILE* f = fopen(FileName.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_SET);
    //IDD_ERR_NOT_EXECUTABLE
    int readed = fread(&DosHeader, 1, sizeof(IMAGE_DOS_HEADER), f);

    if (readed != sizeof(IMAGE_DOS_HEADER) ||
        DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        fclose(f);
        return false;
    }

    fseek(f, DosHeader.e_lfanew, SEEK_SET);
    //IDD_ERR_NOT_PE_EXECUTABLE
    readed = fread(&NTHeaders, 1, sizeof(IMAGE_NT_HEADERS), f);
    fclose(f);
    if (readed != sizeof(IMAGE_NT_HEADERS) ||
        NTHeaders.Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::IsIdp(String FileName)
{
    char    buf[13];

    FILE* f = fopen(FileName.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_SET);
    fread(buf, 1, 12, f); buf[12] = 0;
    fclose(f);
    
    if (!strcmp(buf, "IDR proj v.3")) return true;
    return false;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miAutodetectVersionClick(TObject *Sender)
{
    LoadDelphiFile(0);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2Click(TObject *Sender)
{
    LoadDelphiFile(2);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi3Click(TObject *Sender)
{
    LoadDelphiFile(3);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi4Click(TObject *Sender)
{
    LoadDelphiFile(4);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi5Click(TObject *Sender)
{
    LoadDelphiFile(5);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi6Click(TObject *Sender)
{
    LoadDelphiFile(6);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi7Click(TObject *Sender)
{
    LoadDelphiFile(7);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2005Click(TObject *Sender)
{
    LoadDelphiFile(2005);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2006Click(TObject *Sender)
{
    LoadDelphiFile(2006);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2007Click(TObject *Sender)
{
    LoadDelphiFile(2007);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2009Click(TObject *Sender)
{
    LoadDelphiFile(2009);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphi2010Click(TObject *Sender)
{
    LoadDelphiFile(2010);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphiXE1Click(TObject *Sender)
{
    LoadDelphiFile(2011);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphiXE2Click(TObject *Sender)
{
    LoadDelphiFile(2012);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphiXE3Click(TObject *Sender)
{
    LoadDelphiFile(2013);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miDelphiXE4Click(TObject *Sender)
{
    LoadDelphiFile(2014);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::LoadFile(String FileName, int version)
{
    if (ProjectModified)
    {
        int res = Application->MessageBox("Save active Project?", "Confirmation", MB_YESNOCANCEL);
        if (res == IDCANCEL) return;
        if (res == IDYES)
        {
            if (IDPFile == "") IDPFile = ChangeFileExt(SourceFile, ".idp");

            SaveDlg->InitialDir = WrkDir;
            SaveDlg->Filter = "IDP|*.idp";
            SaveDlg->FileName = IDPFile;

            if (SaveDlg->Execute()) SaveProject(SaveDlg->FileName);
        }
    }

    if (IsExe(FileName))
    {
        CloseProject();
        Init();
        LoadDelphiFile1(FileName, version, true, true);
    }
    else if (IsIdp(FileName))
    {
        CloseProject();
        Init();
        OpenProject(FileName);
    }
    else
    {
        ShowMessage("File " + FileName + " is not executable or IDR project file");
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::LoadDelphiFile(int version)
{
	DoOpenDelphiFile(version, "", true, true);
}
//---------------------------------------------------------------------------
//version: 0 for autodetect, else - exact version
//
void __fastcall TFMain_11011981::DoOpenDelphiFile(int version, String FileName, bool loadExp, bool loadImp)
{
    if (ProjectModified)
    {
        int res = Application->MessageBox("Save active Project?", "Confirmation", MB_YESNOCANCEL);
        if (res == IDCANCEL) return;
        if (res == IDYES)
        {
            if (IDPFile == "") IDPFile = ChangeFileExt(SourceFile, ".idp");

            SaveDlg->InitialDir = WrkDir;
            SaveDlg->Filter = "IDP|*.idp";
            SaveDlg->FileName = IDPFile;

            if (SaveDlg->Execute()) SaveProject(SaveDlg->FileName);
        }
    }
    if (FileName == "")
    {
    	OpenDlg->InitialDir = WrkDir;
    	OpenDlg->FileName = "";
    	OpenDlg->Filter = "EXE, DLL|*.exe;*.dll|All files|*.*";
        if (OpenDlg->Execute()) FileName = OpenDlg->FileName;
    }
    if (FileName != "")
    {
        if (!FileExists(FileName))
        {
            ShowMessage("File " + FileName + " not exists");
            return;
        }
        CloseProject();
        Init();
        WrkDir = ExtractFileDir(FileName);
        LoadDelphiFile1(FileName, version, loadExp, loadImp);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::LoadDelphiFile1(String FileName, int version, bool loadExp, bool loadImp)
{
    int			pos, ver;
    String		dprName, KBFileName, msg;

    SourceFile = FileName;
    FILE *f = fopen(FileName.c_str(), "rb");

    Screen->Cursor = crHourGlass;
    int res = LoadImage(f, loadExp, loadImp);
    fclose(f);

    if (res <= 0)
    {
        if (!res) ShowMessage("LoadImage error");
        Screen->Cursor = crDefault;
        return;
    }

    FindExports();
    FindImports();

    ResInfo->EnumResources(SourceFile);
    ResInfo->ShowResources(lbForms);
    tsForms->Enabled = (lbForms->Items->Count > 0);

    if (version == DELHPI_VERSION_AUTO)   //Autodetect
    {
        DelphiVersion = GetDelphiVersion();
        if (DelphiVersion == 1)
        {
            Screen->Cursor = crDefault;
            ShowMessage("File " + FileName + " is probably Delphi 4, 5, 6, 7, 2005, 2006 or 2007 file, try manual selection");
            FInputDlg_11011981->Caption = "Enter number of version (4, 5, 6, 7, 2005, 2006 or 2007)";
            FInputDlg_11011981->edtName->Text = "";
            if (FInputDlg_11011981->ShowModal() == mrCancel)
            {
                CleanProject();
                return;
            }
            if (!TryStrToInt(FInputDlg_11011981->edtName->Text.Trim(), DelphiVersion))
            {
                CleanProject();
                return;
            }
        }
        if (DelphiVersion == -1)
        {
            Screen->Cursor = crDefault;
            ShowMessage("File " + FileName + " is probably not Delphi file");
            CleanProject();
            return;
        }
    }
    else
        DelphiVersion = version;
    Screen->Cursor = crDefault;

    UserKnowledgeBase = false;
    if (Application->MessageBox("Use native Knowledge Base?", "Knowledge Base kind selection", MB_YESNO) == IDNO)
    {
        OpenDlg->InitialDir = WrkDir;
    	OpenDlg->FileName = "";
    	OpenDlg->Filter = "BIN|*.bin|All files|*.*";
        if (OpenDlg->Execute())
        {
            KBFileName = OpenDlg->FileName;
            UserKnowledgeBase = true;
        }
    }
    else
        KBFileName = AppDir + "kb" + DelphiVersion + ".bin";

    if (KBFileName == "")
    {
        ShowMessage("Knowledge Base file not selected");
        CleanProject();
        return;
    }

    Screen->Cursor = crHourGlass;
    res = KnowledgeBase.Open(KBFileName.c_str());

    if (!res)
    {
        Screen->Cursor = crDefault;
        ShowMessage("Cannot open Knowledge Base file " + String(KBFileName) + " (may be incorrect Version)");
        CleanProject();
        return;
    }

    SetVmtConsts(DelphiVersion);
    InitSysProcs();

    dprName = ExtractFileName(FileName);
    pos = dprName.Pos(".");
    if (pos) dprName.SetLength(pos - 1);
    
    if (DelphiVersion == 2)
        UnitsNum = GetUnits2(dprName);
    else
        UnitsNum = GetUnits(dprName);

    if (UnitsNum > 0)
    {
        ShowUnits(false);
    }
    else
    {
        //May be BCB file?
        UnitsNum = GetBCBUnits(dprName);
        if (!UnitsNum)
        {
            Screen->Cursor = crDefault;
            ShowMessage("Cannot find table of initialization and finalization procedures");
            CleanProject();
            return;
        }
    }

    if (DelphiVersion <= 2010)
        Caption = "Interactive Delphi Reconstructor by crypto: " + SourceFile + " (Delphi-" + String(DelphiVersion) + ")";
    else
        Caption = "Interactive Delphi Reconstructor by crypto: " + SourceFile + " (Delphi-XE" + String(DelphiVersion - 2010) + ")";

    //Show code to allow user make something useful
    tsCodeView->Enabled = true;
    //ShowCode(EP, 0, -1, -1);

    bEP->Enabled = true;
    //На время загрузки файла отключаем пункты меню
    miLoadFile->Enabled = false;
    miOpenProject->Enabled = false;
    miMRF->Enabled = false;
    miSaveProject->Enabled = false;
    miSaveDelphiProject->Enabled = false;
    lbCXrefs->Enabled = false;

    FProgressBar->Show();

    AnalyzeThread = new TAnalyzeThread(FMain_11011981, FProgressBar, true);
    AnalyzeThread->Resume();

    WrkDir = ExtractFileDir(FileName);
    lbCode->ItemIndex = -1;
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
//Actions after analyzing
void __fastcall TFMain_11011981::AnalyzeThreadDone(TObject* Sender)
{
    if (!AnalyzeThread) return;

    AnalyzeThreadRetVal = AnalyzeThread->GetRetVal();
    if (AnalyzeThread->all && AnalyzeThreadRetVal >= LAST_ANALYZE_STEP)
    {
        ProjectLoaded = true;
        ProjectModified = true;
        AddExe2MRF(SourceFile);
    }

    FProgressBar->Close();
    //Restore menu items
    miLoadFile->Enabled = true;
    miOpenProject->Enabled = true;
    miMRF->Enabled = true;
    miSaveProject->Enabled = true;
    miSaveDelphiProject->Enabled = true;
    lbCXrefs->Enabled = true;

    miEditFunctionC->Enabled = true;
    miEditFunctionI->Enabled = true;
    miFuzzyScanKB->Enabled = true;
    miSearchItem->Enabled = true;
    miName->Enabled = true;
    miViewProto->Enabled = true;
    bDecompile->Enabled = true;

    miMapGenerator->Enabled = true;
    miCommentsGenerator->Enabled = true;
    miIDCGenerator->Enabled = true;
    miLister->Enabled = true;
    miKBTypeInfo->Enabled = true;
    miCtdPassword->Enabled = IsValidCodeAdr(CtdRegAdr);
    miHex2Double->Enabled = true;

    delete AnalyzeThread;
    AnalyzeThread = 0;
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::ImportsValid(DWORD ImpRVA, DWORD ImpSize)
{
    if (ImpRVA || ImpSize)
    {
        DWORD EntryRVA = ImpRVA;
        DWORD EndRVA = ImpRVA + ImpSize;
        IMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

        while (1)
        {
            memmove(&ImportDescriptor, (Image + Adr2Pos(EntryRVA + ImageBase)), sizeof(IMAGE_IMPORT_DESCRIPTOR));

            if (!ImportDescriptor.OriginalFirstThunk &&
                !ImportDescriptor.TimeDateStamp &&
                !ImportDescriptor.ForwarderChain &&
                !ImportDescriptor.Name &&
                !ImportDescriptor.FirstThunk) break;

            if (!IsValidImageAdr(ImportDescriptor.Name + ImageBase)) return false;
            int NameLength = strlen((char*)(Image + Adr2Pos(ImportDescriptor.Name + ImageBase)));
            if (NameLength < 0 || NameLength > 256) return false;
            if (!IsValidModuleName(NameLength, Adr2Pos(ImportDescriptor.Name + ImageBase))) return false;

            EntryRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
            if (EntryRVA >= EndRVA) break;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
int __fastcall TFMain_11011981::LoadImage(FILE* f, bool loadExp, bool loadImp)
{
    int         	        i, n, m, bytes, pos, SectionsNum, ExpNum, NameLength;
    DWORD       	        DataEnd, Items;
    String      	        moduleName, modName, sEP;
    String      	        impFuncName;
    IMAGE_DOS_HEADER        DosHeader;
    IMAGE_NT_HEADERS        NTHeaders;
    PIMAGE_SECTION_HEADER   SectionHeaders;
    char                    segname[9];
    char                    msg[1024];

	fseek(f, 0L, SEEK_SET);
    //IDD_ERR_NOT_EXECUTABLE
    if (fread(&DosHeader, 1, sizeof(IMAGE_DOS_HEADER), f) != sizeof(IMAGE_DOS_HEADER) ||
        DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        ShowMessage("File is not executable");
        return 0;
    }

    fseek(f, DosHeader.e_lfanew, SEEK_SET);
    //IDD_ERR_NOT_PE_EXECUTABLE
    if (fread(&NTHeaders, 1, sizeof(IMAGE_NT_HEADERS), f) != sizeof(IMAGE_NT_HEADERS) ||
        NTHeaders.Signature != IMAGE_NT_SIGNATURE)
    {
        ShowMessage("File is not PE-executable");
        return 0;
    }
    //IDD_ERR_INVALID_PE_EXECUTABLE
    if (NTHeaders.FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER) ||
        NTHeaders.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        ShowMessage("File is invalid 32-bit PE-executable");
        return 0;
    }
    //IDD_ERR_INVALID_PE_EXECUTABLE
    SectionsNum = NTHeaders.FileHeader.NumberOfSections;
    if (!SectionsNum)
    {
        ShowMessage("File is invalid PE-executable");
        return 0;
    }
    //SizeOfOptionalHeader may be > than sizeof(IMAGE_OPTIONAL_HEADER)
    fseek(f, NTHeaders.FileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER), SEEK_CUR);
    SectionHeaders = new IMAGE_SECTION_HEADER[SectionsNum];

    if (fread(SectionHeaders, 1, sizeof(IMAGE_SECTION_HEADER)*SectionsNum, f) !=
        sizeof(IMAGE_SECTION_HEADER)*SectionsNum)
    {
        ShowMessage("Invalid section headers");
        delete[] SectionHeaders;
        return 0;
    }

	ImageBase = NTHeaders.OptionalHeader.ImageBase;
    ImageSize = NTHeaders.OptionalHeader.SizeOfImage;
    EP = NTHeaders.OptionalHeader.AddressOfEntryPoint;

    TotalSize = 0;
    DWORD rsrcVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
    DWORD relocVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    //Fill SegmentList
    for (i = 0; i < SectionsNum; i++)
    {
        PSegmentInfo segInfo = new SegmentInfo;
        segInfo->Start = SectionHeaders[i].VirtualAddress + ImageBase;
        segInfo->Flags = SectionHeaders[i].Characteristics;
        
        if (i + 1 < SectionsNum)
            segInfo->Size = SectionHeaders[i + 1].VirtualAddress - SectionHeaders[i].VirtualAddress;
        else
            segInfo->Size = SectionHeaders[i].Misc.VirtualSize;

        if (!SectionHeaders[i].SizeOfRawData)//uninitialized data
        {
            //segInfo->Size = SectionHeaders[i].Misc.VirtualSize;
            segInfo->Flags |= 0x80000;
        }
        else if (SectionHeaders[i].VirtualAddress == rsrcVA || SectionHeaders[i].VirtualAddress == relocVA)
        {
            //segInfo->Size = SectionHeaders[i].SizeOfRawData;
            segInfo->Flags |= 0x80000;
        }
        else
        {
            //segInfo->Size = SectionHeaders[i].SizeOfRawData;
            TotalSize += segInfo->Size;
        }
        memset(segname, 0, 9);
        memmove(segname, SectionHeaders[i].Name, 8);
        segInfo->Name = String(segname);
        SegmentList->Add((void*)segInfo);
    }
    //DataEnd = TotalSize;

    //Load Image into memory
    Image = new BYTE[TotalSize];
    memset((void*)Image, 0, TotalSize);
    int num;
    BYTE *p = Image;
    for (i = 0; i < SectionsNum; i++)
    {
        if (SectionHeaders[i].VirtualAddress == rsrcVA || SectionHeaders[i].VirtualAddress == relocVA) continue;
        BYTE *sp = p;
        fseek(f, SectionHeaders[i].PointerToRawData, SEEK_SET);
        DWORD Items = SectionHeaders[i].SizeOfRawData;
        if (Items)
        {
            for (n = 0; Items >= MAX_ITEMS; n++)
            {
                fread(p, 1, MAX_ITEMS, f);
                Items -= MAX_ITEMS;
                p += MAX_ITEMS;
            }
            if (Items)
            {
                fread(p, 1, Items, f);
                p += Items;
            }
            num = p - Image;
            if (i + 1 < SectionsNum)
                p = sp + (SectionHeaders[i + 1].VirtualAddress - SectionHeaders[i].VirtualAddress);
        }
    }

    CodeStart = 0;
    Code = Image + CodeStart;
    CodeBase = ImageBase + SectionHeaders[0].VirtualAddress;

    DWORD evalInitTable = EvaluateInitTable(Image, TotalSize, CodeBase);
    if (!evalInitTable)
    {
        ShowMessage("Cannot find initialization table");
        delete[] SectionHeaders;
        delete[] Image;
        Image = 0;
        return 0;
    }

    DWORD evalEP = 0;
    //Find instruction mov eax,offset InitTable
    for (n = 0; n < TotalSize - 5; n++)
    {
        if (Image[n] == 0xB8 && *((DWORD*)(Image + n + 1)) == evalInitTable)
        {
            evalEP = n;
            break;
        }
    }
    //Scan up until bytes 0x55 (push ebp) and 0x8B,0xEC (mov ebp,esp)
    if (evalEP)
    {
        while (evalEP != 0)
        {
            if (Image[evalEP] == 0x55 && Image[evalEP + 1] == 0x8B && Image[evalEP + 2] == 0xEC)
                break;
            evalEP--;
        }
    }
    //Check evalEP
    if (evalEP + CodeBase != NTHeaders.OptionalHeader.AddressOfEntryPoint + ImageBase)
    {
        sprintf(msg, "Possible invalid EP (NTHeader:%lX, Evaluated:%lX). Input valid EP?", NTHeaders.OptionalHeader.AddressOfEntryPoint + ImageBase, evalEP + CodeBase);
        if (Application->MessageBox(msg, "Confirmation", MB_YESNO) == IDYES)
        {
            sEP = InputDialogExec("New EP", "EP:", Val2Str0(NTHeaders.OptionalHeader.AddressOfEntryPoint + ImageBase));
            if (sEP != "")
            {
                sscanf(sEP.c_str(), "%lX", &EP);
                if (!IsValidImageAdr(EP))
                {
                    delete[] SectionHeaders;
                    delete[] Image;
                    Image = 0;
                    return 0;
                }
            }
            else
            {
                delete[] SectionHeaders;
                delete[] Image;
                Image = 0;
                return 0;
            }
        }
        else
        {
            delete[] SectionHeaders;
            delete[] Image;
            Image = 0;
            return 0;
        }
    }
    else
    {
        EP = NTHeaders.OptionalHeader.AddressOfEntryPoint + ImageBase;
    }
    //Find DataStart
    //DWORD _codeEnd = DataEnd;
    //DataStart = CodeStart;
    //for (i = 0; i < SectionsNum; i++)
    //{
    //    if (SectionHeaders[i].VirtualAddress + ImageBase > EP)
    //    {
    //        _codeEnd = SectionHeaders[i].VirtualAddress;
    //        DataStart = SectionHeaders[i].VirtualAddress;
    //        break;
    //    }
    //}
    delete[] SectionHeaders;

    CodeSize = TotalSize;//_codeEnd - SectionHeaders[0].VirtualAddress;
    //DataSize = DataEnd - DataStart;
    //DataBase = ImageBase + DataStart;

    Flags = new DWORD[TotalSize];
    memset(Flags, cfUndef, sizeof(DWORD) * TotalSize);
    Infos = new PInfoRec[TotalSize];
    memset(Infos, 0, sizeof(PInfoRec) * TotalSize);
    BSSInfos = new TStringList;
    BSSInfos->Sorted = true;

    if (loadExp)
    {
        //Load Exports
        DWORD ExpRVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        //DWORD ExpSize = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

        if (ExpRVA)
        {
            IMAGE_EXPORT_DIRECTORY ExportDescriptor;
            memmove(&ExportDescriptor, (Image + Adr2Pos(ExpRVA + ImageBase)), sizeof(IMAGE_EXPORT_DIRECTORY));
            ExpNum = ExportDescriptor.NumberOfFunctions;
            DWORD ExpFuncNamPos = ExportDescriptor.AddressOfNames;
            DWORD ExpFuncAdrPos = ExportDescriptor.AddressOfFunctions;
            DWORD ExpFuncOrdPos = ExportDescriptor.AddressOfNameOrdinals;

            for (i = 0; i < ExpNum; i++)
            {
                PExportNameRec recE = new ExportNameRec;

                DWORD dp = *((DWORD*)(Image + Adr2Pos(ExpFuncNamPos + ImageBase)));
                NameLength = strlen((char*)(Image + Adr2Pos(dp + ImageBase)));
                recE->name = String((char*)(Image + Adr2Pos(dp + ImageBase)), NameLength);

                WORD dw = *((WORD*)(Image + Adr2Pos(ExpFuncOrdPos + ImageBase)));
                recE->address = *((DWORD*)(Image + Adr2Pos(ExpFuncAdrPos + 4*dw + ImageBase))) + ImageBase;
                recE->ord = dw + ExportDescriptor.Base;
                ExpFuncList->Add((void*)recE);

                ExpFuncNamPos += 4;
                ExpFuncOrdPos += 2;
            }
            ExpFuncList->Sort(ExportsCmpFunction);
        }
    }

    DWORD	ImpRVA = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    DWORD	ImpSize = NTHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

    if (loadImp && (ImpRVA || ImpSize))
    {
        if (!ImportsValid(ImpRVA, ImpSize))
        {
            ShowMessage("Imports not valid, will skip!");
        }
        else
        {
            //Load Imports
            DWORD	EntryRVA;		//Next import decriptor RVA
            DWORD   EndRVA;         //End of imports
            DWORD	ThunkRVA;		//RVA of next thunk (from FirstThunk)
            DWORD	LookupRVA;		//RVA of next thunk (from OriginalFirstTunk or FirstThunk)
            DWORD	ThunkValue;		//Value of next thunk (from OriginalFirstTunk or FirstThunk)
            WORD	Hint;			//Ordinal or hint of imported symbol

            IMAGE_IMPORT_DESCRIPTOR ImportDescriptor;

            //DWORD fnProc = 0;

            //First import descriptor
            EntryRVA = ImpRVA;
            EndRVA = ImpRVA + ImpSize;

            while (1)
            {
                memmove(&ImportDescriptor, (Image + Adr2Pos(EntryRVA + ImageBase)), sizeof(IMAGE_IMPORT_DESCRIPTOR));
                //All descriptor fields are NULL - end of list, break
                if (!ImportDescriptor.OriginalFirstThunk &&
                    !ImportDescriptor.TimeDateStamp &&
                    !ImportDescriptor.ForwarderChain &&
                    !ImportDescriptor.Name &&
                    !ImportDescriptor.FirstThunk) break;

                NameLength = strlen((char*)(Image + Adr2Pos(ImportDescriptor.Name + ImageBase)));
                moduleName = String((char*)(Image + Adr2Pos(ImportDescriptor.Name + ImageBase)), NameLength);
            
                int pos = moduleName.Pos(".");
                if (pos)
                    modName = moduleName.SubString(1, pos - 1);
                else
                    modName = moduleName;

                if (-1 == ImpModuleList->IndexOf(moduleName))
                    ImpModuleList->Add(moduleName);

                //HINSTANCE hLib = LoadLibraryEx(moduleName.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);

                //Define the source of import names (OriginalFirstThunk or FirstThunk)
                if (ImportDescriptor.OriginalFirstThunk)
                    LookupRVA = ImportDescriptor.OriginalFirstThunk;
                else
                    LookupRVA = ImportDescriptor.FirstThunk;

                // ThunkRVA get from FirstThunk always
                ThunkRVA = ImportDescriptor.FirstThunk;
                //Get Imported Functions
                while (1)
                {
                    //Names or ordinals get from LookupTable (this table can be inside OriginalFirstThunk or FirstThunk)
                    ThunkValue = *((DWORD*)(Image + Adr2Pos(LookupRVA + ImageBase)));
                    if (!ThunkValue) break;

                    //fnProc = 0;
                    PImportNameRec recI = new ImportNameRec;

                    if (ThunkValue & 0x80000000)
                    {
                        //By ordinal
                        Hint = (WORD)(ThunkValue & 0xFFFF);

                        //if (hLib) fnProc = (DWORD)GetProcAddress(hLib, (char*)Hint);

                        //Addresse get from FirstThunk only
                        //recI->name = modName + "." + String(Hint);
                        recI->name = String(Hint);
                    }
                    else
                    {
                        // by name
                        Hint = *((WORD*)(Image + Adr2Pos(ThunkValue + ImageBase)));
                        NameLength = lstrlen((char*)(Image + Adr2Pos(ThunkValue + 2 + ImageBase)));
                        impFuncName = String((char*)(Image + Adr2Pos(ThunkValue + 2 + ImageBase)), NameLength);

                        //if (hLib)
                        //{
                        //    fnProc = (DWORD)GetProcAddress(hLib, impFuncName.c_str());
                        //    memmove((void*)(Image + ThunkRVA), (void*)&fnProc, sizeof(DWORD));
                        //}

                        recI->name = impFuncName;
                    }
                    recI->module = modName;
                    recI->address = ImageBase + ThunkRVA;
                    ImpFuncList->Add((void*)recI);
                    //
                    SetFlag(cfImport, Adr2Pos(recI->address));
                    PInfoRec recN = new InfoRec(Adr2Pos(recI->address), ikData);
                    recN->SetName(impFuncName);
                    //
                    ThunkRVA += 4;
                    LookupRVA += 4;
                }
                EntryRVA += sizeof(IMAGE_IMPORT_DESCRIPTOR);
                if (EntryRVA >= EndRVA) break;

                //if (hLib)
                //{
                //    FreeLibrary(hLib);
                //    hLib = NULL;
                //}
            }
            ImpFuncList->Sort(ImportsCmpFunction);
        }
    }
    return 1;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miOpenProjectClick(TObject *Sender)
{
	DoOpenProjectFile("");
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::DoOpenProjectFile(String FileName)
{
    char		    *buf;
    int			    n, m, num, len, size, pos;
    PInfoRec        recN;
    PUnitRec	    recU;
    PTypeRec    recT;
    PFIELDINFO      fInfo;

    if (ProjectModified)
    {
        int res = Application->MessageBox("Save active Project?", "Confirmation", MB_YESNOCANCEL);
        if (res == IDCANCEL) return;
        if (res == IDYES)
        {
            if (IDPFile == "") IDPFile = ChangeFileExt(SourceFile, ".idp");

            SaveDlg->InitialDir = WrkDir;
            SaveDlg->Filter = "IDP|*.idp";
            SaveDlg->FileName = IDPFile;

            if (SaveDlg->Execute()) SaveProject(SaveDlg->FileName);
        }
    }
    if (FileName == "")
    {
    	OpenDlg->InitialDir = WrkDir;
    	OpenDlg->FileName = "";
    	OpenDlg->Filter = "IDP|*.idp";
    	if (OpenDlg->Execute()) FileName = OpenDlg->FileName;
    }
    if (FileName != "")
    {
        if (!FileExists(FileName))
        {
            ShowMessage("File " + FileName + " not exists");
            return;
        }
        CloseProject();
        Init();
        WrkDir = ExtractFileDir(FileName);
        OpenProject(FileName);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ReadNode(TStream* stream, TTreeNode* node, char* buf)
{
    //Count
    int itemsCount;
    stream->Read(&itemsCount, sizeof(itemsCount));

    //Text
    int len;
    stream->Read(&len, sizeof(len));
    stream->Read(buf, len);
    node->Text = String(buf, len);
    FProgressBar->pb->StepIt();

    for (int n = 0; n < itemsCount; n++)
    {
        TTreeNode* snode = node->Owner->AddChild(node, "");
        ReadNode(stream, snode, buf);
    }
    Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::OpenProject(String FileName)
{
    bool        _useFuzzy = true;
    int         n, m, k, u, pos, len, num, cnum, evnum, size, infosCnt, bssCnt, _ver;
    int         topIdxU, itemIdxU, topIdxI, itemIdxI, topIdxC;
    String      KBFileName;
    PInfoRec    recN;

    IDPFile = FileName;

    Screen->Cursor = crHourGlass;
    FILE* projectFile = fopen(FileName.c_str(), "rb");
    //Read Delphi version and maximum length of buffer
    fseek(projectFile, 12, SEEK_SET);
    fread(&_ver, sizeof(_ver), 1, projectFile);

    DelphiVersion = _ver & ~(USER_KNOWLEDGEBASE | SOURCE_LIBRARY);
    UserKnowledgeBase = false;
    SourceIsLibrary = (_ver & SOURCE_LIBRARY);
    if (_ver & USER_KNOWLEDGEBASE)
    {
        ShowMessage("Choose original Knowledge Base");
        OpenDlg->InitialDir = WrkDir;
    	OpenDlg->FileName = "";
    	OpenDlg->Filter = "BIN|*.bin|All files|*.*";
        if (OpenDlg->Execute())
        {
            KBFileName = OpenDlg->FileName;
            UserKnowledgeBase = true;
        }
        else
        {
            ShowMessage("Native Knowledge Base will be used!");
            _useFuzzy = false;
        }
    }
    if (!UserKnowledgeBase) KBFileName = AppDir + "kb" + DelphiVersion + ".bin";
    
    MaxBufLen = 0;
    fseek(projectFile, -4L, SEEK_END);
    fread(&MaxBufLen, sizeof(MaxBufLen), 1, projectFile);
    fclose(projectFile);

    if (!KnowledgeBase.Open(KBFileName.c_str()))
    {
        Screen->Cursor = crDefault;
        ShowMessage("Cannot open KnowledgeBase (may be incorrect Version)");
        return;
    }

    Caption = "Interactive Delphi Reconstructor by crypto: " + IDPFile + " (D" + DelphiVersion + ")";

    SetVmtConsts(DelphiVersion);

    //Disable menu items
    miLoadFile->Enabled = false;
    miOpenProject->Enabled = false;
    miMRF->Enabled = false;
    miSaveProject->Enabled = false;
    miSaveDelphiProject->Enabled = false;
    lbCXrefs->Enabled = false;

    Update();

    char* buf = new BYTE[MaxBufLen];
    TMemoryStream* inStream = new TMemoryStream();
    inStream->LoadFromFile(IDPFile);

    char magic[12];
    inStream->Read(magic, 12);
    inStream->Read(&_ver, sizeof(_ver));
    DelphiVersion = _ver & ~(USER_KNOWLEDGEBASE | SOURCE_LIBRARY);

    inStream->Read(&EP, sizeof(EP));
    inStream->Read(&ImageBase, sizeof(ImageBase));
    inStream->Read(&ImageSize, sizeof(ImageSize));
    inStream->Read(&TotalSize, sizeof(TotalSize));
    inStream->Read(&CodeBase, sizeof(CodeBase));
    inStream->Read(&CodeSize, sizeof(CodeSize));
    inStream->Read(&CodeStart, sizeof(CodeStart));

    inStream->Read(&DataBase, sizeof(DataBase));
    inStream->Read(&DataSize, sizeof(DataSize));
    inStream->Read(&DataStart, sizeof(DataStart));
    

    //SegmentList
    inStream->Read(&num, sizeof(num));
    for (n = 0; n < num; n++)
    {
        PSegmentInfo segInfo = new SegmentInfo;
        inStream->Read(&segInfo->Start, sizeof(segInfo->Start));
        inStream->Read(&segInfo->Size, sizeof(segInfo->Size));
        inStream->Read(&segInfo->Flags, sizeof(segInfo->Flags));
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        segInfo->Name = String(buf, len);
        SegmentList->Add((void*)segInfo);
    }

    Image = new BYTE[TotalSize];
    Code = Image + CodeStart;
    Data = Image + DataStart;
    DWORD Items = TotalSize;
    BYTE* pImage = Image;

    while (Items >= MAX_ITEMS)
    {
        inStream->Read(pImage, MAX_ITEMS);
        pImage += MAX_ITEMS;
        Items -= MAX_ITEMS;
    }
    if (Items) inStream->Read(pImage, Items);

    Flags = new DWORD[TotalSize];
    Items = TotalSize;
    DWORD* pFlags = Flags;

    while (Items >= MAX_ITEMS)
    {
        inStream->Read(pFlags, sizeof(DWORD)*MAX_ITEMS);
        pFlags += MAX_ITEMS;
        Items -= MAX_ITEMS;
    }
    if (Items) inStream->Read(pFlags, sizeof(DWORD)*Items);

    Infos = new PInfoRec[TotalSize];
    memset((void*)Infos, 0, sizeof(PInfoRec)*TotalSize);

    inStream->Read(&infosCnt, sizeof(infosCnt));
    BYTE kind;
    for (n = 0; n < TotalSize; n++)
    {
        inStream->Read(&pos, sizeof(pos));
        if (pos == -1) break;
        inStream->Read(&kind, sizeof(kind));
        recN = new InfoRec(pos, kind);
        recN->Load(inStream, buf);
    }
    //BSSInfos
    BSSInfos = new TStringList;
    inStream->Read(&bssCnt, sizeof(bssCnt));
    for (n = 0; n < bssCnt; n++)
    {
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        String _adr = String(buf, len);
        inStream->Read(&kind, sizeof(kind));
        recN = new InfoRec(-1, kind);
        recN->Load(inStream, buf);
        BSSInfos->AddObject(_adr, (TObject*)recN);
    }
    BSSInfos->Sorted = true;

    lbCXrefs->Enabled = true;

    //Units
    inStream->Read(&num, sizeof(num));

    UnitsNum = num;
    for (n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = new UnitRec;
        inStream->Read(&recU->trivial, sizeof(recU->trivial));
        inStream->Read(&recU->trivialIni, sizeof(recU->trivialIni));
        inStream->Read(&recU->trivialFin, sizeof(recU->trivialFin));
        inStream->Read(&recU->kb, sizeof(recU->kb));
        inStream->Read(&recU->fromAdr, sizeof(recU->fromAdr));
        inStream->Read(&recU->toAdr, sizeof(recU->toAdr));
        inStream->Read(&recU->finadr, sizeof(recU->finadr));
        inStream->Read(&recU->finSize, sizeof(recU->finSize));
        inStream->Read(&recU->iniadr, sizeof(recU->iniadr));
        inStream->Read(&recU->iniSize, sizeof(recU->iniSize));
        recU->matchedPercent = 0.0;
        inStream->Read(&recU->iniOrder, sizeof(recU->iniOrder));
        recU->names = new TStringList;
        int namesNum = 0;
        inStream->Read(&namesNum, sizeof(namesNum));
        for (u = 0; u < namesNum; u++)
        {
            inStream->Read(&len, sizeof(len));
            inStream->Read(buf, len);
            SetUnitName(recU, String(buf, len));
        }
        Units->Add((void*)recU);
    }
    UnitSortField = 0;
    CurUnitAdr = 0;
    topIdxU = 0; itemIdxU = -1;
    topIdxI = 0; itemIdxI = -1;
    
    if (UnitsNum)
    {
        inStream->Read(&UnitSortField, sizeof(UnitSortField));
        inStream->Read(&CurUnitAdr, sizeof(CurUnitAdr));
        inStream->Read(&topIdxU, sizeof(topIdxU));
        inStream->Read(&itemIdxU, sizeof(itemIdxU));
        //UnitItems
        if (CurUnitAdr)
        {
      	  	inStream->Read(&topIdxI, sizeof(topIdxI));
        	inStream->Read(&itemIdxI, sizeof(itemIdxI));
        }
    }

    tsUnits->Enabled = true;
    switch (UnitSortField)
    {
    case 0:
        miSortUnitsByAdr->Checked = true;
        miSortUnitsByOrd->Checked = false;
        miSortUnitsByNam->Checked = false;
        break;
    case 1:
        miSortUnitsByAdr->Checked = false;
        miSortUnitsByOrd->Checked = true;
        miSortUnitsByNam->Checked = false;
        break;
    case 2:
        miSortUnitsByAdr->Checked = false;
        miSortUnitsByOrd->Checked = false;
        miSortUnitsByNam->Checked = true;
        break;
    }
    ShowUnits(true);
    lbUnits->TopIndex = topIdxU;
    lbUnits->ItemIndex = itemIdxU;

    ShowUnitItems(GetUnit(CurUnitAdr), topIdxI, itemIdxI);

    miRenameUnit->Enabled = true;
    miSearchUnit->Enabled = true;
    miSortUnits->Enabled = true;
    miCopyList->Enabled = true;

    miEditFunctionC->Enabled = true;
    miEditFunctionI->Enabled = true;
    miFuzzyScanKB->Enabled = true;
    miSearchItem->Enabled = true;

    //Types
    inStream->Read(&num, sizeof(num));
    for (n = 0; n < num; n++)
    {
        PTypeRec recT = new TypeRec;
        inStream->Read(&recT->kind, sizeof(recT->kind));
        inStream->Read(&recT->adr, sizeof(recT->adr));
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        recT->name = String(buf, len);
        OwnTypeList->Add((void*)recT);
    }
    RTTISortField = 0;
    if (num) inStream->Read(&RTTISortField, sizeof(RTTISortField));
    //UpdateRTTIs
    tsRTTIs->Enabled = true;
    miSearchRTTI->Enabled = true;
    miSortRTTI->Enabled = true;

    switch (RTTISortField)
    {
    case 0:
        miSortRTTIsByAdr->Checked = true;
        miSortRTTIsByKnd->Checked = false;
        miSortRTTIsByNam->Checked = false;
        break;
    case 1:
        miSortRTTIsByAdr->Checked = false;
        miSortRTTIsByKnd->Checked = true;
        miSortRTTIsByNam->Checked = false;
        break;
    case 2:
        miSortRTTIsByAdr->Checked = false;
        miSortRTTIsByKnd->Checked = false;
        miSortRTTIsByNam->Checked = true;
        break;
    }
    ShowRTTIs();

    //Forms
    inStream->Read(&num, sizeof(num));
    for (n = 0; n < num; n++)
    {
        TDfm* dfm = new TDfm;
        //Flags
        inStream->Read(&dfm->Flags, sizeof(dfm->Flags));
        //ResName
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        dfm->ResName = String(buf, len);
        //Name
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        dfm->Name = String(buf, len);
        //ClassName
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        dfm->ClassName = String(buf, len);
        //MemStream
        inStream->Read(&size, sizeof(size));
        dfm->MemStream->Size = size;
        while (size >= 4096)
        {
            inStream->Read(buf, 4096);
            dfm->MemStream->Write(buf, 4096);
            size -= 4096;
        }
        if (size)
        {
            inStream->Read(buf, size);
            dfm->MemStream->Write(buf, size);
        }
        //Events
        dfm->Events = new TList;
        inStream->Read(&evnum, sizeof(evnum));
        for (m = 0; m < evnum; m++)
        {
            PEventInfo eInfo = new EventInfo;
            //EventName
            inStream->Read(&len, sizeof(len));
            inStream->Read(buf, len);
            eInfo->EventName = String(buf, len);
            //ProcName
            inStream->Read(&len, sizeof(len));
            inStream->Read(buf, len);
            eInfo->ProcName = String(buf, len);
            dfm->Events->Add((void*)eInfo);
        }
        //Components
        inStream->Read(&cnum, sizeof(cnum));
        if (cnum)
        {
        	dfm->Components = new TList;
            for (m = 0; m < cnum; m++)
            {
                PComponentInfo cInfo = new ComponentInfo;
                //Inherited
                inStream->Read(&cInfo->Inherit, sizeof(cInfo->Inherit));
                //HasGlyph
                inStream->Read(&cInfo->HasGlyph, sizeof(cInfo->HasGlyph));
                //Name
                inStream->Read(&len, sizeof(len));
                inStream->Read(buf, len);
                cInfo->Name = String(buf, len);
                //ClassName
                inStream->Read(&len, sizeof(len));
                inStream->Read(buf, len);
                cInfo->ClassName = String(buf, len);
                //Events
                cInfo->Events = new TList;
                inStream->Read(&evnum, sizeof(evnum));
                for (k = 0; k < evnum; k++)
                {
                    PEventInfo eInfo = new EventInfo;
                    //EventName
                    inStream->Read(&len, sizeof(len));
                    inStream->Read(buf, len);
                    eInfo->EventName = String(buf, len);
                    //ProcName
                    inStream->Read(&len, sizeof(len));
                    inStream->Read(buf, len);
                    eInfo->ProcName = String(buf, len);
                    cInfo->Events->Add((void*)eInfo);
                }
                dfm->Components->Add((void*)cInfo);
            }
        }
        ResInfo->FormList->Add((void*)dfm);
    }
    //UpdateForms
    ResInfo->ShowResources(lbForms);
    //Aliases
    inStream->Read(&num, sizeof(num));
    for (n = 0; n < num; n++)
    {
        inStream->Read(&len, sizeof(len));
        inStream->Read(buf, len);
        ResInfo->Aliases->Add(String(buf, len));
    }
    InitAliases(false);
    tsForms->Enabled = (lbForms->Items->Count > 0);

    //CodeHistory
    inStream->Read(&CodeHistorySize, sizeof(CodeHistorySize));
    inStream->Read(&CodeHistoryPtr, sizeof(CodeHistoryPtr));
    inStream->Read(&CodeHistoryMax, sizeof(CodeHistoryMax));
    bCodePrev->Enabled = (CodeHistoryPtr >= 0);
    bCodeNext->Enabled = (CodeHistoryPtr < CodeHistoryMax);

    CodeHistory.Length = CodeHistorySize;
    for (n = 0; n < CodeHistorySize; n++)
        inStream->Read(&CodeHistory[n], sizeof(PROCHISTORYREC));

    inStream->Read(&CurProcAdr, sizeof(CurProcAdr));
    inStream->Read(&topIdxC, sizeof(topIdxC));

    //Important variables
    inStream->Read(&HInstanceVarAdr, sizeof(HInstanceVarAdr));
    inStream->Read(&LastTls, sizeof(LastTls));

    inStream->Read(&Reserved, sizeof(Reserved));
    inStream->Read(&LastResStrNo, sizeof(LastResStrNo));

	inStream->Read(&CtdRegAdr, sizeof(CtdRegAdr));

    //UpdateVmtList
    FillVmtList();
    //UpdateCode
    tsCodeView->Enabled = true;
    miGoTo->Enabled = true;
    miExploreAdr->Enabled = true;
    miSwitchFlag->Enabled = cbMultipleSelection->Checked;
    bEP->Enabled = true;
    DWORD adr = CurProcAdr;
    CurProcAdr = 0;
    ShowCode(adr, 0, -1, topIdxC);
    //UpdateStrings
    tsStrings->Enabled = true;
    miSearchString->Enabled = true;
    ShowStrings(0);
    //UpdateNames
    tsNames->Enabled = true;
    ShowNames(0);

    Update();

    //Class Viewer
    //Total nodes num (for progress bar)
    int nodesNum;
    inStream->Read(&nodesNum, sizeof(nodesNum));
    if (nodesNum)
    {
        tvClassesFull->Items->BeginUpdate();
        TTreeNode* root = tvClassesFull->Items->Add(0, "");
        ReadNode(inStream, root, buf);
        tvClassesFull->Items->EndUpdate();
        ClassTreeDone = true;
    }
    //UpdateClassViewer
    tsClassView->Enabled = true;
    miViewClass->Enabled = true;
    miSearchVMT->Enabled = true;
    miCollapseAll->Enabled = true;
    miEditClass->Enabled = true;

    if (ClassTreeDone)
    {
        TTreeNode *root = tvClassesFull->Items->Item[0];
        root->Expanded = true;
        rgViewerMode->ItemIndex = 0;
        rgViewerMode->Enabled = true;
        tvClassesFull->BringToFront();
    }
    else
    {
        rgViewerMode->ItemIndex = 1;
        rgViewerMode->Enabled = false;
        tvClassesShort->BringToFront();
    }
    miClassTreeBuilder->Enabled = true;

    //Just cheking
    inStream->Read(&MaxBufLen, sizeof(MaxBufLen));

    if (buf) delete[] buf;
    delete inStream;

    ProjectLoaded = true;
    ProjectModified = false;

    AddIdp2MRF(FileName);

    //Enable lemu items
    miLoadFile->Enabled = true;
    miOpenProject->Enabled = true;
    miMRF->Enabled = true;
    miSaveProject->Enabled = true;
    miSaveDelphiProject->Enabled = true;

    miEditFunctionC->Enabled = true;
    miEditFunctionI->Enabled = true;
    miFuzzyScanKB->Enabled = _useFuzzy;
    miSearchItem->Enabled = true;
    miName->Enabled = true;
    miViewProto->Enabled = true;
    bDecompile->Enabled = true;

    miMapGenerator->Enabled = true;
    miCommentsGenerator->Enabled = true;
    miIDCGenerator->Enabled = true;
    miLister->Enabled = true;
    miKBTypeInfo->Enabled = true;
    miCtdPassword->Enabled = IsValidCodeAdr(CtdRegAdr);
    miHex2Double->Enabled = true;

    WrkDir = ExtractFileDir(FileName);
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe1Click(TObject *Sender)
{
    LoadFile(miExe1->Caption, miMRF->Items[0]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe2Click(TObject *Sender)
{
    LoadFile(miExe2->Caption, miMRF->Items[1]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe3Click(TObject *Sender)
{
    LoadFile(miExe3->Caption, miMRF->Items[2]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe4Click(TObject *Sender)
{
    LoadFile(miExe4->Caption, miMRF->Items[3]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe5Click(TObject *Sender)
{
    LoadFile(miExe5->Caption, miMRF->Items[4]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe6Click(TObject *Sender)
{
    LoadFile(miExe6->Caption, miMRF->Items[5]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe7Click(TObject *Sender)
{
    LoadFile(miExe7->Caption, miMRF->Items[6]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miExe8Click(TObject *Sender)
{
    LoadFile(miExe8->Caption, miMRF->Items[7]->Tag);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp1Click(TObject *Sender)
{
    LoadFile(miIdp1->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp2Click(TObject *Sender)
{
    LoadFile(miIdp2->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp3Click(TObject *Sender)
{
    LoadFile(miIdp3->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp4Click(TObject *Sender)
{
    LoadFile(miIdp4->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp5Click(TObject *Sender)
{
    LoadFile(miIdp5->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp6Click(TObject *Sender)
{
    LoadFile(miIdp6->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp7Click(TObject *Sender)
{
    LoadFile(miIdp7->Caption, -1);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIdp8Click(TObject *Sender)
{
    LoadFile(miIdp8->Caption, -1);
}
//---------------------------------------------------------------------------
//SAVE PROJECT
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSaveProjectClick(TObject *Sender)
{
    if (IDPFile == "") IDPFile = ChangeFileExt(SourceFile, ".idp");

    SaveDlg->InitialDir = WrkDir;
    SaveDlg->Filter = "IDP|*.idp";
    SaveDlg->FileName = IDPFile;

    if (SaveDlg->Execute()) SaveProject(SaveDlg->FileName);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::WriteNode(TStream* stream, TTreeNode* node)
{
    //Count
    int itemsCount = node->Count;
    stream->Write(&itemsCount, sizeof(itemsCount));
    FProgressBar->pb->StepIt();

    //Text
    int len = node->Text.Length(); if (len > MaxBufLen) MaxBufLen = len;
    stream->Write(&len, sizeof(len));
    stream->Write(node->Text.c_str(), len);

    for (int n = 0; n < itemsCount; n++)
    {
        WriteNode(stream, node->Item[n]);
    }
    Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::SaveProject(String FileName)
{
    int             n, m, k, len, num, cnum, evnum, size, pos, res, infosCnt, topIdx, itemIdx;
    TMemoryStream*    outStream = 0;
    BYTE            buf[4096];

    if (FileExists(FileName))
    {
        if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
    }

    Screen->Cursor = crHourGlass;
    IDPFile = FileName;

    try
    {
        outStream = new TMemoryStream();

        FProgressBar->Show();

        char* magic = "IDR proj v.3";
        outStream->Write(magic, 12);
        int _ver = DelphiVersion;
        if (UserKnowledgeBase) _ver |= USER_KNOWLEDGEBASE;
        if (SourceIsLibrary) _ver |= SOURCE_LIBRARY;
        outStream->Write(&_ver, sizeof(_ver));

        outStream->Write(&EP, sizeof(EP));
        outStream->Write(&ImageBase, sizeof(ImageBase));
        outStream->Write(&ImageSize, sizeof(ImageSize));
        outStream->Write(&TotalSize, sizeof(TotalSize));
        outStream->Write(&CodeBase, sizeof(CodeBase));
        outStream->Write(&CodeSize, sizeof(CodeSize));
        outStream->Write(&CodeStart, sizeof(CodeStart));

        outStream->Write(&DataBase, sizeof(DataBase));
        outStream->Write(&DataSize, sizeof(DataSize));
        outStream->Write(&DataStart, sizeof(DataStart));
        //SegmentList
        num = SegmentList->Count;
        outStream->Write(&num, sizeof(num));
        for (n = 0; n < num; n++)
        {
            PSegmentInfo segInfo = (PSegmentInfo)SegmentList->Items[n];
            outStream->Write(&segInfo->Start, sizeof(segInfo->Start));
            outStream->Write(&segInfo->Size, sizeof(segInfo->Size));
            outStream->Write(&segInfo->Flags, sizeof(segInfo->Flags));
            len = segInfo->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(segInfo->Name.c_str(), len);
        }

        DWORD Items = TotalSize;
        BYTE *pImage = Image;
        FProgressBar->StartProgress("Writing Image...", "", (Items + MAX_ITEMS - 1)/MAX_ITEMS);
        while (Items >= MAX_ITEMS)
        {
            FProgressBar->pb->StepIt();
            outStream->Write(pImage, MAX_ITEMS);
            pImage += MAX_ITEMS;
            Items -= MAX_ITEMS;
        }
        if (Items) outStream->Write(pImage, Items);

        Items = TotalSize;
        DWORD *pFlags = Flags;
        FProgressBar->StartProgress("Writing Flags...", "", (Items + MAX_ITEMS - 1)/MAX_ITEMS);
        while (Items >= MAX_ITEMS)
        {
            FProgressBar->pb->StepIt();
            outStream->Write(pFlags, sizeof(DWORD)*MAX_ITEMS);
            pFlags += MAX_ITEMS;
            Items -= MAX_ITEMS;
        }
        if (Items) outStream->Write(pFlags, sizeof(DWORD)*Items);

        infosCnt = 0;
        for (n = 0; n < TotalSize; n++)
        {
            PInfoRec recN = GetInfoRec(Pos2Adr(n));
            if (recN) infosCnt++;
        }
        outStream->Write(&infosCnt, sizeof(infosCnt));

        FProgressBar->StartProgress("Writing Infos Objects (number = " + String(infosCnt) + ")...", "", TotalSize / 4096);
        MaxBufLen = 0;
        BYTE kind;
        try
        {
            for (n = 0; n < TotalSize; n++)
            {
                if ((n & 4095) == 0)
                {
                    FProgressBar->pb->StepIt();
                    Application->ProcessMessages();
                }

                PInfoRec recN = GetInfoRec(Pos2Adr(n));
                if (recN)
                {
                    //Position
                    pos = n;
                    outStream->Write(&pos, sizeof(pos));
                    kind = recN->kind;
                    outStream->Write(&kind, sizeof(kind));
                    recN->Save(outStream);
                }
            }
        }
        catch (Exception &exception)
        {
            ShowMessage("Error at " + Val2Str8(Pos2Adr(n)));
        }
        //Last position = -1 -> end of items
        pos = -1; outStream->Write(&pos, sizeof(pos));

        //BSSInfos
        String _adr;
        int bssCnt = BSSInfos->Count;
        outStream->Write(&bssCnt, sizeof(bssCnt));
        for (n = 0; n < bssCnt; n++)
        {
            _adr = BSSInfos->Strings[n];
            len = _adr.Length();  if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(_adr.c_str(), len);
            PInfoRec recN = (PInfoRec)BSSInfos->Objects[n];
            kind = recN->kind;
            outStream->Write(&kind, sizeof(kind));
            recN->Save(outStream);
        }

        //Units
        num = UnitsNum;
        FProgressBar->StartProgress("Writing Units (number = "+String(num)+")...", "", num);
        outStream->Write(&num, sizeof(num));
        for (n = 0; n < num; n++)
        {
            FProgressBar->pb->StepIt();
            Application->ProcessMessages();
            PUnitRec recU = (PUnitRec)Units->Items[n];
            outStream->Write(&recU->trivial, sizeof(recU->trivial));
            outStream->Write(&recU->trivialIni, sizeof(recU->trivialIni));
            outStream->Write(&recU->trivialFin, sizeof(recU->trivialFin));
            outStream->Write(&recU->kb, sizeof(recU->kb));
            outStream->Write(&recU->fromAdr, sizeof(recU->fromAdr));
            outStream->Write(&recU->toAdr, sizeof(recU->toAdr));
            outStream->Write(&recU->finadr, sizeof(recU->finadr));
            outStream->Write(&recU->finSize, sizeof(recU->finSize));
            outStream->Write(&recU->iniadr, sizeof(recU->iniadr));
            outStream->Write(&recU->iniSize, sizeof(recU->iniSize));
            outStream->Write(&recU->iniOrder, sizeof(recU->iniOrder));
            int namesNum = recU->names->Count;
            outStream->Write(&namesNum, sizeof(namesNum));
            for (int u = 0; u < namesNum; u++)
            {
                len = recU->names->Strings[u].Length(); if (len > MaxBufLen) MaxBufLen = len;
                outStream->Write(&len, sizeof(len));
                outStream->Write(recU->names->Strings[u].c_str(), len);
            }
        }
        if (num)
        {
            outStream->Write(&UnitSortField, sizeof(UnitSortField));
            outStream->Write(&CurUnitAdr, sizeof(CurUnitAdr));
            topIdx = lbUnits->TopIndex;
            outStream->Write(&topIdx, sizeof(topIdx));
            itemIdx = lbUnits->ItemIndex;
            outStream->Write(&itemIdx, sizeof(itemIdx));
            //UnitItems
            if (CurUnitAdr)
            {
            	topIdx = lbUnitItems->TopIndex;
            	outStream->Write(&topIdx, sizeof(topIdx));
                itemIdx = lbUnitItems->ItemIndex;
                outStream->Write(&itemIdx, sizeof(itemIdx));
            }
        }

        //Types
        num = OwnTypeList->Count;
        FProgressBar->StartProgress("Writing Types (number = "+String(num)+")...", "", num);
        outStream->Write(&num, sizeof(num));
        for (n = 0; n < num; n++)
        {
            FProgressBar->pb->StepIt();
            Application->ProcessMessages();
            PTypeRec recT = (PTypeRec)OwnTypeList->Items[n];
            outStream->Write(&recT->kind, sizeof(recT->kind));
            outStream->Write(&recT->adr, sizeof(recT->adr));
            len = recT->name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(recT->name.c_str(), len);
        }
        if (num) outStream->Write(&RTTISortField, sizeof(RTTISortField));

        //Forms
        num = ResInfo->FormList->Count;
        FProgressBar->StartProgress("Writing Forms (number = "+String(num)+")...", "", num);
        outStream->Write(&num, sizeof(num));
        for (n = 0; n < num; n++)
        {
            FProgressBar->pb->StepIt();
            Application->ProcessMessages();
            TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
            //Flags
            outStream->Write(&dfm->Flags, sizeof(dfm->Flags));
            //ResName
            len = dfm->ResName.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(dfm->ResName.c_str(), len);
            //Name
            len = dfm->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(dfm->Name.c_str(), len);
            //ClassName
            len = dfm->ClassName.Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(dfm->ClassName.c_str(), len);
            //MemStream
            size = dfm->MemStream->Size; if (4096 > MaxBufLen) MaxBufLen = 4096;
            outStream->Write(&size, sizeof(size));
            dfm->MemStream->Seek(0, soFromBeginning);
            while (size >= 4096)
            {
                dfm->MemStream->Read(buf, 4096);
                outStream->Write(buf, 4096);
                size -= 4096;
            }
            if (size)
            {
                dfm->MemStream->Read(buf, size);
                outStream->Write(buf, size);
            }
            //Events
            evnum = (dfm->Events) ? dfm->Events->Count : 0;
            outStream->Write(&evnum, sizeof(evnum));
            for (m = 0; m < evnum; m++)
            {
            	PEventInfo eInfo = (PEventInfo)dfm->Events->Items[m];
                //EventName
                len = eInfo->EventName.Length(); if (len > MaxBufLen) MaxBufLen = len;
                outStream->Write(&len, sizeof(len));
                outStream->Write(eInfo->EventName.c_str(), len);
                //ProcName
                len = eInfo->ProcName.Length(); if (len > MaxBufLen) MaxBufLen = len;
                outStream->Write(&len, sizeof(len));
                outStream->Write(eInfo->ProcName.c_str(), len);
            }
            //Components
            cnum = (dfm->Components) ? dfm->Components->Count : 0;
            outStream->Write(&cnum, sizeof(cnum));
            for (m = 0; m < cnum; m++)
            {
                PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
                //Inherited
                outStream->Write(&cInfo->Inherit, sizeof(cInfo->Inherit));
                //HasGlyph
                outStream->Write(&cInfo->HasGlyph, sizeof(cInfo->HasGlyph));
                //Name
                len = cInfo->Name.Length(); if (len > MaxBufLen) MaxBufLen = len;
                outStream->Write(&len, sizeof(len));
                outStream->Write(cInfo->Name.c_str(), len);
                //ClassName
                len = cInfo->ClassName.Length(); if (len > MaxBufLen) MaxBufLen = len;
                outStream->Write(&len, sizeof(len));
                outStream->Write(cInfo->ClassName.c_str(), len);
                //Events
                evnum = (cInfo->Events) ? cInfo->Events->Count : 0;
                outStream->Write(&evnum, sizeof(evnum));
                for (k = 0; k < evnum; k++)
                {
                    PEventInfo eInfo = (PEventInfo)cInfo->Events->Items[k];
                    //EventName
                    len = eInfo->EventName.Length(); if (len > MaxBufLen) MaxBufLen = len;
                    outStream->Write(&len, sizeof(len));
                    outStream->Write(eInfo->EventName.c_str(), len);
                    //ProcName
                    len = eInfo->ProcName.Length(); if (len > MaxBufLen) MaxBufLen = len;
                    outStream->Write(&len, sizeof(len));
                    outStream->Write(eInfo->ProcName.c_str(), len);
                }
            }
        }
        //Aliases
        num = ResInfo->Aliases->Count;
        FProgressBar->StartProgress("Writing Aliases  (number = "+String(num)+")...", "", num);
        outStream->Write(&num, sizeof(num));
        for (n = 0; n < num; n++)
        {
            FProgressBar->pb->StepIt();
            Application->ProcessMessages();
            len = ResInfo->Aliases->Strings[n].Length(); if (len > MaxBufLen) MaxBufLen = len;
            outStream->Write(&len, sizeof(len));
            outStream->Write(ResInfo->Aliases->Strings[n].c_str(), len);
        }

        //CodeHistory
        outStream->Write(&CodeHistorySize, sizeof(CodeHistorySize));
        outStream->Write(&CodeHistoryPtr, sizeof(CodeHistoryPtr));
        outStream->Write(&CodeHistoryMax, sizeof(CodeHistoryMax));
        PROCHISTORYREC phRec;
        FProgressBar->StartProgress("Writing Code History Items (number = "+String(CodeHistorySize)+")...", "", CodeHistorySize);
        for (n = 0; n < CodeHistorySize; n++)
        {
            FProgressBar->pb->StepIt();
            Application->ProcessMessages();
            outStream->Write(&CodeHistory[n], sizeof(PROCHISTORYREC));
        }

        outStream->Write(&CurProcAdr, sizeof(CurProcAdr));
        topIdx = lbCode->TopIndex;
        outStream->Write(&topIdx, sizeof(topIdx));

        //Important variables
        outStream->Write(&HInstanceVarAdr, sizeof(HInstanceVarAdr));
        outStream->Write(&LastTls, sizeof(LastTls));

        outStream->Write(&Reserved, sizeof(Reserved));
        outStream->Write(&LastResStrNo, sizeof(LastResStrNo));

        outStream->Write(&CtdRegAdr, sizeof(CtdRegAdr));

        FProgressBar->Close();
        //Class Viewer
        //Total nodes (for progress)
        num = 0; if (ClassTreeDone) num = tvClassesFull->Items->Count;
        if (num && Application->MessageBox("Save full Tree of Classes?", "Warning", MB_YESNO) == IDYES)
        {
            FProgressBar->Show();
            outStream->Write(&num, sizeof(num));
            if (num)
            {
                FProgressBar->StartProgress("Writing ClassViewer Tree Nodes (number = "+String(num)+")...", "", num);
                TTreeNode* root = tvClassesFull->Items->GetFirstNode();
                WriteNode(outStream, root);
            }
            FProgressBar->Close();
        }
        else
        {
            num = 0;
            outStream->Write(&num, sizeof(num));
        }
        //At end write MaxBufLen
        outStream->Write(&MaxBufLen, sizeof(MaxBufLen));
        outStream->SaveToFile(IDPFile);
        delete outStream;

        ProjectModified = false;

        AddIdp2MRF(FileName);
    }
    catch (EFCreateError &E)
    {
        ShowMessage("Cannot open output file " + IDPFile);
    }

    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AddExe2MRF(String FileName)
{
    int     n, m;
    for (n = 0; n < 8; n++)
    {
        TMenuItem* item = miMRF->Items[n];
        if (SameText(FileName, item->Caption)) break;
    }
    if (n == 8) n--;

    for (m = n; m >= 1; m--)
    {
        miMRF->Items[m]->Caption = miMRF->Items[m - 1]->Caption;
        miMRF->Items[m]->Tag = miMRF->Items[m - 1]->Tag;
        miMRF->Items[m]->Visible = (miMRF->Items[m]->Caption != "");
    }
    miMRF->Items[0]->Caption = FileName;
    miMRF->Items[0]->Tag = DelphiVersion;
    miMRF->Items[0]->Visible = true;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AddIdp2MRF(String FileName)
{
    int     n, m;
    for (n = 9; n < 17; n++)
    {
        TMenuItem* item = miMRF->Items[n];
        if (SameText(FileName, item->Caption)) break;
    }
    if (n == 17) n--;

    for (m = n; m >= 10; m--)
    {
        miMRF->Items[m]->Caption = miMRF->Items[m - 1]->Caption;
        miMRF->Items[m]->Visible = (miMRF->Items[m]->Caption != "");
    }
    miMRF->Items[9]->Caption = FileName;
    miMRF->Items[9]->Visible = true;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miKBTypeInfoClick(TObject *Sender)
{
    int         idx;
    MProcInfo   pInfo;
    MTypeInfo   tInfo;
    String      typeName, className, propName, sName;

    sName = InputDialogExec("Enter Type Name", "Name:", "");
    if (sName != "")
    {
        //Procedure
        if (KnowledgeBase.GetKBProcInfo(sName, &pInfo, &idx))
        {
            FTypeInfo_11011981->memDescription->Clear();
            FTypeInfo_11011981->memDescription->Lines->Add(KnowledgeBase.GetProcPrototype(&pInfo));
            FTypeInfo_11011981->ShowModal();
            return;
        }
        //Type
        if (KnowledgeBase.GetKBTypeInfo(sName, &tInfo))
        {
            FTypeInfo_11011981->ShowKbInfo(&tInfo);
            return;
        }
        //Property
        className = ExtractClassName(sName);
        propName = ExtractProcName(sName);
        while (1)
        {
            if (KnowledgeBase.GetKBPropertyInfo(className, propName, &tInfo))
            {
                FTypeInfo_11011981->ShowKbInfo(&tInfo);
                return;
            }
            className = GetParentName(className);
            if (className == "") break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormResize(TObject *Sender)
{
    lbCode->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::EditFunction(DWORD Adr)
{
    BYTE        tag, callKind;
    DWORD       adr, rootAdr;
    int         n, m, a, cnt, size, ofs, offset, dotpos;
    char        *p;
    PUnitRec    recU;
    PInfoRec    recN, recN1;
    PXrefRec 	recX;
    PVmtListRec recV;
    PMethodRec  recM;
    PARGINFO 	argInfo;
    String      line, name, typeDef, item, className, procName;
    char        buf[1024];

	//if (Adr == EP) return;

    recU = GetUnit(Adr);
    if (!recU) return;
    if (Adr == recU->iniadr || Adr == recU->finadr) return;

    recN = GetInfoRec(Adr);
    if (recN)
    {
        FEditFunctionDlg_11011981->Adr = Adr;

        if (FEditFunctionDlg_11011981->ShowModal() == mrOk)
        {
            //local vars
            if (0)//recN->info.procInfo->locals)
            {
                cnt = FEditFunctionDlg_11011981->lbVars->Count;
                recN->procInfo->DeleteLocals();
                for (n = 0; n < cnt; n++)
                {
                    line = FEditFunctionDlg_11011981->lbVars->Items->Strings[n];
                    //'-' - deleted line
                    strcpy(buf, line.c_str());
                    p = strtok(buf, " ");
                    //offset
                    sscanf(p, "%lX", &offset);
                    //size
                    p = strtok(0, " ");
                    sscanf(p, "%lX", &size);
                    //name
                    p = strtok(0, " :");
                    if (stricmp(p, "?"))
                        name = String(p).Trim();
                    else
                        name = "";
                    //type
                    p = strtok(0, " ");
                    if (stricmp(p, "?"))
                        typeDef = String(p).Trim();
                    else
                        typeDef = "";
                    recN->procInfo->AddLocal(-offset, size, name, typeDef);
                }
            }

            ClearFlag(cfPass2, Adr2Pos(Adr));
            AnalyzeProc2(Adr, false, false);
            AnalyzeArguments(Adr);

            //If virtual then propogate VMT names
            //!!! prototype !!!
            procName = ExtractProcName(recN->GetName());
            if (recN->procInfo->flags & PF_VIRTUAL)
            {
                cnt = recN->xrefs->Count;
                for (n = 0; n < cnt; n++)
                {
                    recX = (PXrefRec)recN->xrefs->Items[n];
                    if (recX->type == 'D')
                    {
                        recN1 = GetInfoRec(recX->adr);
                        ofs = GetMethodOfs(recN1, Adr);
                        if (ofs != -1)
                        {
                            //Down (to root)
                            adr = recX->adr; rootAdr = adr;
                            while (adr)
                            {
                                recM = GetMethodInfo(adr, 'V', ofs);
                                if (recM) rootAdr = adr;
                                adr = GetParentAdr(adr);
                            }
                            //Up (all classes that inherits rootAdr)
                            for (m = 0; m < VmtList->Count; m++)
                            {
                                recV = (PVmtListRec)VmtList->Items[m];
                                if (IsInheritsByAdr(recV->vmtAdr, rootAdr))
                                {
                                    recM = GetMethodInfo(recV->vmtAdr, 'V', ofs);
                                    if (recM)
                                    {
                                        className = GetClsName(recV->vmtAdr);
                                        recM->name = className + "." + procName;
                                        if (recM->address != Adr && !recM->abstract)
                                        {
                                            recN1 = GetInfoRec(recM->address);
                                            if (!recN1->HasName())
                                                recN1->SetName(className + "." + procName);
                                            else
                                            {
                                                dotpos = recN1->GetName().Pos(".");
                                                recN1->SetName(recN1->GetName().SubString(1, dotpos) + procName);
                                            }
                                            //recN1->name = className + "." + procName;
                                            recN1->kind = recN->kind;
                                            recN1->type = recN->type;
                                            recN1->procInfo->flags |= PF_VIRTUAL;
                                            recN1->procInfo->DeleteArgs();
                                            recN1->procInfo->AddArg(0x21, 0, 4, "Self", className);
                                            for (a = 1; a < recN->procInfo->args->Count; a++)
                                            {
                                                argInfo = (PARGINFO)recN->procInfo->args->Items[a];
                                                recN1->procInfo->AddArg(argInfo);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            //DWORD adr = CurProcAdr;

            //Edit current proc
            if (Adr == CurProcAdr)
            {
                RedrawCode();
                //Current proc from current unit
                if (recU->fromAdr == CurUnitAdr)
                    ShowUnitItems(recU, lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
            }
            else
            {
                ShowUnitItems(recU, lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
            }
            ProjectModified = true;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miEditFunctionCClick(TObject *Sender)
{
	EditFunction(CurProcAdr);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miMapGeneratorClick(TObject *Sender)
{
    String  procName;

    String  mapName = "";
    if (SourceFile != "") mapName = ChangeFileExt(SourceFile, ".map");
    if (IDPFile != "") mapName = ChangeFileExt(IDPFile, ".map");

    SaveDlg->InitialDir = WrkDir;
    SaveDlg->Filter = "MAP|*.map";
    SaveDlg->FileName = mapName;

    if (!SaveDlg->Execute()) return;

    mapName = SaveDlg->FileName;
    if (FileExists(mapName))
    {
        if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
    }

    Screen->Cursor = crHourGlass;
    FILE *fMap = fopen(mapName.c_str(), "wt+");
    if (!fMap)
    {
        ShowMessage("Cannot open map file");
        return;
    }
    fprintf(fMap, "\n Start         Length     Name                   Class\n");
    fprintf(fMap, " 0001:00000000 %09XH CODE                   CODE\n", CodeSize);
    fprintf(fMap, "\n\n  Address         Publics by Value\n\n");

    for (int n = 0; n < CodeSize; n++)
    {
        if (IsFlagSet(cfProcStart, n) && !IsFlagSet(cfEmbedded, n))
        {
            int adr = Pos2Adr(n);
            PInfoRec recN = GetInfoRec(adr);
            if (recN)
            {
                if (adr != EP)
                {
                    PUnitRec recU = GetUnit(adr);
                    if (recU)
                    {
                        String moduleName = GetUnitName(recU);
                        if (adr == recU->iniadr)
                            procName = "Initialization";
                        else if (adr == recU->finadr)
                            procName = "Finalization";
                        else
                            procName = recN->MakeMapName(adr);

                        fprintf(fMap, " 0001:%08X       %s.%s\n", n, moduleName.c_str(), procName.c_str());
                    }
                    else
                    {
                        procName = recN->MakeMapName(adr);
                        fprintf(fMap, " 0001:%08X       %s\n", n, procName.c_str());
                    }
                    //if (!IsFlagSet(cfImport, n))
                    //{
                    //    fprintf(fMap, "%lX %s\n", adr, recN->MakePrototype(adr, true, true, false, true, false).c_str());
                    //}
                }
                else
                {
                    fprintf(fMap, " 0001:%08X       EntryPoint\n", n);
                }
            }
        }
    }

    fprintf(fMap, "\nProgram entry point at 0001:%08X\n", EP - CodeBase);
    fclose(fMap);
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCommentsGeneratorClick(TObject *Sender)
{
	String		line;

    String txtName = "";
    if (SourceFile != "") txtName = ChangeFileExt(SourceFile, ".txt");
    if (IDPFile != "") txtName = ChangeFileExt(IDPFile, ".txt");
    
    SaveDlg->InitialDir = WrkDir;
    SaveDlg->Filter = "TXT|*.txt";
    SaveDlg->FileName = txtName;

    if (!SaveDlg->Execute()) return;

    txtName = SaveDlg->FileName;

    if (FileExists(txtName))
    {
        if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
    }

	Screen->Cursor = crHourGlass;

    FILE* lstF = fopen(txtName.c_str(), "wt+");
    /*
    for (int n = 0; n < CodeSize; n++)
    {
        PInfoRec recN = GetInfoRec(Pos2Adr(n));
        if (recN && recN->picode) fprintf(lstF, "C %08lX  %s\n", CodeBase + n, MakeComment(recN->picode).c_str());
    }
    */
    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        if (recU->kb || recU->trivial) continue;

        for (DWORD adr = recU->fromAdr; adr < recU->toAdr; adr++)
        {
            if (adr == recU->finadr)
            {
            	if (!recU->trivialFin) OutputCode(lstF, adr, "", true);
                continue;
            }
            if (adr == recU->iniadr)
            {
            	if (!recU->trivialIni) OutputCode(lstF, adr, "", true);
                continue;
            }

            int pos = Adr2Pos(adr);
            PInfoRec recN = GetInfoRec(adr);
            if (!recN) continue;

            BYTE kind = recN->kind;

            if (kind == ikProc        ||
                kind == ikFunc        ||
                kind == ikConstructor ||
                kind == ikDestructor)
            {
               	OutputCode(lstF, adr, "", true);
                continue;
            }

            if (IsFlagSet(cfProcStart, pos))
            {
                if (recN->kind == ikConstructor)
                {
                    OutputCode(lstF, adr, "", true);
                }
                else if (recN->kind == ikDestructor)
                {
                    OutputCode(lstF, adr, "", true);
                }
                else
                    OutputCode(lstF, adr, "", true);
            }
        }
    }
    fclose(lstF);
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miIDCGeneratorClick(TObject *Sender)
{
    String idcName = "", idcTemplate = "";
    if (SourceFile != "")
    {
        idcName = ChangeFileExt(SourceFile, ".idc");
        idcTemplate = ChangeFileExt(SourceFile, "");
    }
    if (IDPFile != "")
    {
        idcName = ChangeFileExt(IDPFile, ".idc");
        idcTemplate = ChangeFileExt(IDPFile, "");
    }

    TSaveIDCDialog* SaveIDCDialog = new TSaveIDCDialog(this, "SAVEIDCDLG");
    SaveIDCDialog->InitialDir = WrkDir;
    SaveIDCDialog->Filter = "IDC|*.idc";
    SaveIDCDialog->FileName = idcName;
 
    if (!SaveIDCDialog->Execute()) return;

    idcName = SaveIDCDialog->FileName;
    delete SaveIDCDialog;

    if (FileExists(idcName))
    {
        if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
    }

    if (SplitIDC)
    {
        if (FIdcSplitSize->ShowModal() == mrCancel) return;
    }

	Screen->Cursor = crHourGlass;

    FILE* idcF = fopen(idcName.c_str(), "wt+");
    TIDCGen *idcGen = new TIDCGen(idcF, SplitSize);

    idcGen->OutputHeaderFull();

    int     pos, curSize;

    for (pos = 0, curSize = 0; pos < TotalSize; pos++)
    {
        PInfoRec recN = GetInfoRec(Pos2Adr(pos));
        if (!recN) continue;

        if (SplitIDC && idcGen->CurrentBytes >= SplitSize)
        {
            fprintf(idcF, "}");
            fclose(idcF);
            idcName = idcTemplate + "_" + idcGen->CurrentPartNo + ".idc";
            idcF = fopen(idcName.c_str(), "wt+");
            idcGen->NewIDCPart(idcF);
            idcGen->OutputHeaderShort();
        }

        BYTE kind = recN->kind;
        BYTE len;

        if (IsFlagSet(cfRTTI, pos))
        {
            PUnitRec recU = GetUnit(Pos2Adr(pos));
            if (!recU) continue;
            
            if (recU->names->Count == 1)
                idcGen->unitName = recU->names->Strings[0];
            else
                idcGen->unitName = ".Unit" + String(recU->iniOrder);

            switch (kind)
            {
            case ikInteger:         //1
                idcGen->OutputRTTIInteger(kind, pos);
                break;
            case ikChar:            //2
                idcGen->OutputRTTIChar(kind, pos);
                break;
            case ikEnumeration:     //3
                idcGen->OutputRTTIEnumeration(kind, pos, Pos2Adr(pos));
                break;
            case ikFloat:           //4
                idcGen->OutputRTTIFloat(kind, pos);
                break;
            case ikString:          //5
                idcGen->OutputRTTIString(kind, pos);
                break;
            case ikSet:             //6
                idcGen->OutputRTTISet(kind, pos);
                break;
            case ikClass:           //7
                idcGen->OutputRTTIClass(kind, pos);
                break;
            case ikMethod:          //8
                idcGen->OutputRTTIMethod(kind, pos);
                break;
            case ikWChar:           //9
                idcGen->OutputRTTIWChar(kind, pos);
                break;
            case ikLString:         //0xA
                idcGen->OutputRTTILString(kind, pos);
                break;
            case ikWString:         //0xB
                idcGen->OutputRTTIWString(kind, pos);
                break;
            case ikVariant:         //0xC
                idcGen->OutputRTTIVariant(kind, pos);
                break;
            case ikArray:           //0xD
                idcGen->OutputRTTIArray(kind, pos);
                break;
            case ikRecord:          //0xE
                idcGen->OutputRTTIRecord(kind, pos);
                break;
            case ikInterface:       //0xF
                idcGen->OutputRTTIInterface(kind, pos);
                break;
            case ikInt64:           //0x10
                idcGen->OutputRTTIInt64(kind, pos);
                break;
            case ikDynArray:        //0x11
                idcGen->OutputRTTIDynArray(kind, pos);
                break;
            case ikUString:         //0x12
                idcGen->OutputRTTIUString(kind, pos);
                break;
            case ikClassRef:        //0x13
                idcGen->OutputRTTIClassRef(kind, pos);
                break;
            case ikPointer:         //0x14
                idcGen->OutputRTTIPointer(kind, pos);
                break;
            case ikProcedure:       //0x15
                idcGen->OutputRTTIProcedure(kind, pos);
                break;
            }
            continue;
        }
        if (kind == ikVMT)
        {
            idcGen->OutputVMT(pos, recN);
            continue;
        }
        if (kind == ikString)
        {
            idcGen->MakeShortString(pos);
            continue;
        }
        if (kind == ikLString)
        {
            idcGen->MakeLString(pos);
            continue;
        }
        if (kind == ikWString)
        {
            idcGen->MakeWString(pos);
            continue;
        }
        if (kind == ikUString)
        {
            idcGen->MakeUString(pos);
            continue;
        }
        if (kind == ikCString)
        {
            idcGen->MakeCString(pos);
            continue;
        }
        if (kind == ikResString)
        {
            idcGen->OutputResString(pos, recN);
            continue;
        }
        if (kind == ikGUID)
        {
            idcGen->MakeArray(pos, 16);
            continue;
        }
        if (kind == ikData)
        {
            idcGen->OutputData(pos, recN);
            continue;
        }
        if (IsFlagSet(cfProcStart, pos))
        {
            pos += idcGen->OutputProc(pos, recN, IsFlagSet(cfImport, pos));
        }
    }
    fprintf(idcF, "}");
    fclose(idcF);
    delete idcGen;
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmUnitsPopup(TObject *Sender)
{
    if (lbUnits->ItemIndex < 0) return;

    String item = lbUnits->Items->Strings[lbUnits->ItemIndex];
    DWORD adr;
    sscanf(item.c_str() + 1, "%lX", &adr);
    PUnitRec recU = GetUnit(adr);
    miRenameUnit->Enabled = (!recU->kb && recU->names->Count <= 1);
}
//---------------------------------------------------------------------------
//MXXXXXXXXM   COP Op1, Op2, Op3;commentF
//XXXXXXXX - address
//F - flags (1:cfLoc; 2:cfSkip; 4:cfLoop; 8:jmp or jcc
void __fastcall TFMain_11011981::lbCodeDrawItem(TWinControl *Control,
      int Index, TRect &Rect, TOwnerDrawState State)
{
    bool        ib;
    BYTE        _f, _db;
    int         n, flags, _instrlen, _textLen, _len, _sWid, _cPos, _offset, _ap;
    int         _dbPos, _ddPos;
    DWORD       _adr, _val, _dd;
    TColor      _color;
    TListBox    *lb;
    TCanvas     *canvas;
    String      text, _item, _comment;
    PInfoRec    _recN;
    DISINFO     _disInfo;

    //After closing Project we cannot execute this handler (Code = 0)
    if (!Image) return;

    lb = (TListBox*)Control;
    canvas = lb->Canvas;

    if (Index < lb->Count)
    {
        flags = Control->DrawTextBiDiModeFlags(DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        if (!Control->UseRightToLeftAlignment())
            Rect.Left += 2;
        else
            Rect.Right -= 2;

        text = lb->Items->Strings[Index]; _textLen = text.Length();
        //lb->ItemHeight = canvas->TextHeight("T");

        //First row (name of procedure with prototype) output without highlighting
        if (!Index)
        {
            Rect.Right = Rect.Left;
            DrawOneItem(text, canvas, Rect, 0, flags);
            return;
        }
        //F
        _f = text[_textLen];
        canvas->Brush->Color = TColor(0xFFFFFF);
        if (State.Contains(odSelected))
            canvas->Brush->Color = TColor(0xFFFFC0);
        else if (_f & 2)//skip
            canvas->Brush->Color = TColor(0xF5F5FF);
        canvas->FillRect(Rect);

        //Width of space
        _sWid = canvas->TextWidth(" ");
        //Comment position
        _cPos = text.Pos(";");
        //Sign for > (blue)
        _item = text.SubString(1, 1);
        Rect.Right = Rect.Left;
        DrawOneItem(_item, canvas, Rect, TColor(0xFF8080), flags);

        //Address (loop is blue, loc is black, others are light gray)
        _item = text.SubString(2, 8); _adr = StrToInt(String("$") + _item);
        //loop or loc
        if (_f & 5)
            _color = TColor(0xFF8080);
        else
            _color = TColor(0xBBBBBB); 	//LightGray
        DrawOneItem(_item, canvas, Rect, _color, flags);

        //Sign for > (blue)
        _item = text.SubString(10, 1);
        DrawOneItem(_item, canvas, Rect, TColor(0xFF8080), flags);

        //Data (case or exeption table)
        _dbPos = text.Pos(" db ");
        _ddPos = text.Pos(" dd ");
        if (_dbPos || _ddPos)
        {
            Rect.Right += 7 * _sWid;
            if (_dbPos)
            {
                DrawOneItem("db", canvas, Rect, TColor(0), flags);
                //Spaces after db
                Rect.Right += (ASMMAXCOPLEN - 2) * _sWid;
                _db = *(Code + Adr2Pos(_adr));
                DrawOneItem(Val2Str0((DWORD)_db), canvas, Rect, TColor(0xFF8080), flags);
            }
            else if (_ddPos)
            {
                DrawOneItem("dd", canvas, Rect, TColor(0), flags);
                //Spaces after dd
                Rect.Right += (ASMMAXCOPLEN - 2) * _sWid;
                _dd = *((DWORD*)(Code + Adr2Pos(_adr)));
                DrawOneItem(Val2Str8(_dd), canvas, Rect, TColor(0xFF8080), flags);
            }
            //Comment (light gray)
            if (_cPos)
            {
                _item = text.SubString(_cPos, _textLen);
                _item.SetLength(_item.Length() - 1);
                DrawOneItem(_item, canvas, Rect, TColor(0xBBBBBB), flags);
            }
            return;
        }
        //Get instruction tokens
        Disasm.Disassemble(Code + Adr2Pos(_adr), (__int64)_adr, &_disInfo, 0);
        //repprefix
        _len = 0;
        if (_disInfo.RepPrefix != -1)
        {
            _item = RepPrefixTab[_disInfo.RepPrefix];
            _len = _item.Length();
        }
        Rect.Right += (6 - _len) * _sWid;
        if (_disInfo.RepPrefix != -1)
        {
            DrawOneItem(_item, canvas, Rect, TColor(0), flags);
        }
        Rect.Right += _sWid;

        //Cop (black, if float then green)
        _item = String(_disInfo.Mnem); _len = _item.Length();
        if (!_disInfo.Float)
            _color = TColor(0);
        else
            _color = TColor(0x808000);
        if (!SameText(_item, "movs"))
        {
            DrawOneItem(_item, canvas, Rect, _color, flags);
            //Operands
            if (_disInfo.OpNum)
            {
                Rect.Right += (ASMMAXCOPLEN - _len) * _sWid;
                for (n = 0; n < _disInfo.OpNum; n++)
                {
                    if (n) DrawOneItem(",", canvas, Rect, TColor(0), flags);
                
                    ib = (_disInfo.BaseReg != -1 || _disInfo.IndxReg != -1);
                    _offset = _disInfo.Offset;
                    //Op1
                    if (_disInfo.OpType[n] == otIMM)
                    {
                        _val = _disInfo.Immediate;
                        _ap = Adr2Pos(_val);
                        _color = TColor(0xFF8080);
                        if (_ap >= 0 && (_disInfo.Call || _disInfo.Branch))
                        {
                            _recN = GetInfoRec(_val);
                            if (_recN && _recN->HasName())
                            {
                                _item = _recN->GetName();
                                _color = TColor(0xC08000);
                            }
                            else
                                _item = Val2Str8(_val);
                        }
                        else
                        {
                            if (_val <= 9)
                                _item = String(_val);
                            else
                            {
                                _item = Val2Str0(_val);
                                if (!isdigit(_item[1])) _item = "0" + _item;
                            }
                        }
                        DrawOneItem(_item, canvas, Rect, _color, flags);
                    }
                    else if (_disInfo.OpType[n] == otREG || _disInfo.OpType[n] == otFST)
                    {
                        _item = GetAsmRegisterName(_disInfo.OpRegIdx[n]);
                        DrawOneItem(_item, canvas, Rect, TColor(0x0000B0), flags);
                    }
                    else if (_disInfo.OpType[n] == otMEM)
                    {
                        if (_disInfo.OpSize)
                        {
                            _item = String(_disInfo.sSize) + " ptr ";
                            DrawOneItem(_item, canvas, Rect, TColor(0), flags);
                        }
                        if (_disInfo.SegPrefix != -1)
                        {
                            _item = String(SegRegTab[_disInfo.SegPrefix]);
                            DrawOneItem(_item, canvas, Rect, TColor(0x0000B0), flags);
                            DrawOneItem(":", canvas, Rect, TColor(0), flags);
                        }
                        DrawOneItem("[", canvas, Rect, TColor(0), flags);
                        if (ib)
                        {
                            if (_disInfo.BaseReg != -1)
                            {
                                _item = GetAsmRegisterName(_disInfo.BaseReg);
                                DrawOneItem(_item, canvas, Rect, TColor(0x0000B0), flags);
                            }
                            if (_disInfo.IndxReg != -1)
                            {
                                if (_disInfo.BaseReg != -1)
                                {
                                    DrawOneItem("+", canvas, Rect, TColor(0), flags);
                                }
                                _item = GetAsmRegisterName(_disInfo.IndxReg);
                                DrawOneItem(_item, canvas, Rect, TColor(0x0000B0), flags);
                                if (_disInfo.Scale != 1)
                                {
                                    DrawOneItem("*", canvas, Rect, TColor(0), flags);
                                    _item = String(_disInfo.Scale);
                                    DrawOneItem(_item, canvas, Rect, TColor(0xFF8080), flags);
                                }
                            }
                            if (_offset)
                            {
                                if (_offset < 0)
                                {
                                    _item = "-";
                                    _offset = -_offset;
                                }
                                else
                                {
                                    _item = "+";
                                }
                                DrawOneItem(_item, canvas, Rect, TColor(0), flags);
                                if (_offset < 9)
                                    _item = String(_offset);
                                else
                                {
                                    _item = Val2Str0(_offset);
                                    if (!isdigit(_item[1])) _item = "0" + _item;
                                }
                                DrawOneItem(_item, canvas, Rect, TColor(0xFF8080), flags);
                            }
                        }
                        else
                        {
                            if (_offset < 0) _offset = -_offset;
                            if (_offset < 9)
                                _item = String(_offset);
                            else
                            {
                                _item = Val2Str0(_offset);
                                if (!isdigit(_item[1])) _item = "0" + _item;
                            }
                            DrawOneItem(_item, canvas, Rect, TColor(0xFF8080), flags);
                        }
                        DrawOneItem("]", canvas, Rect, TColor(0), flags);
                    }
                }
            }
        }
        //movsX
        else
        {
            _item += String(_disInfo.sSize[0]);
            DrawOneItem(_item, canvas, Rect, _color, flags);
        }
        //Comment (light gray)
        if (_cPos)
        {
            _item = text.SubString(_cPos, _textLen);
            _item.SetLength(_item.Length() - 1);
            DrawOneItem(_item, canvas, Rect, TColor(0xBBBBBB), flags);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miListerClick(TObject *Sender)
{
    bool        imp, emb;
	String		line;

    String lstName = "";
    if (SourceFile != "") lstName = ChangeFileExt(SourceFile, ".lst");
    if (IDPFile != "") lstName = ChangeFileExt(IDPFile, ".lst");

    SaveDlg->InitialDir = WrkDir;
    SaveDlg->Filter = "LST|*.lst";
    SaveDlg->FileName = lstName;

    if (!SaveDlg->Execute()) return;

    lstName = SaveDlg->FileName;

    if (FileExists(lstName))
    {
        if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
    }

	Screen->Cursor = crHourGlass;

    FILE* lstF = fopen(lstName.c_str(), "wt+");
    for (int n = 0; n < UnitsNum; n++)
    {
        PUnitRec recU = (PUnitRec)Units->Items[n];
        if (recU->kb || recU->trivial) continue;
        fprintf(lstF, "//===========================================================================\n");
        fprintf(lstF, "//Unit%03d", recU->iniOrder);
        if (recU->names->Count) fprintf(lstF, " (%s)", recU->names->Strings[0]);
        fprintf(lstF, "\n");

        for (DWORD adr = recU->fromAdr; adr < recU->toAdr; adr++)
        {
            if (adr == recU->finadr)
            {
            	if (!recU->trivialFin)
                {
                    OutputCode(lstF, adr, "procedure Finalization;", false);
                }
                continue;
            }
            if (adr == recU->iniadr)
            {
            	if (!recU->trivialIni)
                {
                    OutputCode(lstF, adr, "procedure Initialization;", false);
                }
                continue;
            }

            int pos = Adr2Pos(adr);
            PInfoRec recN = GetInfoRec(adr);
            if (!recN) continue;

            imp = emb = false;
            BYTE kind = recN->kind;
            if (IsFlagSet(cfProcStart, pos))
            {
                imp = IsFlagSet(cfImport, pos);
                emb = (recN->procInfo->flags & PF_EMBED);
            }

            if (kind == ikUnknown) continue;

            if (kind > ikUnknown && kind <= ikProcedure && recN->HasName())
            {
            	if (kind == ikEnumeration || kind == ikSet)
                {
                	line = FTypeInfo_11011981->GetRTTI(adr);
                    fprintf(lstF, "%s = %s;\n", recN->GetName(), line);
                }
                else
                	fprintf(lstF, "%08lX <%s> %s\n", adr, TypeKind2Name(kind), recN->GetName());
                continue;
            }

            if (kind == ikResString)
            {
                fprintf(lstF, "%08lX <ResString> %s=%s\n", adr, recN->GetName(), recN->rsInfo->value);
                continue;
            }

            if (kind == ikVMT)
            {
            	fprintf(lstF, "%08lX <VMT> %s\n", adr, recN->GetName());
                continue;
            }

            if (kind == ikGUID)
            {
                fprintf(lstF, "%08lX <TGUID> %s\n", adr, Guid2String(Code + pos));
                continue;
            }

            if (kind == ikConstructor)
            {
               	OutputCode(lstF, adr, recN->MakePrototype(adr, true, false, false, true, false), false);
                continue;
            }

            if (kind == ikDestructor)
            {
                OutputCode(lstF, adr, recN->MakePrototype(adr, true, false, false, true, false), false);
                continue;
            }

            if (kind == ikProc)
            {
            	line = "";
                if (imp)
                	line += "import ";
                else if (emb)
                	line += "embedded ";
                line += recN->MakePrototype(adr, true, false, false, true, false);
                OutputCode(lstF, adr, line, false);
                continue;
            }

            if (kind == ikFunc)
            {
            	line = "";
                if (imp)
                	line += "import ";
                else if (emb)
                	line += "embedded ";
                line += recN->MakePrototype(adr, true, false, false, true, false);
                OutputCode(lstF, adr, line, false);
                continue;
            }

            if (IsFlagSet(cfProcStart, pos))
            {
                if (kind == ikDestructor)
                {
                    OutputCode(lstF, adr, recN->MakePrototype(adr, true, false, false, true, false), false);
                }
                else if (kind == ikDestructor)
                {
                    OutputCode(lstF, adr, recN->MakePrototype(adr, true, false, false, true, false), false);
                }
                else
                {
                    line = "";
                    if (emb) line += "embedded ";
                    line += recN->MakePrototype(adr, true, false, false, true, false);
                    OutputCode(lstF, adr, line, false);
                }
            }
        }
    }
    fclose(lstF);
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::OutputLine(FILE* OutF, BYTE flags, DWORD Adr, String Content)
{
    //Ouput comments
    if (flags & 0x10)
    {
        char *p = strchr(Content.c_str(), ';');
        if (p) fprintf(OutF, "C %08lX %s\n", Adr, p + 1);
        return;
    }
    
    //Jump direction
	if (flags & 4)
    	fprintf(OutF, "<");
    else if (flags & 8)
    	fprintf(OutF, ">");
    else
        fprintf(OutF, " ");
    /*
	if (flags & 1)
    	fprintf(OutF, "%08lX\n", Adr);
    else
    	fprintf(OutF, "        ");
    */
    fprintf(OutF, "%08lX    %s\n", Adr, Content.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::OutputCode(FILE* outF, DWORD fromAdr, String prototype, bool onlyComments)
{
    BYTE        op, flags;
    int			row = 0, num, instrLen, instrLen1, instrLen2;
    DWORD       Adr, Adr1, Pos, lastMovAdr = 0;
    int         fromPos, curPos, _procSize, _ap, _pos, _idx;
    DWORD       curAdr;
    DWORD       lastAdr = 0;
    PInfoRec    recN, recN1;
    String		line;
    DISINFO     DisInfo, DisInfo1;
    char        disLine[100];

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return;

    recN = GetInfoRec(fromAdr);
    int outRows = MAX_DISASSEMBLE;
    if (IsFlagSet(cfImport, fromPos)) outRows = 1;

    if (!onlyComments && prototype != "")
    {
        fprintf(outF, "//---------------------------------------------------------------------------\n");
        fprintf(outF, "//%s\n", prototype);
    }
    _procSize = GetProcSize(fromAdr);
    curPos = fromPos; curAdr = fromAdr;

    while (row < outRows)
    {
        //End of procedure
        if (curAdr != fromAdr && _procSize && curAdr - fromAdr >= _procSize) break;
        flags = 0;
        //Only comments?
        if (onlyComments) flags |= 0x10;
        //Loc?
        if (IsFlagSet(cfLoc, curPos)) flags |= 1;
        //Skip?
        if (IsFlagSet(cfSkip | cfDSkip, curPos)) flags |= 2;

        //If exception table, output it
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            OutputLine(outF, flags, curAdr, "dd          " + String(num)); row++;
            curPos += 4; curAdr += 4;

            for (int k = 0; k < num; k++)
            {
                //dd offset ExceptionInfo
                Adr = *((DWORD*)(Code + curPos));
                line = "dd          " + Val2Str8(Adr);
                //Name of Exception
                if (IsValidCodeAdr(Adr))
                {
                    recN = GetInfoRec(Adr);
                    if (recN && recN->HasName()) line += ";" + recN->GetName();
                }
                OutputLine(outF, flags, curAdr, line); row++;

                //dd offset ExceptionProc
                curPos += 4; curAdr += 4;
                Adr = *((DWORD*)(Code + curPos));
                OutputLine(outF, flags, curAdr, "dd          " + Val2Str8(Adr)); row++;

                curPos += 4; curAdr += 4;
            }
            continue;
        }

        BYTE b1 = Code[curPos];
        BYTE b2 = Code[curPos + 1];

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, disLine);
        if (!instrLen)
        {
            OutputLine(outF, flags, curAdr, "???"); row++;
            curPos++; curAdr++;
            continue;
        }
        op = Disasm.GetOp(DisInfo.Mnem);

        //Check inside instruction Fixup or ThreadVar
        bool NameInside = false; DWORD NameInsideAdr;
        for (int k = 1; k < instrLen; k++)
        {
            if (Infos[curPos + k])
            {
                NameInside = true;
                NameInsideAdr= curAdr + k;
                break;
            }
        }

        line = String(disLine);

        if (curAdr >= lastAdr) lastAdr = 0;

        //Proc end
        if (DisInfo.Ret && (!lastAdr || curAdr == lastAdr))
        {
            OutputLine(outF, flags, curAdr, line); row++;
            break;
        }

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        if (b1 == 0xEB ||				 //short relative abs jmp or cond jmp
        	(b1 >= 0x70 && b1 <= 0x7F) ||
            (b1 == 0xF && b2 >= 0x80 && b2 <= 0x8F))
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                if (op == OP_JMP)
                {
                    _ap = Adr2Pos(Adr);
                    recN = GetInfoRec(Adr);
                    if (recN && IsFlagSet(cfProcStart, _ap) && recN->HasName())
                    {
                        line = "jmp         " + recN->GetName();
                    }
                }
                flags |= 8;
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            OutputLine(outF, flags, curAdr, line); row++;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                _ap = Adr2Pos(Adr);
                recN = GetInfoRec(Adr);
                if (recN && IsFlagSet(cfProcStart, _ap) && recN->HasName())
                {
                    line = "jmp         " + recN->GetName();
                }
                flags |= 8;
                if (!recN && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            OutputLine(outF, flags, curAdr, line); row++;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        
        if (DisInfo.Call)  //call sub_XXXXXXXX
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                recN = GetInfoRec(Adr);
                if (recN && recN->HasName())
                {
                    line = "call        " + recN->GetName();
                    //Found @Halt0 - exit
                    if (recN->SameName("@Halt0") && fromAdr == EP && !lastAdr)
                    {
                        OutputLine(outF, flags, curAdr, line); row++;
                        break;
                    }
                }
            }
            recN = GetInfoRec(curAdr);
            if (recN && recN->picode) line += ";" + MakeComment(recN->picode);
            OutputLine(outF, flags, curAdr, line); row++;
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            OutputLine(outF, flags, curAdr, line); row++;
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            /*
            //First instruction
            if (curAdr == fromAdr) break;
            */
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Table address - last 4 bytes of instruction
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Analyze address range to find table cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //If exist cTblAdr, skip this table
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                for (int k = 0; k < CNum; k++)
                {
                    BYTE db = Code[Pos];
                    CTab[k] = db;
                    OutputLine(outF, flags, curAdr, "db          " + String(db)); row++;
                    Pos++; Adr++;
                }
            }
            //Check transitions by negative register values (in Delphi 2009)
            //bool neg = false;
            //Adr1 = *((DWORD*)(Code + Pos - 4));
            //if (IsValidCodeAdr(Adr1) && IsFlagSet(cfLoc, Adr2Pos(Adr1))) neg = true;

            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidCodeAdr(Adr1) || Adr1 < fromAdr) break;

                //Add row to assembler listing
                OutputLine(outF, flags, curAdr, "dd          " + Val2Str8(Adr1)); row++;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }
//----------------------------------
//PosTry: xor reg, reg
//        push ebp
//        push offset @1
//        push fs:[reg]
//        mov fs:[reg], esp
//        ...
//@2:     ...
//At @1 various variants:
//----------------------------------
//@1:     jmp @HandleFinally
//        jmp @2
//----------------------------------
//@1:     jmp @HandleAnyException
//        call DoneExcept
//----------------------------------
//@1:     jmp HandleOnException
//        dd num
//Далее таблица из num записей вида
//        dd offset ExceptionInfo
//        dd offset ExceptionProc
//----------------------------------
        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    if (Pos >= 0)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    /*
                                    //@2
                                    Adr1 = DisInfo.Immediate - 4;
                                    Adr = *((DWORD*)(Code + Adr2Pos(Adr1)));
                                    if (IsValidCodeAdr(Adr) && Adr > lastAdr) lastAdr = Adr;
                                    */
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Set flag cfETable to correct output data
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                }
                OutputLine(outF, flags, curAdr, line); row++;
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }

        //Name inside instruction (Fixip, ThreadVar)
        String namei = "", comment = "", name, pname, type, ptype;
        if (NameInside)
        {
            recN = GetInfoRec(NameInsideAdr);
            if (recN && recN->HasName())
            {
                namei += recN->GetName();
                if (recN->type != "") namei +=  ":" + recN->type;
            }
        }
        //comment
        recN = GetInfoRec(curAdr);
        if (recN && recN->picode) comment = MakeComment(recN->picode);

        DWORD targetAdr = 0;
        if (IsValidImageAdr(DisInfo.Immediate))
        {
        	if (!IsValidImageAdr(DisInfo.Offset)) targetAdr = DisInfo.Immediate;
        }
        else if (IsValidImageAdr(DisInfo.Offset))
        	targetAdr = DisInfo.Offset;

        if (targetAdr)
        {
            name = pname = type = ptype = "";
            _pos = Adr2Pos(targetAdr);
            if (_pos >= 0)
            {
                recN = GetInfoRec(targetAdr);
                if (recN)
                {
                    if (recN->kind == ikResString)
                    {
                        name = recN->GetName() + ":PResStringRec";
                    }
                    else
                    {
                        if (recN->HasName())
                        {
                            name = recN->GetName();
                            if (recN->type != "") type = recN->type;
                        }
                        else if (IsFlagSet(cfProcStart, _pos))
                            name = GetDefaultProcName(targetAdr);
                    }
                }
                //For Delphi2 pointers to VMT are distinct
                else if (DelphiVersion == 2)
                {
                    recN = GetInfoRec(targetAdr + VmtSelfPtr);
                    if (recN && recN->kind == ikVMT && recN->HasName())
                    {
                        name = recN->GetName();
                    }
                }
                Adr = *((DWORD*)(Code + _pos));
                if (IsValidImageAdr(Adr))
                {
                    recN = GetInfoRec(Adr);
                    if (recN)
                    {
                        if (recN->HasName())
                        {
                            pname = recN->GetName();
                            ptype = recN->type;
                        }
                        else if (IsFlagSet(cfProcStart, _pos))
                            pname = GetDefaultProcName(Adr);
                    }
                }
            }
            else
            {
                _idx = BSSInfos->IndexOf(Val2Str8(targetAdr));
                if (_idx != -1)
                {
                    recN = (PInfoRec)BSSInfos->Objects[_idx];
                    name = recN->GetName();
                    type = recN->type;
                }
            }
        }
        if (SameText(comment, name)) name = "";
        if (pname != "")
        {
            if (comment != "") comment += " ";
            comment += "^" + pname;
            if (ptype != "") comment += ":" + ptype;
        }
        else if (name != "")
        {
            if (comment != "") comment += " ";
           	comment += name;
            if (type != "") comment += ":" + type;
        }

        if (comment != "" || namei != "")
        {
            line += ";";
            if (comment != "") line += comment;
            if (namei != "") line += "{" + namei + "}";
        }
        if (line.Length() > MAXLEN) line = line.SubString(1, MAXLEN) + "...";
        OutputLine(outF, flags, curAdr, line); row++;
        curPos += instrLen; curAdr += instrLen;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::wm_dropFiles(TWMDropFiles& msg)
{
    TFileDropper* fc = new TFileDropper((HDROP)msg.Drop);
    try
    {
        for (int i = 0; i < fc->FileCount; ++i)
        {
            String droppedFile = fc->Files[i];
            String ext = ExtractFileExt(droppedFile).LowerCase();

            if (SameText(ext, ".lnk"))
            	DoOpenDelphiFile(DELHPI_VERSION_AUTO, GetFilenameFromLink(droppedFile), true, true);
            else if (SameText(ext, ".idp") && miOpenProject->Enabled)
                DoOpenProjectFile(droppedFile);
            else if ((SameText(ext, ".exe") || SameText(ext, ".bpl") || SameText(ext, ".dll") || SameText(ext, ".scr")) && miLoadFile->Enabled)
                DoOpenDelphiFile(DELHPI_VERSION_AUTO, droppedFile, true, true);

            //Processed the first - and go out - we cannot process more than one file yet
            break;
        }
        //TPoint ptDrop = fc->DropPoint;
    }
    catch (...)
    {
    }
    delete fc;
    msg.Result = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miAboutClick(TObject *Sender)
{
	FAboutDlg_11011981->ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miHelpClick(TObject *Sender)
{
	ShellExecute(Handle, "open", Application->HelpFile.c_str(), 0, 0, 1);
}
//---------------------------------------------------------------------------
#include "TabUnits.cpp"
#include "TabRTTIs.cpp"
#include "TabStrings.cpp"
#include "TabNames.cpp"
#include "CXrefs.cpp"
#include "Analyze1.cpp"
#include "Analyze2.cpp"
#include "AnalyzeArguments.cpp"
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::FormCloseQuery(TObject *Sender, bool &CanClose)
{
    int _res;

    if (AnalyzeThread)
    {
        AnalyzeThread->Suspend();
        String sbtext0 = FProgressBar->sb->Panels->Items[0]->Text;
        String sbtext1 = FProgressBar->sb->Panels->Items[1]->Text;
        FProgressBar->Visible = false;

        _res = Application->MessageBox("Analysis is not yet completed. Do You really want to exit IDR?", "Confirmation", MB_YESNO);
    	if (_res == IDNO)
        {
            FProgressBar->Visible = true;
            FProgressBar->sb->Panels->Items[0]->Text = sbtext0;
            FProgressBar->sb->Panels->Items[1]->Text = sbtext1;
            FProgressBar->Update();

            AnalyzeThread->Resume();
        	CanClose = false;
            return;
        }
        AnalyzeThread->Terminate();
    }

    if (ProjectLoaded && ProjectModified)
    {
        _res = Application->MessageBox("Save active Project?", "Confirmation", MB_YESNOCANCEL);
        if (_res == IDCANCEL)
        {
          	CanClose = false;
            return;
        }
    	if (_res == IDYES)
        {
            if (IDPFile == "") IDPFile = ChangeFileExt(SourceFile, ".idp");

            SaveDlg->InitialDir = WrkDir;
            SaveDlg->Filter = "IDP|*.idp";
            SaveDlg->FileName = IDPFile;

            if (!SaveDlg->Execute())
            {
            	CanClose = false;
                return;
            }
            SaveProject(SaveDlg->FileName);
        }
        CloseProject();
    }

    IniFileWrite();
    
    CanClose = true;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCtdPasswordClick(TObject *Sender)
{
    BYTE        op;
    DWORD       curPos, curAdr;
    int         instrLen, pwdlen = 0, beg;
    String      pwds = "";
    BYTE        pwd[256];
    DISINFO     DisInfo;

	PInfoRec recN = GetInfoRec(CtdRegAdr);
    if (recN->xrefs->Count != 1) return;
    PXrefRec recX = (PXrefRec)recN->xrefs->Items[0];

    int ofs;
    for (ofs = recX->offset; ofs >= 0; ofs--)
    {
        if (IsFlagSet(cfPush, Adr2Pos(recX->adr) + ofs)) break;
    }
    /*    
    curPos = Adr2Pos(recX->adr) + ofs;
    curAdr = Pos2Adr(curPos);
    //pwdlen
    instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo);
    pwdlen = DisInfo.Immediate + 1;
    curPos += instrLen; curAdr += instrLen;
    //pwd
    beg = 128;
    for (int n = 0; n < pwdlen;)
    {
        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo);
        op = Disasm.GetOp(DisInfo.Mnem);
        //mov [ebp-Ofs], B
        if (op == OP_MOV && DisInfo.Op1Type == otMEM && DisInfo.Op2Type == otIMM && DisInfo.BaseReg == 21 && (int)DisInfo.Offset < 0)
        {
            ofs = DisInfo.Offset; if (128 + ofs < beg) beg = 128 + ofs;
            pwd[128 + ofs] = DisInfo.Immediate;
            n++;
        }
        curPos += instrLen; curAdr += instrLen;
    }
    for (int n = beg; n < beg + pwdlen; n++)
    {
        pwds += Val2Str2(pwd[n]);
    }
    */
	PROCHISTORYREC  rec;

    rec.adr = CurProcAdr;
    rec.itemIdx = lbCode->ItemIndex;
    rec.xrefIdx = lbCXrefs->ItemIndex;
    rec.topIdx = lbCode->TopIndex;
    ShowCode(recX->adr, recX->adr + ofs, -1, -1);
    CodeHistoryPush(&rec);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmCodePanelPopup(TObject *Sender)
{
    miEmptyHistory->Enabled = (CodeHistoryPtr > 0);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miEmptyHistoryClick(TObject *Sender)
{
    memmove(&CodeHistory[0], &CodeHistory[CodeHistoryPtr], sizeof(PROCHISTORYREC));
    CodeHistoryPtr = 0;
    CodeHistoryMax = CodeHistoryPtr;
}
//---------------------------------------------------------------------------
PFIELDINFO __fastcall TFMain_11011981::GetField(String TypeName, int Offset, bool* vmt, DWORD* vmtAdr, String prefix)
{
    int         n, idx, kind, size, Ofs, Ofs1, Ofs2, pos1, pos2;
    DWORD       classAdr;
    BYTE        *p, *ps;
    WORD        *uses, Len;
    MTypeInfo   atInfo;
    MTypeInfo   *tInfo = &atInfo;
    PFIELDINFO	fInfo, fInfo1, fInfo2;

    *vmt = false; *vmtAdr = 0;
    classAdr = GetClassAdr(TypeName);
    if (IsValidImageAdr(classAdr))
    {
        *vmt = true; *vmtAdr = classAdr;
        DWORD prevClassAdr = 0;
    	while (classAdr && Offset < GetClassSize(classAdr))
        {
        	prevClassAdr = classAdr;
        	classAdr = GetParentAdr(classAdr);
        }
        classAdr = prevClassAdr;

        if (classAdr)
        {
            *vmtAdr = classAdr;
            PInfoRec recN = GetInfoRec(classAdr);
            if (recN && recN->vmtInfo && recN->vmtInfo->fields)
            {
                if (recN->vmtInfo->fields->Count == 1)
                {
                    fInfo = (PFIELDINFO)recN->vmtInfo->fields->Items[0];
                    if (Offset == fInfo->Offset)
                    {
                        fInfo->Name = prefix + "." + fInfo->Name;
                        return fInfo;
                    }
                    return 0;
                }
                for (int n = 0; n < recN->vmtInfo->fields->Count; n++)
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
                        if (Offset == Ofs1) return fInfo1;
                        kind = GetTypeKind(fInfo1->Type, &size);
                        if (kind == ikClass || kind == ikRecord)
                        {
                            prefix = fInfo1->Name;
                            fInfo = GetField(fInfo1->Type, Offset - Ofs1, vmt, vmtAdr, prefix);
                            if (fInfo)
                            {
                                fInfo->Offset = Offset;
                                fInfo->Name = prefix + "." + fInfo->Name;
                                return fInfo;
                            }
                            return 0;
                        }
                        if (kind == ikArray)
                            return fInfo1;
                        return 0;
                    }
                }
            }
        }
        return 0;
    }

    //try KB
    uses = KnowledgeBase.GetTypeUses(TypeName.c_str());
    idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, TypeName.c_str());
    if (uses) delete[] uses;

    if (idx != -1)
    {
        idx = KnowledgeBase.TypeOffsets[idx].NamId;
        if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS, tInfo))
        if (tInfo->Fields)
        {
            p = tInfo->Fields;
            for (n = 0; n < tInfo->FieldsNum; n++)
            {
                ps = p;
                p++;//scope
                Ofs1 = *((int*)p); p += 4;//offset
                p += 4;//case
                Len = *((WORD*)p); p += Len + 3;//name
                Len = *((WORD*)p); p += Len + 3;//type
                if (n == tInfo->FieldsNum - 1)
                {
                    Ofs2 = 0;
                }
                else
                {
                    Ofs2 = *((int*)(p + 1));
                }
                if (Offset >= Ofs1 && Offset < Ofs2)
                {
                    p = ps;
                    p++;//Scope
                    Ofs = *((int*)p); p += 4;//offset
                    fInfo = new FIELDINFO;
                    fInfo->Offset = Offset - Ofs;
                    fInfo->Scope = SCOPE_TMP;
                    fInfo->Case = *((int*)p); p += 4;
                    fInfo->xrefs = 0;
                    Len = *((WORD*)p); p += 2;
                    fInfo->Name = String((char*)p, Len); p += Len + 1;
                    Len = *((WORD*)p); p += 2;
                    fInfo->Type = TrimTypeName(String((char*)p, Len));
                    return fInfo;
                }
            }
            return 0;
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
PFIELDINFO __fastcall TFMain_11011981::AddField(DWORD ProcAdr, int ProcOfs, String TypeName, BYTE Scope, int Offset, int Case, String Name, String Type)
{
    DWORD classAdr = GetClassAdr(TypeName);
    if (IsValidImageAdr(classAdr))
    {
    	if (Offset < 4) return 0;

    	DWORD prevClassAdr = 0;
    	while (classAdr && Offset < GetClassSize(classAdr))
        {
        	prevClassAdr = classAdr;
        	classAdr = GetParentAdr(classAdr);
        }
        classAdr = prevClassAdr;

        if (classAdr)
        {
            PInfoRec recN = GetInfoRec(classAdr);
            if (!recN) return 0;
            if (!recN->vmtInfo) return 0;
            return recN->vmtInfo->AddField(ProcAdr, ProcOfs, Scope, Offset, Case, Name, Type);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miUnitDumperClick(TObject *Sender)
{
;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miFuzzyScanKBClick(TObject *Sender)
{
    FKBViewer_11011981->Position = -1;
    if (CurProcAdr)
    {
        PInfoRec recN = GetInfoRec(CurProcAdr);
        if (recN && recN->kbIdx != -1)
        {
            FKBViewer_11011981->Position = recN->kbIdx;
            FKBViewer_11011981->ShowCode(CurProcAdr, recN->kbIdx);
            FKBViewer_11011981->Show();
            return;
        }

        PUnitRec recU = GetUnit(CurProcAdr);
        if (recU)
        {
            int fromAdr = recU->fromAdr, toAdr = recU->toAdr;
            int upIdx = -1, dnIdx = -1, upCnt = -1, dnCnt = -1;
            if (1)//!recU->names->Count)
            {
                if (FKBViewer_11011981->cbUnits->Text != "")
                {
                    FKBViewer_11011981->cbUnitsChange(this);
                }
                else if (recU->names->Count)
                {
                    FKBViewer_11011981->cbUnits->Text = recU->names->Strings[0];
                    FKBViewer_11011981->cbUnitsChange(this);
                }
                if (FKBViewer_11011981->Position != -1)
                {
                    FKBViewer_11011981->Show();
                }
                else
                {
                    for (int adr = CurProcAdr; adr >= fromAdr; adr--)
                    {
                        if (IsFlagSet(cfProcStart, Adr2Pos(adr)))
                        {
                            upCnt++;
                            recN = GetInfoRec(adr);
                            if (recN && recN->kbIdx != -1)
                            {
                                upIdx = recN->kbIdx;
                                break;
                            }
                        }
                    }
                    for (int adr = CurProcAdr; adr < toAdr; adr++)
                    {
                        if (IsFlagSet(cfProcStart, Adr2Pos(adr)))
                        {
                            dnCnt++;
                            recN = GetInfoRec(adr);
                            if (recN && recN->kbIdx != -1)
                            {
                                dnIdx = recN->kbIdx;
                                break;
                            }
                        }
                    }
                    if (upIdx != -1)
                    {
                        if (dnIdx != -1)
                        {
                            //Up proc is nearest
                            if (upCnt < dnCnt)
                            {
                                FKBViewer_11011981->Position = upIdx + upCnt;
                                FKBViewer_11011981->ShowCode(CurProcAdr, upIdx + upCnt);
                                FKBViewer_11011981->Show();
                            }
                            //Down is nearest
                            else
                            {
                                FKBViewer_11011981->Position = dnIdx - dnCnt;
                                FKBViewer_11011981->ShowCode(CurProcAdr, dnIdx - dnCnt);
                                FKBViewer_11011981->Show();
                            }
                        }
                        else
                        {
                            FKBViewer_11011981->Position = upIdx + upCnt;
                            FKBViewer_11011981->ShowCode(CurProcAdr, upIdx + upCnt);
                            FKBViewer_11011981->Show();
                        }
                    }
                    else if (dnIdx != -1)
                    {
                        FKBViewer_11011981->Position = dnIdx - dnCnt;
                        FKBViewer_11011981->ShowCode(CurProcAdr, dnIdx - dnCnt);
                        FKBViewer_11011981->Show();
                    }
                    //Nothing found!
                    else
                    {
                        FKBViewer_11011981->Position = -1;
                        FKBViewer_11011981->ShowCode(CurProcAdr, -1);
                        FKBViewer_11011981->Show();
                    }
                }
                return;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::InitAliases(bool find)
{
    TCursor curCursor = Screen->Cursor;
    Screen->Cursor = crHourGlass;

    if (find) ResInfo->InitAliases();

    lClassName->Caption = "";
    lbAliases->Clear();

    for (int n = 0; n < ResInfo->Aliases->Count; n++)
    {
        String item = ResInfo->Aliases->Strings[n];
        if (item.Pos("="))
        {
            char *p = AnsiLastChar(item);
            if (p && *p != '=') lbAliases->Items->Add(item);
        }
    }

    cbAliases->Clear();

    for (int n = 0;; n++)
    {
        if (!RegClasses[n].RegClass) break;

        if (RegClasses[n].ClassName)
            cbAliases->Items->Add(String(RegClasses[n].ClassName));
    }

    pnlAliases->Visible = false;
    lbAliases->Enabled = true;
    Screen->Cursor = curCursor;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbAliasesDblClick(TObject *Sender)
{
    lClassName->Caption = "";
    cbAliases->Text = "";
    String item = lbAliases->Items->Strings[lbAliases->ItemIndex];
    int pos = item.Pos("=");
    if (pos)
    {
        pnlAliases->Visible = true;
        lClassName->Caption = item.SubString(1, pos - 1);
        cbAliases->Text = item.SubString(pos + 1, item.Length() - pos);
        lbAliases->Enabled = false;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bApplyAliasClick(TObject *Sender)
{
    ResInfo->Aliases->Values[lClassName->Caption] = cbAliases->Text;
    pnlAliases->Visible = false;
    lbAliases->Items->Strings[lbAliases->ItemIndex] = lClassName->Caption + "=" + cbAliases->Text;
    lbAliases->Enabled = true;

    //as: we any opened Forms -> repaint (take into account new aliases)
    ResInfo->ReopenAllForms();    
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bCancelAliasClick(TObject *Sender)
{
    pnlAliases->Visible = false;
    lbAliases->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miLegendClick(TObject *Sender)
{
    FLegend_11011981->ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopyListClick(TObject *Sender)
{
    int         n, m, k, u, dot, idx, usesNum;
    PInfoRec    recN;
    PUnitRec    recU;
    MProcInfo   aInfo;
    MProcInfo   *pInfo = &aInfo;
    FILE        *outFile;
    TStringList *tmpList;
    String      moduleName, importName;
    WORD        uses[128];

    SaveDlg->InitialDir = WrkDir;
    SaveDlg->FileName = "units.lst";

    if (SaveDlg->Execute())
    {
        if (FileExists(SaveDlg->FileName))
        {
            if (Application->MessageBox("File already exists. Overwrite?", "Warning", MB_YESNO) == IDNO) return;
        }
        
        outFile = fopen(SaveDlg->FileName.c_str(), "wt+");
        if (!outFile)
        {
            ShowMessage("Cannot save units list");
            return;
        }
        Screen->Cursor = crHourGlass;
        tmpList = new TStringList;
        for (n = 0; n < UnitsNum; n++)
        {
            recU = (UnitRec*)Units->Items[n];
            for (u = 0; u < recU->names->Count; u++)
            {
                if (tmpList->IndexOf(recU->names->Strings[u]) == -1) tmpList->Add(recU->names->Strings[u]);
            }
            //Add Imports
            for (m = 0; m < CodeSize; m += 4)
            {
                if (IsFlagSet(cfImport, m))
                {
                    recN = GetInfoRec(Pos2Adr(m));
                    dot = recN->GetName().Pos(".");
                    importName = recN->GetName().SubString(dot + 1, recN->GetNameLength());
                    usesNum = KnowledgeBase.GetProcUses(importName.c_str(), uses);
                    for (k = 0; k < usesNum; k++)
                    {
                        idx = KnowledgeBase.GetProcIdx(uses[k], importName.c_str());
                        if (idx != -1)
                        {
                            idx = KnowledgeBase.ProcOffsets[idx].NamId;
                            if (KnowledgeBase.GetProcInfo(idx, INFO_ARGS, pInfo))
                            {
                                moduleName = KnowledgeBase.GetModuleName(pInfo->ModuleID);
                                if (tmpList->IndexOf(moduleName) == -1) tmpList->Add(moduleName);
                            }
                        }
                    }
                }
            }
            tmpList->Sort();
        }
        //Output result
        for (n = 0; n < tmpList->Count; n++)
        {
            fprintf(outFile, "%s.dcu\n", tmpList->Strings[n].c_str());
        }
        delete tmpList;
        fclose(outFile);
        Screen->Cursor = crDefault;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::wm_updAnalysisStatus(TMessage& msg)
{
    if (taUpdateUnits == msg.WParam)
    {
        const long isLastStep = msg.LParam;
        tsUnits->Enabled = true;
        ShowUnits(isLastStep);
        ShowUnitItems(GetUnit(CurUnitAdr), lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
    }
    else if (taUpdateRTTIs == msg.WParam)
    {
        miSearchRTTI->Enabled = true;
        miSortRTTI->Enabled = true;
        tsRTTIs->Enabled = true;
        ShowRTTIs();
    }
    else if (taUpdateVmtList == msg.WParam)
    {
        FillVmtList();
        InitAliases(true);
    }
    else if (taUpdateStrings == msg.WParam)
    {
        tsStrings->Enabled = true;
        miSearchString->Enabled = true;
        ShowStrings(0);
        tsNames->Enabled = true;
        ShowNames(0);
    }
    else if (taUpdateCode == msg.WParam)
    {
        tsCodeView->Enabled = true;
        bEP->Enabled = true;
        DWORD adr = CurProcAdr;
        CurProcAdr = 0;
        ShowCode(adr, lbCode->ItemIndex, -1, lbCode->TopIndex);
    }
    else if (taUpdateXrefs == msg.WParam)
    {
        lbCXrefs->Enabled = true;
        miGoTo->Enabled = true;
        miExploreAdr->Enabled = true;
        miSwitchFlag->Enabled = cbMultipleSelection->Checked;
    }
    else if (taUpdateShortClassViewer == msg.WParam)
    {
        tsClassView->Enabled = true;
        miViewClass->Enabled = true;
        miSearchVMT->Enabled = true;
        miCollapseAll->Enabled = true;

        rgViewerMode->ItemIndex = 1;
        rgViewerMode->Enabled = false;
    }
    else if (taUpdateClassViewer == msg.WParam)
    {
        tsClassView->Enabled = true;
        miSearchVMT->Enabled = true;
        miCollapseAll->Enabled = true;

        if (ClassTreeDone)
        {
            TTreeNode *root = tvClassesFull->Items->Item[0];
            root->Expanded = true;
            miViewClass->Enabled = true;
            rgViewerMode->ItemIndex = 0;
            rgViewerMode->Enabled = true;
        }
        else
        {
            miViewClass->Enabled = true;
            rgViewerMode->ItemIndex = 1;
            rgViewerMode->Enabled = false;
        }
        miClassTreeBuilder->Enabled = true;
    }
    else if (taUpdateBeforeClassViewer == msg.WParam)
    {
        miSearchUnit->Enabled = true;
        miRenameUnit->Enabled = true;
        miSortUnits->Enabled = true;
        miCopyList->Enabled = true;
        miKBTypeInfo->Enabled = true;
        miCtdPassword->Enabled = IsValidCodeAdr(CtdRegAdr);
        miName->Enabled = true;
        miViewProto->Enabled = true;
        miEditFunctionC->Enabled = true;
        miEditFunctionI->Enabled = true;
        miEditClass->Enabled = true;
    }
    else if (taFinished == msg.WParam)
    {
        AnalyzeThreadDone(0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::wm_dfmClosed(TMessage& msg)
{
}
//---------------------------------------------------------------------------
//Fill ClassViewerTree for 1 class
void __fastcall TFMain_11011981::FillClassViewerOne(int n, TStringList* tmpList, const bool* terminated)
{
    bool        vmtProc;
    WORD        _dx, _bx, _si;
    int         m, size, sizeParent, pos, cnt, vmtOfs, _pos;
    DWORD       vmtAdr, vmtAdrParent, vAdr, iAdr;
    TTreeNode   *rootNode, *node;
    String      className, nodeTextParent, nodeText, line, name;
    PInfoRec    recN;
    PMethodRec  recM;
    PVmtListRec recV;
    DISINFO     disInfo;

    recV = (PVmtListRec)VmtList->Items[n];
    vmtAdr = recV->vmtAdr;

    className = GetClsName(vmtAdr);

    size = GetClassSize(vmtAdr); if (DelphiVersion >= 2009) size += 4;

    vmtAdrParent = GetParentAdr(vmtAdr);
    sizeParent = GetClassSize(vmtAdrParent); if (DelphiVersion >= 2009) sizeParent += 4;

    nodeTextParent = GetParentName(vmtAdr) + " #" + Val2Str8(vmtAdrParent) + " Sz=" + Val2Str1(sizeParent);
    nodeText = className + " #" + Val2Str8(vmtAdr) + " Sz=" + Val2Str1(size);
    node = FindTreeNodeByName(nodeTextParent);
    node = AddClassTreeNode(node, nodeText);

    rootNode = node;

    if (rootNode)
    {
        //Interfaces
        const int intfsNum = LoadIntfTable(vmtAdr, tmpList);
        if (intfsNum)
        {
            for (m = 0; m < intfsNum && !*terminated; m++)
            {
                nodeText = tmpList->Strings[m];
                sscanf(nodeText.c_str(), "%lX", &vAdr);
                if (IsValidCodeAdr(vAdr))
                {
                    TTreeNode *intfsNode = AddClassTreeNode(rootNode, "<I> " + nodeText.SubString(nodeText.Pos(' ') + 1, nodeText.Length()));
                    cnt = 0;
                    pos = Adr2Pos(vAdr);
                    for (int v = 0;;v += 4)
                    {
                        if (IsFlagSet(cfVTable, pos)) cnt++;
                        if (cnt == 2) break;
                        iAdr = *((DWORD*)(Code + pos));
                        DWORD _adr = iAdr;
                        _pos = Adr2Pos(_adr);
                        vmtProc = false; vmtOfs = 0;
                        _dx = 0; _bx = 0; _si = 0;
                        while (1)
                        {
                            int instrlen = Disasm.Disassemble(Code + _pos, (__int64)_adr, &disInfo, 0);
                            if ((disInfo.OpType[0] == otMEM || disInfo.OpType[1] == otMEM) &&
                                disInfo.BaseReg != 20)//to exclude instruction "xchg reg, [esp]"
                            {
                                vmtOfs = disInfo.Offset;
                            }
                            if (disInfo.OpType[0] == otREG && disInfo.OpType[1] == otIMM)
                            {
                                if (disInfo.OpRegIdx[0] == 10)//dx
                                    _dx = disInfo.Immediate;
                                else if (disInfo.OpRegIdx[0] == 11)//bx
                                    _bx = disInfo.Immediate;
                                else if (disInfo.OpRegIdx[0] == 14)//si
                                    _si = disInfo.Immediate;
                            }
                            if (disInfo.Call)
                            {
                                recN = GetInfoRec(disInfo.Immediate);
                                if (recN)
                                {
                                    if (recN->SameName("@CallDynaInst") ||
                                        recN->SameName("@CallDynaClass"))
                                    {
                                        if (DelphiVersion <= 5)
                                            GetDynaInfo(vmtAdr, _bx, &iAdr);
                                        else
                                            GetDynaInfo(vmtAdr, _si, &iAdr);
                                        break;
                                    }
                                    else if (recN->SameName("@FindDynaInst") ||
                                             recN->SameName("@FindDynaClass"))
                                    {
                                        GetDynaInfo(vmtAdr, _dx, &iAdr);
                                        break;
                                    }
                                }
                            }
                            if (disInfo.Branch && !disInfo.Conditional)
                            {
                                if (IsValidImageAdr(disInfo.Immediate))
                                {
                                    iAdr = disInfo.Immediate;
                                }
                                else
                                {
                                    vmtProc = true;
                                    iAdr = *((DWORD*)(Code + Adr2Pos(vmtAdr - VmtSelfPtr + vmtOfs)));
                                    recM = GetMethodInfo(vmtAdr, 'V', vmtOfs);
                                    if (recM) name = recM->name;
                                }
                                break;
                            }
                            else if (disInfo.Ret)
                            {
                                vmtProc = true;
                                iAdr = *((DWORD*)(Code + Adr2Pos(vmtAdr - VmtSelfPtr + vmtOfs)));
                                recM = GetMethodInfo(vmtAdr, 'V', vmtOfs);
                                if (recM) name = recM->name;
                                break;
                            }
                            _pos += instrlen; _adr += instrlen;
                        }
                        if (!vmtProc && IsValidImageAdr(iAdr))
                        {
                            recN = GetInfoRec(iAdr);
                            if (recN && recN->HasName())
                                name = recN->GetName();
                            else
                                name = "";
                        }
                        line = "I" + Val2Str4(v) + " #" + Val2Str8(iAdr);
                        if (name != "") line += " " + name;
                        AddClassTreeNode(intfsNode, line);
                        pos += 4;
                    }
                }
                else
                {
                    TTreeNode *intfsNode = AddClassTreeNode(rootNode, "<I> " + nodeText);
                }
            }
        }
        if (*terminated) return;
        //Automated
        const int autoNum = LoadAutoTable(vmtAdr, tmpList);
        if (autoNum)
        {
            nodeText = "<A>";
            TTreeNode* autoNode = AddClassTreeNode(rootNode, nodeText);
            for (m = 0; m < autoNum && !*terminated; m++)
            {
                nodeText = tmpList->Strings[m];
                AddClassTreeNode(autoNode, nodeText);
            }
        }
        /*
        //Fields
        const int fieldsNum = form->LoadFieldTable(vmtAdr, fieldsList);
        if (fieldsNum)
        {
            node = rootNode;
            nodeText = "<F>";
            //node = form->tvClassesFull->Items->AddChild(node, nodeText);
            node = AddClassTreeNode(node, nodeText);
            TTreeNode* fieldsNode = node;
            for (m = 0; m < fieldsNum && !Terminated; m++)
            {
                //node = fieldsNode;
                PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[m];
                nodeText = Val2Str5(fInfo->Offset) + " ";
                if (fInfo->Name != "")
                    nodeText += fInfo->Name;
                else
                    nodeText += "?";
                nodeText += ":";
                if (fInfo->Type != "")
                    nodeText += fInfo->Type;
                else
                    nodeText += "?";

                //node = form->tvClassesFull->Items->AddChild(node, nodeText);
                AddClassTreeNode(fieldsNode, nodeText);
            }
        }
        */
        if (*terminated) return;
        //Events
        const int methodsNum = LoadMethodTable(vmtAdr, tmpList);
        if (methodsNum)
        {
            nodeText = "<E>";
            TTreeNode* methodsNode = AddClassTreeNode(rootNode, nodeText);
            for (m = 0; m < methodsNum && !*terminated; m++)
            {
                nodeText = tmpList->Strings[m];
                AddClassTreeNode(methodsNode, nodeText);
            }
        }
        if (*terminated) return;
        //Dynamics
        const int dynamicsNum = LoadDynamicTable(vmtAdr, tmpList);
        if (dynamicsNum)
        {
            nodeText = "<D>";
            TTreeNode* dynamicsNode = AddClassTreeNode(rootNode, nodeText);
            for (m = 0; m < dynamicsNum && !*terminated; m++)
            {
                nodeText = tmpList->Strings[m];
                AddClassTreeNode(dynamicsNode, nodeText);
            }
        }
        if (*terminated) return;
        //Virtual
        const int virtualsNum = LoadVirtualTable(vmtAdr, tmpList);
        if (virtualsNum)
        {
            nodeText = "<V>";
            TTreeNode* virtualsNode = AddClassTreeNode(rootNode, nodeText);
            for (m = 0; m < virtualsNum && !*terminated; m++)
            {
                nodeText = tmpList->Strings[m];
                AddClassTreeNode(virtualsNode, nodeText);
            }
        }
    }
}
//---------------------------------------------------------------------------
TTreeNode* __fastcall TFMain_11011981::AddClassTreeNode(TTreeNode* node, String nodeText)
{
    TTreeNode* newNode = 0;
    if (!node)  //Root
        newNode = tvClassesFull->Items->Add(0, nodeText);
    else
        newNode = tvClassesFull->Items->AddChild(node, nodeText);

    AddTreeNodeWithName(newNode, nodeText);

    return newNode;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSaveDelphiProjectClick(TObject *Sender)
{
    bool            typePresent, _isForm, comment;
    BYTE            kind;
    int             n, m, k, num, dotpos, len, minValue, maxValue;
    DWORD           adr, adr1, parentAdr;
    FILE            *f;
    TList           *tmpList;
    TStringList     *intBodyLines;
    TStringList     *intUsesLines;
    //TStringList     *impBodyLines;
    //TStringList     *impUsesLines;
    TStringList     *unitsList;
    TStringList     *formList;
    TStringList     *publishedList;
    TStringList     *publicList;
    TSearchRec      sr;
    PUnitRec        recU;
    PInfoRec        recN;
    PFIELDINFO      fInfo;
    PMethodRec      recM;
    PVmtListRec     recV;
    TDfm            *dfm;
    PComponentInfo  cInfo;
    String          curDir, DelphiProjectPath;
    String          unitName, className, parentName, fieldName, typeName;
    String          procName, formName, dfmName, line, uName;

    curDir = GetCurrentDir();
    DelphiProjectPath = AppDir + "Projects";
    if (DirectoryExists(DelphiProjectPath))
    {
        ChDir(DelphiProjectPath);
        if (!FindFirst("*.*", faArchive, sr))
        {
            do
            {
                DeleteFile(sr.Name);
            } while (!FindNext(sr));

            FindClose(sr);
        }
    }
    else
    {
        if (!CreateDir(DelphiProjectPath)) return;
        ChDir(DelphiProjectPath);
    }
    Screen->Cursor = crHourGlass;
    //Save Forms
    for (n = 0; n < ResInfo->FormList->Count; n++)
    {
    	dfm = (TDfm*)ResInfo->FormList->Items[n];
        formList = new TStringList;
        ResInfo->GetFormAsText(dfm, formList);
        dfmName = dfm->Name;
        //If system name add F at start
        if (SameText(dfmName, "prn")) dfmName = "F" + dfmName;
        formList->SaveToFile(dfmName + ".dfm");
        delete formList;
    }

    unitsList = new TStringList;

    for (n = 0; n < UnitsNum; n++)
    {
        recU = (PUnitRec)Units->Items[n];
        if (recU->trivial) continue;
        typePresent = false;
        _isForm = false;
        unitName = GetUnitName(recU);
        tmpList = new TList;
        intBodyLines = new TStringList;
        intUsesLines = new TStringList;
        //impBodyLines = new TStringList;
        //impUsesLines = new TStringList;
        publishedList = new TStringList;
        publicList = new TStringList;

        intUsesLines->Add("SysUtils");
        intUsesLines->Add("Classes");
        for (adr = recU->fromAdr; adr < recU->toAdr; adr++)
        {
            recN = GetInfoRec(adr);
            if (!recN) continue;

            kind = recN->kind;
            switch (kind)
            {
            case ikEnumeration:
            case ikSet:
            case ikMethod:
            case ikArray:
            case ikRecord:
            case ikDynArray:
                typePresent = true;
                line = FTypeInfo_11011981->GetRTTI(adr);
                len = sprintf(StringBuf, "  %s = %s;", recN->GetName().c_str(), line.c_str());
                intBodyLines->Add(String(StringBuf, len));
                break;
            //class
            case ikVMT:
                typePresent = true;
                className = recN->GetName();
                publishedList->Clear();
                publicList->Clear();

                dfm = ResInfo->GetFormByClassName(className);
                if (dfm)
                {
                    _isForm = true;
                    len = sprintf(StringBuf, "%s in '%s.pas' {%s}", unitName, unitName, dfm->Name);
                    unitsList->Add(String(StringBuf, len));
                }

                parentAdr = GetParentAdr(adr);
                parentName = GetClsName(parentAdr);
                len = sprintf(StringBuf, "  %s = class(%s)", className.c_str(), parentName.c_str());
                intBodyLines->Add(String(StringBuf, len));

                num = LoadFieldTable(adr, tmpList);
                for (m = 0; m < num; m++)
                {
                    fInfo = (PFIELDINFO)tmpList->Items[m];
                    if (fInfo->Name != "")
                        fieldName = fInfo->Name;
                    else
                        fieldName = "f" + Val2Str0(fInfo->Offset);
                    if (fInfo->Type != "")
                    {
                        comment = false;
                        typeName = TrimTypeName(fInfo->Type);
                    }
                    else
                    {
                        comment = true;
                        typeName = "?";
                    }
                    //Add UnitName to UsesList if necessary
                    for (k = 0; k < VmtList->Count; k++)
                    {
                        recV = (PVmtListRec)VmtList->Items[k];
                        if (recV && SameText(typeName, recV->vmtName))
                        {
                            uName = GetUnitName(recV->vmtAdr);
                            if (intUsesLines->IndexOf(uName) == -1)
                                intUsesLines->Add(uName);
                            break;
                        }
                    }
                    
                    if (!comment)
                        len = sprintf(StringBuf, "    %s:%s;//f%X", fieldName.c_str(), typeName.c_str(), fInfo->Offset);
                    else
                        len = sprintf(StringBuf, "    //%s:%s;//f%X", fieldName.c_str(), typeName.c_str(), fInfo->Offset);
                    if (_isForm && dfm && dfm->IsFormComponent(fieldName))
                        publishedList->Add(String(StringBuf, len));
                    else
                        publicList->Add(String(StringBuf, len));

                }

                num = LoadMethodTable(adr, tmpList);
                for (m = 0; m < num; m++)
                {
                    recM = (PMethodRec)tmpList->Items[m];
                    recN = GetInfoRec(recM->address);
                    procName = recN->MakePrototype(recM->address, true, false, false, false, false);
                    if (!procName.Pos(":?"))
                        len = sprintf(StringBuf, "    %s//%08lX", procName, recM->address);
                    else
                        len = sprintf(StringBuf, "    //%s//%08lX", procName, recM->address);
                    publishedList->Add(String(StringBuf, len));
                }

                num = LoadVirtualTable(adr, tmpList);
                for (m = 0; m < num; m++)
                {
                    recM = (PMethodRec)tmpList->Items[m];
                    //Check if procadr from other class
                    if (!IsOwnVirtualMethod(adr, recM->address)) continue;

                    recN = GetInfoRec(recM->address);
                    procName = recN->MakeDelphiPrototype(recM->address, recM);

                    len = sprintf(StringBuf, "    ");
                    if (procName.Pos(":?")) len += sprintf(StringBuf + len, "//");
                    len += sprintf(StringBuf + len, "%s", procName.c_str());
                    if (recM->id >= 0) len += sprintf(StringBuf + len, "//v%X", recM->id);
                    len += sprintf(StringBuf + len, "//%08lX", recM->address);

                    publicList->Add(String(StringBuf, len));
                }

                num = LoadDynamicTable(adr, tmpList);
                for (m = 0; m < num; m++)
                {
                    recM = (PMethodRec)tmpList->Items[m];
                    recN = GetInfoRec(recM->address);
                    procName = recN->MakePrototype(recM->address, true, false, false, false, false);
                    PMsgMInfo _info = GetMsgInfo(recM->id);
                    if (_info && _info->msgname != "")
                    {
                        procName += String(" message ") + _info->msgname + ";";
                    }
                    else
                        procName += " dynamic;";

                    if (!procName.Pos(":?"))
                        len = sprintf(StringBuf, "    %s//%08lX", procName.c_str(), recM->address);
                    else
                        len = sprintf(StringBuf, "    //%s//%08lX", procName.c_str(), recM->address);
                    publicList->Add(String(StringBuf, len));
                }

                if (publishedList->Count)
                {
                    intBodyLines->Add("  published");
                    for (m = 0; m < publishedList->Count; m++)
                    {
                        intBodyLines->Add(publishedList->Strings[m]);
                    }
                }
                if (publicList->Count)
                {
                    intBodyLines->Add("  public");
                    for (m = 0; m < publicList->Count; m++)
                    {
                        intBodyLines->Add(publicList->Strings[m]);
                    }
                }

                for (adr1 = recU->fromAdr; adr1 < recU->toAdr; adr1++)
                {
                    //Skip Initialization and Finalization procs
                    if (adr1 == recU->iniadr || adr1 == recU->finadr) continue;
                    recN = GetInfoRec(adr1);
                    if (!recN || !recN->procInfo) continue;
                    dotpos = recN->GetName().Pos(".");
                    if (!dotpos || !SameText(className, recN->GetName().SubString(1, dotpos - 1))) continue;
                    if ((recN->procInfo->flags & PF_VIRTUAL) ||
                        (recN->procInfo->flags & PF_DYNAMIC) ||
                        (recN->procInfo->flags & PF_EVENT))
                        continue;

                    if (recN->kind == ikConstructor || (recN->procInfo->flags & PF_METHOD))
                    {
                        procName = recN->MakePrototype(adr1, true, false, false, false, false);
                        if (!procName.Pos(":?"))
                            len = sprintf(StringBuf, "    %s//%08lX", procName.c_str(), adr1);
                        else
                            len = sprintf(StringBuf, "    //%s//%08lX", procName.c_str(), adr1);
                        if (intBodyLines->IndexOf(String(StringBuf, len)) == -1)
                            intBodyLines->Add(String(StringBuf, len));
                    }
                }
                intBodyLines->Add("  end;");
                break;
            }
        }
        //Output information
        f = fopen((unitName + ".pas").c_str(), "wt+");
        OutputDecompilerHeader(f);
        fprintf(f, "unit %s;\n\n", unitName);
        fprintf(f, "interface\n");
        //Uses
        if (intUsesLines->Count)
        {
            fprintf(f, "\nuses\n  ");
            for (m = 0; m < intUsesLines->Count; m++)
            {
                if (m) fprintf(f, ", ");
                fprintf(f, "%s", intUsesLines->Strings[m].c_str());
            }
            fprintf(f, ";\n\n");
        }
        //Type
        if (typePresent) fprintf(f, "type\n");
        for (m = 0; m < intBodyLines->Count; m++)
        {
            fprintf(f, "%s\n", intBodyLines->Strings[m].c_str());
        }
        //Other procs (not class members)
        for (adr = recU->fromAdr; adr < recU->toAdr; adr++)
        {
            //Skip Initialization and Finalization procs
            if (adr == recU->iniadr || adr == recU->finadr) continue;

            recN = GetInfoRec(adr);
            if (!recN || !recN->procInfo) continue;

            procName = recN->MakePrototype(adr, true, false, false, false, false);
            if (!procName.Pos(":?"))
                len = sprintf(StringBuf, "    %s//%08lX", procName.c_str(), adr);
            else
                len = sprintf(StringBuf, "    //%s//%08lX", procName.c_str(), adr);

            if (intBodyLines->IndexOf(String(StringBuf, len)) != -1) continue;

            fprintf(f, "%s\n", StringBuf);
        }

        fprintf(f, "\nimplementation\n\n");
        if (_isForm) fprintf(f, "{$R *.DFM}\n\n");
        for (adr = recU->fromAdr; adr < recU->toAdr; adr++)
        {
            //Initialization and Finalization procs
            if (adr == recU->iniadr || adr == recU->finadr) continue;

            recN = GetInfoRec(adr);
            if (!recN || !recN->procInfo) continue;

            kind = recN->kind;
            if (kind == ikConstructor ||
                kind == ikDestructor  ||
                kind == ikProc        ||
                kind == ikFunc)
            {
                fprintf(f, "//%08lX\n", adr);
                procName = recN->MakePrototype(adr, true, false, false, true, false);
                if (!procName.Pos(":?"))
                {
                    fprintf(f, "%s\n", procName);
                    fprintf(f, "begin\n");
                    fprintf(f, "{*\n");
                    OutputCode(f, adr, "", false);
                    fprintf(f, "*}\n");
                    fprintf(f, "end;\n\n");
                }
                else
                {
                    fprintf(f, "{*%s\n", procName);
                    fprintf(f, "begin\n");
                    OutputCode(f, adr, "", false);
                    fprintf(f, "end;*}\n\n");
                }
            }
        }

        if (!recU->trivialIni || !recU->trivialFin)
        {
            fprintf(f, "Initialization\n");
            if (!recU->trivialIni)
            {
                fprintf(f, "//%08lX\n", recU->iniadr);
                fprintf(f, "{*\n");
                OutputCode(f, recU->iniadr, "", false);
                fprintf(f, "*}\n");
            }
            fprintf(f, "Finalization\n");
            if (!recU->trivialFin)
            {
                fprintf(f, "//%08lX\n", recU->finadr);
                fprintf(f, "{*\n");
                OutputCode(f, recU->finadr, "", false);
                fprintf(f, "*}\n");
            }
        }

        fprintf(f, "end.");
        fclose(f);

        delete tmpList;
        delete intBodyLines;
        delete intUsesLines;
        //delete impBodyLines;
        //delete impUsesLines;
        delete publishedList;
        delete publicList;
    }
    //dpr
    recU = (PUnitRec)Units->Items[UnitsNum - 1];
    unitName = recU->names->Strings[0];
    f = fopen((unitName + ".dpr").c_str(), "wt+");

    OutputDecompilerHeader(f);

    if (SourceIsLibrary)
        fprintf(f, "library %s;\n\n", unitName);
    else
        fprintf(f, "program %s;\n\n", unitName);

    fprintf(f, "uses\n");
    fprintf(f, "  SysUtils, Classes;\n\n");

    fprintf(f, "{$R *.res}\n\n");

    if (SourceIsLibrary)
    {
        bool _expExists = false;
        for (n = 0; n < ExpFuncList->Count; n++)
        {
            PExportNameRec recE = (PExportNameRec)ExpFuncList->Items[n];
            adr = recE->address;
            if (IsValidImageAdr(adr))
            {
                fprintf(f, "//%08lX\n", adr);
                recN = GetInfoRec(adr);
                if (recN)
                {
                    fprintf(f, "%s\n", recN->MakePrototype(adr, true, false, false, true, false));
                    fprintf(f, "begin\n");
                    fprintf(f, "{*\n");
                    OutputCode(f, adr, "", false);
                    fprintf(f, "*}\n");
                    fprintf(f, "end;\n\n");
                    _expExists = true;
                }
                else
                {
                    fprintf(f, "//No information\n\n");
                }
            }
        }
        if (_expExists)
        {
            fprintf(f, "exports\n");
            for (n = 0; n < ExpFuncList->Count; n++)
            {
                PExportNameRec recE = (PExportNameRec)ExpFuncList->Items[n];
                adr = recE->address;
                if (IsValidImageAdr(adr))
                {
                    recN = GetInfoRec(adr);
                    if (recN)
                    {
                        fprintf(f, "%s", recN->GetName());
                        if (n < ExpFuncList->Count - 1) fprintf(f, ",\n");
                    }
                }
            }
            fprintf(f, ";\n\n");
        }
    }

    fprintf(f, "//%08lX\n", EP);
    fprintf(f, "begin\n");
    fprintf(f, "{*\n");
    OutputCode(f, EP, "", false);
    fprintf(f, "*}\n");
    fprintf(f, "end.\n");
    fclose(f);

    delete unitsList;

    ChDir(curDir);
    Screen->Cursor = crDefault;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::bDecompileClick(TObject *Sender)
{
    int procSize = GetProcSize(CurProcAdr);
    if (procSize > 0)
    {
        TDecompileEnv *DeEnv = new TDecompileEnv(CurProcAdr, procSize, GetInfoRec(CurProcAdr));
        try
        {
            DeEnv->DecompileProc();
        }
        catch(Exception &exception)
        {
            ShowCode(DeEnv->StartAdr, DeEnv->ErrAdr, lbCXrefs->ItemIndex, -1);
            Application->ShowException(&exception);
        }
        DeEnv->OutputSourceCode();
        if (DeEnv->Alarm)
            tsSourceCode->Highlighted = true;
        else
            tsSourceCode->Highlighted = false;
        if (!DeEnv->ErrAdr)
            pcWorkArea->ActivePage = tsSourceCode;
        delete DeEnv;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miHex2DoubleClick(TObject *Sender)
{
    FHex2DoubleDlg_11011981->ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::acFontAllExecute(TObject *Sender)
{
    FontsDlg->Font->Assign(lbCode->Font);
    if (FontsDlg->Execute()) SetupAllFonts(FontsDlg->Font);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::SetupAllFonts(TFont* font)
{
    TListBox* formListBoxes[] =
    {
      lbUnits,
      lbRTTIs,
      lbForms,
      lbAliases,
      lbCode,
      lbStrings,
      lbNames,
      lbNXrefs,
      lbSXrefs,
      lbCXrefs,
      lbSourceCode,
      lbUnitItems,
      0
    };

    TTreeView* formTreeViews[] =
    {
      tvClassesShort,
      tvClassesFull,
      0
    };

    for (int n = 0; formListBoxes[n]; n++)
    {
        formListBoxes[n]->Font->Assign(font);
    }

    for (int n = 0; formTreeViews[n]; n++)
    {
        formTreeViews[n]->Font->Assign(font);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmUnitItemsPopup(TObject *Sender)
{
    miEditFunctionI->Enabled = (lbUnitItems->ItemIndex >= 0);
    miCopyAddressI->Enabled = (lbUnitItems->ItemIndex >= 0);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::CopyAddress(String line, int ofs, int bytes)
{
    char    buf[9], *p = buf;

    Clipboard()->Open();
    for (int n = 1; n <= bytes; n++)
    {
        *p = line[n + ofs]; p++;
    }
    *p = 0;
    Clipboard()->SetTextBuf(buf);
    Clipboard()->Close();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopyAddressCodeClick(TObject *Sender)
{
    int bytes = (lbCode->ItemIndex) ? 8 : 0;
    CopyAddress(lbCode->Items->Strings[lbCode->ItemIndex], 1, bytes);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ClearPassFlags()
{
    ClearFlags(cfPass0 | cfPass1 | cfPass2 | cfPass, 0, TotalSize);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopySource2ClipboardClick(TObject *Sender)
{
    Copy2Clipboard(lbSourceCode->Items, 0, false);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmCodePopup(TObject *Sender)
{
    int         _ap;
    DWORD       _adr;
    DISINFO     _disInfo;

    miXRefs->Enabled = false;
    miXRefs->Clear();

    if (ActiveControl == lbCode)
    {
        if (lbCode->ItemIndex <= 0) return;
        DWORD   adr;
        sscanf(lbCode->Items->Strings[lbCode->ItemIndex].c_str() + 2, "%lX", &adr);
        if (adr != CurProcAdr && IsFlagSet(cfLoc, Adr2Pos(adr)))
        {
            PInfoRec recN = GetInfoRec(adr);
            if (recN && recN->xrefs && recN->xrefs->Count > 0)
            {
                miXRefs->Enabled = true;
                miXRefs->Clear();
                for (int n = 0; n < recN->xrefs->Count; n++)
                {
                    PXrefRec recX = (PXrefRec)recN->xrefs->Items[n];
                    _adr = recX->adr + recX->offset;
                    _ap = Adr2Pos(_adr);
                    Disasm.Disassemble(Code + _ap, (__int64)_adr, &_disInfo, 0);
                    TMenuItem* mi = new TMenuItem(pmCode);
                    mi->Caption = Val2Str8(_adr) + " " + _disInfo.Mnem;
                    mi->Tag = _adr;
                    mi->OnClick = GoToXRef;
                    miXRefs->Add(mi);
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::GoToXRef(TObject *Sender)
{
    TMenuItem* mi = (TMenuItem*)Sender;
    DWORD Adr = mi->Tag;
    if (Adr && IsValidCodeAdr(Adr))
    {
        PROCHISTORYREC rec;

        rec.adr = CurProcAdr;
        rec.itemIdx = lbCode->ItemIndex;
        rec.xrefIdx = lbCXrefs->ItemIndex;
        rec.topIdx = lbCode->TopIndex;
        ShowCode(CurProcAdr, Adr, lbCXrefs->ItemIndex, -1);
        CodeHistoryPush(&rec);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbFormsClick(TObject *Sender)
{
    FormsSearchFrom = lbForms->ItemIndex;
    WhereSearch = SEARCH_FORMS;
}
//---------------------------------------------------------------------------

void __fastcall TFMain_11011981::lbCodeClick(TObject *Sender)
{
    WhereSearch = SEARCH_CODEVIEWER;

    if (lbCode->ItemIndex <= 0) return;
    
    String prevItem = SelectedAsmItem;
    SelectedAsmItem = "";
    String text = lbCode->Items->Strings[lbCode->ItemIndex];
    int textLen = text.Length();

    TPoint cursorPos;
    GetCursorPos(&cursorPos);
    cursorPos = lbCode->ScreenToClient(cursorPos);
    for (int n = 1, wid = 0; n <= textLen; n++)
    {
        int cwid = lbCode->Canvas->TextWidth(text[n]);
        if (wid > cursorPos.x)
        {
            char c;
            int beg = n - 1;
            while (beg >= 1)
            {
                c = text[beg];
                if (!isalpha(c) && !isdigit(c) && c != '@')
                {
                    beg++;
                    break;
                }
                beg--;
            }
            int end = beg;
            while (end <= textLen)
            {
                c = text[end];
                if (!isalpha(c) && !isdigit(c) && c != '@')
                {
                    end--;
                    break;
                }
                end++;
            }
            SelectedAsmItem = text.SubString(beg, end - beg + 1);
            break;
        }
        wid += cwid;
    }
    if (SelectedAsmItem != prevItem) lbCode->Invalidate();
}
//---------------------------------------------------------------------------

void __fastcall TFMain_11011981::pcInfoChange(TObject *Sender)
{
    switch (pcInfo->TabIndex)
    {
      case 0: WhereSearch = SEARCH_UNITS;
      break;
      case 1: WhereSearch = SEARCH_RTTIS;
      break;
      case 2: WhereSearch = SEARCH_FORMS;
      break;
    }
}
//---------------------------------------------------------------------------

void __fastcall TFMain_11011981::pcWorkAreaChange(TObject *Sender)
{
    switch (pcWorkArea->TabIndex)
    {
      case 0: WhereSearch = SEARCH_CODEVIEWER;
      break;
      case 1: WhereSearch = SEARCH_CLASSVIEWER;
      break;
      case 2: WhereSearch = SEARCH_STRINGS;
      break;
      case 3: WhereSearch = SEARCH_NAMES;
      break;
      case 4: WhereSearch = SEARCH_SOURCEVIEWER;
      break;      
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchFormClick(TObject *Sender)
{
    WhereSearch = SEARCH_FORMS;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < FormsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(FormsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbForms->ItemIndex == -1)
            FormsSearchFrom = 0;
        else
            FormsSearchFrom = lbForms->ItemIndex;

        FormsSearchText = FindDlg_11011981->cbText->Text;
        if (FormsSearchList->IndexOf(FormsSearchText) == -1) FormsSearchList->Add(FormsSearchText);
        FindText(FormsSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchNameClick(TObject *Sender)
{
    WhereSearch = SEARCH_NAMES;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < NamesSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(NamesSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbNames->ItemIndex == -1)
            NamesSearchFrom = 0;
        else
            NamesSearchFrom = lbNames->ItemIndex;

        NamesSearchText = FindDlg_11011981->cbText->Text;
        if (NamesSearchList->IndexOf(NamesSearchText) == -1) NamesSearchList->Add(NamesSearchText);
        FindText(NamesSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miPluginsClick(TObject *Sender)
{
    String PluginsPath = AppDir + "Plugins";
    if (!DirectoryExists(PluginsPath))
    {
        if (!CreateDir(PluginsPath))
        {
            ShowMessage("Cannot create subdirectory for plugins");
            return;
        }
    }

    ResInfo->FormPluginName = "";
    if (ResInfo->hFormPlugin)
    {
        FreeLibrary(ResInfo->hFormPlugin);
        ResInfo->hFormPlugin = 0;
    }
    FPlugins->PluginsPath = PluginsPath;
    FPlugins->PluginName = "";
    if (FPlugins->ShowModal() == mrOk) ResInfo->FormPluginName = FPlugins->PluginName;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopyStringsClick(TObject *Sender)
{
    Copy2Clipboard(lbStrings->Items, 0, false);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miViewAllClick(TObject *Sender)
{
;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbSourceCodeMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (lbSourceCode->CanFocus()) ActiveControl = lbSourceCode;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::cbMultipleSelectionClick(TObject *Sender)
{
//    lbCode->MultiSelect = cbMultipleSelection->Checked;
//    miSwitchFlag->Enabled = cbMultipleSelection->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbSourceCodeDrawItem(TWinControl *Control,
      int Index, TRect &Rect, TOwnerDrawState State)
{
/*
    if (hHighlight && HighlightDrawItem)
    {
	    HighlightDrawItem(DelphiLbId, Index, Rect, false);
    }
*/
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSwitchSkipFlagClick(TObject *Sender)
{
    DWORD   _adr;

    if (lbCode->SelCount > 0)
    {
        for (int n = 0; n < lbCode->Count; n++)
        {
            if (lbCode->Selected[n])
            {
                sscanf(lbCode->Items->Strings[n].c_str() + 2, "%lX", &_adr);
                Flags[Adr2Pos(_adr)] ^= (cfDSkip | cfSkip);
            }
        }
        RedrawCode();
        ProjectModified = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSwitchFrameFlagClick(TObject *Sender)
{
    DWORD   _adr;

    if (lbCode->SelCount > 0)
    {
        for (int n = 0; n < lbCode->Count; n++)
        {
            if (lbCode->Selected[n])
            {
                sscanf(lbCode->Items->Strings[n].c_str() + 2, "%lX", &_adr);
                Flags[Adr2Pos(_adr)] ^= cfFrame;
            }
        }
        RedrawCode();
        ProjectModified = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::cfTry1Click(TObject *Sender)
{
    DWORD   _adr;

    if (lbCode->SelCount > 0)
    {
        for (int n = 0; n < lbCode->Count; n++)
        {
            if (lbCode->Selected[n])
            {
                sscanf(lbCode->Items->Strings[n].c_str() + 2, "%lX", &_adr);
                Flags[Adr2Pos(_adr)] ^= cfTry;
            }
        }
        RedrawCode();
        ProjectModified = true;
    }
}
//---------------------------------------------------------------------------
/*
void __fastcall TFMain_11011981::ChangeDelphiHighlightTheme(TObject *Sender)
{
    TMenuItem* mi = (TMenuItem*)Sender;
    ChangeTheme(DelphiLbId, mi->Tag);
}
//---------------------------------------------------------------------------
*/
void __fastcall TFMain_11011981::miProcessDumperClick(TObject *Sender)
{
    FActiveProcesses->ShowProcesses();
    FActiveProcesses->ShowModal();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbSourceCodeClick(TObject *Sender)
{
    WhereSearch = SEARCH_SOURCEVIEWER;

    if (lbSourceCode->ItemIndex <= 0) return;

    String prevItem = SelectedSourceItem;
    SelectedSourceItem = "";
    String text = lbSourceCode->Items->Strings[lbSourceCode->ItemIndex];
    int textLen = text.Length();

    TPoint cursorPos;
    GetCursorPos(&cursorPos);
    cursorPos = lbSourceCode->ScreenToClient(cursorPos);

    for (int n = 1, wid = 0; n <= textLen; n++)
    {
        int cwid = lbSourceCode->Canvas->TextWidth(text[n]);
        if (wid > cursorPos.x)
        {
            char c;
            int beg = n - 1;
            while (beg >= 1)
            {
                c = text[beg];
                if (!isalpha(c) && !isdigit(c) && c != '_' && c != '.')
                {
                    beg++;
                    break;
                }
                beg--;
            }
            int end = beg;
            while (end <= textLen)
            {
                c = text[end];
                if (!isalpha(c) && !isdigit(c) && c != '_' && c != '.')
                {
                    end--;
                    break;
                }
                end++;
            }
            SelectedSourceItem = text.SubString(beg, end - beg + 1);
            break;
        }
        wid += cwid;
    }
    if (SelectedSourceItem != prevItem) lbSourceCode->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSetlvartypeClick(TObject *Sender)
{
    PInfoRec recN = GetInfoRec(CurProcAdr);
    PLOCALINFO pLocInfo = recN->procInfo->GetLocal(SelectedSourceItem);
    if (recN && recN->procInfo->locals && SelectedSourceItem != "")
    {
        String ftype = InputDialogExec("Enter Type of " + SelectedSourceItem, "Type", pLocInfo->TypeDef).Trim();
        pLocInfo->TypeDef = ftype;
        recN->procInfo->SetLocalType(pLocInfo->Ofs, ftype);
        bDecompileClick(this);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmSourceCodePopup(TObject *Sender)
{
    PLOCALINFO  pLocalInfo;

    PInfoRec recN = GetInfoRec(CurProcAdr);
    if (recN && recN->procInfo->locals && SelectedSourceItem != "")
        pLocalInfo = recN->procInfo->GetLocal(SelectedSourceItem);

    miSetlvartype->Enabled = (pLocalInfo);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopytoClipboardNamesClick(
      TObject *Sender)
{
    Copy2Clipboard(lbNames->Items, 0, false);
}
//---------------------------------------------------------------------------

