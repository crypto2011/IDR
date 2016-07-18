void __fastcall TFMain_11011981::ShowStrings(int idx)
{
    int			n, itemidx, wid, maxwid = 0;
    PInfoRec    recN;
    String		line, line1, str;
    TCanvas*    canvas = lbStrings->Canvas;

    lbStrings->Clear();
    lbStrings->Items->BeginUpdate();

    for (n = 0; n < CodeSize; n++)
    {
	    recN = GetInfoRec(Pos2Adr(n));
    	if (recN && !IsFlagSet(cfRTTI, n))
        {
        	if (recN->kind == ikResString && recN->rsInfo->value != "")
            {
            	line = " " + Val2Str8(Pos2Adr(n)) + " <ResString> " + recN->rsInfo->value;
                if (recN->rsInfo->value.Length() <= MAXLEN)
                	line1 = line;
                else
                {
                	line1 = line.SubString(1, MAXLEN) + "..."; line1[1] ^= 1;
                }
                lbStrings->Items->Add(line1);
                wid = canvas->TextWidth(line1); if (wid > maxwid) maxwid = wid;
                continue;
            }
            if (recN->HasName())
            {
                switch (recN->kind)
                {
                case ikString:
                    str = "<ShortString>";
                    break;
                case ikLString:
                    str = "<AnsiString>";
                    break;
                case ikWString:
                    str = "<WideString>";
                    break;
                case ikCString:
                    str = "<PAnsiChar>";
                    break;
                case ikWCString:
                    str = "<PWideChar>";
                    break;
                case ikUString:
                    str = "<UString>";
                    break;
                default:
                	str = "";
                    break;
                }
                if (str != "")
                {
                	line = " " + Val2Str8(Pos2Adr(n)) + " " + str + " " + recN->GetName();
                    if (recN->GetNameLength() <= MAXLEN)
                    	line1 = line;
                    else
                    {
                    	line1 = line.SubString(1, MAXLEN) + "..."; line1[1] ^= 1;
                    }
                    lbStrings->Items->Add(line1);
                    wid = canvas->TextWidth(line1); if (wid > maxwid) maxwid = wid;
                }
            }
        }
    }
    lbStrings->ItemIndex = idx;
    lbStrings->ScrollWidth = maxwid + 2;
    lbStrings->ItemHeight = lbStrings->Canvas->TextHeight("T");
    lbStrings->Items->EndUpdate();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbStringsClick(TObject *Sender)
{
    StringsSearchFrom = lbStrings->ItemIndex;
    WhereSearch = SEARCH_STRINGS;

    if (lbStrings->ItemIndex >= 0)
    {
    	DWORD adr;
        String line = lbStrings->Items->Strings[lbStrings->ItemIndex];
        sscanf(line.c_str() + 1, "%lX", &adr);
        ShowStringXrefs(adr, -1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbStringsDblClick(TObject *Sender)
{
	DWORD adr;
    String line = lbStrings->Items->Strings[lbStrings->ItemIndex];
    sscanf(line.c_str() + 1, "%lX", &adr);
    if (IsValidImageAdr(adr))
    {
        PInfoRec recN = GetInfoRec(adr);

        if (recN->kind == ikResString)
        {
            FStringInfo_11011981->Caption = "ResString context";
        	FStringInfo_11011981->memStringInfo->Clear();
            FStringInfo_11011981->memStringInfo->Lines->Add(recN->rsInfo->value);
        	FStringInfo_11011981->ShowModal();
        }
        else
        {
            FStringInfo_11011981->Caption = "String context";
        	FStringInfo_11011981->memStringInfo->Clear();
            FStringInfo_11011981->memStringInfo->Lines->Add(recN->GetName());
        	FStringInfo_11011981->ShowModal();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbStringsDrawItem(TWinControl *Control,
      int Index, TRect &Rect, TOwnerDrawState State)
{
    int         flags;
    TColor      _color;
    TListBox    *lb;
    TCanvas     *canvas;
    String      text, str;

    lb = (TListBox*)Control;
    canvas = lb->Canvas;
    SaveCanvas(canvas);

    if (Index < lb->Count)
    {
        flags = Control->DrawTextBiDiModeFlags(DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        if (!Control->UseRightToLeftAlignment())
            Rect.Left += 2;
        else
            Rect.Right -= 2;
        canvas->FillRect(Rect);

        text = lb->Items->Strings[Index];
        //lb->ItemHeight = canvas->TextHeight(text);
        str = text.SubString(2, text.Length() - 1);

        //Long strings
        if (text[1] & 1)
            _color = TColor(0xBBBBBB); //LightGray
        else
            _color = TColor(0);//Black

        Rect.Right = Rect.Left;
        DrawOneItem(str, canvas, Rect, _color, flags);
    }
    RestoreCanvas(canvas);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchStringClick(TObject *Sender)
{
    WhereSearch = SEARCH_STRINGS;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < StringsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(StringsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbStrings->ItemIndex < 0)
            StringsSearchFrom = 0;
        else
            StringsSearchFrom = lbStrings->ItemIndex;
            
        StringsSearchText = FindDlg_11011981->cbText->Text;
        if (StringsSearchList->IndexOf(StringsSearchText) == -1) StringsSearchList->Add(StringsSearchText);
        FindText(StringsSearchText);
        
    	DWORD adr;
        String line = lbStrings->Items->Strings[lbStrings->ItemIndex];
        sscanf(line.c_str() + 1, "%lX", &adr);
        ShowStringXrefs(adr, -1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbStringsMouseMove(
      TObject *Sender, TShiftState Shift, int X, int Y)
{
    if (lbStrings->CanFocus()) ActiveControl = lbStrings;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowSXrefsClick(TObject *Sender)
{
    if (lbSXrefs->Visible)
    {
        ShowSXrefs->BevelOuter = bvRaised;
        lbSXrefs->Visible = false;
    }
    else
    {
        ShowSXrefs->BevelOuter = bvLowered;
        lbSXrefs->Visible = true;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowStringXrefs(DWORD Adr, int selIdx)
{
    lbSXrefs->Clear();

    PInfoRec recN = GetInfoRec(Adr);
    if (recN && recN->xrefs)
    {
        int     wid, maxwid = 0;
        TCanvas *canvas = lbSXrefs->Canvas;
        DWORD   pAdr = 0;
        char	f = 2;

        lbSXrefs->Items->BeginUpdate();
        for (int m = 0; m < recN->xrefs->Count; m++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[m];
            String line = " " + Val2Str8(recX->adr + recX->offset) + " " + recX->type;
            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
            PUnitRec recU = GetUnit(recX->adr);
            if (recU && recU->kb) line[1] ^= 1;
            if (pAdr != recX->adr) f ^= 2; line[1] ^= f;
            pAdr = recX->adr;
            lbSXrefs->Items->Add(line);
        }
        lbSXrefs->Items->EndUpdate();
        
        lbSXrefs->ScrollWidth = maxwid + 2;
        lbSXrefs->ItemIndex = selIdx;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::pmStringsPopup(TObject *Sender)
{
    if (lbStrings->ItemIndex < 0) return;
}
//---------------------------------------------------------------------------
