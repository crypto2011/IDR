//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Explorer.h"
#include "Main.h"
#include "Misc.h"
#include <Clipbrd.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

extern  MDisasm Disasm;
extern  DWORD   ImageBase;
extern	DWORD   ImageSize;
extern  DWORD   TotalSize;
extern  DWORD   CodeBase;
extern  DWORD   CodeSize;
extern  BYTE    *Image;
extern  BYTE    *Code;

extern  char        StringBuf[MAXSTRBUFFER];
extern  PInfoRec    *Infos;

TFExplorer_11011981 *FExplorer_11011981;
//---------------------------------------------------------------------------
__fastcall TFExplorer_11011981::TFExplorer_11011981(TComponent* Owner)
    : TForm(Owner)
{
	WAlign = 0;
    DefineAs = 0;
    Adr = 0;
    lbCode->Clear();
    lbData->Clear();
    lbString->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::ShowCode(DWORD fromAdr, int maxBytes)
{
    int     pos;
    DISINFO DisInfo;
    char    disLine[100];

    lbCode->Clear();

    pos = Adr2Pos(fromAdr);
    if (pos == -1) return;

    Adr = fromAdr;
    DWORD curAdr = Adr;

    for (int n = 0; n < maxBytes;)
    {
        int instrLen = Disasm.Disassemble(Code + pos, (__int64)curAdr, &DisInfo, disLine);
        //if (!instrLen) break;
        if (!instrLen)
        {
            instrLen = 1;
            lbCode->Items->Add(Val2Str8(curAdr) + "  ???");
        }
        else
        {
            lbCode->Items->Add(Val2Str8(curAdr) + "  " + disLine);
        }
        if (n + instrLen > maxBytes) break;
        if (pos + instrLen >= CodeSize) break;
        pos += instrLen;
        curAdr += instrLen;
        n += instrLen;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::ShowData(DWORD fromAdr, int maxBytes)
{
    BYTE        b;
    int         n, k, m, pos;
    float       singleVal;
    double      doubleVal;
    long double extendedVal;
    double      realVal;
    Comp        compVal;
    String      line1, line2;

    lbData->Clear();

    pos = Adr2Pos(fromAdr);
    if (pos == -1) return;

    Adr = fromAdr;
    DWORD curAdr = Adr;

    switch (rgDataViewStyle->ItemIndex)
    {
    //Hex (default)
    case 0:
        for (n = 0, k = 0; n < maxBytes; n++)
        {
            if (pos > ImageSize) break;
            if (!k)
            {
                line1 = Val2Str8(curAdr) + " ";
                line2 = "  ";
            }
            line1 += " " + Val2Str2(Image[pos]);
            b = Image[pos];
            if (b < ' ') b = ' ';
            line2 += String((char)b);
            k++;
            if (k == 16)
            {
                lbData->Items->Add(line1 + line2);
                curAdr += 16;
                k = 0;
            }
            pos++;
        }
        if (k > 0 && k < 16)
        {
            for (m = k; m < 16; m++) line1 += "   ";
            lbData->Items->Add(line1 + line2);
        }
        break;
    //Single = float
    case 1:
        singleVal = 0; memmove((void*)&singleVal, Image + pos, 4);
        line1 = Val2Str8(curAdr) + "  " + FloatToStr(singleVal);
        lbData->Items->Add(line1);
        break;
    //Double = doulbe
    case 2:
        doubleVal = 0; memmove((void*)&doubleVal, Image + pos, 8);
        line1 = Val2Str8(curAdr) + "  " + FloatToStr(doubleVal);
        lbData->Items->Add(line1);
        break;
    //Extended = long double
    case 3:
        try
        {
            extendedVal = 0; memmove((void*)&extendedVal, Image + pos, 10);
            line1 = Val2Str8(curAdr) + "  " + FloatToStr(extendedVal);
        }
        catch (Exception &E)
        {
            line1 = "Impossible!";
        }
        lbData->Items->Add(line1);
        break;
    //Real = double
    case 4:
        realVal = 0; memmove((void*)&realVal, Image + pos, 4);
        line1 = Val2Str8(curAdr) + "  " + FloatToStr(realVal);
        lbData->Items->Add(line1);
        break;
    //Comp = Comp
    case 5:
        compVal = 0; memmove((void*)&compVal, Image + pos, 8);
        line1 = Val2Str8(curAdr) + "  " + FloatToStr(compVal);
        lbData->Items->Add(line1);
        break;
    }
    lbData->Update();
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::ShowString(DWORD fromAdr, int maxBytes)
{
    int         len, size, pos;
    DWORD       adr, resid;
    char        *tmpBuf;
    HINSTANCE   hInst;
    String      str;
    WideString  wStr;
    char        buf[1024];

    lbString->Clear();

    pos = Adr2Pos(fromAdr);
    if (pos == -1) return;

    switch (rgStringViewStyle->ItemIndex)
    {
    case 0:     //PAnsiChar
        len = strlen((char*)(Image + pos));
        if (len < 0) len = 0;
        if (len > maxBytes) len = maxBytes;
        str = TransformString(Image + pos, len);
        break;
    case 1:     //PWideChar
        len = wcslen((wchar_t*)(Image + pos));
        if (len < 0) len = 0;
        if (len > maxBytes) len = maxBytes;
        wStr = WideString((wchar_t*)(Image + pos));
        size = WideCharToMultiByte(CP_ACP, 0, wStr, len, 0, 0, 0, 0);
        if (size)
        {
            tmpBuf = new char[size + 1];
            WideCharToMultiByte(CP_ACP, 0, wStr, len, (LPSTR)tmpBuf, size, 0, 0);
            str = TransformString(tmpBuf, size);
            delete[] tmpBuf;
        }
        break;
    case 2:     //ShortString
        len = *(Image + pos);
        if (len < 0) len = 0;
        if (len > maxBytes) len = maxBytes;
        str = TransformString(Image + pos + 1, len);
        break;
    case 3:     //AnsiString
        len = *((int*)(Image + pos));
        if (len < 0) len = 0;
        if (len > maxBytes) len = maxBytes;
        str = TransformString(Image + pos + 4, len);
        break;
    case 4:     //WideString
        len = *((int*)(Image + pos + WAlign));
        if (len < 0) len = 0;
        if (len > maxBytes) len = maxBytes;
        wStr = WideString((wchar_t*)(Image + pos + WAlign + 4));
        size = WideCharToMultiByte(CP_ACP, 0, wStr, len, 0, 0, 0, 0);
        if (size)
        {
            tmpBuf = new char[size + 1];
            WideCharToMultiByte(CP_ACP, 0, wStr, len, (LPSTR)tmpBuf, size, 0, 0);
            str = TransformString(tmpBuf, size);
            delete[] tmpBuf;
        }
        break;
    }

    lbString->Lines->Add(str);
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::ShowArray(DWORD fromAdr)
{
/*
    lbArray->Clear();

    int pos = Adr2Pos(fromAdr);
    DWORD curAdr = fromAdr;

    for (int n = 0; n < 32; n++)
    {
        if (pos > TotalSize) break;
        DWORD Adr = *((DWORD*)(Image + pos));
        if (!IsValidImageAdr(Adr)) break;
        char* p = Image + Adr2Pos(Adr);
        String s = String(p);
        lbArray->Items->Add(s);
        pos += 4;
    }
*/
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::btnDefCodeClick(TObject *Sender)
{
    DefineAs = DEFINE_AS_CODE;
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::btnUndefCodeClick(TObject *Sender)
{
    DefineAs = UNDEFINE;
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::btnDefStringClick(TObject *Sender)
{
    DefineAs = DEFINE_AS_STRING;
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::rgStringViewStyleClick(TObject *Sender)
{
    ShowString(Adr, 1024);
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::miCopy2ClipboardClick(TObject *Sender)
{
    TStrings* items;

    if (pc1->ActivePage == tsCode)
        items = lbCode->Items;
    else if (pc1->ActivePage == tsData)
        items = lbData->Items;
    else if (pc1->ActivePage == tsString)
        items = lbString->Lines;
    else if (pc1->ActivePage == tsText)
        items = lbText->Items;

    Copy2Clipboard(items, 0, false);
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
    if (Key == VK_ESCAPE) ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::FormShow(TObject *Sender)
{
    DefineAs = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::rgDataViewStyleClick(TObject *Sender)
{
    ShowData(Adr, 1024);
}
//---------------------------------------------------------------------------
void __fastcall TFExplorer_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

