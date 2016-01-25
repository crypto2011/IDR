//---------------------------------------------------------------------------

#ifndef ProgressBarH
#define ProgressBarH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFProgressBar : public TForm
{
__published:	// IDE-managed Components
    TProgressBar *pb;
private:	// User declarations
public:		// User declarations
    __fastcall TFProgressBar(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFProgressBar *FProgressBar;
//---------------------------------------------------------------------------
#endif
