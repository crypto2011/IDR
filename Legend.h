//---------------------------------------------------------------------------
#ifndef LegendH
#define LegendH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TFLegend_11011981 : public TForm
{
__published:	// IDE-managed Components
    TGroupBox *gb1;
    TLabel *Label1;
    TLabel *Label2;
    TLabel *Label3;
    TLabel *Label5;
    TLabel *lblUnitStd;
    TLabel *lblUnitUser;
    TLabel *lblUnitTrivial;
    TLabel *lblUnitUserUnk;
    TGroupBox *gb2;
    TLabel *Label4;
    TLabel *Label6;
    TLabel *Label7;
    TLabel *lblInit;
    TLabel *lblFin;
    TLabel *lblUnk;
    TButton *btnOK;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TFLegend_11011981(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFLegend_11011981 *FLegend_11011981;
//---------------------------------------------------------------------------
#endif
