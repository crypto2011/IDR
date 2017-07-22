//---------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Hex2Double.h"
#include <stdio.h>
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TFHex2DoubleDlg_11011981 *FHex2DoubleDlg_11011981;
//---------------------------------------------------------------------
__fastcall TFHex2DoubleDlg_11011981::TFHex2DoubleDlg_11011981(TComponent* AOwner)
	: TForm(AOwner)
{
}
//---------------------------------------------------------------------
void __fastcall TFHex2DoubleDlg_11011981::FormShow(TObject *Sender)
{
    rgDataViewStyle->ItemIndex = 0;
    PrevIdx = 0;
    edtValue->Text = "";
    if (edtValue->CanFocus()) ActiveControl = edtValue;
}
//---------------------------------------------------------------------------
void __fastcall TFHex2DoubleDlg_11011981::edtValueEnter(TObject *Sender)
{
 	edtValue->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TFHex2DoubleDlg_11011981::Str2Binary(String AStr)
{
    BYTE    c;
    int     val, pos, n;
    char    *src;

    src = AStr.c_str();
    memset(BinData, 0, 16); n = 0;

    while (1)
    {
        c = *src;
        if (!c) break;
        if (c != ' ')
        {
            sscanf(src, "%X", &val);
            BinData[n] = val; n++;
            while (1)
            {
                c = *src;
                if (!c || c == ' ') break;
                src++;
            }
            if (!c) break;
        }
        src++;
    }
}
//---------------------------------------------------------------------------
String __fastcall TFHex2DoubleDlg_11011981::Binary2Str(int BytesNum)
{
    String result = "";
    for (int n = 0; n < BytesNum; n++)
    {
        if (n) result += " ";
        result += Val2Str2(BinData[n]);
    }
    return result;
}
//---------------------------------------------------------------------------
void __fastcall TFHex2DoubleDlg_11011981::rgDataViewStyleClick(TObject *Sender)
{
    int         n;
    float       singleVal;
    double      doubleVal;
    long double extendedVal;
    double      realVal;
    Comp        compVal;
    String      result;

    if (edtValue->Text == "")
    {
        PrevIdx = rgDataViewStyle->ItemIndex;
        return;
    }
    if (rgDataViewStyle->ItemIndex == PrevIdx) return;

    result = edtValue->Text;
    try
    {
        if (rgDataViewStyle->ItemIndex == 0)//Hex
        {
            memset((void*)BinData, 0, 16);
            if (PrevIdx == FT_SINGLE)
            {
                singleVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &singleVal, 4);
                result = Binary2Str(4);
            }
            else if (PrevIdx == FT_DOUBLE)
            {
                doubleVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &doubleVal, 8);
                result = Binary2Str(8);
            }
            else if (PrevIdx == FT_EXTENDED)
            {
                extendedVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &extendedVal, 10);
                result = Binary2Str(10);
            }
            else if (PrevIdx == FT_REAL)
            {
                realVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &realVal, 4);
                result = Binary2Str(4);
            }
            else if (PrevIdx == FT_COMP)
            {
                compVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &compVal, 8);
                result = Binary2Str(8);
            }
        }
        else
        {
            if (PrevIdx == 0)
            {
                Str2Binary(edtValue->Text);
            }
            else if (PrevIdx == FT_SINGLE)
            {
                singleVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &singleVal, 4);
            }
            else if (PrevIdx == FT_DOUBLE)
            {
                doubleVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &doubleVal, 8);
            }
            else if (PrevIdx == FT_EXTENDED)
            {
                extendedVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &extendedVal, 10);
            }
            else if (PrevIdx == FT_REAL)
            {
                realVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &realVal, 4);
            }
            else if (PrevIdx == FT_COMP)
            {
                compVal = StrToFloat(edtValue->Text);
                memmove((void*)BinData, &compVal, 8);
            }
            if (rgDataViewStyle->ItemIndex == FT_SINGLE)
            {
                singleVal = 0; memmove((void*)&singleVal, BinData, 4);
                result = FloatToStr(singleVal);
            }
            else if (rgDataViewStyle->ItemIndex == FT_DOUBLE)
            {
                doubleVal = 0; memmove((void*)&doubleVal, BinData, 8);
                result = FloatToStr(doubleVal);
            }
            else if (rgDataViewStyle->ItemIndex == FT_EXTENDED)
            {
                extendedVal = 0; memmove((void*)&extendedVal, BinData, 10);
                result = FloatToStr(extendedVal);
            }
            else if (rgDataViewStyle->ItemIndex == FT_REAL)
            {
                realVal = 0; memmove((void*)&realVal, BinData, 4);
                result = FloatToStr(realVal);
            }
            else if (rgDataViewStyle->ItemIndex == FT_COMP)
            {
                compVal = 0; memmove((void*)&compVal, BinData, 8);
                result = FloatToStr(compVal);
            }
        }
    }
    catch (Exception &E)
    {
        result = "Impossible!";
    }
    PrevIdx = rgDataViewStyle->ItemIndex;
    edtValue->Text = result;
}
//---------------------------------------------------------------------------
void __fastcall TFHex2DoubleDlg_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

