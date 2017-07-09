// ----------------------------------------------------------------------------
#ifndef InputDlgH
#define InputDlgH
// ----------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>

// ----------------------------------------------------------------------------
class TFInputDlg_11011981 : public TForm {
__published:
	TButton *OKBtn;
	TButton *CancelBtn;
	TLabeledEdit *edtName;

	void __fastcall FormShow(TObject *Sender);
	void __fastcall edtNameEnter(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);

private:
public:
	virtual __fastcall TFInputDlg_11011981(TComponent* AOwner);
};

// ----------------------------------------------------------------------------
extern PACKAGE TFInputDlg_11011981 *FInputDlg_11011981;
// ----------------------------------------------------------------------------
#endif
