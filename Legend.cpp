//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Legend.h"
#include "Misc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFLegend_11011981 *FLegend_11011981;
//---------------------------------------------------------------------------
__fastcall TFLegend_11011981::TFLegend_11011981(TComponent* Owner)
    : TForm(Owner)
{
    lblUnitStd->Font->Color     = (TColor)0xC08000; //blue
    lblUnitUser->Font->Color    = (TColor)0x00B000; //green
    lblUnitTrivial->Font->Color = (TColor)0x0000B0; //brown
    lblUnitUserUnk->Font->Color = (TColor)0x8080FF; //red
}
//---------------------------------------------------------------------------
void __fastcall TFLegend_11011981::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (VK_ESCAPE == Key) Close();    
}
//---------------------------------------------------------------------------
void __fastcall TFLegend_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

