// ---------------------------------------------------------------------------

#ifndef PluginsH
#define PluginsH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.CheckLst.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>

// ---------------------------------------------------------------------------
class TFPlugins : public TForm {
__published: // IDE-managed Components
	TCheckListBox *cklbPluginsList;
	TButton *bOk;
	TButton *bCancel;

	void __fastcall FormShow(TObject *Sender);
	void __fastcall cklbPluginsListClickCheck(TObject *Sender);
	void __fastcall cklbPluginsListDblClick(TObject *Sender);
	void __fastcall bOkClick(TObject *Sender);
	void __fastcall bCancelClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);

private: // User declarations
public: // User declarations
	__fastcall TFPlugins(TComponent* Owner);

	String PluginsPath;
	String PluginName;
};

// ---------------------------------------------------------------------------
extern PACKAGE TFPlugins *FPlugins;
// ---------------------------------------------------------------------------
#endif
