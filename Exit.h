//----------------------------------------------------------------------------
#ifndef ExitH
#define ExitH
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
class TFExit_11011981 : public TForm
{
__published:        
	TButton *OKBtn;
	TButton *CancelBtn;
	TBevel *Bevel1;
    TCheckBox *cbDontSaveProject;
    void __fastcall OKBtnClick(TObject *Sender);
    void __fastcall CancelBtnClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
private:
public:
	virtual __fastcall TFExit_11011981(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TFExit_11011981 *FExit_11011981;
//----------------------------------------------------------------------------
#endif    
