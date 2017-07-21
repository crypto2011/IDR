//---------------------------------------------------------------------------

#ifndef ExplorerH
#define ExplorerH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
#define     DEFINE_AS_CODE      1
#define     DEFINE_AS_STRING    2
#define     UNDEFINE            3

class TFExplorer_11011981 : public TForm
{
__published:	// IDE-managed Components
    TPageControl *pc1;
	TTabSheet *tsCode;
    TTabSheet *tsData;
    TListBox *lbCode;
    TListBox *lbData;
    TTabSheet *tsString;
    TRadioGroup *rgStringViewStyle;
    TTabSheet *tsText;
    TListBox *lbText;
    TPopupMenu *pm1;
    TMenuItem *miCopy2Clipboard;
    TPanel *Panel2;
    TRadioGroup *rgDataViewStyle;
    TMemo *lbString;
    TPanel *Panel3;
    TButton *btnDefCode;
    TButton *btnUndefCode;
    TPanel *Panel1;
    TButton *btnDefString;
    TButton *btnUndefString;
    void __fastcall btnDefCodeClick(TObject *Sender);
    void __fastcall btnUndefCodeClick(TObject *Sender);
    void __fastcall rgStringViewStyleClick(TObject *Sender);
    void __fastcall miCopy2ClipboardClick(TObject *Sender);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall rgDataViewStyleClick(TObject *Sender);
    void __fastcall btnDefStringClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
    DWORD   Adr;    //Адрес, с которого показывается информация
public:		// User declarations
    __fastcall TFExplorer_11011981(TComponent* Owner);
    void __fastcall ShowCode(DWORD fromAdr, int maxBytes);
    void __fastcall ShowData(DWORD fromAdr, int maxBytes);
    void __fastcall ShowString(DWORD fromAdr, int maxbytes);
    void __fastcall ShowArray(DWORD fromAdr);
    int    	DefineAs;   //1- Define as Code, 2 - Undefine
    int		WAlign;		//Alignment for WideString visualization
};
//---------------------------------------------------------------------------
extern PACKAGE TFExplorer_11011981 *FExplorer_11011981;
//---------------------------------------------------------------------------
#endif
