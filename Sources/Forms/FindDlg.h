// ----------------------------------------------------------------------------
#ifndef FindDlgH
#define FindDlgH
// ----------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>

// ----------------------------------------------------------------------------
class TFindDlg_11011981 : public TForm {
__published:
	TButton *OKBtn;
	TButton *CancelBtn;
	TBevel *Bevel1;
	TComboBox *cbText;
	TLabel *Label1;

	void __fastcall FormShow(TObject *Sender);
	void __fastcall cbTextEnter(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);

private:
public:
	virtual __fastcall TFindDlg_11011981(TComponent* AOwner);
};

// ----------------------------------------------------------------------------
extern PACKAGE TFindDlg_11011981 *FindDlg_11011981;
// ----------------------------------------------------------------------------
#endif
