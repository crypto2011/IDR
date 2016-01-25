//----------------------------------------------------------------------------
#ifndef Hex2DoubleH
#define Hex2DoubleH
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
#include "Misc.h"
//----------------------------------------------------------------------------
class TFHex2DoubleDlg_11011981 : public TForm
{
__published:
    TRadioGroup *rgDataViewStyle;
    TLabeledEdit *edtValue;
    void __fastcall FormShow(TObject *Sender);
    void __fastcall edtValueEnter(TObject *Sender);
    void __fastcall rgDataViewStyleClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
private:
    int     PrevIdx;
    BYTE    BinData[16];
    void __fastcall Str2Binary(String AStr);
    String __fastcall Binary2Str(int BytesNum);
public:
	virtual __fastcall TFHex2DoubleDlg_11011981(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TFHex2DoubleDlg_11011981 *FHex2DoubleDlg_11011981;
//----------------------------------------------------------------------------
#endif    
