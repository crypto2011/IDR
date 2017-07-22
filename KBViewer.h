//---------------------------------------------------------------------------

#ifndef KBViewerH
#define KBViewerH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TFKBViewer_11011981 : public TForm
{
__published:	// IDE-managed Components
    TListBox *lbKB;
    TListBox *lbIDR;
    TPanel *Panel3;
    TButton *bPrev;
    TButton *bNext;
    TButton *btnOk;
    TButton *btnCancel;
    TLabel *lPosition;
    TSplitter *Splitter1;
    TEdit *edtCurrIdx;
    TLabel *lKBIdx;
    TPanel *Panel1;
    TComboBox *cbUnits;
    TLabel *Label1;
    TLabel *lblKbIdxs;
    void __fastcall bPrevClick(TObject *Sender);
    void __fastcall bNextClick(TObject *Sender);
    void __fastcall btnOkClick(TObject *Sender);
    void __fastcall btnCancelClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall edtCurrIdxChange(TObject *Sender);
    void __fastcall cbUnitsChange(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
private:	// User declarations
    int     UnitsNum;
    int     CurrIdx;
    DWORD   CurrAdr;
public:		// User declarations
    int     Position;
    DWORD   FromIdx;//First Unit idx
    DWORD   ToIdx;  //Last Unit idx
    __fastcall TFKBViewer_11011981(TComponent* Owner);
    void __fastcall ShowCode(DWORD adr, int idx);
};
//---------------------------------------------------------------------------
extern PACKAGE TFKBViewer_11011981 *FKBViewer_11011981;
//---------------------------------------------------------------------------
#endif
