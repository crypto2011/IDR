//---------------------------------------------------------------------------
#ifndef ResourcesH
#define ResourcesH
//---------------------------------------------------------------------------
#include <ComCtrls.hpp>
#include <AppEvnts.hpp>

#include "UfrmFormTree.h"
//---------------------------------------------------------------------------
typedef struct
{
    String      CompName;       //Component Name
    String      EventName;      //Event Name
    int         Adr;            //Event Address
} EventListItem, *PEventListItem;

typedef struct
{
    TMetaClass  *RegClass;
    char        *ClassName;
} RegClassInfo, *PRegClassInfo;

typedef struct
{
    String      EventName;      //Event name (OnClose)
    String      ProcName;       //Event handler name (CloseBtnClick)
} EventInfo, *PEventInfo;

typedef struct
{
    bool        Inherit;        //Component is inherited
    bool		HasGlyph;		//Component has property "Glyph"
    String      Name;           //Component name
    String      ClassName;      //Component class
    TList		*Events;		//EventInfo list
} ComponentInfo, *PComponentInfo;

class   IdrDfmLoader;

#define     FF_INHERITED        1
#define     FF_HASNOTEBOOK      2

//Records to resource list
class TDfm
{
public:
    __fastcall TDfm();
    __fastcall ~TDfm();
    BYTE    Open;               //2 - form opened; 1 - form closed but loader not destroyed; 0 - form closed
    BYTE    Flags;              //Form flags (see FF_...)
    String  ResName;            //Resource name (ABOUTDIALOG)
    String  Name;           	//Form name (AboutDialog)
    String  ClassName;      	//Form class (TAboutDialog)
    TMemoryStream *MemStream;   //Memory Stream, containing Resource Data
    TDfm    *ParentDfm;         //Parent form
    TList	*Events;			//Form events list
    TList   *Components;        //Form components list
    TForm   *Form;              //Pointer to opened form
    IdrDfmLoader  *Loader;      //Form loader
    bool __fastcall IsFormComponent(String CompName);
};

class TResourceInfo
{
public:
    __fastcall TResourceInfo();
    __fastcall ~TResourceInfo();
    void __fastcall GetFormAsText(TDfm* Dfm, TStrings* DstList);
    bool __fastcall EnumResources(String FileName);
    void __fastcall ShowResources(TListBox* ListBox);
    void __fastcall GetBitmap(TDfm* Dfm, Graphics::TBitmap* DstBitmap);
    TDfm* __fastcall GetParentDfm(TDfm* Dfm);
    void __fastcall CloseAllForms();
    void __fastcall ReopenAllForms();
    void __fastcall InitAliases();
    String __fastcall GetAlias(String ClassName);
    TDfm* __fastcall GetFormIdx(String FormName, int* idx);
    TDfm* __fastcall GetFormByClassName(String ClassName);
    void __fastcall GetEventsList(String FormName, TList* Lst);
    bool		citadel;
    int         Counter;
    HINSTANCE   hFormPlugin;
    String      FormPluginName;
    TList       *FormList;      //Form names list
    TStringList *Aliases;       //Aliases list
};

class TMyControl : public TControl
{
public:
    __property OnClick;
    __property OnMouseDown;
    __property PopupMenu;
};

class IdrDfmReader: public TReader
{
public:
    __fastcall IdrDfmReader(TStream* Stream, int BufSize);
    __property PropName;
};

class TComponentEvents : public TCollectionItem
{
public:
    __fastcall TComponentEvents(TCollection* Owner);
    virtual __fastcall ~TComponentEvents();

    TComponent	*Component;
    TStringList	*FoundEvents;
} ;

typedef void __fastcall (__closure *TFindMethodSourceEvent)
    (TObject* Sender, String& ClassName, String& MethodName);

class IdrDfmForm : public TForm
{
public:
    __fastcall IdrDfmForm(TComponent* Owner);
    __fastcall IdrDfmForm(TComponent* Owner, int Dummy);
    virtual __fastcall ~IdrDfmForm();

    String 	originalClassName;
    String 	originalClassType;
    TComponentEvents *current;
    TCollection *compsEventsList;
    TPopupMenu* evPopup;
    TIdrDfmFormTree_11011981* frmTree;

