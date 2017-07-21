//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "SyncObjs.hpp"

USEFORM("Main.cpp", FMain_11011981);
USEFORM("TypeInfo.cpp", FTypeInfo_11011981);
USEFORM("StringInfo.cpp", FStringInfo_11011981);
USEFORM("Explorer.cpp", FExplorer_11011981);
USEFORM("InputDlg.cpp", FInputDlg_11011981);
USEFORM("FindDlg.cpp", FindDlg_11011981);
USEFORM("EditFunctionDlg.cpp", FEditFunctionDlg_11011981);
USEFORM("AboutDlg.cpp", FAboutDlg_11011981);
USEFORM("EditFieldsDlg.cpp", FEditFieldsDlg_11011981);
USEFORM("KBViewer.cpp", FKBViewer_11011981);
USEFORM("UfrmFormTree.cpp", IdrDfmFormTree_11011981);
USEFORM("Legend.cpp", FLegend_11011981);
USEFORM("Hex2Double.cpp", FHex2DoubleDlg_11011981);
USEFORM("Plugins.cpp", FPlugins);
USEFORM("ActiveProcesses.cpp", FActiveProcesses);
USEFORM("IdcSplitSize.cpp", FIdcSplitSize);
USEFORM("ProgressBar.cpp", FProgressBar);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
         Randomize();
         Application->Initialize();
         Application->HelpFile = "";
		 Application->CreateForm(__classid(TFMain_11011981), &FMain_11011981);
         Application->CreateForm(__classid(TFExplorer_11011981), &FExplorer_11011981);
         Application->CreateForm(__classid(TFTypeInfo_11011981), &FTypeInfo_11011981);
         Application->CreateForm(__classid(TFStringInfo_11011981), &FStringInfo_11011981);
         Application->CreateForm(__classid(TFInputDlg_11011981), &FInputDlg_11011981);
         Application->CreateForm(__classid(TFindDlg_11011981), &FindDlg_11011981);
         Application->CreateForm(__classid(TFEditFunctionDlg_11011981), &FEditFunctionDlg_11011981);
         Application->CreateForm(__classid(TFEditFieldsDlg_11011981), &FEditFieldsDlg_11011981);
         Application->CreateForm(__classid(TFAboutDlg_11011981), &FAboutDlg_11011981);
         Application->CreateForm(__classid(TFKBViewer_11011981), &FKBViewer_11011981);
         Application->CreateForm(__classid(TFLegend_11011981), &FLegend_11011981);
         Application->CreateForm(__classid(TFHex2DoubleDlg_11011981), &FHex2DoubleDlg_11011981);
         Application->CreateForm(__classid(TFPlugins), &FPlugins);
         Application->CreateForm(__classid(TFActiveProcesses), &FActiveProcesses);
         Application->CreateForm(__classid(TFIdcSplitSize), &FIdcSplitSize);
         Application->CreateForm(__classid(TFProgressBar), &FProgressBar);
         Application->Run();
    }
    catch (Exception &exception)
    {
         Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------

