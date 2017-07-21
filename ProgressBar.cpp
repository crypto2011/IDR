//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ProgressBar.h"
#include "Threads.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFProgressBar *FProgressBar;
//---------------------------------------------------------------------------
__fastcall TFProgressBar::TFProgressBar(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFProgressBar::FormShow(TObject *Sender)
{
    pb->Position = 0;
    sb->Panels->Items[0]->Text = "";
    sb->Panels->Items[1]->Text = "";
}
//---------------------------------------------------------------------------
void __fastcall TFProgressBar::StartProgress(String text0, String text1, int steps)
{
    pb->Min = 0;
    pb->Max = steps;
    pb->Step = 1;
    pb->Position = 0;
    pb->Update();
    sb->Panels->Items[0]->Text = text0;
    sb->Panels->Items[1]->Text = text1;
    sb->Update();
}
//---------------------------------------------------------------------------
void __fastcall TFProgressBar::wm_updAnalysisStatus(TMessage& msg)
{
    if (taStartPrBar == msg.WParam)
    {
        const ThreadAnalysisData* startOperation = (ThreadAnalysisData*)msg.LParam;
        FProgressBar->StartProgress(startOperation ? startOperation->sbText : String(""), "", startOperation ? startOperation->pbSteps : 0);
        if (startOperation) delete startOperation;
    }
    else if (taUpdatePrBar == msg.WParam)
    {
        FProgressBar->pb->StepIt();
        FProgressBar->pb->Invalidate();
    }
    else if (taUpdateStBar == msg.WParam)
    {
        const ThreadAnalysisData* updStatusBar = (ThreadAnalysisData*)msg.LParam;
        FProgressBar->sb->Panels->Items[1]->Text = updStatusBar ? updStatusBar->sbText : String("?");
        FProgressBar->sb->Invalidate();

        if (updStatusBar) delete updStatusBar;
        Application->ProcessMessages();
    }
}
//---------------------------------------------------------------------------

