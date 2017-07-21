//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "IdcSplitSize.h"
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
extern  int     SplitSize;

TFIdcSplitSize *FIdcSplitSize;
//---------------------------------------------------------------------
__fastcall TFIdcSplitSize::TFIdcSplitSize(TComponent* AOwner)
	: TForm(AOwner)
{
    Caption = "Split size: 1 Mbyte";
}
//---------------------------------------------------------------------
void __fastcall TFIdcSplitSize::OKBtnClick(TObject *Sender)
{
    SplitSize = (1 << (tbSplitSize->Position + 19));//MBytes
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFIdcSplitSize::CancelBtnClick(TObject *Sender)
{
    SplitSize = 0;
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFIdcSplitSize::tbSplitSizeChange(TObject *Sender)
{
    Caption = "Split size: " + String(tbSplitSize->Position) + " MByte";
}
//---------------------------------------------------------------------------

