//---------------------------------------------------------------------------
#ifndef TypeInfoH
#define TypeInfoH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "KnowledgeBase.h"
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
String __fastcall Guid2String(BYTE* Guid);
//---------------------------------------------------------------------------
class TFTypeInfo_11011981 : public TForm
{
__published:	// IDE-managed Components
    TMemo *memDescription;
    TPanel *Panel1;
    TButton *bSave;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall bSaveClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
    int     RTTIKind;
    DWORD   RTTIAdr;
    String  RTTIName;
public:		// User declarations
	__fastcall TFTypeInfo_11011981(TComponent* Owner);
    void __fastcall ShowKbInfo(MTypeInfo* tInfo);
    String __fastcall GetRTTI(DWORD adr);
    void __fastcall ShowRTTI(DWORD adr);
};
//---------------------------------------------------------------------------
extern PACKAGE TFTypeInfo_11011981 *FTypeInfo_11011981;
//---------------------------------------------------------------------------
#endif
