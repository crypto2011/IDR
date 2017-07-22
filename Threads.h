//---------------------------------------------------------------------------
#ifndef ThreadsH
#define ThreadsH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include "Main.h"
#include "ProgressBar.h"
//---------------------------------------------------------------------------
enum ThreadAnalysisOperation
{
    taStartPrBar, taUpdatePrBar, taUpdateStBar,
    taUpdateUnits, taUpdateRTTIs, taUpdateVmtList, taUpdateStrings, taUpdateCode, taUpdateXrefs,
    taUpdateShortClassViewer, taUpdateClassViewer, taUpdateBeforeClassViewer,
    taFinished
};
struct ThreadAnalysisData
{
    ThreadAnalysisData(int steps, const String& txt)
        :pbSteps(steps), sbText(txt) {}
    int pbSteps;
    String sbText;
};

typedef struct
{
    int     height;
    DWORD   vmtAdr;
} CVPair, *PCVPair;
//---------------------------------------------------------------------------
#define LAST_ANALYZE_STEP   19
class TAnalyzeThread : public TThread
{
private:
    TFMain_11011981 *mainForm;
    TFProgressBar   *pbForm;
    int             adrCnt;

    int __fastcall StartProgress(int pbMaxCount, const String& sbText);
    void __fastcall UpdateProgress();
    void __fastcall StopProgress();
    void __fastcall UpdateStatusBar(int adr);
    void __fastcall UpdateStatusBar(const String& sbText);
    void __fastcall UpdateAddrInStatusBar(DWORD adr);
    void __fastcall UpdateUnits();
    void __fastcall UpdateRTTIs();
    void __fastcall UpdateVmtList();
    void __fastcall UpdateStrings();
    void __fastcall UpdateCode();
    void __fastcall UpdateXrefs();
    void __fastcall UpdateShortClassViewer();
    void __fastcall UpdateClassViewer();
    void __fastcall UpdateBeforeClassViewer();
    void __fastcall StrapSysProcs();
    void __fastcall FindRTTIs();
    void __fastcall FindVMTs2();                //Для версии Дельфи2 (другая структура!)
    void __fastcall FindVMTs();
    void __fastcall FindTypeFields();
    String __fastcall FindEvent(DWORD VmtAdr, String Name);
    void __fastcall FindPrototypes();
    void __fastcall StrapVMTs();
    void __fastcall ScanCode();
    void __fastcall ScanCode1();
    void __fastcall ScanVMTs();
    void __fastcall ScanConsts();
    void __fastcall ScanGetSetStoredProcs();
    void __fastcall FindStrings();
    void __fastcall AnalyzeCode1();
    void __fastcall AnalyzeCode2(bool args);
    void __fastcall AnalyzeCode3();
    void __fastcall PropagateClassProps();
    void __fastcall FillClassViewer();
    void __fastcall AnalyzeDC();

    void __fastcall ClearPassFlags();

protected:
    void __fastcall Execute();
public:
    __fastcall TAnalyzeThread(TFMain_11011981* AForm, TFProgressBar* ApbForm, bool AllValues);
    __fastcall ~TAnalyzeThread();
    int __fastcall GetRetVal();
    bool            all;    //if false, only ClassViewer
};
//---------------------------------------------------------------------------
#endif
