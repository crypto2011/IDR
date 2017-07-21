//----------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Misc.h"
#include "EditFieldsDlg.h"
#include "Main.h"
//----------------------------------------------------------------------------
#pragma resource "*.dfm"

extern  bool        ProjectModified;
extern  DWORD       CurProcAdr;
extern  DWORD   	CodeBase;
extern  DWORD   	*Flags;
extern	PInfoRec    *Infos;

TFEditFieldsDlg_11011981 *FEditFieldsDlg_11011981;
//----------------------------------------------------------------------------
__fastcall TFEditFieldsDlg_11011981::TFEditFieldsDlg_11011981(TComponent *Owner)
	: TForm(Owner)
{
    Op = FD_OP_EDIT;
    SelIndex = -1;
    FieldOffset = -1;
	VmtAdr = 0;
    fieldsList = new TList;
}
//----------------------------------------------------------------------------
__fastcall TFEditFieldsDlg_11011981::~TFEditFieldsDlg_11011981()
{
	if (fieldsList) delete fieldsList;
}
//----------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::ShowFields()
{
	lbFields->Enabled = true;
	lbFields->Clear();
	fieldsList->Clear();
    int fieldsNum = FMain_11011981->LoadFieldTable(VmtAdr, fieldsList);
    if (fieldsNum)
    {
        SelIndex = -1;
    	for (int n = 0; n < fieldsNum; n++)
        {
        	PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[n];
            String line = Val2Str5(fInfo->Offset) + " ";
            if (fInfo->Name != "")
            	line += fInfo->Name;
            else
            	line += "?";
            line += ":";
            if (fInfo->Type != "")
            	line += fInfo->Type;
            else
            	line += "?";
            lbFields->Items->Add(line);
            if (fInfo->Offset == FieldOffset)
                SelIndex = n;
        }
    }
}
//----------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::FormShow(TObject *Sender)
{
	Caption = GetClsName(VmtAdr) + " fields";

    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;

    lbFXrefs->Clear();
    ShowFields();
    lbFields->ItemIndex = SelIndex;

    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bClose->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::lbFXrefsDblClick(TObject *Sender)
{
    char            type[2];
    DWORD           adr;

    String item = lbFXrefs->Items->Strings[lbFXrefs->ItemIndex];
    sscanf(item.c_str() + 1, "%lX%2c", &adr, type);

    for (int m = Adr2Pos(adr); m >= 0; m--)
    {
        if (IsFlagSet(cfProcStart, m))
        {
            FMain_11011981->ShowCode(Pos2Adr(m), adr, -1, -1);
            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::lbFieldsClick(TObject *Sender)
{
    lbFXrefs->Clear();

    if (lbFields->ItemIndex == -1) return;

    String line;
    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    if (fInfo->xrefs)
    {
        for (int n = 0; n < fInfo->xrefs->Count; n++)
        {
            PXrefRec recX = (PXrefRec)fInfo->xrefs->Items[n];
            line = Val2Str8(recX->adr + recX->offset);
            if (recX->type == 'c') line += " <-";
            lbFXrefs->Items->Add(line);
        }
    }
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bEditClick(TObject *Sender)
{
    Op = FD_OP_EDIT;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    edtOffset->Text = Val2Str0(fInfo->Offset);
    edtOffset->Enabled = false;
    edtName->Text = fInfo->Name;
    edtName->Enabled = true;
    edtType->Text = fInfo->Type;
    edtType->Enabled = true;
    edtName->SetFocus();

    lbFields->Enabled = false;
    bApply->Enabled = false;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::edtNameChange(TObject *Sender)
{
	bApply->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::edtTypeChange(TObject *Sender)
{
	bApply->Enabled = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bApplyClick(TObject *Sender)
{
    bool        vmt, ok = false;
    DWORD       adr;
    int         n, size, offset, fromOfs, toOfs;
    PInfoRec    recN;
    PFIELDINFO  fInfo;
    String      rtti;

    recN = GetInfoRec(VmtAdr);
    switch (Op)
    {
    case FD_OP_EDIT:
        fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
        fInfo->Name = Trim(edtName->Text);
        fInfo->Type = Trim(edtType->Text);
        //Delete all fields (if exists) that covered by new type
        fromOfs = fInfo->Offset;
        if (GetTypeKind(edtType->Text, &size) == ikRecord)
        {
            toOfs = fromOfs + size;
            for (n = 0; n < fieldsList->Count; n++)
            {
                fInfo = (PFIELDINFO)fieldsList->Items[n];
                offset = fInfo->Offset;
                if (offset > fromOfs && offset < toOfs)
                {
                    recN->vmtInfo->RemoveField(offset);
                }
            }
        }
        break;
    case FD_OP_ADD:
    case FD_OP_DELETE:
        sscanf(edtOffset->Text.Trim().c_str(), "%lX", &offset);
        if (Op == FD_OP_ADD)
        {
            fInfo = FMain_11011981->GetField(recN->GetName(), offset, &vmt, &adr, "");
            if (!fInfo || Application->MessageBox("Field already exists", "Replace?", MB_YESNO) == IDYES)
                recN->vmtInfo->AddField(0, 0, FIELD_PUBLIC, offset, -1, Trim(edtName->Text), Trim(edtType->Text));
            if (fInfo && fInfo->Scope == SCOPE_TMP)
                delete fInfo;
        }
        if (Op == FD_OP_DELETE)
        {
            if (Application->MessageBox("Delete field?", "Confirmation", MB_YESNO) == IDYES)
                recN->vmtInfo->RemoveField(offset);
        }
        break;
    }
    int itemidx = lbFields->ItemIndex;
    int topidx = lbFields->TopIndex;
    ShowFields();
    lbFields->ItemIndex = itemidx;
    lbFields->TopIndex = topidx;

    FMain_11011981->RedrawCode();
    FMain_11011981->ShowClassViewer(VmtAdr);

    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;

    lbFields->Enabled = true;
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);

    ProjectModified = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bCloseClick(
      TObject *Sender)
{
    edtPanel->Visible = false;
    lbFields->Height = lbFXrefs->Height;
    
    lbFields->Enabled = true;
    bEdit->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
    bAdd->Enabled = true;
    bRemove->Enabled = (lbFields->Count && lbFields->ItemIndex != -1);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bAddClick(TObject *Sender)
{
    Op = FD_OP_ADD;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    edtOffset->Text = "";
    edtOffset->Enabled = true;
    edtName->Text = "";
    edtName->Enabled = true;
    edtType->Text = "";
    edtType->Enabled = true;
    edtOffset->SetFocus();

    lbFields->Enabled = false;
    bApply->Enabled = false;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::bRemoveClick(TObject *Sender)
{
    Op = FD_OP_DELETE;
	lbFields->Height = lbFXrefs->Height - edtPanel->Height;
    edtPanel->Visible = true;

    PFIELDINFO fInfo = (PFIELDINFO)fieldsList->Items[lbFields->ItemIndex];
    edtOffset->Text = Val2Str0(fInfo->Offset);
    edtOffset->Enabled = false;
    edtName->Text = fInfo->Name;
    edtName->Enabled = false;
    edtType->Text = fInfo->Type;
    edtType->Enabled = false;

    lbFields->Enabled = false;
    bApply->Enabled = true;
    bClose->Enabled = true;
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemove->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFieldsDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