    void __fastcall SetupControls();
    void __fastcall SetupControlResetShortcut(TComponent* component);
    void __fastcall SetDesigning(bool value, bool SetChildren);
    void __fastcall SetMyHandlers();
    __property TFindMethodSourceEvent OnFindMethod = {read = FOnFindMethod, write = FOnFindMethod, default = 0};
protected:
    virtual void __fastcall CreateHandle();
    void __fastcall SetupControlHint(String FormName, TControl* Control, String InitHint);
    void __fastcall SetupMenuItem(TMenuItem* mi, String searchName);
    void __fastcall ActionExecute(TObject* Sender);
    void __fastcall miClick(TObject* Sender);
    void __fastcall ControlMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall miPopupClick(TObject *Sender);

    BEGIN_MESSAGE_MAP
    VCL_MESSAGE_HANDLER(CM_DIALOGKEY, TCMDialogKey, CMDialogKey)
    END_MESSAGE_MAP(TForm)
private:
    TFindMethodSourceEvent FOnFindMethod;
    String __fastcall getFormName();
    void __fastcall DoFindMethod(String& methodName);
    void __fastcall CMDialogKey(TCMDialogKey& Message);
    void __fastcall MyFormShow(TObject *Sender);
    void __fastcall MyFormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall MyFormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
    void __fastcall MyShortCutEvent(Messages::TWMKey &Msg, bool &Handled);
	void __fastcall ShowMyPopupMenu(String FormName, String ControlName, bool show);
};

class IdrDfmLoader : public TComponent
{
public:
    __fastcall IdrDfmLoader(TComponent* Owner);
    virtual __fastcall ~IdrDfmLoader();

    TForm* __fastcall LoadForm(TStream* dfmStream, TDfm* dfm, bool loadingParent = false);
    __property TFindMethodSourceEvent OnFindMethod = {read = FOnFindMethod, write = SetOnFindMethod, default = 0};

protected:
    int     Counter;
    TComponentEvents	*Current;
    TCollection			*CompsEventsList;
    String          lastClassAliasName;

    void __fastcall AncestorNotFound(TReader* Reader, String ComponentName, TMetaClass* ComponentClass, TComponent* &Component);
    void __fastcall ReaderError(TReader* Reader, String Message, bool &Handled);
    void __fastcall FindComponentClass(TReader* Reader, String ClassName, TMetaClass* &ComponentClass);
    void __fastcall CreateComponent(TReader* Reader, TMetaClass* ComponentClass, TComponent* &Component);
    void __fastcall FindMethod(TReader* Reader, String MethodName, void* &Address, bool &Error);
    void __fastcall SetComponentName(TReader* Reader, TComponent* Component, String &Name);

    void __fastcall DoReferenceName(TReader* Reader, String &Name);
    void __fastcall SetOnFindMethod(TFindMethodSourceEvent method);
    bool __fastcall HasGlyph(String ClassName);
private:
    IdrDfmForm* dfmForm;
    TFindMethodSourceEvent FOnFindMethod;
    TComponent	*FindComp(TComponent* Owner, String& compnay);
};

class IdrDfmDefaultControl : public TPanel
{
public:
    __fastcall IdrDfmDefaultControl(TObject* Owner);

    bool IsVisible();
    void SetClassName(const String& name, const String& mappedName);
    String GetOrigClassName(){return originalClassName;}
        
protected:
    virtual void __fastcall Loaded();
    virtual void __fastcall ReadState(TReader* Reader);
    virtual void __fastcall Paint();

private:
    String originalClassName, mappedClassName;

    bool HasIconForClass(const String& name);
    void CreateImageIconForClass(const String& imgFile);
    
    //image for std nonvisual controls (dialogs, etc) (dclstd60.bpl has images)
    TImage* imgIcon;
    void __fastcall ImageMouseDown(TObject* Sender, TMouseButton Button, TShiftState Shift, int X, int Y);    
        
};

class IdrImageControl : public TImage
{
public:
    __fastcall IdrImageControl(TComponent* Owner);
protected:
    virtual void __fastcall Paint();
};
//---------------------------------------------------------------------------
#endif

