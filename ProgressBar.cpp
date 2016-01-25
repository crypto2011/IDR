//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "ProgressBar.h"
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
