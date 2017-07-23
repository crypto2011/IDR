//----------------------------------------------------------------------------
#ifndef IdcSplitSizeH
#define IdcSplitSizeH
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
#include <ComCtrls.hpp>
//----------------------------------------------------------------------------
class TFIdcSplitSize : public TForm
{
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
//----------------------------------------------------------------------------
extern PACKAGE TFIdcSplitSize *FIdcSplitSize;
//----------------------------------------------------------------------------
#endif    
