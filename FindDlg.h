//----------------------------------------------------------------------------
#ifndef FindDlgH
#define FindDlgH
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
class TFindDlg_11011981 : public TForm
{
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
//----------------------------------------------------------------------------
extern PACKAGE TFindDlg_11011981 *FindDlg_11011981;
//----------------------------------------------------------------------------
#endif    
