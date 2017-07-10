// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

// #include <stdio.h>
#include "Main.h"
#include "ActiveProcesses.h"
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TFActiveProcesses *FActiveProcesses;

// ---------------------------------------------------------------------
__fastcall TFActiveProcesses::TFActiveProcesses(TComponent* AOwner) : TForm(AOwner) {
}

// ---------------------------------------------------------------------------
void TFActiveProcesses::GenerateProcessList() {
	ListViewProcesses->Items->BeginUpdate();
	ListViewProcesses->Clear();

	TProcessManager PM;

	if (!PM.GenerateProcessList()) {
		ShowMessage("Error getting the list of processes");
		return;
	}

	for (std::vector<TProcess>::iterator it = PM.ProcessList.begin(); it != PM.ProcessList.end(); ++it) {
		TListItem *ListItem = ListViewProcesses->Items->Add();
		ListItem->Caption = IntToHex((int)it->Pid, 4);
		ListItem->SubItems->Add(it->Name);
		ListItem->SubItems->Add(IntToHex((int)it->ImageSize, 8));
		ListItem->SubItems->Add(IntToHex((int)it->EntryPoint, 8));
		ListItem->SubItems->Add(IntToHex((int)it->BaseAddress, 8));
	}

	ListViewProcesses->Items->EndUpdate();
	ListViewProcesses->Repaint();
	ListViewProcesses->Refresh();
}

// ---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::FormShow(TObject *Sender) {
	GenerateProcessList();
}

// ---------------------------------------------------------------------------
void __fastcall TFActiveProcesses::PMProcessDumpClick(TObject *Sender) {
	DWORD _pid, _boc, _poc, _imb;
	TListItem *_li;
	TMemoryStream *_stream;
	TMemoryStream *_stream1;
	String _item;
	TProcessManager PM;

	try {
		_li = ListViewProcesses->Selected;
		_pid = StrToInt("0x" + _li->Caption);
		_stream = new TMemoryStream;
		PM.DumpProcess(_pid, _stream, &_boc, &_poc, &_imb);
		if (!_stream->Size) {
			delete _stream;
			return;
		}
		_stream->SaveToFile("__idrtmp__.exe");
		delete _stream;
		FMain_11011981->DoOpenDelphiFile(DELHPI_VERSION_AUTO, "__idrtmp__.exe", false, false);
	}
	catch (const Exception &ex) {
		ShowMessage("Dumper failed: " + ex.Message);
	}

	Close();
}
// ---------------------------------------------------------------------------

void __fastcall TFActiveProcesses::PMProcessPopup(TObject *Sender) {
	PMProcessDump->Enabled = (ListViewProcesses->Selected);
}
// ---------------------------------------------------------------------------

void __fastcall TFActiveProcesses::PMProcessRefreshClick(TObject *Sender) {
	GenerateProcessList();
}
// ---------------------------------------------------------------------------
