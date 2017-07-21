//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "FindDlg.h"
#include "Misc.h"
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TFindDlg_11011981 *FindDlg_11011981;
//---------------------------------------------------------------------
__fastcall TFindDlg_11011981::TFindDlg_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------
void __fastcall TFindDlg_11011981::FormShow(TObject *Sender)
{
    if (cbText->Items->Count) cbText->Text = cbText->Items->Strings[0];
    ActiveControl = cbText;    
}
//---------------------------------------------------------------------------
void __fastcall TFindDlg_11011981::cbTextEnter(TObject *Sender)
{
	cbText->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TFindDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

