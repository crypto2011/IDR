//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "UfrmFormTree.h"
#include "Main.h"
#include "Misc.h"
#include <memory>
#include <StrUtils.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
//as: todo: refactor! extern linkage is bad practice (idea: use singleton) 
extern  TResourceInfo   *ResInfo;
extern  PInfoRec        *Infos;
extern  DWORD           CodeBase;
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::AddEventsToNode(String compName, TTreeNode* dstNode, TList* evList)
{
    const int evCnt = evList->Count;
    for (int m = 0; m < evCnt; m++)
    {
        EventListItem *item = (EventListItem*)evList->Items[m];
        if (SameText(item->CompName, compName))
        {
            tvForm->Items->AddChildObject(dstNode, MakeNodeEventCaption(item), (void*)item->Adr);
        }
    }
}
//---------------------------------------------------------------------------
__fastcall TIdrDfmFormTree_11011981::TIdrDfmFormTree_11011981(TComponent* Owner)
    : TForm(Owner)
{
    ThinkCursor waitCursor;
     
    TForm* frmOwner = (TForm*)Owner;

    const int compCnt = frmOwner->ComponentCount;
    if (compCnt > 1)
        Caption = frmOwner->Name + " (" + String(compCnt) + " components)";
    else
        Caption = frmOwner->Name + " (1 component)";

    std::auto_ptr<TList> evList(new TList);
    ResInfo->GetEventsList(frmOwner->Name, evList.get());

    tvForm->Items->BeginUpdate();

    TTreeNode* rootNode = tvForm->Items->AddObject(0, frmOwner->Name, frmOwner);
    AddTreeNodeWithTag(rootNode, frmOwner);
    AddEventsToNode(frmOwner->Name, rootNode, evList.get());

    TComponent* currComp;
    TControl* currControl;
    TTreeNode *parentNode, *childNode;
    bool externalParent = false;
    String externalNames;

    for (int i = 0; i < compCnt; i++)
    {
        currComp = frmOwner->Components[i];
        if (currComp == this) continue;     //Don't display Form TreeView

        String tnCaption = MakeNodeCaption(currComp);

        TComponent* parCompo = currComp->GetParentComponent();

        if (currComp->HasParent())
        {
            //in some very rare cases parent could be located in another module!
            //so in this case we have true for currComp->HasParent() but NULL for parCompo
            if (!parCompo)
            {
                externalParent = true;
                
                if (!externalNames.IsEmpty())
                    externalNames += ", ";
                externalNames += tnCaption;
            }

            String parName = parCompo ? parCompo->Name : String("N/A");
            String ownName = currComp->Owner->Name;

            parentNode = FindTreeNodeByTag(parCompo);

            //tricky case for special components (eg: TPage, TTabPage)
            //they are a) noname (but renamed in IDR)
            //         b) not present in Components[] array

            if (!parentNode && parCompo)
            {
                TComponent* parCompo2  = parCompo->GetParentComponent();
                String parName2    = parCompo2->Name;
                TTreeNode *parentNode2 = FindTreeNodeByTag(parCompo2);

                // add noname parent
                parentNode = tvForm->Items->AddChildObject(parentNode2, MakeNodeCaption(parCompo), parCompo);
                AddTreeNodeWithTag(parentNode, parCompo);
            }
        }
        else
            parentNode = FindTreeNodeByTag(currComp->Owner);

        childNode = tvForm->Items->AddChildObject(parentNode, tnCaption, currComp);
        AddTreeNodeWithTag(childNode, currComp);
        AddEventsToNode(currComp->Name, childNode, evList.get());
    }

    rootNode->Expand(false);
    tvForm->Items->EndUpdate();

    if (externalParent)
    {
        Application->MessageBox(("Form has some component classes defined in external modules:\r\n"
            + externalNames + ".\r\n\r\nVisualization of these components is not yet implemented").c_str(), "Warning", MB_OK | MB_ICONWARNING);
    }
}
//---------------------------------------------------------------------------
String TIdrDfmFormTree_11011981::MakeNodeCaption(TComponent* curCompo)
{
    String className = curCompo->ClassName();

    IdrDfmDefaultControl* replControl;
    if ((replControl = dynamic_cast<IdrDfmDefaultControl*>(curCompo))!=0)
        className = "!" + replControl->GetOrigClassName();

    return curCompo->Name + ":" + className;

    //as: old code
    //return curCompo->Name + ":" + curCompo->ClassName();
}
//---------------------------------------------------------------------------
String TIdrDfmFormTree_11011981::MakeNodeEventCaption(void* item)
{
    if (((EventListItem*)item)->Adr)
    {
        PInfoRec recN = GetInfoRec(((EventListItem*)item)->Adr);
        if (recN && recN->HasName())
            return ((EventListItem*)item)->EventName + "=" + recN->GetName();
    }
    return ((EventListItem*)item)->EventName;
}
//---------------------------------------------------------------------------
TTreeNode* __fastcall TIdrDfmFormTree_11011981::FindTreeNodeByTag(const void* tag)
{
    TTreeNodeMap::const_iterator it = NodesMap.find(tag);
    if (it != NodesMap.end()) return it->second;

    return 0;
}

