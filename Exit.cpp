//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Exit.h"
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TFExit_11011981 *FExit_11011981;
//---------------------------------------------------------------------
__fastcall TFExit_11011981::TFExit_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------
void __fastcall TFExit_11011981::OKBtnClick(TObject *Sender)
{
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFExit_11011981::CancelBtnClick(TObject *Sender)
{
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFExit_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

