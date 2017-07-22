//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "StringInfo.h"
#include "Misc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TFStringInfo_11011981 *FStringInfo_11011981;
//---------------------------------------------------------------------------
__fastcall TFStringInfo_11011981::TFStringInfo_11011981(TComponent* Owner)
    : TForm(Owner)
{
}
//---------------------------------------------------------------------------
void __fastcall TFStringInfo_11011981::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key == VK_ESCAPE) ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFStringInfo_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