//---------------------------------------------------------------------------
TTreeNode* __fastcall TIdrDfmFormTree_11011981::FindTreeNodeByText(TTreeNode* nodeFrom, const String& txt, bool caseSensitive)
{
    TTreeNode* tn = 0;
    bool startScan = nodeFrom ? false : true;


    TTreeNodes* nodes = tvForm->Items;
    for (int i=0; i < nodes->Count; ++i)
    {
        if (!startScan)
        {
            if (nodeFrom == nodes->Item[i])
                startScan = true;
            continue;
        }

        String ttt = nodes->Item[i]->Text;
        if ((caseSensitive && AnsiContainsStr(nodes->Item[i]->Text, txt))
            || (!caseSensitive && AnsiContainsText(nodes->Item[i]->Text, txt))
            )
        {
            tn = nodes->Item[i];
            break;
        }
    }
    
    return tn;
}

//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::AddTreeNodeWithTag(TTreeNode* node, const void* tag)
{
    NodesMap[tag] = node;
}
//---------------------------------------------------------------------------
bool __fastcall TIdrDfmFormTree_11011981::IsEventNode(TTreeNode* selNode)
{
    return (selNode && !selNode->Text.IsEmpty() && selNode->Text.Pos("="));
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    if (VK_ESCAPE == Key)
    {
        Key = 0;
        Close();
        ((TForm*)Owner)->SetFocus();
    }
    else if (VK_F3 == Key)
    {
        dlgFindFind(Sender);
    }
    else if (Shift.Contains(ssCtrl)
            && 'F' == Key
         )
    {
        dlgFind->Execute();
    }
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::tvFormKeyPress(TObject *Sender, char &Key)
{
    if (VK_RETURN == Key)
    {
        Key = 0;
        tvFormDblClick(Sender);
    }
}
//---------------------------------------------------------------------------
void TIdrDfmFormTree_11011981::BorderTheControl(TControl* aControl)
{
    std::auto_ptr<TControlCanvas> C(new TControlCanvas());
    TWinControl* parCtrl = aControl->Parent;

    if (!parCtrl) parCtrl = (TWinControl*)aControl;

    HDC aDC;
    TRect aRect;
    HWND aHandle;
    bool bMenu;

    C->Control = parCtrl;

    aHandle = parCtrl->Handle;
    aDC = GetWindowDC(aHandle);
    C->Handle = aDC;
    C->Brush->Style = bsSolid;
    C->Pen->Width = 1;
    C->Pen->Color = clRed;

    aRect = aControl->ClientRect;
    TForm* frm = dynamic_cast<TForm*>(parCtrl);
    bMenu = frm && frm->Menu != 0;
    AdjustWindowRectEx(&aRect,
                       GetWindowLong(aHandle, GWL_STYLE),
                       bMenu,
                       GetWindowLong(aHandle, GWL_EXSTYLE));
    MoveWindowOrg(aDC, -aRect.Left, -aRect.Top);

    if (parCtrl == aControl)
    {
        aRect = aControl->ClientRect;
    }
    else
    {
        aRect = aControl->BoundsRect;
        InflateRect(&aRect, 2, 2);
    }
    //C->Rectangle(aRect);
    C->DrawFocusRect(aRect);

    ReleaseDC(aHandle, aDC);
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::tvFormDblClick(TObject *Sender)
{
    TTreeNode* selNode = tvForm->Selected;
    if (!selNode) return;
    
    if (IsEventNode(selNode))
    {
        const DWORD Adr = (const DWORD)selNode->Data;
        if (Adr && IsValidCodeAdr(Adr))
        {
            PInfoRec recN = GetInfoRec(Adr);
            if (recN)
            {
                if (recN->kind == ikVMT)
                    FMain_11011981->ShowClassViewer(Adr);
                else
                    FMain_11011981->ShowCode(Adr, 0, -1, -1);
            }
        }
        return;
    }

    TControl* selControl = dynamic_cast<TControl*>((TComponent*)selNode->Data);
    if (!selControl) return;

    TWinControl* parControl = selControl->Parent;
    if (!parControl) return;

    TForm* ownerForm = (TForm*)Owner;

    ownerForm->BringToFront();
    selControl->BringToFront();

    for (int i = 0; i < 2; i++)
    {
        BorderTheControl(selControl);
        selControl->Hide();
        selControl->Update();
        Sleep(150);
        BorderTheControl(selControl);
        selControl->Show();
        selControl->Update();
        Sleep(150);
    }

    BringToFront();
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::dlgFindFind(TObject *Sender)
{
    bool caseSensitive = dlgFind->Options.Contains(frMatchCase);

    TTreeNode* tn = FindTreeNodeByText(tvForm->Selected, dlgFind->FindText, caseSensitive);
    if (!tn)
        tn = FindTreeNodeByText(0, dlgFind->FindText.Trim(), caseSensitive);
    
    if (tn)
    {
        tvForm->Selected = tn;
        tn->Expand(false);
        tvForm->SetFocus();
    }
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::Expand1Click(TObject *Sender)
{
    if (tvForm->Selected)
        tvForm->Selected->Expand(true);
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::Collapse1Click(TObject *Sender)
{
    if (tvForm->Selected)
        tvForm->Selected->Collapse(false);
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::Find1Click(TObject *Sender)
{
    dlgFind->Execute();
}
//---------------------------------------------------------------------------

void __fastcall TIdrDfmFormTree_11011981::FormClose(TObject *Sender,
      TCloseAction &Action)
{
    dlgFind->CloseDialog();
}
//---------------------------------------------------------------------------
void __fastcall TIdrDfmFormTree_11011981::FormCreate(TObject *Sender)
{
    ScaleForm(this);
}
//---------------------------------------------------------------------------

