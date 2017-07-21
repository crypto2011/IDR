void __fastcall TFMain_11011981::ShowCodeXrefs(DWORD Adr, int selIdx)
{
    lbCXrefs->Clear();
    PInfoRec recN = GetInfoRec(Adr);
    if (recN && recN->xrefs)
    {
        int     wid, maxwid = 0;
        TCanvas *canvas = lbCXrefs->Canvas;
        DWORD   pAdr = 0;
        char	f = 2;

        lbCXrefs->Items->BeginUpdate();

        for (int m = 0; m < recN->xrefs->Count; m++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[m];
            String line = " " + Val2Str8(recX->adr + recX->offset) + " " + recX->type;
            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
            PUnitRec recU = GetUnit(recX->adr);
            if (recU && recU->kb) line[1] ^= 1;
            if (pAdr != recX->adr) f ^= 2; line[1] ^= f;
            pAdr = recX->adr;
            lbCXrefs->Items->Add(line);
        }
        lbCXrefs->ScrollWidth = maxwid + 2;
        lbCXrefs->ItemIndex = selIdx;
        lbCXrefs->ItemHeight = lbCXrefs->Canvas->TextHeight("T");
        lbCXrefs->Items->EndUpdate();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbXrefsMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    TListBox *lb = (TListBox*)Sender;
    if (lb->CanFocus()) ActiveControl = lb;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbXrefsDblClick(TObject *Sender)
{
    char            type[2];
    DWORD           adr;
    PROCHISTORYREC  rec;

    TListBox *lb = (TListBox*)Sender;
    if (lb->ItemIndex < 0) return;

    String item = lb->Items->Strings[lb->ItemIndex];
    sscanf(item.c_str() + 1, "%lX%2c", &adr, type);

    if (type[1] == 'D')
    {
        PInfoRec recN = GetInfoRec(adr);
        if (recN && recN->kind == ikVMT)
        {
            ShowClassViewer(adr);
            WhereSearch = SEARCH_CLASSVIEWER;

            if (!rgViewerMode->ItemIndex)
            {
                TreeSearchFrom = tvClassesFull->Items->Item[0];
            }
            else
            {
                BranchSearchFrom = tvClassesShort->Items->Item[0];
            }
            //Сначала ищем класс
            String text = "#" + Val2Str8(adr);
            FindText(text);
            //Потом - текущую процедуру
            if (!rgViewerMode->ItemIndex)
            {
                if (tvClassesFull->Selected)
                    TreeSearchFrom = tvClassesFull->Selected;
                else
                    TreeSearchFrom = tvClassesFull->Items->Item[0];
            }
            else
            {
                if (tvClassesShort->Selected)
                    BranchSearchFrom = tvClassesShort->Selected;
                else
                    BranchSearchFrom = tvClassesShort->Items->Item[0];
            }
            text = "#" + Val2Str8(CurProcAdr);
            FindText(text);
            return;
        }
    }

    for (int m = Adr2Pos(adr); m >= 0; m--)
    {
        if (IsFlagSet(cfProcStart, m))
        {
            rec.adr = CurProcAdr;
            rec.itemIdx = lbCode->ItemIndex;
            rec.xrefIdx = lbCXrefs->ItemIndex;
            rec.topIdx = lbCode->TopIndex;
            ShowCode(Pos2Adr(m), adr, -1, -1);
            CodeHistoryPush(&rec);
            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbXrefsKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN) lbXrefsDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbXrefsDrawItem(TWinControl *Control,
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

        text = lb->Items->Strings[Index];
        //lb->ItemHeight = canvas->TextHeight(text);
        str = text.SubString(2, text.Length() - 1);

        if (text[1] & 2)
        {
            if (!State.Contains(odSelected))
                canvas->Brush->Color = TColor(0xE7E7E7);
            else
                canvas->Brush->Color = TColor(0xFF0000);
        }
        canvas->FillRect(Rect);

        //Xrefs to Kb units
        if (text[1] & 1)
        {
            if (!State.Contains(odSelected))
                _color = TColor(0xC08000); //Blue
            else
                _color = TColor(0xBBBBBB); //LightGray
        }
        //Others
        else
        {
            if (!State.Contains(odSelected))
                _color = TColor(0x00B000); //Green
            else
                _color = TColor(0xBBBBBB); //LightGray
        }
        Rect.Right = Rect.Left;
        DrawOneItem(str, canvas, Rect, _color, flags);
    }
    RestoreCanvas(canvas);
}
//---------------------------------------------------------------------------
