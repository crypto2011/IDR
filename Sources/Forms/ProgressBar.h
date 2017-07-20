// ---------------------------------------------------------------------------

#ifndef ProgressBarH
#define ProgressBarH
// ---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Controls.hpp>
#include "Main.h"
// #include "sysmac.h"

// #define WM_UPDANALYSISSTATUS WM_APP + 1
// ---------------------------------------------------------------------------
class TFProgressBar : public TForm {
__published: // IDE-managed Components
	TProgressBar *pb;
	TStatusBar *sb;

	void __fastcall FormShow(TObject *Sender);

private: // User declarations
public: // User declarations
	__fastcall TFProgressBar(TComponent* Owner);
	void __fastcall StartProgress(String text0, String text1, int steps);
	void __fastcall wm_updAnalysisStatus(TMessage& msg);

	BEGIN_MESSAGE_MAP VCL_MESSAGE_HANDLER(WM_UPDANALYSISSTATUS, TMessage, wm_updAnalysisStatus);
	END_MESSAGE_MAP(TForm)
};

// ---------------------------------------------------------------------------
extern PACKAGE TFProgressBar *FProgressBar;
// ---------------------------------------------------------------------------
#endif
