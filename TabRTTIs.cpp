//---------------------------------------------------------------------------
int __fastcall SortRTTIsByAdr(void *item1, void *item2)
{
    PTypeRec rec1 = (PTypeRec)item1;
    PTypeRec rec2 = (PTypeRec)item2;
    if (rec1->adr > rec2->adr) return 1;
    if (rec1->adr < rec2->adr) return -1;
    return 0;
}
//---------------------------------------------------------------------------
int __fastcall SortRTTIsByKnd(void *item1, void *item2)
{
    PTypeRec rec1 = (PTypeRec)item1;
    PTypeRec rec2 = (PTypeRec)item2;
    if (rec1->kind > rec2->kind) return 1;
    if (rec1->kind < rec2->kind) return -1;
    return CompareText(rec1->name, rec2->name);
}
//---------------------------------------------------------------------------
int __fastcall SortRTTIsByNam(void *item1, void *item2)
{
    PTypeRec rec1 = (PTypeRec)item1;
    PTypeRec rec2 = (PTypeRec)item2;
    return CompareText(rec1->name, rec2->name);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowRTTIs()
{
    lbRTTIs->Clear();
    //as
    lbRTTIs->Items->BeginUpdate();

    if (OwnTypeList->Count)
    {
        switch (RTTISortField)
        {
        case 0:
            OwnTypeList->Sort(SortRTTIsByAdr);
            break;
        case 1:
            OwnTypeList->Sort(SortRTTIsByKnd);
            break;
        case 2:
            OwnTypeList->Sort(SortRTTIsByNam);
            break;
        }
    }

    int     wid, maxwid = 0;
    TCanvas *canvas = lbUnits->Canvas;
    String line;
    for (int n = 0; n < OwnTypeList->Count; n++)
    {
        PTypeRec recT = (PTypeRec)OwnTypeList->Items[n];
        if (recT->kind == ikVMT)
            line = Val2Str8(recT->adr) + " <VMT> " + recT->name;
        else
            line = Val2Str8(recT->adr) + " <" + TypeKind2Name(recT->kind) + "> " + recT->name;
        lbRTTIs->Items->Add(line);
        wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
    }
    lbRTTIs->Items->EndUpdate();

    lbRTTIs->ScrollWidth = maxwid + 2;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbRTTIsDblClick(TObject *Sender)
{
    DWORD   adr;
    char    tkName[32], typeName[1024];

    sscanf(lbRTTIs->Items->Strings[lbRTTIs->ItemIndex].c_str(), "%lX%s%s", &adr, tkName, typeName);
    String name = String(tkName);

    if (SameText(name, "<VMT>") && tsClassView->TabVisible)
    {
        ShowClassViewer(adr);
        return;
    }

    FTypeInfo_11011981->ShowRTTI(adr);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbRTTIsMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (lbRTTIs->CanFocus()) ActiveControl = lbRTTIs;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbRTTIsClick(TObject *Sender)
{
    RTTIsSearchFrom = lbRTTIs->ItemIndex;
    WhereSearch = SEARCH_RTTIS;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbRTTIsKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN) lbRTTIsDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchRTTIClick(TObject *Sender)
{
    WhereSearch = SEARCH_RTTIS;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < RTTIsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(RTTIsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbRTTIs->ItemIndex < 0)
            RTTIsSearchFrom = 0;
        else
            RTTIsSearchFrom = lbRTTIs->ItemIndex;
            
        RTTIsSearchText = FindDlg_11011981->cbText->Text;
        if (RTTIsSearchList->IndexOf(RTTIsSearchText) == -1) RTTIsSearchList->Add(RTTIsSearchText);
        FindText(RTTIsSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmRTTIsPopup(TObject *Sender)
{
	if (lbRTTIs->ItemIndex < 0) return;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortRTTIsByAdrClick(TObject *Sender)
{
    miSortRTTIsByAdr->Checked = true;
    miSortRTTIsByKnd->Checked = false;
    miSortRTTIsByNam->Checked = false;
    RTTISortField = 0;
    ShowRTTIs();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortRTTIsByKndClick(TObject *Sender)
{
    miSortRTTIsByAdr->Checked = false;
    miSortRTTIsByKnd->Checked = true;
    miSortRTTIsByNam->Checked = false;
    RTTISortField = 1;
    ShowRTTIs();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortRTTIsByNamClick(TObject *Sender)
{
    miSortRTTIsByAdr->Checked = false;
    miSortRTTIsByKnd->Checked = false;
    miSortRTTIsByNam->Checked = true;
    RTTISortField = 2;
    ShowRTTIs();
}
//---------------------------------------------------------------------------
