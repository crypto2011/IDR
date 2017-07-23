//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Resources.h"
#include "Main.h"
#include "Misc.h"
#include "ProgressBar.h"

#include <Math.hpp>
#include <CheckLst.hpp>
#include <Chart.hpp>
#include <Tabnotbk.hpp>
#include <MPlayer.hpp>
#include <Tabs.hpp>
#include <FileCtrl.hpp>
#include <Perfgrap.h>
#include <Cspin.h>
#include <Colorgrd.hpp>
#include <CDiroutl.h>
#include <CCalendr.h>
#include <CGauges.h>
#include <Pies.h>

#include <StrUtils.hpp>
#include <StdActns.hpp>
#include <ExtActns.hpp>
#include <series.hpp>
#include <valedit.hpp>

int (__stdcall *fnGetDstSize)(BYTE*, int);
boolean (__stdcall *fnDecrypt)(BYTE*, int, BYTE*, int);

extern  TResourceInfo   *ResInfo;
extern  DWORD       CodeBase;
extern  PInfoRec    *Infos;

//---------------------------------------------------------------------------
bool	ClassesRegistered = false;

RegClassInfo RegClasses[] =
{
    //Standard
    {__classid(Forms::TFrame), "TFrame"},
    {__classid(Menus::TMainMenu), "TMainMenu"},
    {__classid(Menus::TPopupMenu), "TPopupMenu"},
    {__classid(TLabel), "TLabel"},
    {__classid(TEdit), "TEdit"},
    {__classid(TLabeledEdit), "TLabeledEdit"},
    {__classid(TMemo), "TMemo"},
    {__classid(TButton), "TButton"},
    {__classid(TCheckBox), "TCheckBox"},
    {__classid(TRadioButton), "TRadioButton"},
    {__classid(TListBox), "TListBox"},
    {__classid(TComboBox), "TComboBox"},
    {__classid(TScrollBar), "TScrollBar"},
    {__classid(TGroupBox), "TGroupBox"},
    {__classid(TRadioGroup), "TRadioGroup"},
    {__classid(TPanel), "TPanel"},
    {__classid(TActionList), "TActionList"},
    {__classid(TAction), "TAction"},
    {__classid(Stdactns::TEditCut), "TEditCut"},
    {__classid(Stdactns::TEditCopy), "TEditCopy"},
    {__classid(Stdactns::TEditPaste), "TEditPaste"},
    {__classid(Stdactns::TEditSelectAll), "TEditSelectAll"},
    {__classid(Stdactns::TEditUndo), "TEditUndo"},
    {__classid(Stdactns::TEditDelete), "TEditDelete"},
    {__classid(Stdactns::TWindowClose), "TWindowClose"},
    {__classid(Stdactns::TWindowCascade), "TWindowCascade"},
    {__classid(Stdactns::TWindowTileHorizontal), "TWindowTileHorizontal"},
    {__classid(Stdactns::TWindowTileVertical), "TWindowTileVertical"},
    {__classid(Stdactns::TWindowMinimizeAll), "TWindowMinimizeAll"},
    {__classid(Stdactns::TWindowArrange), "TWindowArrange"},
    {__classid(Stdactns::THelpContents), "THelpContents"},
    {__classid(Stdactns::THelpTopicSearch), "THelpTopicSearch"},
    {__classid(Stdactns::THelpOnHelp), "THelpOnHelp"},
    {__classid(Stdactns::THelpContextAction), "THelpContextAction"},
    {__classid(Stdactns::TFileOpen), "TFileOpen"},
    {__classid(Stdactns::TFileOpenWith), "TFileOpenWith"},
    {__classid(Stdactns::TFileSaveAs), "TFileSaveAs"},
    {__classid(Stdactns::TFilePrintSetup), "TFilePrintSetup"},
    {__classid(Stdactns::TFileExit), "TFileExit"},
    {__classid(Stdactns::TSearchFind), "TSearchFind"},
    {__classid(Stdactns::TSearchReplace), "TSearchReplace"},
    {__classid(Stdactns::TSearchFindFirst), "TSearchFindFirst"},
    {__classid(Stdactns::TSearchFindNext), "TSearchFindNext"},
    {__classid(Stdactns::TFontEdit), "TFontEdit"},
    {__classid(Stdactns::TColorSelect), "TColorSelect"},
    {__classid(Stdactns::TPrintDlg), "TPrintDlg"},
    //TRichEditxxx actions?
    //TListControlXXX ?
    {__classid(Extactns::TOpenPicture), "TOpenPicture"},
    {__classid(Extactns::TSavePicture), "TSavePicture"},
    //Additional
    {__classid(Buttons::TBitBtn), "TBitBtn"},
    {__classid(Buttons::TSpeedButton), "TSpeedButton"},
    {__classid(Mask::TMaskEdit), "TMaskEdit"},
    {__classid(Grids::TStringGrid), "TStringGrid"},
    {__classid(Grids::TDrawGrid), "TDrawGrid"},
    {__classid(Extctrls::TImage), "TImage"},
    {__classid(Graphics::TPicture), "TPicture"},
    {__classid(Graphics::TBitmap), "TBitmap"},
    {__classid(Graphics::TGraphic), "TGraphic"},
    {__classid(Graphics::TMetafile), "TMetafile"},
    {__classid(Graphics::TIcon), "TIcon"},

    {__classid(TShape), "TShape"},
    {__classid(TBevel), "TBevel"},
    {__classid(Forms::TScrollBox), "TScrollBox"},
    {__classid(TCheckListBox), "TCheckListBox"},
    {__classid(TSplitter), "TSplitter"},
    {__classid(TStaticText), "TStaticText"},
    {__classid(TControlBar), "TControlBar"},
    {__classid(Chart::TChart), "TChart"},
    {__classid(Series::TBarSeries), "TBarSeries"},
    {__classid(Series::THorizBarSeries), "THorizBarSeries"},
    {__classid(Series::TPointSeries),  "TPointSeries"},
    {__classid(Series::TAreaSeries), "TAreaSeries"},
    {__classid(Series::TLineSeries), "TLineSeries"},
    {__classid(Series::TFastLineSeries), "TFastLineSeries"},
    {__classid(Series::TPieSeries), "TPieSeries"},
    {__classid(TColorBox), "TColorBox"},
    //Win32
    {__classid(TTabControl), "TTabControl"},
    {__classid(TPageControl), "TPageControl"},
    {__classid(TTabSheet), "TTabSheet"},
    {__classid(TImageList), "TImageList"},
    {__classid(TRichEdit), "TRichEdit"},
    {__classid(TTrackBar), "TTrackBar"},
    {__classid(TProgressBar), "TProgressBar"},
    {__classid(TUpDown), "TUpDown"},
    {__classid(THotKey), "THotKey"},
    {__classid(TAnimate), "TAnimate"},
    {__classid(TDateTimePicker), "TDateTimePicker"},
    {__classid(TMonthCalendar), "TMonthCalendar"},
    {__classid(TTreeView), "TTreeView"},
    {__classid(TListView), "TListView"},
    {__classid(THeaderControl), "THeaderControl"},
    {__classid(TStatusBar), "TStatusBar"},
    {__classid(TToolBar), "TToolBar"},
    {__classid(TToolButton), "TToolButton"},
    {__classid(TCoolBar), "TCoolBar"},
    {__classid(TComboBoxEx), "TComboBoxEx"},
    {__classid(TPageScroller), "TPageScroller"},
    //System
    //__classid(TPaintBox),
    {__classid(TMediaPlayer), "TMediaPlayer"},
    //Win 3.1
    {__classid(Tabs::TTabSet), "TTabSet"},
    {__classid(Outline::TOutline), "TOutline"},
    {__classid(Tabnotbk::TTabbedNotebook), "TTabbedNotebook"},
    {__classid(TNotebook), "TNotebook"},
    {__classid(TPage), "TPage"},
    {__classid(THeader), "THeader"},
    {__classid(TFileListBox), "TFileListBox"},
    {__classid(TDirectoryListBox), "TDirectoryListBox"},
    {__classid(TDriveComboBox), "TDriveComboBox"},
    {__classid(TFilterComboBox), "TFilterComboBox"},
    //Samples
    {__classid(TPerformanceGraph), "TPerformanceGraph"},
    {__classid(TCSpinButton), "TCSpinButton"},
    {__classid(TTimerSpeedButton), "TTimerSpeedButton"},
    {__classid(TCSpinEdit), "TCSpinEdit"},
    {__classid(TColorGrid), "TColorGrid"},
    {__classid(TCGauge), "TCGauge"},
    {__classid(TCDirectoryOutline), "TCDirectoryOutline"},
    {__classid(TCCalendar), "TCCalendar"},
    {__classid(TPie), "TPie"},
    //
    {__classid(TValueListEditor), "TValueListEditor"},
    //
    {__classid(IdrDfmDefaultControl), "Default"},
    {0, 0}
};
//---------------------------------------------------------------------------
__fastcall TDfm::TDfm()
{
    Open = 0;
	Flags = 0;
    ResName = "";
    Name = "";
    ClassName = "";
    MemStream = new TMemoryStream;
    ParentDfm = 0;
    Events = new TList;
    Components = new TList;
    Form = 0;
    Loader = 0;
}
//---------------------------------------------------------------------------
__fastcall TDfm::~TDfm()
{
    delete MemStream;

    int cnt = Events->Count;
    for (int n = 0; n < cnt; n++)
    	delete (PEventInfo)Events->Items[n];
    delete Events;

    cnt = Components->Count;
    for (int n = 0; n < cnt; n++)
    {
    	PComponentInfo cInfo = (PComponentInfo)Components->Items[n];
    	if (cInfo->Events)
        {
            for (int m = 0; m < cInfo->Events->Count; m++)
                delete (PEventInfo)cInfo->Events->Items[m];
            delete cInfo->Events;
        }
        delete cInfo;
    }
    delete Components;
    if (Form) delete Form;
    if (Loader) delete Loader;
}
//---------------------------------------------------------------------------
bool __fastcall TDfm::IsFormComponent(String CompName)
{
    int cnt = Components->Count;
    for (int n = 0; n < cnt; n++)
    {
        PComponentInfo cInfo = (PComponentInfo)Components->Items[n];
        if (SameText(CompName, cInfo->Name)) return true;
    }
    return false;
}
//---------------------------------------------------------------------------
__fastcall TResourceInfo::TResourceInfo()
{
	citadel = false;
    Counter = 0;
    hFormPlugin = 0;
    FormPluginName = "";
    FormList = new TList;
    Aliases = new TStringList;

    if (!ClassesRegistered)
    {
        for (int n = 0;; n++)
        {
            if (!RegClasses[n].RegClass) break;
            Classes::RegisterClass(RegClasses[n].RegClass);
        }
        ClassesRegistered = true;
    }
}
//---------------------------------------------------------------------------
__fastcall TResourceInfo::~TResourceInfo()
{
    if (hFormPlugin)
    {
        FreeLibrary(hFormPlugin);
        hFormPlugin = 0;
    }
    int cnt = FormList->Count;
    for (int n = 0; n < cnt; n++)
    {
        TDfm* dfm = (TDfm*)FormList->Items[n];
        delete dfm;
    }

    delete FormList;
    delete Aliases;

}
//---------------------------------------------------------------------------
int __fastcall AnalyzeSection(TDfm* Dfm, TStringList* FormText, int From, PComponentInfo CInfo, int objAlign)
{
    int     n, pos, align;
    String  componentName, componentClass, eventName, procName;

    for (n = From; n < FormText->Count; n++)
    {
        String line = FormText->Strings[n].Trim();
        if (line == "") continue;
        align = FormText->Strings[n].Pos(line);

        if (SameText(line, "end") && align == objAlign) break;

        bool inherited = (line.Pos("inherited ") == 1);

        if (line.Pos("object ") == 1 || inherited)
        {
            int pos = line.Pos(":");
            int end = line.Length();
            int tmp = line.Pos(" [");   //eg: object Label1: TLabel [1]
            if (tmp) end = tmp - 1;
            int off = (inherited ? 10 : 7);

            //object Label1: TLabel
            if (pos)
            {
                componentName = line.SubString(off, pos - off).Trim();
                componentClass = line.SubString(pos + 1, end - pos).Trim();
            }
            //object TLabel
            else
            {
                componentName = "_" + Val2Str4(ResInfo->Counter) + "_"; ResInfo->Counter++;
                componentClass = line.SubString(off, end - off + 1).Trim();
            }

            PComponentInfo cInfo = new ComponentInfo;
            cInfo->Inherit = inherited;
            cInfo->HasGlyph = false;
            cInfo->Name = componentName;
            cInfo->ClassName = componentClass;
            cInfo->Events = new TList;

            Dfm->Components->Add((void*)cInfo);

            if (ResInfo->Aliases->IndexOf(cInfo->ClassName) == -1)
                ResInfo->Aliases->Add(cInfo->ClassName);
             
            n = AnalyzeSection(Dfm, FormText, n + 1, cInfo, align);
        }
        //Events begins with "On" or "Action". But events of TDataSet may begin with no these prefixes!!!
        if ((pos = line.Pos("=")) != 0 &&
        	((line[1] == 'O' && line[2] == 'n') || (line.Pos("Action ") == 1)))
        {
            eventName = line.SubString(1, pos - 1).Trim();   //Include "On"
            procName = line.SubString(pos + 1, line.Length() - pos).Trim();
            PEventInfo eInfo = new EventInfo;
            eInfo->EventName = eventName;
            eInfo->ProcName = procName;
            //Form itself
            if (!CInfo)
            {
                Dfm->Events->Add((void*)eInfo);
            }
            else
            {
            	CInfo->Events->Add((void*)eInfo);
            }
        }
        //Has property Glyph?
        if (line.Pos("Glyph.Data =") == 1)
        	CInfo->HasGlyph = true;
    }
    return n;
}
//---------------------------------------------------------------------------
bool __stdcall EnumResNameProcedure(int hModule, char* Type, char* Name, long Param)
{
    bool                res;
    TFilerFlags         flags;
    int                 position, srcSize, dstSize;
    TDfm*               dfm;
    TStringList*        formText;
    TMemoryStream*      ms;
    TResourceStream*    resStream;
    String              className, vmtName;
    BYTE                signature[4];

    if (Type == RT_RCDATA || Type == RT_BITMAP)
    {
        resStream = new TResourceStream(hModule, Name, Type);
        if (Type == RT_RCDATA)
        {
            ms = new TMemoryStream;
            ms->LoadFromStream(resStream);
            res = true;
            if (ResInfo->hFormPlugin)
            {
                fnGetDstSize = (int (__stdcall*)(BYTE*, int))GetProcAddress(ResInfo->hFormPlugin, "GetDstSize");
                fnDecrypt = (boolean (__stdcall*)(BYTE*, int, BYTE*, int))GetProcAddress(ResInfo->hFormPlugin, "Decrypt");
                res = (fnGetDstSize && fnDecrypt);
                if (res)
                {
                    srcSize = ms->Size;
                    dstSize = fnGetDstSize((BYTE*)ms->Memory, srcSize);
                    if (srcSize != dstSize)
                    {
                        TMemoryStream *ds = new TMemoryStream;
                        ds->Size = dstSize;
                        res = fnDecrypt((BYTE*)ms->Memory, srcSize, (BYTE*)ds->Memory, dstSize);
                        ms->Size = dstSize;
                        ms->CopyFrom(ds, dstSize);
                        delete ds;
                    }
                    else
                    {
                        res = fnDecrypt((BYTE*)ms->Memory, srcSize, (BYTE*)ms->Memory, dstSize);
                    }
                }
                if (res)
                {
                    BYTE *mp = (BYTE*)ms->Memory;
                    BYTE len = *(mp + 4);
                    res = (len == strlen(Name) && SameText(Name, String((char*)mp + 5, len)));
                }
            }
            if (res)
            {
                ms->Seek(0, soFromBeginning);
                ms->Read(signature, 4);
                if (signature[0] == 'T' && signature[1] == 'P' && signature[2] == 'F' && (signature[3] >= '0' && signature[3] <= '7'))
                {
                    if (signature[3] == '0')
                    {
                        //Добавляем в список ресурсов
                        dfm = new TDfm;
                        dfm->ResName = Name;
                        dfm->MemStream = ms;

                        //Analyze text representation
                        formText = new TStringList;
                        ResInfo->GetFormAsText(dfm, formText);

                        String line = formText->Strings[0];
                        bool inherited = (line.Pos("inherited ") == 1);
                        if (inherited)
                            dfm->Flags |= FF_INHERITED;
                        if (line.Pos("object ") == 1 || inherited)
                        {
                            int off = (inherited ? 10 : 7);
                            int pos = line.Pos(":");
                            dfm->Name = line.SubString(off, pos - off).Trim();
                            dfm->ClassName = line.SubString(pos + 1, line.Length() - pos).Trim();
                            AnalyzeSection(dfm, formText, 1, 0, 1);
                        }
                        delete formText;

                        ((TResourceInfo*)Param)->FormList->Add((void*)dfm);
                    }
                    else if (!ResInfo->citadel)
                    {
                        ShowMessage("Citadel for Delphi detected, forms cannot be loaded");
                        ResInfo->citadel = true;
                    }
                }
                else
                {
                    delete ms;
                }
            }
            else
            {
                delete ms;
            }
        }
        else if (Type == RT_BITMAP)
        {
            dfm = new TDfm;
            dfm->ResName = Name;
            dfm->MemStream->LoadFromStream(resStream);
            ((TResourceInfo*)Param)->FormList->Add((void*)dfm);
        }
        delete resStream;
    }
    return true;
}
//---------------------------------------------------------------------------
bool __fastcall TResourceInfo::EnumResources(String FileName)
{
	citadel = false;
    hFormPlugin = LoadLibrary((FMain_11011981->AppDir + "Plugins\\" + FormPluginName).c_str());
    HINSTANCE hInst = LoadLibraryEx(FileName.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst)
    {
        EnumResourceNames(hInst, RT_RCDATA, (int (__stdcall*)())EnumResNameProcedure, (long)this);
        FreeLibrary(hInst);
        return true;
    }
    if (hFormPlugin)
    {
        FreeLibrary(hFormPlugin);
        hFormPlugin = 0;
    }
    return false;
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::ShowResources(TListBox* ListBox)
{
    ListBox->Clear();

    int     wid, maxwid = 0;
    TCanvas *canvas = ListBox->Canvas;
    int cnt = FormList->Count;
    for (int n = 0; n < cnt; n++)
    {
        TDfm *dfm = (TDfm*)FormList->Items[n];
        ListBox->Items->Add(dfm->ResName + " {" + dfm->Name + "}");
        wid = canvas->TextWidth(dfm->ResName); if (wid > maxwid) maxwid = wid;
    }
    ListBox->ScrollWidth = maxwid + 2;
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::GetFormAsText(TDfm* Dfm, TStrings* DstList)
{
    TMemoryStream *memStream = new TMemoryStream;
    memStream->Size = Dfm->MemStream->Size;
    Dfm->MemStream->Seek(0, soFromBeginning);
    ObjectBinaryToText(Dfm->MemStream, memStream);
    memStream->Seek(0, soFromBeginning);
    DstList->LoadFromStream(memStream);
    delete memStream;
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::GetBitmap(TDfm* Dfm, Graphics::TBitmap* DstBitmap)
{
    Dfm->MemStream->Seek(0, soFromBeginning);
    DstBitmap->LoadFromStream(Dfm->MemStream);
}
//---------------------------------------------------------------------------
TDfm* __fastcall TResourceInfo::GetParentDfm(TDfm* Dfm)
{
    String parentName = GetParentName(Dfm->ClassName);
    int cnt = FormList->Count;
    for (int n = 0; n < cnt; n++)
    {
        TDfm* dfm1 = (TDfm*)FormList->Items[n];
        if (SameText(dfm1->ClassName, parentName)) return dfm1;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::CloseAllForms()
{
    int cnt = FormList->Count;
    for (int n = 0; n < cnt; n++)
    {
        TDfm* dfm = (TDfm*)FormList->Items[n];
        if (dfm->Open == 2)
        {
            if (dfm->Form && dfm->Form->Visible) dfm->Form->Close();
            dfm->Form = 0;
            dfm->Open = 1;
        }
        if (dfm->Open == 1)
        {
            if (dfm->Loader) delete dfm->Loader;
            dfm->Form = 0;
            dfm->Loader = 0;
            dfm->Open = 0;
        }
    }
    
    //this is a must have call!
    //it removes just closed form from the Screen->Forms[] list!
    //and we'll not have a new form with name [old_name]_1 ! 
    Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::ReopenAllForms()
{
    int cnt = FormList->Count;
    for (int n = 0; n < cnt; n++)
    {
        TDfm* dfm = (TDfm*)FormList->Items[n];
        if (dfm->Open == 2) //still opened
        {
            if (dfm->Form && dfm->Form->Visible)
            {
                String FormName = dfm->Form->Name;
                dfm->Form->Close();

                //this is a must have call!
                //it removes just closed form from the Screen->Forms[] list!
                //and we'll not have a new form with name [old_name]_1 ! 
                Application->ProcessMessages();

                int frmIdx = -1;
                TDfm* dfm = GetFormIdx(FormName, &frmIdx);
                FMain_11011981->ShowDfm(dfm);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::InitAliases()
{
    int cnt = Aliases->Count;
    for (int n = 0; n < cnt; n++)
    {
        String className = Aliases->Strings[n];
        if (!className.Pos("="))
        {
            String alias = "";
            try
            {
                //TMetaClass *componentClass = FindClass(className);
            }
            catch (Exception& e)
            {
                if (SameText(className, "TDBGrid"))
                    alias = "TStringGrid";
                else if (SameText(className, "TDBText"))
                    alias = "TLabel";
                else if (SameText(className, "TDBEdit"))
                    alias = "TEdit";
                else if (SameText(className, "TDBMemo"))
                    alias = "TMemo";
                else if (SameText(className, "TDBImage"))
                    alias = "TImage";
                else if (SameText(className, "TDBListBox"))
                    alias = "TListBox";
                else if (SameText(className, "TDBComboBox"))
                    alias = "TComboBox";
                else if (SameText(className, "TDBCheckBox"))
                    alias = "TCheckBox";
                else if (SameText(className, "TDBRadioGroup"))
                    alias = "TRadioGroup";
                else if (SameText(className, "TDBLookupListBox"))
                    alias = "TListBox";
                else if (SameText(className, "TDBLookupComboBox"))
                    alias = "TComboBox";
                else if (SameText(className, "TDBRichEdit"))
                    alias = "TRichEdit";
                else if (SameText(className, "TDBCtrlGrid"))
                    alias = "TStringGrid";
                else if (SameText(className, "TDBChart"))
                    alias = "TChart";
                //sample components are redirected to TCxxx components
                else if (SameText(className, "TGauge"))
                    alias = "TCGauge";
                else if (SameText(className, "TSpinEdit"))
                    alias = "TCSpinEdit";
                else if (SameText(className, "TSpinButton"))
                    alias = "TCSpinButton";
                else if (SameText(className, "TCalendar"))
                    alias = "TCCalendar";
                else if (SameText(className, "TDirectoryOutline"))
                    alias = "TCDirectoryOutline";
                else if (SameText(className, "TShellImageList"))
                    alias = "TImageList"; //special case, if not handled- crash (eg:1st Autorun Express app)
                else if (!GetClass(className))
                {
                    if (IsInheritsByClassName(className, "TBasicAction"))
                        alias = "TAction";
                    else if (IsInheritsByClassName(className, "TMainMenu"))
                        alias = "TMainMenu";
                    else if (IsInheritsByClassName(className, "TPopupMenu"))
                        alias = "TPopupMenu";
                    else if (IsInheritsByClassName(className, "TCustomLabel")
                        || AnsiContainsText (className, "Label") //evristika
                        )
                        alias = "TLabel";
                    else if (IsInheritsByClassName(className, "TCustomEdit"))
                        alias = "TEdit";
                    else if (IsInheritsByClassName(className, "TCustomMemo"))
                        alias = "TMemo";
                    else if (IsInheritsByClassName(className, "TButton"))
                        alias = "TButton";
                    else if (IsInheritsByClassName(className, "TCustomCheckBox"))
                        alias = "TCheckBox";
                    else if (IsInheritsByClassName(className, "TRadioButton"))
                        alias = "TRadioButton";
                    else if (IsInheritsByClassName(className, "TCustomListBox"))
                        alias = "TListBox";
                    else if (IsInheritsByClassName(className, "TCustomComboBox"))
                        alias = "TComboBox";
                    else if (IsInheritsByClassName(className, "TCustomGroupBox"))
                        alias = "TGroupBox";
                    else if (IsInheritsByClassName(className, "TCustomRadioGroup"))
                        alias = "TRadioGroup";
                    else if (IsInheritsByClassName(className, "TCustomPanel"))
                        alias = "TPanel";
                    else if (IsInheritsByClassName(className, "TBitBtn"))
                        alias = "TBitBtn";
                    else if (IsInheritsByClassName(className, "TSpeedButton"))
                        alias = "TSpeedButton";
                    else if (IsInheritsByClassName(className, "TImage"))
                        alias = "TImage";
                    else if (IsInheritsByClassName(className, "TImageList"))
                        alias = "TImageList";
                    else if (IsInheritsByClassName(className, "TBevel"))
                        alias = "TBevel";
                    else if (IsInheritsByClassName(className, "TSplitter"))
                        alias = "TSplitter";
                    else if (IsInheritsByClassName(className, "TPageControl"))
                        alias = "TPageControl";
                    else if (IsInheritsByClassName(className, "TToolBar"))
                        alias = "TToolBar";
                    else if (IsInheritsByClassName(className, "TToolButton"))
                        alias = "TToolButton";
                    else if (IsInheritsByClassName(className, "TCustomStatusBar"))
                        alias = "TStatusBar";
                    else if (IsInheritsByClassName(className, "TDateTimePicker"))
                        alias = "TDateTimePicker";
                    else if (IsInheritsByClassName(className, "TCustomListView"))
                        alias = "TListView";
                    else if (IsInheritsByClassName(className, "TCustomTreeView"))
                        alias = "TTreeView";
                    else
                        alias = "Default";
                }
            }
            if (alias != "")
                Aliases->Strings[n] = className + "=" + alias;
        }
    }
}
//---------------------------------------------------------------------------
String __fastcall TResourceInfo::GetAlias(String ClassName)
{
    for (int n = 0; n < Aliases->Count; n++)
    {
        if (SameText(ClassName, Aliases->Names[n]))
            return Aliases->Values[ClassName];
    }
    return "";
}
//---------------------------------------------------------------------------
TDfm* __fastcall TResourceInfo::GetFormIdx(String FormName, int* idx)
{
    for (int n = 0; n < FormList->Count; n++)
    {
    	TDfm* dfm = (TDfm*)FormList->Items[n];
        //Find form
        if (SameText(dfm->Name, FormName))
        {
            *idx = n;
            return dfm;
        }
    }
    *idx = -1;
    return 0;
}
//---------------------------------------------------------------------------
TDfm* __fastcall TResourceInfo::GetFormByClassName(String ClassName)
{
    for (int n = 0; n < FormList->Count; n++)
    {
    	TDfm* dfm = (TDfm*)FormList->Items[n];
        //Find form
        if (SameText(dfm->ClassName, ClassName)) return dfm;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TResourceInfo::GetEventsList(String FormName, TList* Lst)
{
    int             n, evCnt, evCnt1, compCnt, compCnt1, frmCnt;
    PEventListItem  item;
    PInfoRec        recN, recN1;

    TDfm* dfm = GetFormIdx(FormName, &n);
    if (!dfm) return;

    DWORD classAdr = GetClassAdr(dfm->ClassName);
    recN = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;
    //Form
    //Inherited - begin fill list
    if (dfm->Flags & FF_INHERITED)
    {
        TDfm* parentDfm = GetParentDfm(dfm);
        if (parentDfm)
        {
            GetEventsList(parentDfm->Name, Lst);
            int evCount = Lst->Count;
            evCount = evCount;
        }
    }
    if (dfm->Events->Count)
    {
        TList* ev = dfm->Events;
        evCnt = ev->Count;
        for (int k = 0; k < evCnt; k++)
        {
            PEventInfo eInfo = (PEventInfo)ev->Items[k];
            item = new EventListItem;
            item->CompName = FormName;
            item->EventName = eInfo->EventName;//eInfo->ProcName;
            String methodName = dfm->ClassName + "." + eInfo->ProcName;
            //Ищем адрес соответствующего метода
            PMethodRec recM = FMain_11011981->GetMethodInfo(recN, methodName);
            item->Adr = (recM) ? recM->address : 0;
            Lst->Add(item);
        }
    }
    //Components
    compCnt = dfm->Components->Count;
    for (int m = 0; m < compCnt; m++)
    {
        PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
        if (!cInfo->Events->Count) continue;
                
        TList* ev = cInfo->Events;
        evCnt = ev->Count;
        for (int k = 0; k < evCnt; k++)
        {
            recN1 = recN;
            PEventInfo eInfo = (PEventInfo)ev->Items[k];
            item = new EventListItem;
            item->CompName = cInfo->Name;
            item->EventName = eInfo->EventName;//eInfo->ProcName;
            String methodName = "";
            //Action
            if (SameText(eInfo->EventName, "Action"))
            {
                //Name eInfo->ProcName like FormName.Name
                int dotpos = eInfo->ProcName.Pos(".");
                if (dotpos)
                {
                    String moduleName = eInfo->ProcName.SubString(1, dotpos - 1);
                    String actionName = eInfo->ProcName.SubString(dotpos + 1, eInfo->ProcName.Length() - dotpos);
                    //Find form moduleName
                    frmCnt = FormList->Count;
                    for (int j = 0; j < frmCnt; j++)
                    {
                        TDfm* dfm1 = (TDfm*)FormList->Items[j];
                        if (SameText(dfm1->Name, moduleName))
                        {
                            classAdr = GetClassAdr(dfm1->ClassName);
                            recN1 = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;
                            //Find component actionName
                            compCnt1 = dfm1->Components->Count;
                            for (int r = 0; r < compCnt1; r++)
                            {
                                PComponentInfo cInfo1 = (PComponentInfo)dfm1->Components->Items[r];
                                if (SameText(cInfo1->Name, actionName))
                                {
                                    //Find event OnExecute
                                    TList* ev1 = cInfo1->Events;
                                    evCnt1 = ev1->Count;
                                    for (int i = 0; i < evCnt1; i++)
                                    {
                                        PEventInfo eInfo1 = (PEventInfo)ev1->Items[i];
                                        if (SameText(eInfo1->EventName, "OnExecute"))
                                        {
                                            methodName = dfm1->ClassName + "." + eInfo1->ProcName;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                else
                {
                    //Find component eInfo->ProcName
                    compCnt = dfm->Components->Count;
                    for (int r = 0; r < compCnt; r++)
                    {
                        PComponentInfo cInfo1 = (PComponentInfo)dfm->Components->Items[r];
                        if (SameText(cInfo1->Name, eInfo->ProcName))
                        {
                            //Find event OnExecute
                            TList* ev1 = cInfo1->Events;
                            evCnt1 = ev1->Count;
                            for (int i = 0; i < evCnt1; i++)
                            {
                                PEventInfo eInfo1 = (PEventInfo)ev1->Items[i];
                                if (SameText(eInfo1->EventName, "OnExecute"))
                                {
                                    methodName = dfm->ClassName + "." + eInfo1->ProcName;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                methodName = dfm->ClassName + "." + eInfo->ProcName;
            }
            //Find method address
            PMethodRec recM = FMain_11011981->GetMethodInfo(recN1, methodName);
            item->Adr = (recM) ? recM->address : 0;
            Lst->Add(item);
        }
    }
}
//---------------------------------------------------------------------------
//IdrDfmReader
//---------------------------------------------------------------------------
__fastcall IdrDfmReader::IdrDfmReader(TStream* Stream, int BufSize) :TReader(Stream, BufSize)
{
}
//---------------------------------------------------------------------------
//TComponentEvents
//---------------------------------------------------------------------------
__fastcall TComponentEvents::TComponentEvents(TCollection* Owner) : TCollectionItem(Owner)
{
    Component = 0;
    FoundEvents = new TStringList();
    FoundEvents->Sorted = true;
}
//---------------------------------------------------------------------------
__fastcall TComponentEvents::~TComponentEvents()
{
	delete FoundEvents;
}
//---------------------------------------------------------------------------
//IdrDfmForm
//---------------------------------------------------------------------------
__fastcall IdrDfmForm::IdrDfmForm(TComponent* Owner) : TForm(Owner)
{
	compsEventsList = 0;
    current = 0;
    evPopup = new TPopupMenu(0);
    evPopup->AutoPopup = false;
    evPopup->AutoHotkeys = maManual;
    frmTree = 0;

    KeyPreview = true;
}
//---------------------------------------------------------------------------
__fastcall IdrDfmForm::IdrDfmForm(TComponent* Owner, int Dummy) : TForm(Owner, Dummy)
{
	compsEventsList = 0;
    current = 0;
    evPopup = new TPopupMenu(0);
    evPopup->AutoPopup = false;
    evPopup->AutoHotkeys = maManual;
    frmTree = 0;

    KeyPreview = true;
}
//---------------------------------------------------------------------------
__fastcall IdrDfmForm::~IdrDfmForm()
{
    if (compsEventsList) delete compsEventsList;
    compsEventsList = 0;
    delete evPopup;
    if (frmTree) delete frmTree;
    frmTree = 0;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::SetDesigning(bool value, bool SetChildren)
{
   TComponent::SetDesigning(value, SetChildren);
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::CreateHandle()
{
    if (FormStyle == fsMDIChild)
    {
        FormStyle = fsNormal;
        return;
    }
    
    TForm::CreateHandle();
}
//---------------------------------------------------------------------------
//this is required action as sometimes ShortCut from target exe could be same as ours...
void __fastcall IdrDfmForm::SetupControlResetShortcut(TComponent* component)
{
    TCustomAction* action = dynamic_cast<TCustomAction*>(component);
    if (action && action->ShortCut)
        action->ShortCut = 0;

    TMenuItem* mi = dynamic_cast<TMenuItem*>(component);
    if (mi && mi->ShortCut)
        mi->ShortCut = 0;
}
//---------------------------------------------------------------------------
//different fine-tuning and fixes for components of the DFM Forms
void __fastcall IdrDfmForm::SetupControls()
{
    TComponent* component = this;
    TControl* control = dynamic_cast<TControl*>(this);
    if (control)
    {
        SetupControlHint(Name, control, GetShortHint(control->Hint));
        ((TMyControl*)control)->OnMouseDown = ControlMouseDown;
        if (((TMyControl*)control)->PopupMenu) ((TMyControl*)control)->PopupMenu = 0;
    }

    TBasicAction* action = dynamic_cast<TBasicAction*>(component);
    if (action) action->OnExecute = ActionExecute;
    SetupControlResetShortcut(component);

    for (int n = 0; n < ComponentCount; n++)
    {
        component = Components[n];
        const String curName = component->Name;
        SetupControlResetShortcut(component);
        
        control = dynamic_cast<TControl*>(component);
        if (control)
        {
            SetupControlHint(Name, control, GetShortHint(control->Hint));
            control->Visible = true;
            control->Enabled = true;
            ((TMyControl*)control)->OnMouseDown = ControlMouseDown;
            if (((TMyControl*)control)->PopupMenu) ((TMyControl*)control)->PopupMenu = 0;
        }

        if (TPageControl* pageControl = dynamic_cast<TPageControl*>(component))
        {
            if (pageControl->PageCount > 0)
                pageControl->ActivePageIndex = pageControl->PageCount - 1;
        }

        else if(TTabSheet* ts = dynamic_cast<TTabSheet*>(component))
            ts->TabVisible = true;
                    
        else if (TBasicAction* action = dynamic_cast<TBasicAction*>(component))
            action->OnExecute = ActionExecute;

        else if (TMenuItem* mi = dynamic_cast<TMenuItem*>(component))
        {
            SetupMenuItem(mi, Name); //we start from this form name
            mi->Enabled = true;
        }

        //else if(TProgressBar* pb = dynamic_cast<TProgressBar*>(component))
        //   FProgressBar->pb->Position = FProgressBar->pb->Max/3;

        //else if (TStatusBar* sb = dynamic_cast<TStatusBar*>(component))
        //    FProgressBar->sb->AutoHint = false;
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::ControlMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (Button == mbRight)
    {
        TControl* control = dynamic_cast<TControl*>(Sender);
        if (control)
        {
            evPopup->Items->Clear();
            ShowMyPopupMenu(Name, control->Name, true);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::miClick(TObject* Sender)
{
    TMenuItem* mi = dynamic_cast<TMenuItem*>(Sender);
    if (mi->Tag) FMain_11011981->ShowCode(mi->Tag, 0, -1, -1);
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::ActionExecute(TObject* Sender)
{
    //note: this is stub, you shold not stop here.....
    //if you are here -> smth not OK
    int x=10;
    ++x;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::ShowMyPopupMenu(String FormName, String ControlName, bool show)
{
    int         n;
    TMenuItem   *mi;
    TDfm* dfm = ResInfo->GetFormIdx(FormName, &n);
    if (!dfm) return;
    
    DWORD classAdr = GetClassAdr(dfm->ClassName);
    PInfoRec recN = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;

    //Форма?
    if (SameText(dfm->Name, ControlName))
    {
        //Inherited - начали заполнять меню
        if (dfm->Flags & FF_INHERITED)
        {
            TDfm* parentDfm = ResInfo->GetParentDfm(dfm);
            if (parentDfm) ShowMyPopupMenu(parentDfm->Name, parentDfm->Name, false);
            //В конце добавляем разделитель
            mi = new TMenuItem(evPopup);
            mi->Caption = "-";
            evPopup->Items->Add(mi);
        }

        //Первая строка меню - название контрола
        mi = new TMenuItem(evPopup);
        mi->Caption = dfm->Name + "." + dfm->ClassName;
        mi->Tag = classAdr;
        mi->OnClick = miPopupClick;
        evPopup->Items->Add(mi);
        //Вторая строка меню - разделитель
        mi = new TMenuItem(evPopup);
        mi->Caption = "-";
        evPopup->Items->Add(mi);

        PInfoRec recN = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;

        TList* ev = dfm->Events;
        for (int k = 0; k < ev->Count; k++)
        {
            PEventInfo eInfo = (PEventInfo)ev->Items[k];
            mi = new TMenuItem(evPopup);
            mi->Caption = eInfo->EventName + " = " + eInfo->ProcName;
            String methodName = dfm->ClassName + "." + eInfo->ProcName;
            //Ищем адрес соответствующего метода
            PMethodRec recM = FMain_11011981->GetMethodInfo(recN, methodName);
            mi->Tag = (recM) ? recM->address : 0;
            mi->OnClick = miPopupClick;
            evPopup->Items->Add(mi);
        }
        if (show && evPopup->Items->Count)
        {
            TPoint cursorPos;
            GetCursorPos(&cursorPos);
            evPopup->Popup(cursorPos.x, cursorPos.y);
        }
        return;
    }
    //Компонента формы?
    else
    {
        //Inherited - начали заполнять меню
        if (dfm->Flags & FF_INHERITED)
        {
            TDfm* parentDfm = ResInfo->GetParentDfm(dfm);
            if (parentDfm) ShowMyPopupMenu(parentDfm->Name, ControlName, false);
            //В конце добавляем разделитель
            mi = new TMenuItem(evPopup);
            mi->Caption = "-";
            evPopup->Items->Add(mi);
        }

        for (int m = 0; m < dfm->Components->Count; m++)
        {
            PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
            //Нашли компоненту, которой принадлежит control
            if (SameText(cInfo->Name, ControlName))
            {
                //Чистим evPopup
                evPopup->Items->Clear();
                //Первая строка меню - название контрола
                mi = new TMenuItem(evPopup);
                mi->Caption = cInfo->Name + "." + cInfo->ClassName;
                mi->Tag = GetClassAdr(cInfo->ClassName);
                mi->OnClick = miPopupClick;
                evPopup->Items->Add(mi);
                //Вторая строка меню - разделитель
                mi = new TMenuItem(evPopup);
                mi->Caption = "-";
                evPopup->Items->Add(mi);

                TList* ev = cInfo->Events;
                for (int k = 0; k < ev->Count; k++)
                {
                    PEventInfo eInfo = (PEventInfo)ev->Items[k];
                    mi = new TMenuItem(evPopup);
                    mi->Caption = eInfo->EventName + " = " + eInfo->ProcName;
                    String methodName = "";
                    //Action
                    if (SameText(eInfo->EventName, "Action"))
                    {
                        //Имя eInfo->ProcName может иметь вид FormName.Name
                        int dotpos = eInfo->ProcName.Pos(".");
                        if (dotpos)
                        {
                            String moduleName = eInfo->ProcName.SubString(1, dotpos - 1);
                            String actionName = eInfo->ProcName.SubString(dotpos + 1, eInfo->ProcName.Length() - dotpos);
                            //Ищем форму moduleName
                            for (int j = 0; j < ResInfo->FormList->Count; j++)
                            {
                                TDfm* dfm1 = (TDfm*)ResInfo->FormList->Items[j];
                                if (SameText(dfm1->Name, moduleName))
                                {
                                    classAdr = GetClassAdr(dfm1->ClassName);
                                    recN = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;
                                    //Ищем компоненту actionName
                                    for (int r = 0; r < dfm1->Components->Count; r++)
                                    {
                                        PComponentInfo cInfo1 = (PComponentInfo)dfm1->Components->Items[r];
                                        if (SameText(cInfo1->Name, actionName))
                                        {
                                            //Ищем событие OnExecute
                                            TList* ev1 = cInfo1->Events;
                                            for (int i = 0; i < ev1->Count; i++)
                                            {
                                                PEventInfo eInfo1 = (PEventInfo)ev1->Items[i];
                                                if (SameText(eInfo1->EventName, "OnExecute"))
                                                {
                                                    methodName = dfm1->ClassName + "." + eInfo1->ProcName;
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                        else
                        {
                            //Ищем компоненту eInfo->ProcName
                            for (int r = 0; r < dfm->Components->Count; r++)
                            {
                                PComponentInfo cInfo1 = (PComponentInfo)dfm->Components->Items[r];
                                if (SameText(cInfo1->Name, eInfo->ProcName))
                                {
                                    //Ищем событие OnExecute
                                    TList* ev1 = cInfo1->Events;
                                    for (int i = 0; i < ev1->Count; i++)
                                    {
                                        PEventInfo eInfo1 = (PEventInfo)ev1->Items[i];
                                        if (SameText(eInfo1->EventName, "OnExecute"))
                                        {
                                            methodName = dfm->ClassName + "." + eInfo1->ProcName;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        methodName = dfm->ClassName + "." + eInfo->ProcName;
                    }
                    //Ищем адрес соответствующего метода
                    PMethodRec recM = FMain_11011981->GetMethodInfo(recN, methodName);
                    mi->Tag = (recM) ? recM->address : 0;
                    mi->OnClick = miPopupClick;
                    evPopup->Items->Add(mi);
                }
                if (evPopup->Items->Count)
                {
                    TPoint cursorPos;
                    GetCursorPos(&cursorPos);
                    evPopup->Popup(cursorPos.x, cursorPos.y);
                }
                return;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::miPopupClick(TObject *Sender)
{
    TMenuItem* mi = (TMenuItem*)Sender;
    DWORD Adr = mi->Tag;
    if (Adr && IsValidImageAdr(Adr))
    {
    	PInfoRec recN = GetInfoRec(Adr);
        if (recN)
        {
        	if (recN->kind == ikVMT)
            	FMain_11011981->ShowClassViewer(Adr);
            else
    			FMain_11011981->ShowCode(Adr, 0, -1, -1);
        }
    }
    else
        ShowMessage("Handler not found");
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::SetupControlHint(String FormName, TControl* Control, String InitHint)
{
    for (int n = 0; n < ResInfo->FormList->Count; n++)
    {
    	TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
        //Нашли форму, которой принадлежит control
        if (SameText(dfm->Name, FormName))
        {
        	String hint = InitHint;
            //Сама форма?
            if (SameText(dfm->Name, Control->Name))
            {
                if (hint != "") hint += "\n";
            	hint += dfm->Name + ":" + dfm->ClassName;

            	TList* ev = dfm->Events;
                for (int k = 0; k < ev->Count; k++)
                {
                    PEventInfo eInfo = (PEventInfo)ev->Items[k];
                    hint += "\n" + eInfo->EventName + " = " + eInfo->ProcName;
                }
                Control->Hint = hint;
                Control->ShowHint = true;
                return;
            }
            else
            {
                for (int m = 0; m < dfm->Components->Count; m++)
                {
                    PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
                    //Нашли компоненту, которой принадлежит control
                    if (SameText(cInfo->Name, Control->Name))
                    {
                    	if (hint != "") hint += "\n";
                    	hint += cInfo->Name + ":" + cInfo->ClassName;

                        TList* ev = cInfo->Events;
                        for (int k = 0; k < ev->Count; k++)
                        {
                            PEventInfo eInfo = (PEventInfo)ev->Items[k];
                            hint += "\n" + eInfo->EventName + " = " + eInfo->ProcName;
                        }
                        Control->Hint = hint;
                        Control->ShowHint = true;
                        return;
                    }
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
//find menu item handler (inherited forms supported)
void __fastcall IdrDfmForm::SetupMenuItem(TMenuItem* mi, String searchName)
{
    bool menuFound = false;
    bool formInherited = false;
    TDfm* curDfm = 0;

    for (int n = 0; n < ResInfo->FormList->Count; n++)
    {
        //as: I dont like this, maybe put TDfm* into IdrDfmForm?
    	TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];

        //Нашли форму, которой принадлежит control
        if (SameText(dfm->Name, searchName))
        {
            curDfm = dfm;
            formInherited = dfm->Flags & FF_INHERITED;

            for (int m = 0; m < dfm->Components->Count; m++)
            {
                PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];

                //Нашли компоненту, которой принадлежит mi
                if (SameText(cInfo->Name, mi->Name))
                {
                    DWORD classAdr = GetClassAdr(dfm->ClassName);
                    PInfoRec recN = (IsValidImageAdr(classAdr)) ? Infos[Adr2Pos(classAdr)] : 0;

                    TList* ev = cInfo->Events;
                    for (int k = 0; k < ev->Count; k++)
                    {
                        PEventInfo eInfo = (PEventInfo)ev->Items[k];
                        //OnClick
                        String methodName = ""; PMethodRec recM;
                        if (SameText(eInfo->EventName, "OnClick"))
                        {
                            methodName = dfm->ClassName + "." + eInfo->ProcName;
                            //Ищем адрес соответствующего метода
                            recM = FMain_11011981->GetMethodInfo(recN, methodName);
                            mi->Tag = (recM) ? recM->address : 0;
                            mi->OnClick = miPopupClick;
                            //mi->Enabled = true;
                            continue;
                        }
                        //Action
                        if (SameText(eInfo->EventName, "Action"))
                        {
                            //Ищем компоненту с именем eInfo->ProcName
                            for (int r = 0; r < dfm->Components->Count; r++)
                            {
                                PComponentInfo cInfo1 = (PComponentInfo)dfm->Components->Items[r];
                                if (SameText(cInfo1->Name, eInfo->ProcName))
                                {
                                    //Ищем событие OnExecute
                                    TList* ev1 = cInfo1->Events;
                                    for (int i = 0; i < ev1->Count; i++)
                                    {
                                        PEventInfo eInfo1 = (PEventInfo)ev1->Items[i];
                                        if (SameText(eInfo1->EventName, "OnExecute"))
                                        {
                                            methodName = dfm->ClassName + "." + eInfo1->ProcName;
                                            recM = FMain_11011981->GetMethodInfo(recN, methodName);
                                            mi->Tag = (recM) ? recM->address : 0;
                                            mi->OnClick = miPopupClick;
                                            //mi->Enabled = true;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    menuFound = true;
                    return;
                }
            }
        }
    }

    if (!menuFound && formInherited && curDfm && curDfm->ParentDfm)
    {
        searchName = curDfm->ParentDfm->Name;
        //recursive call in the parent form, trying to find the menu handler...
        SetupMenuItem(mi, searchName);
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::DoFindMethod(String& methodName)
{
    if (FOnFindMethod) FOnFindMethod(this, getFormName(), methodName);
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::CMDialogKey(TCMDialogKey& Message)
{
	//allows for example Ctrl+Tab in TabSheet
    if (Message.CharCode != VK_ESCAPE) TForm::Dispatch(&Message);
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::MyFormShow(TObject *Sender)
{
    if (!frmTree)
    {
        frmTree = new TIdrDfmFormTree_11011981(this);
        int ScrWid = Screen->WorkAreaWidth;
        int ScrHei = Screen->WorkAreaHeight;
        int FrmWid = this->Width;
        int FrmHei = this->Height;
        int TrvWid = frmTree->Width;
        int TrvHei = frmTree->Height;
        int FrmLeft = (ScrWid - FrmWid) / 2;

        if (TrvWid < FrmLeft)
            frmTree->Left = FrmLeft - TrvWid;
        else
            frmTree->Left = 0;

        frmTree->Top = (ScrHei - TrvHei) / 2;
        frmTree->Show();
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::MyFormClose(TObject *Sender, TCloseAction &Action)
{
    for (int n = 0; n < ResInfo->FormList->Count; n++)
    {
        TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
        if (dfm->Open == 2) dfm->Open = 1;
    }

    //notify main window that form is closed (ugly ref to main form - refactor!)
    ::SendMessage(FMain_11011981->Handle, WM_DFMCLOSED, 0, 0);//Post

    Action = caFree;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::MyFormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
    switch (Key)
    {
    case VK_ESCAPE:
        Close();
        break;
    case VK_F11:
        if (frmTree) frmTree->Show();
        break;
    }
    
    Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::MyShortCutEvent(Messages::TWMKey &Msg, bool &Handled)
{
    if (VK_ESCAPE == Msg.CharCode)
    {
        Close();
        Handled = true;
    }
}
//---------------------------------------------------------------------------
String __fastcall IdrDfmForm::getFormName()
{
    return String("T") + originalClassName;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmForm::SetMyHandlers()
{
    OnShow = MyFormShow;
    OnClose = MyFormClose;
    OnKeyDown = MyFormKeyDown;
    OnShortCut = MyShortCutEvent;
}
//---------------------------------------------------------------------------
//IdrDfmLoader
//---------------------------------------------------------------------------
__fastcall IdrDfmLoader::IdrDfmLoader(TComponent* Owner) : TComponent(Owner)
{
    dfmForm = 0;
    Counter = 0;
	Current = 0;
    CompsEventsList = new TCollection(__classid(TComponentEvents));
}
//---------------------------------------------------------------------------
__fastcall IdrDfmLoader::~IdrDfmLoader()
{
    if (CompsEventsList) delete CompsEventsList;
    CompsEventsList = 0;
}
//---------------------------------------------------------------------------
TForm* __fastcall IdrDfmLoader::LoadForm(TStream* dfmStream, TDfm* dfm, bool loadingParent)
{
    dfmForm = 0;
    dfmStream->Seek(0, soFromBeginning);

    if (dfm && dfm->ParentDfm)
    {
        dfm->Loader = new IdrDfmLoader(0);
        dfmForm = (IdrDfmForm*)dfm->Loader->LoadForm(dfm->ParentDfm->MemStream, dfm->ParentDfm, true);
        delete dfm->Loader;
        dfm->Loader = 0;
    }
    else
    {
        dfmForm = new IdrDfmForm(Application, 1);
    }
        
    dfmStream->Seek(0, soFromBeginning);
    TReader* Reader = new IdrDfmReader(dfmStream, 4096);

    Reader->OnAncestorNotFound   = AncestorNotFound;
    Reader->OnFindComponentClass = FindComponentClass;
    Reader->OnCreateComponent    = CreateComponent;
    Reader->OnFindMethod         = FindMethod;
    Reader->OnError              = ReaderError;
    Reader->OnSetName            = SetComponentName;
    Reader->OnReferenceName      = DoReferenceName;

    Current = (TComponentEvents*)(CompsEventsList->Add());
    Current->Component = dfmForm;

    TFilerFlags Flags;
    int ChildPos;

    try
    {
        Reader->ReadSignature();
        Reader->ReadPrefix(Flags, ChildPos);
        dfmForm->originalClassType = Reader->ReadStr();
        dfmForm->originalClassName = Reader->ReadStr();
        Reader->Position = 0;

        Reader->ReadRootComponent(dfmForm);

        dfmForm->Enabled = true;
        dfmForm->Visible = false;
        dfmForm->Position = poScreenCenter;
        dfmForm->compsEventsList = CompsEventsList;
        dfmForm->OnFindMethod = OnFindMethod;
        dfmForm->SetMyHandlers();

        dfmForm->SetDesigning(false, false);
        if (!loadingParent) dfmForm->SetupControls();

        if (dfmForm->AlphaBlend) dfmForm->AlphaBlendValue = 222;

        dfmForm->AutoSize = false;
        dfmForm->WindowState = wsNormal;
        dfmForm->FormStyle = fsStayOnTop;
        dfmForm->FormState.Clear();

        dfmForm->Constraints->MinWidth = 0;
        dfmForm->Constraints->MinHeight = 0;
        dfmForm->Constraints->MaxWidth = 0;
        dfmForm->Constraints->MaxHeight = 0;

        dfmForm->HelpFile = "";
        CompsEventsList = 0;
    }
    catch(Exception &e)
    {
        ShowMessage("DFM Load Error\r\n" + e.ClassName() +": " + e.Message);
        dfmForm = 0;
    }

    delete Reader;
    return dfmForm;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::AncestorNotFound(TReader* Reader, String ComponentName, TMetaClass* ComponentClass, TComponent* &Component)
{
    String s = ComponentClass->ClassName();
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::FindComponentClass(TReader* Reader, String ClassName, TMetaClass* &ComponentClass)
{
    try
    {
        ComponentClass = FindClass(ClassName);
    }
    catch (Exception& e)
    {
        lastClassAliasName = ClassName;

        String alias = ResInfo->GetAlias(ClassName);
        for (int n = 0;;n++)
        {
            if (!RegClasses[n].RegClass) break;
            if (SameText(alias, RegClasses[n].ClassName))
            {
                ComponentClass = RegClasses[n].RegClass;
                break;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::CreateComponent(TReader* Reader, TMetaClass* ComponentClass, TComponent* &Component)
{
    Current = 0;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::FindMethod(TReader* Reader, String MethodName, void* &Address, bool &Error)
{
    if (Current)
    {
        IdrDfmReader* r = dynamic_cast<IdrDfmReader*>(Reader);
        if (r) Current->FoundEvents->Add(r->PropName + "=" + MethodName);
    }

    Error = false;
    Address = 0;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::ReaderError(TReader* Reader, String Message, bool &Handled)
{
    //if something really wrong happened, in order to fix the
    //"DFM Stream read error" - skip fault value!
    if (AnsiEndsStr("Invalid property value", Message)
        || AnsiEndsStr("Unable to insert a line", Message)
        || AnsiContainsText(Message, "List index out of bounds")
       )
    {
        //move back to a type position byte
        Reader->Position -= 1;
        //skip the unknown value for known property (eg: Top = 11.453, or bad Items for StringList)
        Reader->SkipValue();
    }
    
    Handled = true;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::SetComponentName(TReader* Reader, TComponent* Component, String &Name)
{
    if (Name == "")
    {
        Name = "_" + Val2Str4(Counter) + "_";
        Counter++;
    }

    if (Current && Current->Component == Component) return;

    IdrDfmDefaultControl* rc = dynamic_cast<IdrDfmDefaultControl*>(Component);
    if (rc)
    {
        String mappedClassName;
        try
        {
            mappedClassName =
            //IdrDfmClassNameAnalyzer::GetInstance().GetMapping(lastClassAliasName);
            ResInfo->GetAlias(lastClassAliasName);
        }
        catch(...)
        {
        }

        rc->SetClassName(lastClassAliasName, mappedClassName);
    }

    Current = (TComponentEvents*)(CompsEventsList->Add());
    Current->Component = Component;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmLoader::DoReferenceName(TReader* Reader, String &Name)
{
    if(dfmForm)
    {
        TComponent* Component = dfmForm->FindComponent(Name);
        if(Component)
        {
            IdrDfmDefaultControl* rc = dynamic_cast<IdrDfmDefaultControl*>(Component);
            if (rc)
            {
                Name = "";
            }
        }
    }
}
//---------------------------------------------------------------------------
bool __fastcall IdrDfmLoader::HasGlyph(String ClassName)
{
    for (int n = 0; n < ResInfo->FormList->Count; n++)
    {
    	TDfm* dfm = (TDfm*)ResInfo->FormList->Items[n];
        for (int m = 0; m < dfm->Components->Count; m++)
        {
            PComponentInfo cInfo = (PComponentInfo)dfm->Components->Items[m];
            if (SameText(cInfo->ClassName, ClassName)) return cInfo->HasGlyph;
        }
    }
    return false;
}
//---------------------------------------------------------------------------
//IdrDfmDefaultControl
//---------------------------------------------------------------------------
__fastcall IdrDfmDefaultControl::IdrDfmDefaultControl(TObject* Owner) : TPanel(Owner)
{
    Color = clNone;
    imgIcon = 0;
}
//---------------------------------------------------------------------------
bool IdrDfmDefaultControl::IsVisible()
{
    return (Width > 0 && Height > 0);
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmDefaultControl::ReadState(TReader* Reader)
{
    DisableAlign();

    try{
        Width = 24;
        Height = 24;
        TPanel::ReadState(Reader);
    }
    __finally
    //catch(Exception& e)
    {
        EnableAlign();
    }

    /* wow, intersting case seen, D7:

        object WebDispatcher1: TWebDispatcher
          OldCreateOrder = False
          Actions = <>
          Left = 888
          Top = 8
          Height = 0
          Width = 0
        end

        we are not happy with 0:0 sizes, lets correct
    */

    if (Width <= 0)
        Width = 24;
    if (Height <= 0)
        Height = 24;        
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmDefaultControl::Loaded()
{
    TPanel::Loaded();

    BorderStyle = bsNone;
    if (Color == clNone) Color = clBtnFace;
}
//---------------------------------------------------------------------------
void __fastcall IdrDfmDefaultControl::Paint()
{
    Canvas->Brush->Color = Color;
    Canvas->Brush->Style = bsClear;
    Canvas->Pen->Color = clGray;
    Canvas->Pen->Style = psDot;
    Canvas->Rectangle(GetClientRect());
}
//---------------------------------------------------------------------------
void IdrDfmDefaultControl::SetClassName(const String& name, const String& mappedName)
{
    originalClassName = name;
    //as note:
    //here we should have mapped class, eg:
    //we have on form TMyOpenDialog component, it was mapped to TOpenDialog,
    //so I expect to have a string "TOpenDialog in mappedName, not "Default" !
    
    mappedClassName = name;

    String usedClassName = (mappedClassName.IsEmpty()) ? originalClassName : mappedClassName;

    if (HasIconForClass(usedClassName))
    {
        CreateImageIconForClass(usedClassName);
    }
}
//---------------------------------------------------------------------------
bool IdrDfmDefaultControl::HasIconForClass(const String& name)
{
    return true;
    //TBD: ask dll if it has specified icon for class
    //right now it is implicitly done in CreateImageIconForClass()
}
//---------------------------------------------------------------------------
void IdrDfmDefaultControl::CreateImageIconForClass(const String& imgFile)
{
    static HINSTANCE hResLib = 0;
    static bool IconsDllLoaded = false;
    static bool IconsDllPresent = true;
    if (!IconsDllPresent)
        return;

    if (!IconsDllLoaded)
    {
        hResLib = LoadLibrary("Icons.dll");
        IconsDllLoaded = IconsDllPresent = (hResLib != 0);
        if (!IconsDllPresent)
            return;
    }

    imgIcon = new TImage(this);
    //old way - read from .bmp files on disk
    //imgIcon->Picture->LoadFromFile(imgFile);
    try{
        imgIcon->Picture->Bitmap->LoadFromResourceName((int)hResLib, imgFile.UpperCase());
        imgIcon->AutoSize = true;
        imgIcon->Transparent = true;
        imgIcon->Parent = this;

        //yes, we want same popup as our parent
        //imgIcon->OnMouseDown = ImageMouseDown;

        //update our component size in a case picture has another size
        if (Width != imgIcon->Picture->Bitmap->Width)
            Width = imgIcon->Picture->Bitmap->Width;
        if (Height != imgIcon->Picture->Bitmap->Height)
            Height = imgIcon->Picture->Bitmap->Height;
        
    }catch(Exception& e){
        //icon not found or other error - free the image!
        imgIcon->Parent = 0;
        delete imgIcon;
        imgIcon = 0;
    }
}

//---------------------------------------------------------------------------
void __fastcall IdrDfmDefaultControl::ImageMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (Button == mbRight)
    {
        //Sender here is image, but we need this - replacing control!
        if (Parent)
            ((TMyControl*)Parent)->OnMouseDown(this, Button, Shift, X, Y);
    }
}
//---------------------------------------------------------------------------
//IdrImageControl
//---------------------------------------------------------------------------
__fastcall IdrImageControl::IdrImageControl(TComponent* Owner) : TImage(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall IdrImageControl::Paint()
{
}
//---------------------------------------------------------------------------

