// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>

// ---------------------------------------------------------------------------
#include <Vcl.Styles.hpp>
#include <Vcl.Themes.hpp>
USEFORM("Sources\Forms\UfrmFormTree.cpp", IdrDfmFormTree_11011981);
USEFORM("Sources\Forms\ProgressBar.cpp", FProgressBar);
USEFORM("Sources\Forms\StringInfo.cpp", FStringInfo_11011981);
USEFORM("Sources\Forms\TypeInfo2.cpp", FTypeInfo_11011981);
USEFORM("Sources\Forms\EditFieldsDlg.cpp", FEditFieldsDlg_11011981);
USEFORM("Sources\Forms\EditFunctionDlg.cpp", FEditFunctionDlg_11011981);
USEFORM("Sources\Forms\Explorer.cpp", FExplorer_11011981);
USEFORM("Sources\Forms\ActiveProcesses.cpp", FActiveProcesses);
USEFORM("Sources\Forms\AboutDlg.cpp", FAboutDlg_11011981);
USEFORM("Sources\Forms\FindDlg.cpp", FindDlg_11011981);
USEFORM("Sources\Forms\Legend.cpp", FLegend_11011981);
USEFORM("Sources\Forms\Main.cpp", FMain_11011981);
USEFORM("Sources\Forms\Plugins.cpp", FPlugins);
USEFORM("Sources\Forms\KBViewer.cpp", FKBViewer_11011981);
USEFORM("Sources\Forms\Hex2Double.cpp", FHex2DoubleDlg_11011981);
USEFORM("Sources\Forms\IdcSplitSize.cpp", FIdcSplitSize);
USEFORM("Sources\Forms\InputDlg.cpp", FInputDlg_11011981);
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
	try {
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
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
	catch (Exception &exception) {
		Application->ShowException(&exception);
	}
	catch (...) {
		try {
			throw Exception("");
		}
		catch (Exception &exception) {
			Application->ShowException(&exception);
		}
	}
	return 0;
}
// ---------------------------------------------------------------------------
