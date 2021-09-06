//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "AboutDlg.h"
#include "Misc.h"
extern	String	IDRVersion;
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TFAboutDlg_11011981 *FAboutDlg_11011981;
//---------------------------------------------------------------------
__fastcall TFAboutDlg_11011981::TFAboutDlg_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------
void __fastcall TFAboutDlg_11011981::FormCreate(TObject *Sender)
{
	lVer->Caption = "Version: " + IDRVersion;
    ScaleForm(this);
}
//---------------------------------------------------------------------------
