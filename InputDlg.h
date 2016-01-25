//----------------------------------------------------------------------------
#ifndef InputDlgH
#define InputDlgH
//----------------------------------------------------------------------------
#include <vcl\System.hpp>
#include <vcl\Windows.hpp>
#include <vcl\SysUtils.hpp>
#include <vcl\Classes.hpp>
#include <vcl\Graphics.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Controls.hpp>
#include <vcl\Buttons.hpp>
#include <vcl\ExtCtrls.hpp>
//----------------------------------------------------------------------------
class TFInputDlg_11011981 : public TForm
{
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
//----------------------------------------------------------------------------
extern PACKAGE TFInputDlg_11011981 *FInputDlg_11011981;
//----------------------------------------------------------------------------
#endif    
