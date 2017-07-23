//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "InputDlg.h"
#include "Misc.h"
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TFInputDlg_11011981 *FInputDlg_11011981;
//---------------------------------------------------------------------
__fastcall TFInputDlg_11011981::TFInputDlg_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------
void __fastcall TFInputDlg_11011981::FormShow(TObject *Sender)
{
    if (edtName->CanFocus()) ActiveControl = edtName;
}
//---------------------------------------------------------------------------
void __fastcall TFInputDlg_11011981::edtNameEnter(TObject *Sender)
{
 	edtName->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TFInputDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

