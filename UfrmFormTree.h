//---------------------------------------------------------------------------
#ifndef UfrmFormTreeH
#define UfrmFormTreeH
#include <Classes.hpp>
#include <ComCtrls.hpp>
#include <Controls.hpp>
#include <Dialogs.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
//#include <Classes.hpp>
//#include <Controls.hpp>
//#include <StdCtrls.hpp>
//#include <Forms.hpp>
//#include <ComCtrls.hpp>
#include <map>

typedef std::map<const void*, TTreeNode*> TTreeNodeMap;
//---------------------------------------------------------------------------
class TIdrDfmFormTree_11011981 : public TForm
{
__published:	// IDE-managed Components
    TTreeView *tvForm;
    TFindDialog *dlgFind;
    TPopupMenu *mnuTree;
    TMenuItem *Expand1;
    TMenuItem *Collapse1;
    TMenuItem *Find1;
    void __fastcall tvFormKeyPress(TObject *Sender, char &Key);
    void __fastcall tvFormDblClick(TObject *Sender);
    void __fastcall dlgFindFind(TObject *Sender);
    void __fastcall Expand1Click(TObject *Sender);
    void __fastcall Collapse1Click(TObject *Sender);
    void __fastcall Find1Click(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
    String MakeNodeCaption(TComponent* curCompo);
    String MakeNodeEventCaption(void* item);
    void BorderTheControl(TControl* aControl);

    void __fastcall AddEventsToNode(String compName, TTreeNode* dstNode, TList* evList);
    TTreeNode* __fastcall FindTreeNodeByText(TTreeNode* nodeFrom, const String& txt, bool caseSensitive);    
    TTreeNode* __fastcall FindTreeNodeByTag(const void* tag);
    void __fastcall AddTreeNodeWithTag(TTreeNode* node, const void* tag);    
    TTreeNodeMap NodesMap;

    bool __fastcall IsEventNode(TTreeNode* selNode);
public:		// User declarations
    __fastcall TIdrDfmFormTree_11011981(TComponent* Owner);
};

//---------------------------------------------------------------------------
//show thinking cursor (ctor - show, dtor - hide with restore previous)

class ThinkCursor
{
public:
    inline ThinkCursor()
    {
        prevCursor      = Screen->Cursor;
        Screen->Cursor  = crHourGlass;
        Application->ProcessMessages();
    }
    inline ~ThinkCursor()
    {
        Screen->Cursor = prevCursor;
    }

private:
     Controls::TCursor prevCursor;
};

//---------------------------------------------------------------------------
#endif
