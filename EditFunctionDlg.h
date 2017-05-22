//----------------------------------------------------------------------------
#ifndef EditFunctionDlgH
#define EditFunctionDlgH
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
#include <Grids.hpp>
#include <ComCtrls.hpp>
#include "Main.h"
//----------------------------------------------------------------------------
class TFEditFunctionDlg_11011981 : public TForm
{
__published:
    TPanel *Panel1;
    TButton *bEdit;
    TButton *bAdd;
    TButton *bRemoveSelected;
    TPageControl *pc;
    TTabSheet *tsArgs;
    TListBox *lbArgs;
    TTabSheet *tsVars;
    TListBox *lbVars;
    TPanel *pnlVars;
    TRadioGroup *rgLocBase;
    TLabeledEdit *edtVarOfs;
    TLabeledEdit *edtVarSize;
    TLabeledEdit *edtVarName;
    TLabeledEdit *edtVarType;
    TButton *bApplyVar;
    TButton *bCancelVar;
    TTabSheet *tsType;
    TCheckBox *cbEmbedded;
    TMemo *mType;
    TRadioGroup *rgCallKind;
    TButton *bApplyType;
    TButton *bCancelType;
    TRadioGroup *rgFunctionKind;
    TButton *bOk;
    TComboBox *cbVmtCandidates;
    TCheckBox *cbMethod;
    TLabel *Label1;
    TLabel *lRetBytes;
    TLabel *Label2;
    TLabel *lArgsBytes;
    TLabeledEdit *lEndAdr;
    TLabeledEdit *lStackSize;
    TButton *bRemoveAll;
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall bEditClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall pcChange(TObject *Sender);
    void __fastcall lbVarsClick(TObject *Sender);
    void __fastcall bCancelVarClick(TObject *Sender);
    void __fastcall bApplyVarClick(TObject *Sender);
    void __fastcall bRemoveSelectedClick(TObject *Sender);
    void __fastcall bAddClick(TObject *Sender);
    void __fastcall bApplyTypeClick(TObject *Sender);
    void __fastcall bCancelTypeClick(TObject *Sender);
    void __fastcall bOkClick(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall cbMethodClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall bRemoveAllClick(TObject *Sender);
private:
    bool    TypModified;
    bool    VarModified;
    int     ArgEdited;
    int     VmtCandidatesNum;
    int     StackSize;
    DWORD   SFlags;
    String  SName;
    void __fastcall FillVMTCandidates();
    void __fastcall FillType();
    void __fastcall FillArgs();
    void __fastcall FillVars();
public:
    DWORD   Adr;
    DWORD   EndAdr;
	virtual __fastcall TFEditFunctionDlg_11011981(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TFEditFunctionDlg_11011981 *FEditFunctionDlg_11011981;
//----------------------------------------------------------------------------
#endif    
