//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Misc.h"
#include "EditFunctionDlg.h"
//---------------------------------------------------------------------
#pragma resource "*.dfm"

extern  bool        ProjectModified;
extern  DWORD       CodeBase;
extern  PInfoRec    *Infos;
extern  DWORD       *Flags;
extern  TList       *VmtList;
extern  char        StringBuf[MAXSTRBUFFER];
extern  MKnowledgeBase  KnowledgeBase;

TFEditFunctionDlg_11011981 *FEditFunctionDlg_11011981;
//---------------------------------------------------------------------
__fastcall TFEditFunctionDlg_11011981::TFEditFunctionDlg_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
    VmtCandidatesNum = 0;
}
//---------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bOkClick(TObject *Sender)
{
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key == VK_ESCAPE) ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FormShow(TObject *Sender)
{
    PInfoRec recN = GetInfoRec(Adr);
    SFlags = recN->procInfo->flags;
    SName = recN->GetName();
    EndAdr = Adr + recN->procInfo->procSize - 1;
    lEndAdr->Text = Val2Str0(EndAdr);
    StackSize = recN->procInfo->stackSize;
    lStackSize->Text = Val2Str0(StackSize);

    FillVMTCandidates();
    pc->ActivePage = tsType;
    //Type
    cbMethod->Enabled = false;
    cbVmtCandidates->Enabled = false;
    mType->Enabled = false;
    
    cbEmbedded->Enabled = false;
    lEndAdr->Enabled = false;
    lStackSize->Enabled = false;
    rgFunctionKind->Enabled = false;
    rgCallKind->Enabled = false;
    bApplyType->Enabled = false;
    bCancelType->Enabled = false;
    FillType();
    //Args
    lbArgs->Enabled = true;
    FillArgs();
    //Vars
    lbVars->Enabled = true;
    pnlVars->Visible = false;
    edtVarOfs->Text = "";
    edtVarSize->Text = "";
    edtVarName->Text = "";
    edtVarType->Text = "";
    FillVars();
    //Buttons
    bEdit->Enabled = true;
    bAdd->Enabled = false;
    bRemoveSelected->Enabled = false;
    bRemoveAll->Enabled = false;
    bOk->Enabled = false;
    
    TypModified = false;
    VarModified = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bEditClick(TObject *Sender)
{
    if (pc->ActivePage == tsType)
    {
        cbMethod->Enabled = true;
        cbVmtCandidates->Enabled = true;
        mType->Enabled = true;
        cbEmbedded->Enabled = true;
        lEndAdr->Enabled = true;
        lStackSize->Enabled = true;
        rgFunctionKind->Enabled = true;
        rgCallKind->Enabled = true;
        bApplyType->Enabled = true;
        bCancelType->Enabled = true;
    }
    else if (pc->ActivePage == tsVars)
    {
        edtVarOfs->Text = "";
        edtVarName->Text = "";
        edtVarType->Text = "";

        PInfoRec recN = GetInfoRec(Adr);
        PLOCALINFO locInfo = PLOCALINFO(recN->procInfo->locals->Items[lbVars->ItemIndex]);
        if (locInfo)
        {
            edtVarOfs->Text = IntToHex((int)locInfo->Ofs, 0);
            edtVarSize->Text = String(locInfo->Size);
            edtVarName->Text = String(locInfo->Name);
            edtVarType->Text = String(locInfo->TypeDef);
        }
        
        lbVars->Height = pc->Height - pnlVars->Height;
        pnlVars->Visible = true;
    }

    lbArgs->Enabled = false;
    lbVars->Enabled = false;
    //Buttons
    bEdit->Enabled = false;
    bAdd->Enabled = false;
    bRemoveSelected->Enabled = false;
    bRemoveAll->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::lbVarsClick(TObject *Sender)
{
    bEdit->Enabled = (lbVars->SelCount == 1);
    bRemoveSelected->Enabled = (lbVars->SelCount > 0);
    bRemoveAll->Enabled = (lbVars->Count > 0);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::pcChange(TObject *Sender)
{
    if (pc->ActivePage == tsType)
    {
        bEdit->Enabled = true;
        bAdd->Enabled = false;
        bRemoveSelected->Enabled = false;
        bRemoveAll->Enabled = false;
    }
    else if (pc->ActivePage == tsArgs)
    {
        bEdit->Enabled = false;
        bAdd->Enabled = false;
        bRemoveSelected->Enabled = false;
        bRemoveAll->Enabled = false;
        return;
    }
    else
    {
        bAdd->Enabled = false;
        if (lbVars->Count > 0)
        {
            bEdit->Enabled = (lbVars->SelCount == 1);
            bRemoveSelected->Enabled = (lbVars->SelCount > 0);
            bRemoveAll->Enabled = (lbVars->Count > 0);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bApplyTypeClick(TObject *Sender)
{
    if (cbMethod->Checked && cbVmtCandidates->Text == "")
    {
        ShowMessage("Class name is empty");
        return;
    }

    DWORD   newEndAdr;
    if (lEndAdr->Text == "" || !TryStrToInt(String("$") + lEndAdr->Text, newEndAdr))
    {
        ShowMessage("End address is not valid");
        return;
    }
    newEndAdr = StrToInt(String("$") + lEndAdr->Text);
    if (!IsValidCodeAdr(newEndAdr))
    {
        ShowMessage("End address is not valid");
        return;
    }

    if (lStackSize->Text == "" || !TryStrToInt(String("$") + lStackSize->Text, StackSize))
    {
        ShowMessage("StackSize not valid");
        return;
    }
    StackSize = StrToInt(String("$") + lStackSize->Text);

    PInfoRec recN = GetInfoRec(Adr);
    //Set new procSize
    recN->procInfo->procSize = newEndAdr - Adr + 1;

    switch (rgFunctionKind->ItemIndex)
    {
    case 0:
        recN->kind = ikConstructor;
        break;
    case 1:
        recN->kind = ikDestructor;
        break;
    case 2:
        recN->kind = ikProc;
        recN->type = "";
        break;
    case 3:
        recN->kind = ikFunc;
        break;
    }

    recN->procInfo->flags &= 0xFFFFFFF8;
    recN->procInfo->flags |= rgCallKind->ItemIndex;

    if (cbEmbedded->Checked)
        recN->procInfo->flags |= PF_EMBED;
    else
        recN->procInfo->flags &= ~PF_EMBED;

    String line, decl = "";
    for (int n = 0; n < mType->Lines->Count; n++)
    {
        line = mType->Lines->Strings[n].Trim();
        if (line == "") continue;
        decl += line;
    }

    decl = decl.Trim();
    char* p = decl.AnsiLastChar();
    if (*p == ';') *p = ' ';
    p = decl.c_str();
    String name = "";
    for (int len = 0;; len++)
    {
        char c = *p;
        if (!c || c == ' ' || c == '(' || c == ';' || c == ':')
        {
            *p = 0;
            name = decl.SubString(1, len);
            *p = c;
            break;
        }
        p++;
    }

    if (recN->kind == ikConstructor)
    {
        recN->SetName(cbVmtCandidates->Text + ".Create");
    }
    else if (recN->kind == ikDestructor)
    {
        recN->SetName(cbVmtCandidates->Text + ".Destroy");
    }
    else if (SameText(name, GetDefaultProcName(Adr)))
    {
        if (cbMethod->Checked && (recN->procInfo->flags & PF_ALLMETHODS))
            recN->SetName(cbVmtCandidates->Text + "." + name);
        else
            recN->SetName("");
    }
    else
    {
        if (cbMethod->Checked && (recN->procInfo->flags & PF_ALLMETHODS))
            recN->SetName(cbVmtCandidates->Text + "." + ExtractProcName(name));
        else
            recN->SetName(name);
    }

    recN->procInfo->DeleteArgs();
    int from = 0;
    if (recN->kind == ikConstructor || recN->kind == ikDestructor)
    {
        recN->procInfo->AddArg(0x21, 0, 4, "Self", cbVmtCandidates->Text);
        recN->procInfo->AddArg(0x21, 1, 4, "_Dv__", "Boolean");
        from = 2;
    }
    else if (recN->procInfo->flags & PF_ALLMETHODS)
    {
        recN->procInfo->AddArg(0x21, 0, 4, "Self", cbVmtCandidates->Text);
        from = 1;
    }
    String retType = recN->procInfo->AddArgsFromDeclaration(p, from, rgCallKind->ItemIndex);

    if (recN->kind == ikFunc) recN->type = retType;

    recN->procInfo->stackSize = StackSize;

    FillType();
    FillArgs();

    cbMethod->Enabled = false;
    mType->Enabled = false;
    cbEmbedded->Enabled = false;
    lEndAdr->Enabled = false;
    lStackSize->Enabled = false;
    rgFunctionKind->Enabled = false;
    rgCallKind->Enabled = false;
    bApplyType->Enabled = false;
    bCancelType->Enabled = false;
    //Buttons
    bEdit->Enabled = true;
    bAdd->Enabled = false;
    bRemoveSelected->Enabled = false;
    bRemoveAll->Enabled = false;
    bOk->Enabled = true;

    TypModified = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bCancelTypeClick(
      TObject *Sender)
{
    if (!TypModified)
    {
        PInfoRec recN = GetInfoRec(Adr);
        recN->SetName(SName);
        recN->procInfo->flags = SFlags;
    }
    FillType();
    cbMethod->Enabled = false;
    cbVmtCandidates->Enabled = false;
    mType->Enabled = false;
    cbEmbedded->Enabled = false;
    lEndAdr->Enabled = false;
    lStackSize->Enabled = false;
    rgFunctionKind->Enabled = false;
    rgCallKind->Enabled = false;
    bApplyType->Enabled = false;
    bCancelType->Enabled = false;
    //Buttons
    bEdit->Enabled = true;
    bAdd->Enabled = false;
    bRemoveSelected->Enabled = false;
    bRemoveAll->Enabled = false;
    bOk->Enabled = false;
    TypModified = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bApplyVarClick(TObject *Sender)
{
    int     recofs, size;
    String  fname, ftype, name, type;
    
    try
    {
        recofs = StrToInt("$" + edtVarOfs->Text.Trim());
    }
    catch (Exception &E)
    {
        ShowMessage("Invalid offset");
        edtVarOfs->SetFocus();
        return;
    }
    try
    {
        size = StrToInt("$" + edtVarSize->Text.Trim());
    }
    catch (Exception &E)
    {
        ShowMessage("Invalid size");
        edtVarSize->SetFocus();
        return;
    }

    //Insert by ZGL
    PInfoRec recN = GetInfoRec(Adr);
    PLOCALINFO locInfo = PLOCALINFO(recN->procInfo->locals->Items[lbVars->ItemIndex]);
    ////////////

    recofs = locInfo->Ofs;
    fname = edtVarName->Text.Trim();
    locInfo->Name = fname;  //ZGL add
    ftype = edtVarType->Text.Trim();
    locInfo->TypeDef = ftype;  //ZGL add
    recN->procInfo->SetLocalType(recofs, ftype);
    /*
    if (ftype != "" && GetTypeKind(ftype, &size) == ikRecord)
    {
        String recFileName = FMain_11011981->WrkDir + "\\types.idr";
        FILE* recFile = fopen(recFileName.c_str(), "rt");
        if (recFile)
        {
            while (1)
            {
                if (!fgets(StringBuf, 1024, recFile)) break;
                String str = String(StringBuf);
                if (str.Pos(ftype + "=") == 1)
                {
                    while (1)
                    {
                        if (!fgets(StringBuf, 1024, recFile)) break;
                        str = String(StringBuf);
                        if (str.Pos("end;")) break;
                        int pos2 = str.Pos("//");
                        if (pos2)
                        {
                            String ofs = str.SubString(pos2 + 2, str.Length());
                            int pos1 = str.Pos(":");
                            if (pos1)
                            {
                                name = str.SubString(1, pos1 - 1);
                                type = str.SubString(pos1 + 1, pos2 - pos1 - 1);
                                recN->procInfo->AddLocal(StrToInt("$" + ofs) + recofs, 1, fname + "." + name, type);
                            }
                        }
                    }
                }
            }
            fclose(recFile);
        }
        while (1)
        {
            //KB
            WORD* uses = KnowledgeBase.GetTypeUses(ftype.c_str());
            int idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, ftype.c_str());
            if (uses) delete[] uses;

            if (idx == -1) break;

            idx = KnowledgeBase.TypeOffsets[idx].NamId;
            MTypeInfo tInfo;
            if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS, &tInfo))
            {
                if (tInfo.FieldsNum)
                {
                    char* p = tInfo.Fields;
                    for (int k = 0; k < tInfo.FieldsNum; k++)
                    {
                        //Scope
                        p++;
                        int elofs = *((int*)p); p += 4;
                        p += 4;//case
                        //Name
                        int len = *((WORD*)p); p += 2;
                        name = String((char*)p, len); p += len + 1;
                        //Type
                        len = *((WORD*)p); p += 2;
                        type = TrimTypeName(String((char*)p, len)); p += len + 1;
                        recN->procInfo->AddLocal(recofs + elofs, 1, fname + "." + name, type);
                    }
                    break;
                }
                if (tInfo.Decl != "")
                {
                    ftype = tInfo.Decl;
                }
            }
        }
    }
    */
    FillVars();

    pnlVars->Visible = false;
    lbVars->Enabled = true;
    lbArgs->Enabled = true;

    bEdit->Enabled = true;
    bAdd->Enabled = false;
    bRemoveSelected->Enabled = false;
    bRemoveAll->Enabled = false;
    bOk->Enabled = true;

    VarModified = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bCancelVarClick(TObject *Sender)
{
    pnlVars->Visible = false;
    lbVars->Enabled = true;
    lbArgs->Enabled = true;
    bOk->Enabled = false;
    VarModified = false;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bRemoveSelectedClick(TObject *Sender)
{
    if (pc->ActivePage == tsVars)
    {
        PInfoRec recN = GetInfoRec(Adr);
        for (int n = lbVars->Count - 1; n >= 0; n--)
        {
            if (lbVars->Selected[n])
            {
                PLOCALINFO pLocInfo = (PLOCALINFO)recN->procInfo->locals->Items[n];
                recN->procInfo->DeleteLocal(n);
            }
        }
        FillVars();
        bEdit->Enabled = false;
        bRemoveSelected->Enabled = false;
        bRemoveAll->Enabled = (lbVars->Count > 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bAddClick(TObject *Sender)
{
    String line, item;
    if (pc->ActivePage == tsVars)
    {
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FillVMTCandidates()
{
    if (!VmtCandidatesNum)
    {
        for (int m = 0; m < VmtList->Count; m++)
        {
            PVmtListRec recV = (PVmtListRec)VmtList->Items[m];
            cbVmtCandidates->Items->Add(recV->vmtName);
            VmtCandidatesNum++;
        }
        cbMethod->Visible = (VmtCandidatesNum != 0);
        cbVmtCandidates->Visible = (VmtCandidatesNum != 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FillType()
{
    BYTE        callKind;
    int         argsBytes;
    DWORD       flags;
    PInfoRec    recN;
    String      line;

    mType->Clear();
    recN = GetInfoRec(Adr);

    switch (recN->kind)
    {
    case ikConstructor:
        rgFunctionKind->ItemIndex = 0;
        break;
    case ikDestructor:
        rgFunctionKind->ItemIndex = 1;
        break;
    case ikProc:
        rgFunctionKind->ItemIndex = 2;
        break;
    case ikFunc:
        rgFunctionKind->ItemIndex = 3;
        break;
    }
    flags = recN->procInfo->flags;
    callKind = flags & 7;
    rgCallKind->ItemIndex = callKind;
    cbEmbedded->Checked = (flags & PF_EMBED);

    line = recN->MakeMultilinePrototype(Adr, &argsBytes, (cbMethod->Checked) ? cbVmtCandidates->Text : String(""));
    mType->Lines->Add(line);
    //No VMT - nothing to choose
    if (!VmtCandidatesNum)
    {
        cbMethod->Checked = false;
        cbMethod->Visible = false;
        cbVmtCandidates->Visible = false;
    }
    else
    {
        if (VmtCandidatesNum == 1) cbVmtCandidates->Text = cbVmtCandidates->Items->Strings[0];
        
        if (recN->kind == ikConstructor || recN->kind == ikDestructor || (flags & PF_METHOD))
        {
            cbMethod->Checked = true;
            if (recN->HasName()) cbVmtCandidates->Text = ExtractClassName(recN->GetName());
        }
        else
        {
            cbMethod->Checked = false;
        }

        cbMethod->Visible = true;
        cbVmtCandidates->Visible = true;
    }

    recN->procInfo->flags &= ~PF_ARGSIZEL;
    recN->procInfo->flags &= ~PF_ARGSIZEG;
    if (argsBytes > recN->procInfo->retBytes) recN->procInfo->flags |= PF_ARGSIZEG;
    if (argsBytes < recN->procInfo->retBytes) recN->procInfo->flags |= PF_ARGSIZEL;

    lRetBytes->Caption = String(recN->procInfo->retBytes);
    lArgsBytes->Caption = String(argsBytes);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FillArgs()
{
    BYTE        callKind;
    int         n, cnt, wid, maxwid, offset;
    TCanvas     *canvas;
    PInfoRec    recN;
    PARGINFO    argInfo;
    String      line;

    lbArgs->Clear();
    recN = GetInfoRec(Adr);
    if (recN->procInfo->args)
    {
        canvas = lbArgs->Canvas; maxwid = 0;

        cnt = recN->procInfo->args->Count;
        callKind = recN->procInfo->flags & 7;

        if (callKind == 1 || callKind == 3)//cdecl, stdcall
        {
            for (n = 0; n < cnt; n++)
            {
                argInfo = (PARGINFO)recN->procInfo->args->Items[n];
                line = Val2Str4(argInfo->Ndx) + " " + Val2Str2(argInfo->Size) + " ";

                if (argInfo->Name != "")
                    line += argInfo->Name;
                else
                    line += "?";
                line += ":";
                if (argInfo->TypeDef != "")
                    line += argInfo->TypeDef;
                else
                    line += "?";

                wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
                lbArgs->Items->Add(line);
            }
        }
        else if (callKind == 2)//pascal
        {
            for (n = cnt - 1; n >= 0; n--)
            {
                argInfo = (PARGINFO)recN->procInfo->args->Items[n];
                line = Val2Str4(argInfo->Ndx) + " " + Val2Str2(argInfo->Size) + " ";

                if (argInfo->Name != "")
                    line += argInfo->Name;
                else
                    line += "?";
                line += ":";
                if (argInfo->TypeDef != "")
                    line += argInfo->TypeDef;
                else
                    line += "?";

                wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
                lbArgs->Items->Add(line);
            }
        }
        else//fastcall, safecall
        {
            offset = recN->procInfo->bpBase;
            for (n = 0; n < cnt; n++)
            {
                argInfo = (PARGINFO)recN->procInfo->args->Items[n];
                if (argInfo->Ndx > 2)
                {
                    offset += argInfo->Size;
                    continue;
                }
                if (argInfo->Ndx == 0)
                    line = " eax ";
                else if (argInfo->Ndx == 1)
                    line = " edx ";
                else if (argInfo->Ndx == 2)
                    line = " ecx ";

                line += Val2Str2(argInfo->Size) + " ";

                if (argInfo->Tag == 0x22) line += "var ";
                if (argInfo->Name != "")
                    line += argInfo->Name;
                else
                    line += "?";
                line += ":";
                if (argInfo->TypeDef != "")
                    line += argInfo->TypeDef;
                else
                    line += "?";

                wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
                lbArgs->Items->Add(line);
            }
            for (n = 0; n < cnt; n++)
            {
                argInfo = (PARGINFO)recN->procInfo->args->Items[n];
                if (argInfo->Ndx <= 2) continue;

                offset -= argInfo->Size;
                line = Val2Str4(offset) + " " + Val2Str2(argInfo->Size) + " ";

                if (argInfo->Tag == 0x22) line += "var ";
                if (argInfo->Name != "")
                    line += argInfo->Name;
                else
                    line += "?";
                line += ":";
                if (argInfo->TypeDef != "")
                    line += argInfo->TypeDef;
                else
                    line += "?";

                wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
                lbArgs->Items->Add(line);
            }
        }
        lbArgs->ScrollWidth = maxwid + 2;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FillVars()
{
    int         n, wid, maxwid;
    TCanvas     *canvas;
    PInfoRec    recN;
    PLOCALINFO  locInfo;
    String      line;

    lbVars->Clear();
    rgLocBase->ItemIndex = -1;
    recN = GetInfoRec(Adr);
    if (recN->procInfo->locals)
    {
        rgLocBase->ItemIndex = (recN->procInfo->flags & PF_BPBASED);

        canvas = lbVars->Canvas; maxwid = 0;
        for (n = 0; n < recN->procInfo->locals->Count; n++)
        {
            locInfo = (PLOCALINFO)recN->procInfo->locals->Items[n];
            line = Val2Str8(-locInfo->Ofs) + " " + Val2Str2(locInfo->Size) + " ";
            if (locInfo->Name != "")
                line += locInfo->Name;
            else
                line += "?";
            line += ":";
            if (locInfo->TypeDef != "")
                line += locInfo->TypeDef;
            else
                line += "?";

            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
            lbVars->Items->Add(line);
        }
        lbVars->ScrollWidth = maxwid + 2;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FormClose(TObject *Sender,
      TCloseAction &Action)
{
    if (!TypModified)
    {
        PInfoRec recN = GetInfoRec(Adr);
        recN->SetName(SName);
        recN->procInfo->flags = SFlags;
    }
    else
        ProjectModified = true;
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::cbMethodClick(TObject *Sender)
{
    PInfoRec recN = GetInfoRec(Adr);
    if (cbMethod->Checked)
    {
        cbVmtCandidates->Enabled = true;
        recN->procInfo->flags |= PF_METHOD;
        FillType();
        cbVmtCandidates->Text = ((PARGINFO)recN->procInfo->args->Items[0])->TypeDef;
    }
    else
    {
        cbVmtCandidates->Enabled = false;
        recN->procInfo->flags &= ~PF_METHOD;
        FillType();
        cbVmtCandidates->Text = "";
    }
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------
void __fastcall TFEditFunctionDlg_11011981::bRemoveAllClick(
      TObject *Sender)
{
    PInfoRec recN = GetInfoRec(Adr);
    if (recN->procInfo->locals)
    {
        recN->procInfo->DeleteLocals();
        FillVars();
        bEdit->Enabled = false;
        bRemoveSelected->Enabled = false;
        bRemoveAll->Enabled = false;
    }
}
//---------------------------------------------------------------------------
