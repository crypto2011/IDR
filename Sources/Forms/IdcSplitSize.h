// ----------------------------------------------------------------------------
#ifndef IdcSplitSizeH
#define IdcSplitSizeH
// ----------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>

// ----------------------------------------------------------------------------
class TFIdcSplitSize : public TForm {
__published:
	TButton *OKBtn;
	TButton *CancelBtn;
	TBevel *Bevel1;
	TTrackBar *tbSplitSize;

	void __fastcall OKBtnClick(TObject *Sender);
	void __fastcall CancelBtnClick(TObject *Sender);
	void __fastcall tbSplitSizeChange(TObject *Sender);

private:
public:
	virtual __fastcall TFIdcSplitSize(TComponent* AOwner);
};

// ----------------------------------------------------------------------------
extern PACKAGE TFIdcSplitSize *FIdcSplitSize;
// ----------------------------------------------------------------------------
#endif
