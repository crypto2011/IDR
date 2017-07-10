// ---------------------------------------------------------------------------

#ifndef ActiveProcessesH
#define ActiveProcessesH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <vector>
#include "ProcessManager.h"

// ---------------------------------------------------------------------------
class TFActiveProcesses : public TForm {
__published:
	TListView *ListViewProcesses;
	TPopupMenu *PMProcess;
	TMenuItem *PMProcessDump;
	TMenuItem *N1;
	TMenuItem *PMProcessRefresh;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall PMProcessDumpClick(TObject *Sender);
	void __fastcall PMProcessPopup(TObject *Sender);
	void __fastcall PMProcessRefreshClick(TObject *Sender);

private:
	void GenerateProcessList();

public:

	virtual __fastcall TFActiveProcesses(TComponent* AOwner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TFActiveProcesses *FActiveProcesses;
// ---------------------------------------------------------------------------
#endif
