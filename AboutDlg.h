//----------------------------------------------------------------------------
#ifndef AboutDlgH
#define AboutDlgH
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
#include <jpeg.hpp>
//----------------------------------------------------------------------------
class TFAboutDlg_11011981 : public TForm
{
__published:
    TPanel *Panel3;
    TButton *Button2;
    TPageControl *PageControl1;
    TTabSheet *tsIDR;
    TTabSheet *tsCrypto;
    TImage *Icon;
    TLabel *lProduct;
    TShape *Shape1;
    TShape *Shape2;
    TLabel *lVer;
    TLabel *lEmail;
    TLabel *lWWW;
    TLabel *Label1;
    TLabel *Label2;
    TLabel *Label3;
    TLabel *Label4;
    TLabel *Label5;
    TLabel *Label6;
    TImage *Image1;
    TBitBtn *bDonate;
    TLabel *Label7;
    TLabel *Label8;
    TLabel *lblHint;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall lEmailClick(TObject *Sender);
	void __fastcall lWWWClick(TObject *Sender);
    void __fastcall bDonateClick(TObject *Sender);
private:
public:
	virtual __fastcall TFAboutDlg_11011981(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TFAboutDlg_11011981 *FAboutDlg_11011981;
//----------------------------------------------------------------------------
#endif    
