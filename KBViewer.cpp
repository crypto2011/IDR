//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "KBViewer.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#include "Disasm.h"
#include "KnowledgeBase.h"
#include "Main.h"
#include "Misc.h"

extern  DWORD       CodeBase;
extern  DWORD       CurProcAdr;
extern  DWORD       CurUnitAdr;
extern  MDisasm     Disasm;
extern  BYTE        *Code;
extern  PInfoRec    *Infos;
extern  MKnowledgeBase  KnowledgeBase;
extern  TList       *VmtList;
extern  TList       *OwnTypeList;

TFKBViewer_11011981 *FKBViewer_11011981;
//---------------------------------------------------------------------------
__fastcall TFKBViewer_11011981::TFKBViewer_11011981(TComponent* Owner)
    : TForm(Owner)
{
    UnitsNum = 0;
    CurrIdx = -1;
    CurrAdr = 0;
    FromIdx = 0;
    ToIdx = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::FormCreate(TObject *Sender)
{
    lbIDR->Canvas->Font->Assign(lbIDR->Font);
    lbKB->Canvas->Font->Assign(lbKB->Font);
    ScaleForm(this);
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::ShowCode(DWORD adr, int idx)
{
    bool        outfixup;
    int         n, m, wid, maxwid, row;
    int         _firstProcIdx, _lastProcIdx;
    DWORD       Val, Adr;
    TCanvas*    canvas;
    String      line;
    DISINFO     DisInfo;
    char        disLine[100];

    CurrIdx = idx;
    CurrAdr = adr;
    edtCurrIdx->Text = String(CurrIdx);
    lPosition->Caption = String(CurrIdx - Position);

    lbIDR->Clear();
    canvas = lbIDR->Canvas; maxwid = 0;
    for (m = 0; m < FMain_11011981->lbCode->Count; m++)
    {
        line = FMain_11011981->lbCode->Items->Strings[m];
        //Ignore first byte (for symbol <)
        int _bytes = line.Length() - 1;
        //If instruction, ignore flags (last byte)
        if (m) _bytes --;
        //Extract original instruction
        line = line.SubString(2, _bytes);
        //For first row add prefix IDR:
        if (!m) line = "IDR:" + line;
        lbIDR->Items->Add(line);
        wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
    }
    lbIDR->ScrollWidth = maxwid + 2;

    lbKB->Clear(); row = 0;
    MProcInfo aInfo;
    MProcInfo *pInfo = &aInfo;
    maxwid = 0;
    if (KnowledgeBase.GetProcInfo(CurrIdx, INFO_DUMP, pInfo))
    {
        cbUnits->Text = KnowledgeBase.GetModuleName(pInfo->ModuleID);
        KnowledgeBase.GetProcIdxs(pInfo->ModuleID, &_firstProcIdx, &_lastProcIdx);
        lblKbIdxs->Caption = String(_firstProcIdx) + " - " + String(_lastProcIdx);

        canvas = lbKB->Canvas;
        
        line = "KB:" + pInfo->ProcName;
        lbKB->Items->Add(line); row++;
        wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;
        
        int     instrLen, pos = 0;
        DWORD   curAdr = adr;

        if (pInfo->FixupNum)
        {
            char *p = pInfo->Dump + 2*pInfo->DumpSz;

            for (n = 0; n < pInfo->FixupNum; n++)
            {
                char fixupType = *p; p++;
                int fixupOfs = *((DWORD*)p); p += 4;
                WORD len = *((WORD*)p); p += 2;
                String fixupName = String(p, len); p += len + 1;

                while (pos <= fixupOfs && pos < pInfo->DumpSz)
                {
                    instrLen = Disasm.Disassemble(pInfo->Dump + pos, (__int64)curAdr, &DisInfo, disLine);
                    if (!instrLen)
                    {
                        //return;
                        lbKB->Items->Add(Val2Str8(curAdr) + "      ???"); row++;
                        pos++; curAdr++;
                        continue;
                    }

                    BYTE op = Disasm.GetOp(DisInfo.Mnem);
                    line = Val2Str8(curAdr) + "      " + disLine;
                    outfixup = false;
                    if (pos + instrLen > fixupOfs)
                    {
                        if (DisInfo.Call)
                        {
                            line = Val2Str8(curAdr) + "      call        ";
                            outfixup = true;
                        }
                        else if (op == OP_JMP)
                            line = Val2Str8(curAdr) + "      jmp         ";
                        else
                            line += ";";
                        if (!SameText(fixupName, pInfo->ProcName))
                        {
                            line += fixupName;
                        }
                        else
                        {
                            Val = *((DWORD*)(Code + Adr2Pos(CurrAdr) + fixupOfs));
                            if (fixupType == 'J')
                                Adr = CurrAdr + fixupOfs + Val + 4;
                            else
                                Adr = Val;
                                
                            line += Val2Str8(Adr);
                        }
                    }

                    lbKB->Items->Add(line);
                    if (outfixup) lbKB->Selected[row] = true;
                    row++;
                    wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;

                    pos += instrLen;
                    curAdr += instrLen;
                }
            }
        }
        while (pos < pInfo->DumpSz)
        {
            instrLen = Disasm.Disassemble(pInfo->Dump + pos, (__int64)curAdr, &DisInfo, disLine);
            if (!instrLen)
            {
                lbKB->Items->Add(Val2Str8(curAdr) + "      ???");
                pos++; curAdr++;
                continue;
            }
            line = Val2Str8(curAdr) + "      " + disLine;
            lbKB->Items->Add(line);
            wid = canvas->TextWidth(line); if (wid > maxwid) maxwid = wid;

            pos += instrLen;
            curAdr += instrLen;
        }
    }
    lbKB->ScrollWidth = maxwid + 2;
    lbKB->TopIndex = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::bPrevClick(TObject *Sender)
{
    if (CurrIdx != -1) ShowCode(CurrAdr, CurrIdx - 1);
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::bNextClick(TObject *Sender)
{
    if (CurrIdx != -1) ShowCode(CurrAdr, CurrIdx + 1);
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::btnOkClick(TObject *Sender)
{
    int         m, k1, k2, ap, pos, pos1, pos2, Idx, val;
    DWORD       adr;
    MProcInfo   aInfo;
    MProcInfo   *pInfo = &aInfo;
    PInfoRec    recN;
    String      kbName, idrName, kbLine, idrLine;


    if (KnowledgeBase.GetProcInfo(CurrIdx, INFO_DUMP | INFO_ARGS, pInfo))
    {
        adr = CurProcAdr; ap = Adr2Pos(adr);
        if (ap < 0) return;
        recN = GetInfoRec(adr);
        if (!recN) return;
        recN->procInfo->DeleteArgs();
        FMain_11011981->StrapProc(ap, CurrIdx, pInfo, false, FMain_11011981->EstimateProcSize(CurProcAdr));
        //Strap all selected items (in IDR list box)
        if (lbKB->SelCount == lbIDR->SelCount)
        {
            k1 = 0; k2 = 0;
            for (m = 0; m < lbKB->SelCount; m++)
            {
                kbLine = ""; idrLine = "";
                for (; k1 < lbKB->Items->Count; k1++)
                {
                    if (lbKB->Selected[k1])
                    {
                        kbLine = lbKB->Items->Strings[k1]; k1++;
                        break;
                    }
                }
                for (; k2 < lbIDR->Items->Count; k2++)
                {
                    if (lbIDR->Selected[k2])
                    {
                        idrLine = lbIDR->Items->Strings[k2]; k2++;
                        break;
                    }
                }
                if (kbLine != "" && idrLine != "")
                {
                    pos1 = kbLine.Pos("call");
                    pos2 = idrLine.Pos("call");
                    if (pos1 && pos2)
                    {
                        kbName = kbLine.SubString(pos1 + 4, kbLine.Length()).Trim();
                        idrName = idrLine.SubString(pos2 + 4, idrLine.Length()).Trim();
                        pos = idrName.Pos(";");
                        if (pos) idrName = idrName.SubString(1, pos - 1);
                        adr = 0;
                        if (TryStrToInt(String("$") + idrName, val)) adr = val;
                        ap = Adr2Pos(adr);
                        if (kbName != "" && ap >= 0)
                        {
                            recN = GetInfoRec(adr);
                            recN->procInfo->DeleteArgs();

                            WORD *uses = KnowledgeBase.GetModuleUses(KnowledgeBase.GetModuleID(cbUnits->Text.c_str()));
                            Idx = KnowledgeBase.GetProcIdx(uses, kbName.c_str(), Code + ap);
                            if (Idx != -1)
                            {
                                Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                if (!KnowledgeBase.IsUsedProc(Idx))
                                {
                                    if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                        FMain_11011981->StrapProc(ap, Idx, pInfo, true, pInfo->DumpSz);
                                }
                            }
                            else
                            {
                                Idx = KnowledgeBase.GetProcIdx(uses, kbName.c_str(), 0);
                                if (Idx != -1)
                                {
                                    Idx = KnowledgeBase.ProcOffsets[Idx].NamId;
                                    if (!KnowledgeBase.IsUsedProc(Idx))
                                    {
                                        if (KnowledgeBase.GetProcInfo(Idx, INFO_DUMP | INFO_ARGS, pInfo))
                                            FMain_11011981->StrapProc(ap, Idx, pInfo, false, FMain_11011981->EstimateProcSize(adr));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        FMain_11011981->RedrawCode();
        FMain_11011981->ShowUnitItems(FMain_11011981->GetUnit(CurUnitAdr), FMain_11011981->lbUnitItems->TopIndex, FMain_11011981->lbUnitItems->ItemIndex);
    }
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::btnCancelClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::edtCurrIdxChange(TObject *Sender)
{
    try
    {
        ShowCode(CurrAdr, StrToInt(edtCurrIdx->Text));
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::cbUnitsChange(TObject *Sender)
{
    WORD        _moduleID;
    int         k, _firstProcIdx, _lastProcIdx, _idx;
    MProcInfo   _aInfo;
    MProcInfo   *_pInfo = &_aInfo;

    Position = -1;
    _moduleID = KnowledgeBase.GetModuleID(cbUnits->Text.c_str());
    if (_moduleID != 0xFFFF)
    {
        if (KnowledgeBase.GetProcIdxs(_moduleID, &_firstProcIdx, &_lastProcIdx))
        {
            edtCurrIdx->Text = String(_firstProcIdx);
            lblKbIdxs->Caption = String(_firstProcIdx) + " - " + String(_lastProcIdx);
            for (k = _firstProcIdx; k <= _lastProcIdx; k++)
            {
                _idx = KnowledgeBase.ProcOffsets[k].ModId;
                if (!KnowledgeBase.IsUsedProc(_idx))
                {
                    if (KnowledgeBase.GetProcInfo(_idx, INFO_DUMP | INFO_ARGS, _pInfo))
                    {
                        if (MatchCode(Code + Adr2Pos(CurProcAdr), _pInfo))
                        {
                            edtCurrIdx->Text = String(_idx);
                            Position = _idx;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (Position == -1)
        ShowCode(CurProcAdr, _firstProcIdx);
    else
        ShowCode(CurProcAdr, Position);
}
//---------------------------------------------------------------------------
void __fastcall TFKBViewer_11011981::FormShow(TObject *Sender)
{
    if (!UnitsNum)
    {
      for (int n = 0; n < KnowledgeBase.ModuleCount; n++)
      {
          int ID = KnowledgeBase.ModuleOffsets[n].NamId;
          const BYTE *p = KnowledgeBase.GetKBCachePtr(KnowledgeBase.ModuleOffsets[ID].Offset, KnowledgeBase.ModuleOffsets[ID].Size);
          cbUnits->Items->Add(String((char*)(p + 4)));
          UnitsNum++;
      }
    }
}
//---------------------------------------------------------------------------

