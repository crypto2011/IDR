//---------------------------------------------------------------------------
#ifndef StringInfoH
#define StringInfoH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TFStringInfo_11011981 : public TForm
{
__published:	// IDE-managed Components
	TMemo *memStringInfo;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TFStringInfo_11011981(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFStringInfo_11011981 *FStringInfo_11011981;
//---------------------------------------------------------------------------
#endif
