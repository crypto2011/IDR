//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortUnitsByAdrClick(TObject *Sender)
{
    miSortUnitsByAdr->Checked = true;
    miSortUnitsByOrd->Checked = false;
    miSortUnitsByNam->Checked = false;
    UnitSortField = 0;
    ShowUnits(true);
    lbUnits->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortUnitsByOrdClick(TObject *Sender)
{
    miSortUnitsByAdr->Checked = false;
    miSortUnitsByOrd->Checked = true;
    miSortUnitsByNam->Checked = false;
    UnitSortField = 1;
    ShowUnits(true);
    lbUnits->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSortUnitsByNamClick(TObject *Sender)
{
    miSortUnitsByAdr->Checked = false;
    miSortUnitsByOrd->Checked = false;
    miSortUnitsByNam->Checked = true;
    UnitSortField = 2;
    ShowUnits(true);
    lbUnits->SetFocus();
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::ContainsUnexplored(PUnitRec recU)
{
	if (!recU) return false;
    
	bool    unk = false;
    for (DWORD adr = recU->fromAdr; adr < recU->toAdr; adr++)
    {
        int n = Adr2Pos(adr);
        if (!Flags[n])
        {
            BYTE b0 = *(Code + n);
        	if (!unk)
            {
                BYTE b1 = *(Code + n + 1); BYTE b2 = *(Code + n + 2);
                if ((adr & 3) == 3 && (b0 == 0 || b0 == 0x90))
                    continue;
                if ((adr & 3) == 2 && ((b0 == 0 && b1 == 0) || (b0 == 0x8B && b1 == 0xC0)))
                {
                    adr++;
                    continue;
                }
                if ((adr & 3) == 1 && ((b0 == 0 && b1 == 0 && b2 == 0) || (b0 == 0x8D && b1 == 0x40 && b2 == 0)))
                {
                    adr += 2;
                    continue;
                }
                unk = true;
            }
            continue;
        }

        if (unk) return true;
    }
    return false;
}
//---------------------------------------------------------------------------
#define TRIV_UNIT   1	//Trivial unit
#define USER_UNIT   2   //User unit
#define	UNEXP_UNIT	4   //Unit has undefined bytes
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowUnits(bool showUnk)
{
    int oldItemIdx = lbUnits->ItemIndex;
    DWORD selAdr = 0;
    if (oldItemIdx != -1)
    {
        String item = lbUnits->Items->Strings[oldItemIdx];
        sscanf(item.c_str() + 1, "%lX", &selAdr);
    }
    int oldTopIdx = lbUnits->TopIndex;

    lbUnits->Clear();
    lbUnits->Items->BeginUpdate();

    if (UnitsNum)
    {
        switch (UnitSortField)
        {
        case 0:
            Units->Sort(SortUnitsByAdr);
            break;
        case 1:
            Units->Sort(SortUnitsByOrd);
            break;
        case 2:
            Units->Sort(SortUnitsByNam);
            break;
        }
    }

    int     stringLen;
    int     newItemIdx = -1;
    int     wid, maxwid = 0;
    TCanvas *canvas = lbUnits->Canvas;
    char    ci, cf, orderS[256];

    for (int i = 0; i < UnitsNum; i++)
    {
        PUnitRec recU = (UnitRec*)Units->Items[i];
        if (recU->fromAdr == selAdr) newItemIdx = i;
        ci = (!recU->trivialIni) ? 'I' : ' ';
        cf = (!recU->trivialFin) ? 'F' : ' ';
        stringLen = sprintf(StringBuf, " %08lX #%03d %c%c ", (int)recU->fromAdr, recU->iniOrder, ci, cf);
        if (recU->names->Count)
        {
            for (int u = 0; u < recU->names->Count; u++)
            {
                if (stringLen + recU->names->Strings[u].Length() >= 256)
                {
                    stringLen += sprintf(StringBuf + stringLen, "...");
                    break;
                }
                if (u)
                {
                    StringBuf[stringLen] = ';';
                    stringLen++;
                }
                stringLen += sprintf(StringBuf + stringLen, "%s", recU->names->Strings[u].c_str());
            }
        }
        else
            stringLen += sprintf(StringBuf + stringLen, "_Unit%d", recU->iniOrder);

        if (i != UnitsNum - 1)
        {
            //Trivial units
            if (recU->trivial)
                StringBuf[0] = TRIV_UNIT;
            else if (!recU->kb)
                StringBuf[0] = USER_UNIT;
        }
        //Last unit is user's
        else
        	StringBuf[0] = USER_UNIT;

        //Unit has undefined bytes
        if (showUnk && ContainsUnexplored(recU)) StringBuf[0] |= UNEXP_UNIT;
        String line = String(StringBuf, stringLen);
        lbUnits->Items->Add(line);
        wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
    }
    if (newItemIdx == -1)
        lbUnits->TopIndex = oldTopIdx;
    else
    {
        if (newItemIdx != oldItemIdx)
        {
            lbUnits->ItemIndex = newItemIdx;
            int newTopIdx = newItemIdx - (oldItemIdx - oldTopIdx);
            if (newTopIdx < 0) newTopIdx = 0;
            lbUnits->TopIndex = newTopIdx;
        }
        else
        {
            lbUnits->ItemIndex = newItemIdx;
            lbUnits->TopIndex = oldTopIdx;
        }
    }
    lbUnits->ItemHeight = lbUnits->Canvas->TextHeight("T");
    lbUnits->ScrollWidth = maxwid + 2;
    lbUnits->Items->EndUpdate();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitsMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    if (lbUnits->CanFocus()) ActiveControl = lbUnits;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitsClick(TObject *Sender)
{
    UnitsSearchFrom = lbUnits->ItemIndex;
    WhereSearch = SEARCH_UNITS;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitsDblClick(TObject *Sender)
{
    DWORD   adr;

    String item = lbUnits->Items->Strings[lbUnits->ItemIndex];
    sscanf(item.c_str() + 1, "%lX", &adr);
    PUnitRec recU = GetUnit(adr);

    if (!CurUnitAdr || adr != CurUnitAdr)
    {
        CurUnitAdr = adr;
        ShowUnitItems(recU, 0, -1);
    }
    else
    {
        ShowUnitItems(recU, lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
    }

  	CurUnitAdr = adr;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitsKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN) lbUnitsDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitsDrawItem(
      TWinControl *Control, int Index, TRect &Rect, TOwnerDrawState State)
{
    char        *s, *pos;
    int         flags, len;
    TColor      _color;
    TListBox    *lb;
    TCanvas     *canvas;
    String      text, str1, str2;

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
        canvas->FillRect(Rect);

        s = text.c_str();
        //*XXXXXXXX #XXX XX NAME
        pos = strrchr(s, ' ');
        len = pos - s;
        str1 = text.SubString(2, len - 1);
        str2 = text.SubString(len + 1, text.Length() - len);

        if (!State.Contains(odSelected))
            _color = TColor(0);        //Black
        else
            _color = TColor(0xBBBBBB); //LightGray
        Rect.Right = Rect.Left;
        DrawOneItem(str1, canvas, Rect, _color, flags);

        //Unit name
        //Trivial unit - red
        if (text[1] & TRIV_UNIT)
        {
            if (!State.Contains(odSelected))
                _color = TColor(0x0000B0); //Red
            else
                _color = TColor(0xBBBBBB); //LightGray
        }
        else
        {
            //User unit - green
            if (text[1] & USER_UNIT)
            {
                if (!State.Contains(odSelected))
                {
                	if (text[1] & UNEXP_UNIT)
                    	_color = TColor(0xC0C0FF); //Light Red
                    else
                    	_color = TColor(0x00B000); //Green
                }
                else
                    _color = TColor(0xBBBBBB); //LightGray
            }
            //From knowledge base - blue
            else
            {
                if (!State.Contains(odSelected))
                    _color = TColor(0xC08000); //Blue
                else
                    _color = TColor(0xBBBBBB); //LightGray
            }
        }
        DrawOneItem(str2, canvas, Rect, _color, flags);
    }
    RestoreCanvas(canvas);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchUnitClick(TObject *Sender)
{
    WhereSearch = SEARCH_UNITS;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < UnitsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(UnitsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbUnits->ItemIndex == -1)
            UnitsSearchFrom = 0;
        else
            UnitsSearchFrom = lbUnits->ItemIndex;

        UnitsSearchText = FindDlg_11011981->cbText->Text;
        if (UnitsSearchList->IndexOf(UnitsSearchText) == -1) UnitsSearchList->Add(UnitsSearchText);
        FindText(UnitsSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miRenameUnitClick(TObject *Sender)
{
    if (lbUnits->ItemIndex == -1) return;

    String item = lbUnits->Items->Strings[lbUnits->ItemIndex];
    DWORD adr;
    sscanf(item.c_str() + 1, "%lX", &adr);
    PUnitRec recU = GetUnit(adr);
    
    String text = "";
    for (int u = 0; u < recU->names->Count; u++)
    {
        if (u) text += "+";
        text += recU->names->Strings[u];
    }
    String sName = InputDialogExec("Enter UnitName", "Name:", text);
    if (sName != "")
    {
        recU->names->Clear();
        SetUnitName(recU, sName);
        
        ProjectModified = true;
        ShowUnits(true);
        lbUnits->SetFocus();
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::ShowUnitItems(PUnitRec recU, int topIdx, int itemIdx)
{
    bool        unk = false;
    bool	    imp, exp, emb, xref;
    int         wid, maxwid = 0;
    TCanvas     *canvas = lbUnitItems->Canvas;
    String      line, prefix;

    if (!CurUnitAdr) return;

    //if (AnalyzeThread) AnalyzeThread->Suspend();

    lbUnitItems->Clear();
    lbUnitItems->Items->BeginUpdate();

    for (DWORD adr = recU->fromAdr; adr < recU->toAdr; adr++)
    {
        int unknum, pos = Adr2Pos(adr);
        if (!IsFlagSet(~cfLoc, pos))
        {
            BYTE b0 = *(Code + pos);
        	if (!unk)
            {
                unknum = 0;
                BYTE b1 = *(Code + pos + 1); BYTE b2 = *(Code + pos + 2);
                if ((adr & 3) == 3 && (b0 == 0 || b0 == 0x90))
                    continue;
                if ((adr & 3) == 2 && ((b0 == 0 && b1 == 0) || (b0 == 0x8B && b1 == 0xC0) || (b0 == 0x90 && b1 == 0x90)))
                {
                    adr++;
                    continue;
                }
                if ((adr & 3) == 1 && ((b0 == 0 && b1 == 0 && b2 == 0) || (b0 == 0x8D && b1 == 0x40 && b2 == 0) || (b0 == 0x90 && b1 == 0x90 && b2 == 0x90)))
                {
                    adr += 2;
                    continue;
                }
                line = " " + Val2Str8(adr) + " ????"; line[1] ^= 1;
                line += " " + Val2Str2(b0); unknum++;
                unk = true;
            }
            else
            {
                if (unknum <= 16)
                {
                    if (unknum == 16)
                        line += "...";
                    else
                        line += " " + Val2Str2(b0);
                    unknum++;
                }
            }
            continue;
        }

        if (unk)
        {
            lbUnitItems->Items->Add(line);
            wid = canvas->TextWidth(line);
            if (wid > maxwid) maxwid = wid;
            unk = false;
        }

        if (adr == recU->iniadr) continue;
        if (adr == recU->finadr) continue;

        //EP
        if (adr == EP)
        {
            line = " " + Val2Str8(adr) + " <Proc> EntryPoint";
            lbUnitItems->Items->Add(line);
            wid = canvas->TextWidth(line);
            if (wid > maxwid) maxwid = wid;
            continue;
        }

        PInfoRec recN = GetInfoRec(adr);
        if (!recN) continue;

        BYTE kind = recN->kind;
        //Skip calls, that are in the body of some asm-procs (for example, FloatToText from SysUtils)
        //if (kind >= ikRefine && kind <= ikFunc && recN->procInfo && IsFlagSet(cfImport, pos)) continue;

        imp = IsFlagSet(cfImport, pos);
        exp = IsFlagSet(cfExport, pos);
        emb = false;
        if (IsFlagSet(cfProcStart, pos))
        {
            if (recN->procInfo)
            {
                emb = (recN->procInfo->flags & PF_EMBED);
            }
        }
        xref = false;

        line = "";

        switch (kind)
        {
		case ikInteger:
		case ikChar:
		case ikEnumeration:
		case ikFloat:
        case ikSet:
        case ikClass:
        case ikMethod:
        case ikWChar:
        case ikLString:
        case ikVariant:
        case ikArray:
        case ikRecord:
        case ikInterface:
        case ikInt64:
        case ikDynArray:
        case ikUString:
        case ikClassRef:
        case ikPointer:
        case ikProcedure:
        	line = "<" + TypeKind2Name(kind) + "> ";
        	if (recN->HasName())
            {
                if (recN->GetNameLength() <= MAXLEN)
                    line += recN->GetName();
                else
                {
                    line += recN->GetName().SubString(1, MAXLEN) + "...";
                }
            }
            break;
        case ikString:
        	if (!IsFlagSet(cfRTTI, pos))
                line = "<ShortString> ";
            else
                line = "<" + TypeKind2Name(kind) + "> ";
            if (recN->HasName())
            {
                if (recN->GetNameLength() <= MAXLEN)
                    line += recN->GetName();
                else
                {
                    line += recN->GetName().SubString(1, MAXLEN) + "...";
                }
            }
            break;
        case ikWString:
        	line = "<WideString> ";
        	if (recN->HasName())
            {
                if (recN->GetNameLength() <= MAXLEN)
                    line += recN->GetName();
                else
                {
                    line += recN->GetName().SubString(1, MAXLEN) + "...";
                }
            }
            break;
        case ikCString:
        	line = "<PAnsiChar> ";
        	if (recN->HasName())
            {
                if (recN->GetNameLength() <= MAXLEN)
                    line += recN->GetName();
                else
                {
                    line += recN->GetName().SubString(1, MAXLEN) + "...";
                }
            }    
            break;
        case ikWCString:
        	line = "<PWideChar> ";
        	if (recN->HasName())
            {
                if (recN->GetNameLength() <= MAXLEN)
                    line += recN->GetName();
                else
                {
                    line += recN->GetName().SubString(1, MAXLEN) + "...";
                }
            }    
        	break;
        case ikResString:
        	if (recN->HasName()) line += "<ResString> " + recN->GetName() + "=" + recN->rsInfo->value;
            break;
        case ikVMT:
        	line = "<VMT> ";
            if (recN->HasName()) line += recN->GetName();
            break;
        case ikConstructor:
            xref = true;
            line = "<Constructor> " + recN->MakePrototype(adr, false, true, false, true, false);
            break;
        case ikDestructor:
            xref = true;
            line = "<Destructor> " + recN->MakePrototype(adr, false, true, false, true, false);
            break;
        case ikProc:
            xref = true;
            line = "<";
            if (imp)
                line += "Imp";
            else if (exp)
                line += "Exp";
            else if (emb)
                line += "Emb";
            line += "Proc> " + recN->MakePrototype(adr, false, true, false, true, false);
            break;
        case ikFunc:
            xref = true;
            line = "<";
            if (imp)
                line += "Imp";
            else if (exp)
                line += "Exp";
            else if (emb)
                line += "Emb";
            line += "Func> " + recN->MakePrototype(adr, false, true, false, true, false);
            break;
        case ikGUID:
        	line = "<TGUID> ";
            if (recN->HasName()) line += recN->GetName();
            break;
        case ikRefine:
            xref = true;
            line = "<";
            if (imp)
                line += "Imp";
            else if (exp)
                line += "Exp";
            else if (emb)
                line += "Emb";
            line += "?> " + recN->MakePrototype(adr, false, true, false, true, false);
            break;
        default:
            if (IsFlagSet(cfProcStart, pos))
            {
                xref = true;
                if (recN->kind == ikConstructor)
                    line = "<Constructor> " + recN->MakePrototype(adr, false, true, false, true, false);
                else if (recN->kind == ikDestructor)
                    line = "<Destructor> " + recN->MakePrototype(adr, false, true, false, true, false);
                else
                {
                    line = "<";
                    if (emb) line += "Emb";
                    line += "Proc> " + recN->MakePrototype(adr, false, true, false, true, false);
                }
            }
        	break;
        }

        if (kind >= ikRefine && kind <= ikFunc)
        {
            if (recN->procInfo->flags & PF_VIRTUAL) line += " virtual";
            if (recN->procInfo->flags & PF_DYNAMIC) line += " dynamic";
            if (recN->procInfo->flags & PF_EVENT)   line += " event";
        }
        if (line != "")
        {
            prefix = " " + Val2Str8(adr);
            if (xref && recN->xrefs && recN->xrefs->Count)
            {
                prefix[1] ^= 2;
                for (int m = 0; m < recN->xrefs->Count; m++)
                {
                    PXrefRec recX = (PXrefRec)recN->xrefs->Items[m];
                    PUnitRec recU = GetUnit(recX->adr);
                    if (recU && !recU->kb)
                    {
                        prefix[1] ^= 4;
                        break;
                    }
                }
                prefix += " " + String(recN->xrefs->Count);
                if (recN->xrefs->Count <= 9)
                    prefix += "  ";
                else if (recN->xrefs->Count <= 99)
                    prefix += " ";
            }
            line = prefix + " " + line;
            lbUnitItems->Items->Add(line);
            wid = canvas->TextWidth(line);
            if (wid > maxwid) maxwid = wid;
        }
    }

    //Add initialization procedure
    if (recU->iniadr)
    {
        line = " " + Val2Str8(recU->iniadr) + " <Proc> Initialization;";
        lbUnitItems->Items->Add(line);
        wid = canvas->TextWidth(line);
        if (wid > maxwid) maxwid = wid;
    }
    //Add finalization procedure
    if (recU->finadr)
    {
        line = " " + Val2Str8(recU->finadr) + " <Proc> Finalization;";
        lbUnitItems->Items->Add(line);
        wid = canvas->TextWidth(line);
        if (wid > maxwid) maxwid = wid;
    }
    lbUnitItems->TopIndex = topIdx;
    lbUnitItems->ItemIndex = itemIdx;
    lbUnitItems->ScrollWidth = maxwid + 2;
    lbUnitItems->ItemHeight = lbUnitItems->Canvas->TextHeight("T");
    lbUnitItems->Items->EndUpdate();

    //if (AnalyzeThread) AnalyzeThread->Resume();
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitItemsDblClick(TObject *Sender)
{
    int         idx = -1, len, size, refCnt, pos, bytes = 1024;
    WORD*       uses;
    DWORD       adr;
    char        *tmpBuf;
    PInfoRec    recN;
    MTypeInfo   tInfo;
    String      str;
    char        tkName[32], typeName[1024];

    if (lbUnitItems->ItemIndex == -1) return;

    String item = lbUnitItems->Items->Strings[lbUnitItems->ItemIndex];
    //Xrefs?
    if (item[11] == '<' || item[11] == '?')
        sscanf(item.c_str() + 1, "%lX%s%s", &adr, tkName, typeName);
    else
        sscanf(item.c_str() + 1, "%lX%d%s%s", &adr, &refCnt, tkName, typeName);
    String name = String(tkName);
    pos = Adr2Pos(adr);

    if (SameText(name, "????"))
    {
        //Find end of unexplored Data
        //Get first byte (use later for filtering code?data)
        BYTE db = *(Code + pos);

        FExplorer_11011981->tsCode->TabVisible = true;
        FExplorer_11011981->ShowCode(adr, bytes);
        FExplorer_11011981->tsData->TabVisible = true;
        FExplorer_11011981->ShowData(adr, bytes);
        FExplorer_11011981->tsString->TabVisible = true;
        FExplorer_11011981->ShowString(adr, 1024);
        FExplorer_11011981->tsText->TabVisible = false;
        FExplorer_11011981->WAlign = 0;

        FExplorer_11011981->btnDefCode->Enabled = true;
        if (IsFlagSet(cfCode, pos)) FExplorer_11011981->btnDefCode->Enabled = false;
        FExplorer_11011981->btnUndefCode->Enabled = false;
        if (IsFlagSet(cfCode | cfData, pos)) FExplorer_11011981->btnUndefCode->Enabled = true;

        if (IsValidCode(adr) != -1 && db >= 0xF)
        	FExplorer_11011981->pc1->ActivePage = FExplorer_11011981->tsCode;
        else
        	FExplorer_11011981->pc1->ActivePage = FExplorer_11011981->tsData;

        if (FExplorer_11011981->ShowModal() == mrOk)
        {
            switch (FExplorer_11011981->DefineAs)
            {
            case DEFINE_AS_CODE:
                recN = GetInfoRec(adr);
                if (!recN)
                    recN = new InfoRec(pos, ikRefine);
                else if (recN->kind < ikRefine || recN->kind > ikFunc)
                {
                    delete recN;
                    recN = new InfoRec(pos, ikRefine);
                }

                //AnalyzeProcInitial(adr);
                AnalyzeProc1(adr, 0, 0, 0, false);
                AnalyzeProc2(adr, true, true);
                AnalyzeArguments(adr);
                AnalyzeProc2(adr, true, true);

                if (!ContainsUnexplored(GetUnit(adr))) ShowUnits(true);
                ShowUnitItems(GetUnit(adr), lbUnitItems->TopIndex, lbUnitItems->ItemIndex);
                ShowCode(adr, 0, -1, -1);
                break;
            case DEFINE_AS_STRING:
                break;
            }
        }
        return;
    }

    if (SameText(name, "<VMT>") && tsClassView->TabVisible)
    {
        ShowClassViewer(adr);
        return;
    }
    if (IsFlagSet(cfRTTI, pos))
    {
        FTypeInfo_11011981->ShowRTTI(adr);
        return;
    }
    if (SameText(name, "<ResString>"))
    {
        FStringInfo_11011981->memStringInfo->Clear();
        FStringInfo_11011981->Caption = "ResString";
        recN = GetInfoRec(adr);
        FStringInfo_11011981->memStringInfo->Lines->Add(recN->rsInfo->value);
        FStringInfo_11011981->ShowModal();
        return;
    }
    if (SameText(name, "<ShortString>") ||
        SameText(name, "<AnsiString>")  ||
        SameText(name, "<WideString>")  ||
        SameText(name, "<PAnsiChar>")   ||
        SameText(name, "<PWideChar>"))
    {
        FStringInfo_11011981->memStringInfo->Clear();
        FStringInfo_11011981->Caption = "String";
        recN = GetInfoRec(adr);
        FStringInfo_11011981->memStringInfo->Lines->Add(recN->GetName());
        FStringInfo_11011981->ShowModal();
        return;
    }
    if (SameText(name, "<UString>"))
    {
        FStringInfo_11011981->memStringInfo->Clear();
        FStringInfo_11011981->Caption = "String";
        recN = GetInfoRec(adr);
        len = wcslen((wchar_t*)(Code + Adr2Pos(adr)));
        size = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)(Code + Adr2Pos(adr)), len, 0, 0, 0, 0);
        if (size)
        {
            tmpBuf = new char[size + 1];
            WideCharToMultiByte(CP_ACP, 0, (wchar_t*)(Code + Adr2Pos(adr)), len, tmpBuf, len, 0, 0);
            FStringInfo_11011981->memStringInfo->Lines->Add(String(tmpBuf, len));
            delete[] tmpBuf;
            FStringInfo_11011981->ShowModal();
        }
        return;
    }
    /*
    if (SameText(name, "<Integer>")     ||
        SameText(name, "<Char>")        ||
        SameText(name, "<Enumeration>") ||
        SameText(name, "<Float>")       ||
        SameText(name, "<Set>")         ||
        SameText(name, "<Class>")       ||
        SameText(name, "<Method>")      ||
        SameText(name, "<WChar>")       ||
        SameText(name, "<Array>")       ||
        SameText(name, "<Record>")      ||
        SameText(name, "<Interface>")   ||
        SameText(name, "<Int64>")       ||
        SameText(name, "<DynArray>")    ||
        SameText(name, "<ClassRef>")    ||
        SameText(name, "<Pointer>")     ||
        SameText(name, "<Procedure>"))
    {
        uses = KnowledgeBase.GetTypeUses(typeName);
        idx = KnowledgeBase.GetTypeIdxByModuleIds(uses, typeName);
        if (uses) delete[] uses;

        if (idx != -1)
        {
            idx = KnowledgeBase.TypeOffsets[idx].NamId;
            if (KnowledgeBase.GetTypeInfo(idx, INFO_FIELDS | INFO_PROPS | INFO_METHODS, &tInfo))
            {
                FTypeInfo_11011981->ShowKbInfo(&tInfo);
                //as delete tInfo;
            }
        }
        else
        {
            FTypeInfo_11011981->ShowRTTI(adr);
        }
        return;
    }
    */
    if (SameText(name, "<Proc>")    	||
        SameText(name, "<Func>")     	||
        SameText(name, "<Constructor>") ||
        SameText(name, "<Destructor>")  ||
        SameText(name, "<EmbProc>") 	||
        SameText(name, "<EmbFunc>")  	||
        SameText(name, "<Emb?>")        ||
        SameText(name, "<ImpProc>")  	||
        SameText(name, "<ExpProc>")  	||
        SameText(name, "<ImpFunc>")  	||
        SameText(name, "<ExpFunc>")     ||
        SameText(name, "<Imp?>")        ||
        SameText(name, "<Exp?>")        ||
        SameText(name, "<?>"))
    {
        PROCHISTORYREC  rec;
        rec.adr = CurProcAdr;
        rec.itemIdx = lbCode->ItemIndex;
        rec.xrefIdx = lbCXrefs->ItemIndex;
        rec.topIdx = lbCode->TopIndex;
        ShowCode(adr, 0, -1, -1);
        CodeHistoryPush(&rec);
        pcWorkArea->ActivePage = tsCodeView;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitItemsKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (Key == VK_RETURN) lbUnitItemsDblClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitItemsClick(TObject *Sender)
{
    UnitItemsSearchFrom = lbUnitItems->ItemIndex;
    WhereSearch = SEARCH_UNITITEMS;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitItemsDrawItem(TWinControl *Control, int Index, TRect &Rect, TOwnerDrawState State)
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
        //Procs with Xrefs
        if (text[1] & 6)
        {
            //Xrefs from user units
            if (text[1] & 4)
            {
                if (!State.Contains(odSelected))
                    _color = TColor(0x00B000); //Green
                else
                    _color = TColor(0xBBBBBB); //LightGray
            }
            //No Xrefs from user units, only from KB units 
            else
            {
                if (!State.Contains(odSelected))
                    _color = TColor(0xC08000); //Blue
                else
                    _color = TColor(0xBBBBBB); //LightGray
            }
        }
        //Unresolved items
        else if (text[1] & 1)
        {
            if (!State.Contains(odSelected))
                _color = TColor(0x8080FF); //Red
            else
                _color = TColor(0xBBBBBB); //LightGray
        }
        //Other
        else
        {
            if (!State.Contains(odSelected))
                _color = TColor(0);        //Black
            else
                _color = TColor(0xBBBBBB); //LightGray
        }
        Rect.Right = Rect.Left;
        DrawOneItem(str, canvas, Rect, _color, flags);
    }
    RestoreCanvas(canvas);
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::lbUnitItemsMouseMove(
      TObject *Sender, TShiftState Shift, int X, int Y)
{
    if (lbUnitItems->CanFocus()) ActiveControl = lbUnitItems;
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miSearchItemClick(
      TObject *Sender)
{
    WhereSearch = SEARCH_UNITITEMS;

    FindDlg_11011981->cbText->Clear();
    for (int n = 0; n < UnitItemsSearchList->Count; n++)
        FindDlg_11011981->cbText->AddItem(UnitItemsSearchList->Strings[n], 0);

    if (FindDlg_11011981->ShowModal() == mrOk && FindDlg_11011981->cbText->Text != "")
    {
        if (lbUnitItems->ItemIndex < 0)
            UnitItemsSearchFrom = 0;
        else
            UnitItemsSearchFrom = lbUnitItems->ItemIndex;

        UnitItemsSearchText = FindDlg_11011981->cbText->Text;
        if (UnitItemsSearchList->IndexOf(UnitItemsSearchText) == -1) UnitItemsSearchList->Add(UnitItemsSearchText);
        FindText(UnitItemsSearchText);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miEditFunctionIClick(TObject *Sender)
{
    int         refCnt;
    DWORD       adr;
    char        tkName[32];

    if (lbUnitItems->ItemIndex < 0) return;

    String item = lbUnitItems->Items->Strings[lbUnitItems->ItemIndex];
    //Xrefs?
    if (item[11] == '<' || item[11] == '?')
        sscanf(item.c_str() + 1, "%lX%s", &adr, tkName);
    else
        sscanf(item.c_str() + 1, "%lX%d%s", &adr, &refCnt, tkName);

    String name = String(tkName);

    if (SameText(name, "<?>")           ||
        //SameText(name, "<Imp?>")        ||
        //SameText(name, "<Emb?>")        ||
        SameText(name, "<Constructor>") ||
        SameText(name, "<Destructor>")  ||
        SameText(name, "<Func>")     	||
        //SameText(name, "<EmbFunc>")  	||
        SameText(name, "<Proc>")    	//||
        //SameText(name, "<EmbProc>") 	||
        //SameText(name, "<ImpFunc>")  	||
        //SameText(name, "<ImpProc>")
        )
    {
    	EditFunction(adr);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::miCopyAddressIClick(TObject *Sender)
{
    CopyAddress(lbUnitItems->Items->Strings[lbUnitItems->ItemIndex], 1, 8);    
}
//---------------------------------------------------------------------------

