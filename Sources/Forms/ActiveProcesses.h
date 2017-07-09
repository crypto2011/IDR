// ---------------------------------------------------------------------------

#ifndef ActiveProcessesH
#define ActiveProcessesH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <vector>
#include "ProcessManager.h"

// ---------------------------------------------------------------------------
class TFActiveProcesses : public TForm {
__published:
	TButton *btnDump;
	TButton *btnCancel;
	TListView *lvProcesses;

	void __fastcall btnCancelClick(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall btnDumpClick(TObject *Sender);
	void __fastcall lvProcessesClick(TObject *Sender);

private:
public:
	void ShowProcesses();

	virtual __fastcall TFActiveProcesses(TComponent* AOwner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TFActiveProcesses *FActiveProcesses;
// ---------------------------------------------------------------------------
#endif
