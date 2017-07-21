void __fastcall TFMain_11011981::ShowNames(int idx)
{
    int			n, wid, maxwid = 0;
    PInfoRec    recN;
    String		line;
    TCanvas*    canvas = lbNames->Canvas;

    lbNames->Clear();
    lbNames->Items->BeginUpdate();

    for (n = CodeSize; n < TotalSize; n++)
    {
        if (IsFlagSet(cfImport, n)) continue;
	    recN = GetInfoRec(Pos2Adr(n));
        if (recN && recN->HasName())
        {
            line = Val2Str8(Pos2Adr(n)) + " " + recN->GetName();
            if (recN->type != "") line += ":" + recN->type;
            lbNames->Items->Add(line);
            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
        }
    }
    for (n = 0; n < BSSInfos->Count; n++)
    {
        recN = (PInfoRec)BSSInfos->Objects[n];
        line = BSSInfos->Strings[n] + " " + recN->GetName();
        if (recN->type != "") line += ":" + recN->type;
        lbNames->Items->Add(line);
        wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
    }
    lbNames->Items->EndUpdate();

    lbNames->ItemIndex = idx;
    lbNames->ScrollWidth = maxwid + 2;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowNameXrefs(DWORD Adr, int selIdx)
{
    PInfoRec recN;

    lbNXrefs->Clear();

    recN = GetInfoRec(Adr);
    if (recN && recN->xrefs)
    {
        int     wid, maxwid = 0;
        TCanvas *canvas = lbNXrefs->Canvas;
        DWORD   pAdr = 0;
        char	f = 2;

        lbNXrefs->Items->BeginUpdate();
        for (int m = 0; m < recN->xrefs->Count; m++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[m];
            String line = " " + Val2Str8(recX->adr + recX->offset) + " " + recX->type;
            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
            PUnitRec recU = GetUnit(recX->adr);
            if (recU && recU->kb) line[1] ^= 1;
            if (pAdr != recX->adr) f ^= 2; line[1] ^= f;
            pAdr = recX->adr;
            lbNXrefs->Items->Add(line);
        }
        lbNXrefs->Items->EndUpdate();
        
        lbNXrefs->ScrollWidth = maxwid + 2;
        lbNXrefs->ItemIndex = selIdx;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbNamesClick(TObject *Sender)
{
    //NamesSearchFrom = lbNames->ItemIndex;
    //WhereSearch = SEARCH_NAMES;

    if (lbNames->ItemIndex >= 0)
    {
    	DWORD adr;
        String line = lbNames->Items->Strings[lbNames->ItemIndex];
        sscanf(line.c_str() + 1, "%lX", &adr);
        ShowNameXrefs(adr, -1);
    }
}
//---------------------------------------------------------------------------

