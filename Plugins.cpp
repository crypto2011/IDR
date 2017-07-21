//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Plugins.h"
#include "Misc.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

void (__stdcall *fnRegisterPlugIn)(LPCTSTR *);
void (__stdcall *fnAboutPlugIn)(void);

TFPlugins *FPlugins;
//---------------------------------------------------------------------------
__fastcall TFPlugins::TFPlugins(TComponent* Owner)
    : TForm(Owner)
{
    PluginsPath = "";
    PluginName = "";
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::FormShow(TObject *Sender)
{
    TSearchRec      sr;

    cklbPluginsList->Clear();
    if (PluginsPath != "")
    {
        Screen->Cursor = crHourGlass;
        String curDir = GetCurrentDir();
        ChDir(PluginsPath);
        if (!FindFirst("*.dll", faArchive, sr))
        {
            do
            {
                HINSTANCE hModule = LoadLibrary(sr.Name.c_str());
                if (hModule)
                {
                    String info = "";
                    fnRegisterPlugIn = (void (__stdcall*)(LPCTSTR *))GetProcAddress(hModule, "RegisterPlugIn");
                    if (fnRegisterPlugIn)
                    {
                        LPCTSTR ptr;
                        fnRegisterPlugIn(&ptr);
                        info = ptr;
                    }
                    cklbPluginsList->Items->Add(sr.Name + " - " + info);
                    FreeLibrary(hModule);
                }
            } while (!FindNext(sr));

            FindClose(sr);
        }
        ChDir(curDir);
        Screen->Cursor = crDefault;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::cklbPluginsListClickCheck(TObject *Sender)
{
    if (cklbPluginsList->State[cklbPluginsList->ItemIndex])
    {
        for (int n = 0; n < cklbPluginsList->Items->Count; n++)
        {
            cklbPluginsList->State[n] = (n == cklbPluginsList->ItemIndex);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::cklbPluginsListDblClick(TObject *Sender)
{
    String filename = "";
    String line = cklbPluginsList->Items->Strings[cklbPluginsList->ItemIndex];
    int pos = line.Pos('-');
    if (pos > 0) filename = line.SubString(1, pos - 1).Trim();
    if (filename != "")
    {
        HINSTANCE hModule = LoadLibrary((PluginsPath + "\\" + filename).c_str());
        if (hModule)
        {
            fnAboutPlugIn = (void (__stdcall*)(void))GetProcAddress(hModule, "AboutPlugIn");
            if (fnAboutPlugIn) fnAboutPlugIn();
            FreeLibrary(hModule);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::bOkClick(TObject *Sender)
{
    PluginName = "";
    for (int n = 0; n < cklbPluginsList->Items->Count; n++)
    {
        if (cklbPluginsList->State[n])
        {
            String line = cklbPluginsList->Items->Strings[n];
            int pos = line.Pos('-');
            if (pos > 0) PluginName = line.SubString(1, pos - 1).Trim();
            break;
        }
    }
    ModalResult = mrOk;
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::bCancelClick(TObject *Sender)
{
    PluginName = "";
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
void __fastcall TFPlugins::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

