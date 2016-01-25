//----------------------------------------------------------------------------
#ifndef EditFieldsDlgH
#define EditFieldsDlgH
//----------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Windows.hpp>
#include <Messages.hpp>
#include <Classes.hpp>
#include <Graphics.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//----------------------------------------------------------------------------
#define     FD_OP_EDIT      0
#define     FD_OP_ADD       1
#define     FD_OP_DELETE    2

class TFEditFieldsDlg_11011981 : public TForm
{
__published:
    TListBox *lbFXrefs;
    TListBox *lbFields;
	TPanel *Panel1;
	TButton *bAdd;
	TButton *bRemove;
	TPanel *edtPanel;
	TLabeledEdit *edtName;
	TLabeledEdit *edtType;
	TButton *bApply;
	TButton *bClose;
	TLabeledEdit *edtOffset;
	TButton *bEdit;
	void __fastcall FormShow(TObject *Sender);
    void __fastcall lbFXrefsDblClick(TObject *Sender);
    void __fastcall lbFieldsClick(TObject *Sender);
	void __fastcall edtNameChange(TObject *Sender);
	void __fastcall edtTypeChange(TObject *Sender);
	void __fastcall bCloseClick(TObject *Sender);
	void __fastcall bApplyClick(TObject *Sender);
	void __fastcall bEditClick(TObject *Sender);
    void __fastcall bRemoveClick(TObject *Sender);
    void __fastcall bAddClick(TObject *Sender);
    void __fastcall FormCreate(TObject *Sender);
private:
	void __fastcall ShowFields();
public:
    BYTE    Op;
    int     SelIndex;
    int     FieldOffset;
	DWORD	VmtAdr;
    TList	*fieldsList;
	virtual __fastcall TFEditFieldsDlg_11011981(TComponent *Owner);
    virtual __fastcall ~TFEditFieldsDlg_11011981();
};
//----------------------------------------------------------------------------
extern TFEditFieldsDlg_11011981 *FEditFieldsDlg_11011981;
//----------------------------------------------------------------------------
#endif
